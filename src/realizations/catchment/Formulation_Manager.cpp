#include <Formulation_Manager.hpp>
#include "realizations/config/properties.hpp"
#include "realizations/config/serialization_helper.hpp"

#include <cerrno>
#include <dirent.h>
#include <iostream>
#include <mutex>
#include <regex>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

#if NGEN_WITH_NETCDF
    #include "NetCDFPerFeatureDataProvider.hpp"
#endif

using namespace realization;

Formulation_Manager::Formulation_Manager(boost::property_tree::ptree loaded_tree)
    : tree(std::move(loaded_tree))
{
    initialize_output_config();
}

void Formulation_Manager::read(simulation_time_params &simulation_time_config,
                               geojson::GeoJSON fabric) {
    //TODO seperate the parsing of configuration options like time
    //and routing and other non feature specific tasks from this main function
    //which has to iterate the entire hydrofabric.

    // Output configuration (output_config) is parsed at construction
    // so get_output_config() is valid before read().

    auto possible_global_config = tree.get_child_optional("global");

    if (possible_global_config) {
        global_config = realization::config::Config(*possible_global_config);
    }

    // Parse realization-level inheritable config blocks from the
    // top-level JSON (see realization::config::GlobalConfigKey).
    // Each recognized block is stored in `global_configs` and
    // later merged into every formulation's params via
    // `realization::config::apply_config`. Per-formulation
    // entries still win — the parsed block is a default.
    for (auto which : { realization::config::GlobalConfigKey::SERIALIZATION }) {
        const char* key = realization::config::to_key_string(which);
        auto possible_block = tree.get_child_optional(key);
        if (possible_block) {
            // Resolve path tokens before the block is inherited by formulations.
            if (which == realization::config::GlobalConfigKey::SERIALIZATION) {
                realization::config::apply_serialization_path_token_resolution(*possible_block);
            }
            global_configs.emplace(
                key,
                geojson::JSONProperty(key, *possible_block)
            );
        }
    }

    /**
     * Read the layer descriptions
    */
    // try to get the json node
    auto layers_json_array = tree.get_child_optional("layers");
    //Create the default surface layer
    config::Layer layer;
    // layer description struct
    ngen::LayerDescription layer_desc;
    layer_desc = layer.get_descriptor();
    // add the default surface layer to storage
    layer_storage.put_layer(layer_desc, layer_desc.id);

    if(layers_json_array){

        for (std::pair<std::string, boost::property_tree::ptree> layer_config : *layers_json_array)
        {
            layer = config::Layer(layer_config.second);
            layer_desc = layer.get_descriptor();

            // add the layer to storage
            layer_storage.put_layer(layer_desc, layer_desc.id);
            if (layer.has_formulation() && layer.get_domain() == "catchments") {
                domain_formulations.emplace(
                    layer_desc.id,
                    construct_formulation_from_config(simulation_time_config,
                    "layer-"+std::to_string(layer_desc.id),
                    layer.formulation
                    )
                );
            }
            //TODO for each layer, create deferred providers for use by other layers
            //VERY SIMILAR TO NESTED MODULE INIT
        }
    }

    //TODO use the set of layer providers as input for catchments to lookup from

    /**
     * Read routing configurations from configuration file
     */
    auto possible_routing_configs = tree.get_child_optional("routing");

    if (possible_routing_configs) {
        //Since it is possible to build NGEN without routing support, if we see it in the config
        //but it isn't enabled in the build, we should at least put up a warning
    #if NGEN_WITH_ROUTING
        this->routing_config = (config::Routing(*possible_routing_configs)).params;
        using_routing = true;
    #else
        using_routing = false;
        std::cerr<<"WARNING: Formulation Manager found routing configuration"
                 <<", but routing support isn't enabled. No routing will occur."<<std::endl;
    #endif //NGEN_WITH_ROUTING
     }

    /**
     * Read catchment configurations from configuration file
    */
    // With the global-config map populated and the hydrofabric
    // already loaded for this manager, fill in a default
    // `serialization.restore.id_subset` from the set of ids
    // this run will actually touch. A no-op when the user
    // supplied their own id_subset, or when no restore block
    // is configured. See
    // `realization::config::apply_serialization_restore_subset_default`
    // for the precise no-op cases.
    {
        std::vector<std::string> known_ids;
        if (fabric) {
            known_ids.reserve(fabric->get_size());
            for (const geojson::Feature& location : *fabric) {
                known_ids.push_back(location->get_id());
            }
        }
        realization::config::apply_serialization_restore_subset_default(
            global_configs, known_ids);
    }

    auto possible_catchment_configs = tree.get_child_optional("catchments");

    // For now at least, this isn't allowed
    if (possible_catchment_configs &&
        output_config.nexus.format == realization::config::OutputFormat::netcdf) {
        std::string msg = "ERROR: Individual catchment formulation configs are not allowed when using "
                          "per-formulation nexus files.";
        std::cerr << msg;
        throw std::runtime_error(msg);
    }

    std::mutex formulations_mutex;

    if (possible_catchment_configs) {
        for (std::pair<std::string, boost::property_tree::ptree> catchment_config : *possible_catchment_configs) {
          int catchment_index = fabric->find(catchment_config.first);
          if( catchment_index == -1 )
          {
              #ifndef NGEN_QUIET
              std::cerr<<"WARNING Formulation_Manager::read: Cannot create formulation for catchment "
                      <<catchment_config.first
                      <<" that isn't identified in the hydrofabric or requested subset"<<std::endl;
              #endif
              continue;
          }
          realization::config::Config catchment_formulation(catchment_config.second);

          if(!catchment_formulation.has_formulation()){
            throw std::runtime_error("ERROR: No formulations defined for "+catchment_config.first+".");
          }
          // Parse catchment-specific model_params
          auto catchment_feature = fabric->get_feature(catchment_index);
          catchment_formulation.formulation.link_external(catchment_feature);
          auto formulation = this->construct_formulation_from_config(
                                                                     simulation_time_config,
                                                                     catchment_config.first,
                                                                     catchment_formulation
                                                                     );
          {
              std::lock_guard l(formulations_mutex);
              this->add_formulation(formulation);
          }
        }//end for catchments
    }//end if possible_catchment_configs

    for (geojson::Feature location : *fabric) {
        {
            std::lock_guard presence(formulations_mutex);
            if (this->contains(location->get_id()))
                continue;
        }

        std::shared_ptr<Formulation> missing_formulation =
            this->construct_missing_formulation(location, simulation_time_config);

        std::lock_guard insertion(formulations_mutex);
        this->add_formulation(missing_formulation);
    }
}

void Formulation_Manager::add_formulation(std::shared_ptr<Formulation> formulation) {
    this->formulations.emplace(formulation->get_id(), formulation);
}

std::shared_ptr<Formulation> Formulation_Manager::get_formulation(std::string id) const {
    // TODO: Implement on-the-fly formulation creation using global parameters
    return this->formulations.at(id);
}

std::shared_ptr<Formulation> Formulation_Manager::get_domain_formulation(long id) const {
    return this->domain_formulations.at(id);
}

bool Formulation_Manager::has_domain_formulation(int id) const {
    return this->domain_formulations.count( id ) > 0;
}

bool Formulation_Manager::contains(std::string identifier) const {
    return this->formulations.count(identifier) > 0;
}

int Formulation_Manager::get_size() {
    return this->formulations.size();
}

bool Formulation_Manager::is_empty() {
    return this->formulations.empty();
}

const realization::config::Output& Formulation_Manager::get_output_config() const {
    return output_config;
}

std::map<std::string, std::shared_ptr<Formulation>>::const_iterator Formulation_Manager::begin() const {
    return this->formulations.cbegin();
}

std::map<std::string, std::shared_ptr<Formulation>>::const_iterator Formulation_Manager::end() const {
    return this->formulations.cend();
}

bool Formulation_Manager::get_using_routing() {
    return this->using_routing;
}

std::string Formulation_Manager::get_t_route_config_file_with_path() {
    if(this->routing_config != nullptr)
        return this->routing_config->t_route_config_file_with_path;
    else
        return "";
}

void Formulation_Manager::initialize_output_config() {
    output_config = realization::config::Output::from_realization(tree);
    #if !NGEN_QUIET
    if (output_config.from_legacy_keys) {
        std::cerr << "WARNING: top-level 'output_root', 'disable_catchment_output', and "
                     "'per_formulation_nexus_files' keys are deprecated; prefer an 'output' "
                     "config block (output.root / output.catchment / output.nexus)." << std::endl;
    }
    #endif
    #if !NGEN_WITH_NETCDF
    if (output_config.nexus.format == realization::config::OutputFormat::netcdf) {
        throw std::runtime_error("ERROR: nexus output format 'netcdf' requested, but NGEN was built without NetCDF support.");
    }
    #endif
}

ngen::LayerDataStorage& Formulation_Manager::get_layer_metadata() { return layer_storage; }

std::shared_ptr<Formulation> Formulation_Manager::construct_formulation_from_config(
    simulation_time_params &simulation_time_config,
    std::string identifier,
    realization::config::Config& catchment_formulation
) {
    if(!formulation_exists(catchment_formulation.formulation.type)){
        throw std::runtime_error("Catchment " + identifier + " failed initialization: '" +
                catchment_formulation.formulation.type + "' is not a valid formulation. Options are: "+valid_formulation_keys());
    }

    if(catchment_formulation.forcing.parameters.empty()){
        throw std::runtime_error("No forcing definition was found for " + identifier);
    }

    std::vector<std::string> missing_parameters;

    if (!catchment_formulation.forcing.has_key("path")) {
        missing_parameters.push_back("path");
    }

    if (missing_parameters.size() > 0) {
        std::string message = "A forcing configuration cannot be created for '" + identifier + "'; the following parameters are missing: ";

        for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
            message += missing_parameters[missing_parameter_index];

            if (missing_parameter_index < missing_parameters.size() - 1) {
                message += ", ";
            }
        }

        throw std::runtime_error(message);
    }

    forcing_params forcing_config = this->get_forcing_params(catchment_formulation.forcing.parameters, identifier, simulation_time_config);
    std::shared_ptr<Formulation> constructed_formulation = construct_formulation(catchment_formulation.formulation.type, identifier, forcing_config);
    //, geometry);

    realization::config::pattern_substitution(catchment_formulation.formulation.parameters,
                                              BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG, "{{id}}",
                                              identifier);

    for (auto which : { realization::config::GlobalConfigKey::SERIALIZATION }) {
        realization::config::apply_config(
            catchment_formulation.formulation.parameters, global_configs, which);
    }
    constructed_formulation->create_formulation(catchment_formulation.formulation.parameters);
    return constructed_formulation;
}

std::shared_ptr<Formulation> Formulation_Manager::construct_missing_formulation(geojson::Feature& feature, simulation_time_params &simulation_time_config){
    const std::string identifier = feature->get_id();

    forcing_params forcing_config = this->get_forcing_params(global_config.forcing.parameters, identifier, simulation_time_config);
    std::shared_ptr<Formulation> missing_formulation = construct_formulation(global_config.formulation.type, identifier, forcing_config);
    // Need to work with a copy, since it is altered in-place
    realization::config::Config global_copy = global_config;
    realization::config::pattern_substitution(global_copy.formulation.parameters,
                                              BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG, "{{id}}",
                                              identifier);
    //Some helpful debugging prints, commented out, but left for later
    //because they will eventually be used by someone, someday, looking at configurations
    //being turned into concrecte formulations...
    // geojson::JSONProperty::print_property(global_config.formulation.parameters.at("modules"));

    //Make a copy of the global configuration so parameters don't clash when linking to external data
    auto formulation =  realization::config::Formulation(global_copy.formulation);
    formulation.link_external(feature);
    for (auto which : { realization::config::GlobalConfigKey::SERIALIZATION }) {
        realization::config::apply_config(formulation.parameters, global_configs, which);
    }
    missing_formulation->create_formulation(formulation.parameters);

    return missing_formulation;
}

namespace {
    /**
     * Wrap `closedir` in a deleter to avoid GCC's ignored-attributes warning for `decltype(&closedir)`
     */
    struct DirectoryCloser {
        void operator()(DIR* dir) const {
            ::closedir(dir);
        }
    };
    /**
     * A self-closing pointer to a directory that may be operated upon
     */
    using DirectoryPointer = std::unique_ptr<DIR, DirectoryCloser>;

    /**
     * A mapping of placeholder tokens to the values that should serve as their replacements
     */
    using PlaceholderContext = std::vector<std::pair<std::string, std::string>>;

    /** @brief Test to see if a directory entry at a path is either a regular file or a symlink that
     * points at a file
     *
     * @param path The path to the directory containing the entry of interest
     * @param entry The loaded/assigned directory entry
     * @return Whether the received entry is a regular file or a link to a regular file
     * @throws std::invalid_argument if a null directory entry was passed
     * @throws std::runtime_error if the file could not be called with stat
     */
    bool is_regular_file_or_symlink_target(const std::string& path, const dirent* entry) {
        if (entry == nullptr) {
            throw std::invalid_argument(
                "Cannot test if an object under '" + path +  + "' is a regular file - no directory pointer was passed"
            );
        }
    #ifdef _DIRENT_HAVE_D_TYPE
        if (entry->d_type == DT_REG) {
            return true;
        }

        if (entry->d_type != DT_UNKNOWN && entry->d_type != DT_LNK) {
            return false;
        }
    #endif
        const std::string file_path = path + entry->d_name;
        struct stat file_stat{};
        if (stat((file_path).c_str(), &file_stat) != 0) {
            throw std::runtime_error(
                "Could not stat file '" + file_path + "': " + strerror(errno)
            );
        }
        return S_ISREG(file_stat.st_mode);
    }

    /** @brief Get a self-closing pointer to a directory
     *
     * @see https://www.gnu.org/software/libc/manual/html_node/Opening-a-Directory.html#Opening-a-Directory
     *
     * @note: The following errors prevent any attempt to retry the opening process: ENOENT, ENXIO, EACCES, EPERM, ENOTDIR, EMFILE, ENFILE
     *
     * @param path Where to look for a directory to open
     * @param wait_seconds The number of seconds to wait between attempts to open the directory. Must be 1 or greater
     * @param maximum_attempts The maximum number of times to get an open directory. Must be 1 or greater
     * @return A self-closing pointer to a directory handle
     * @throws std::invalid_argument if the wait_seconds is less than 1
     * @throws std::invalid_argument if the maximum_attempts is less than 1
     * @throws std::runtime_error if an error occurred while attempting to read the directory
     * @throws std::runtime_error if the maximum_attempts was reached before successfully connecting to the directory
     */
    DirectoryPointer open_directory(
        const std::string& path,
        const unsigned int wait_seconds = 2,
        const unsigned int maximum_attempts = 5
    ) {
        if (wait_seconds < 1) {
            throw std::invalid_argument(
                "Cannot get a pointer to '" +
                    path +
                    "' - the seconds to wait between attempts must be at least 1"
                );
        }

        if (maximum_attempts < 1) {
            throw std::invalid_argument(
                "Cannot get a pointer to '" +
                path +
                "' - There must be at least one attempt to search"
            );
        }

        const char* search_path = path.c_str();
        DirectoryPointer directory_pointer(nullptr, DirectoryCloser{});
        int error_code = 0;
        int attempt = 0;

        for (; attempt < maximum_attempts; attempt++) {
            directory_pointer.reset(opendir(search_path));
            if (directory_pointer) {
                return directory_pointer;
            }

            error_code = errno;

            switch (error_code) {
            case ENOENT:    // No such file or directory
            case ENXIO:     // No such device or index
            case EACCES:    // Permission Denied
            case EPERM:     // Operation not permitted
            case ENOTDIR:   // Not a directory
            case EMFILE:    // Too many open files
            case ENFILE:    // Too many open files in system
                throw std::runtime_error(
                    "Error encountered while opening the forcing data directory at '" +
                    path + "': " +
                    std::strerror(error_code)
                );
            default:
                break;
            }

            // Bypass the wait on the last loop
            if (attempt + 1 < maximum_attempts) {
                sleep(wait_seconds);
            }
        }

        throw std::runtime_error(
            "Error opening forcing data dir '" +
                path +
                "' after " +
                std::to_string(attempt) +
                " attempts: " +
                std::strerror(error_code)
        );
    }

    /** @brief Replace all placeholders in a given file pattern with a supplied context and return a prepared regular expression paired with its pattern
     *
     * TODO: Integrate with the path_tokens resolution functionality
     *
     * @param file_pattern A configured pattern to match on when searching for a file
     * @param context A set of placeholders and values that need to be inserted into the file pattern
     * @return A pairing of a prepared regular expression and its pattern
     */
    std::pair<std::regex, std::string> create_file_pattern_matcher(
        std::string file_pattern,
        const PlaceholderContext& context
    ) {
        constexpr std::string_view REGEX_SPECIAL_CHARACTERS = R"(\.^$|()[]{}*+?)";

        for (const auto& [placeholder, value] : context) {
            std::string escaped_identifier;
            escaped_identifier.reserve(value.size() * 2);

            for (const char character : value) {
                if (REGEX_SPECIAL_CHARACTERS.find(character) != std::string::npos) {
                    escaped_identifier += '\\';
                }
                escaped_identifier += character;
            }

            std::size_t position = 0;

            while ((position = file_pattern.find(placeholder, position)) != std::string::npos) {
                file_pattern.replace(position, placeholder.length(), escaped_identifier);
                position += escaped_identifier.size();
            }
        }

        return {std::regex(file_pattern), file_pattern};
    }
} // namespace for get_forcing_params helpers

forcing_params Formulation_Manager::get_forcing_params(
    const geojson::PropertyMap &forcing_prop_map,
    std::string identifier,
    simulation_time_params &simulation_time_config
) {
    std::string path;
    if(forcing_prop_map.count("path") > 0){
        path = forcing_prop_map.at("path").as_string();
    }

    std::string provider;
    if(forcing_prop_map.count("provider") > 0){
        provider = forcing_prop_map.at("provider").as_string();
    }

    /**
    * You can exit early when there isn't a file_pattern since all the following code uses the
    * file pattern to search for an appropriate file
    */
    if (forcing_prop_map.count("file_pattern") == 0) {
        return {
            path,
            provider,
            simulation_time_config.start_time,
            simulation_time_config.end_time
        };
    }

    if (path.empty()) {
        throw std::runtime_error(
            "Error with NGEN config - 'path' in forcing params must be set to a "
            "non-empty parent directory path when 'file_pattern' is used."
        );
    }

    // Since we are given a pattern, we need to identify the directory and pull out anything that matches the pattern
    if (path.compare(path.size() - 1, 1, "/") != 0) {
        path += "/";
    }

    const std::string filepattern = forcing_prop_map.at("file_pattern").as_string();

    const PlaceholderContext placeholder_context = {
        {"{{id}}", identifier}
    };

    // Create a regular expression used to identify proper file names
    auto [pattern_matcher, pattern_string] = create_file_pattern_matcher(
        filepattern,
        placeholder_context
    );

    // A stream providing the functions necessary for evaluating a directory
    const DirectoryPointer pointer_to_directory = open_directory(path);

    // structure representing the member of a directory:
    // https://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html
    dirent *entry = nullptr;
    int invalid_matches = 0;

    // TODO: Determine if this loop should see an implementation similar to `open_directory`
    //  in order to supply more flexible error handling
    while (true) {
        errno = 0;

        // Read the next entry in the opened directory
        entry = readdir(pointer_to_directory.get());

        // If nothing was read, there was either an error or there's nothing else to read
        if (entry == nullptr) {
            if (errno != 0) {
                throw std::runtime_error(
                    "Error reading the forcing directory '" +
                    path + "': " +
                    std::strerror(errno)
                );
            }
            break;
        }

        if (std::regex_match(entry->d_name, pattern_matcher)) {
            if (is_regular_file_or_symlink_target(path, entry)) {
                return {
                    path + entry->d_name,
                    provider,
                    simulation_time_config.start_time,
                    simulation_time_config.end_time
                };
            }
            invalid_matches++;
        }
    }

    throw std::runtime_error(
        "Forcing data could not be found in '" +
        path +
        "' for '" +
        identifier +
        "' with the pattern: '" +
        pattern_string +
        "'. There were " +
        std::to_string(invalid_matches) +
        " actual matches, but none of the matching items were regular files or symlinks to regular files."
    );
}

void Formulation_Manager::parse_external_model_params(boost::property_tree::ptree& model_params, const geojson::Feature catchment_feature) {
     boost::property_tree::ptree attr {};
     for (decltype(auto) param : model_params) {
        if (param.second.count("source") == 0) {
            attr.put_child(param.first, param.second);
            continue;
        }

        decltype(auto) param_source = param.second.get_child("source");
        decltype(auto) param_source_name = param_source.get_value<std::string>();
        if (param_source_name != "hydrofabric") {
            // temporary until the logic for alternative sources is designed
            throw std::logic_error("ERROR: 'model_params' source `" + param_source_name + "` not currently supported. Only `hydrofabric` is supported.");
        }

        decltype(auto) param_name = param.second.find("from") == param.second.not_found()
            ? param.first
            : param.second.get_child("from").get_value<std::string>();

        if (catchment_feature->has_property(param_name)) {
            auto catchment_attribute = catchment_feature->get_property(param_name);
            switch (catchment_attribute.get_type()) {
                case geojson::PropertyType::Natural:
                    attr.put(param.first, catchment_attribute.as_natural_number());
                    break;
                case geojson::PropertyType::Boolean:
                    attr.put(param.first, catchment_attribute.as_boolean());
                    break;
                case geojson::PropertyType::Real:
                    attr.put(param.first, catchment_attribute.as_real_number());
                    break;
                case geojson::PropertyType::String:
                    attr.put(param.first, catchment_attribute.as_string());
                    break;

                case geojson::PropertyType::List:
                case geojson::PropertyType::Object:
                default:
                    std::cerr << "WARNING: property type " << static_cast<int>(catchment_attribute.get_type()) << " not allowed as model parameter. "
                              << "Must be one of: Natural (int), Real (double), Boolean, or String" << '\n';
                    break;
            }
        } else {
            std::cerr << "WARNING Failed to parse external parameter: catchment `"
                      << catchment_feature->get_id()
                      << "` does not contain the property `"
                      << param_name << "`\n";
        }
    }

    model_params.swap(attr);
}

void Formulation_Manager::parse_external_model_params(geojson::PropertyMap& model_params, const geojson::Feature catchment_feature) {
    geojson::PropertyMap attr {};
    for (decltype(auto) param : model_params) {
        // Check for type to short-circuit. If param.second is not an object, `.has_key()` will throw
        if (param.second.get_type() != geojson::PropertyType::Object || !param.second.has_key("source")) {
            attr.emplace(param.first, param.second);
            continue;
        }

        decltype(auto) param_source = param.second.at("source");
        decltype(auto) param_source_name = param_source.as_string();
        if (param_source_name != "hydrofabric") {
            // TODO: temporary until the logic for alternative sources is designed
            throw std::logic_error("ERROR: 'model_params' source `" + param_source_name + "` not currently supported. Only `hydrofabric` is supported.");
        }

        // Property name in the feature properties is either
        // the value of key "from", or has the same name as
        // the expected model parameter key
        decltype(auto) param_name = param.second.has_key("from")
            ? param.second.at("from").as_string()
            : param.first;

        if (catchment_feature->has_property(param_name)) {
            auto catchment_attribute = catchment_feature->get_property(param_name);

            // Use param.first in the `.emplace` calls instead of param_name since
            // the expected name is given by the key of the model_params values.
            switch (catchment_attribute.get_type()) {
                case geojson::PropertyType::Natural:
                    attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_natural_number()));
                    break;
                case geojson::PropertyType::Boolean:
                    attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_boolean()));
                    break;
                case geojson::PropertyType::Real:
                    attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_real_number()));
                    break;
                case geojson::PropertyType::String:
                    attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_string()));
                    break;

                case geojson::PropertyType::List:
                case geojson::PropertyType::Object:
                default:
                    // TODO: Should list/object values be passed to model parameters?
                    //       Typically, feature properties *should* be scalars.
                    std::cerr << "WARNING: property type " << static_cast<int>(catchment_attribute.get_type()) << " not allowed as model parameter. "
                              << "Must be one of: Natural (int), Real (double), Boolean, or String" << '\n';
                    break;
            }
        }
    }

    model_params.swap(attr);
}

void Formulation_Manager::finalize() {
    // The calls in these loops are staticly dispatched to
    // Formulation::finalize(). That does not
    // inherit from DataProvider, with its virtual member
    // function of the same name.
    //
    // If any formulation class needs to customize this
    // behavior through this becoming a virtual dispatch,
    // take care. Bmi_Multi_Formulation was a concern, but
    // does not currently need to because none of its
    // constituent formulations points to any forcing
    // object other than the enclosing
    // Bmi_Multi_Formulation instance itself.
    for (auto const& fmap: formulations) {
        fmap.second->finalize();
    }
    for (auto const& fmap: domain_formulations) {
        fmap.second->finalize();
    }

#if NGEN_WITH_NETCDF
    data_access::NetCDFPerFeatureDataProvider::cleanup_shared_providers();
#endif
}

/*
template<class realization_type>
Realization_Manager<realization_type>::Realization_Manager(std::stringstream &data) {

}

template<class realization_type>
Realization_Manager<realization_type>::Realization_Manager(const std::string &file_path) {

}

template<class realization_type>
void Realization_Manager<realization_type>::read() {
    auto possible_global_config = tree.get_child_optional("global");

    realization_type* global_realization = nullptr;

    if (possible_global_config) {
        this->global_realization_config = get_realizationconfig("all", *possible_global_config);
    }

    auto possible_catchment_configs = tree.get_child_optional("catchments");

    if (possible_catchment_configs) {
        for (auto &config_pair : *possible_catchment_configs) {
            Realization_Config new_config = get_realizationconfig(config_pair.first, config_pair.second, this->global_realization_config);
            this->realization_configs.emplace(config_pair.first, new_config);
        }
    }
}

template<class realization_type>
void Realization_Manager<realization_type>::add_realization(realization_type realization) {
    this->realizations.emplace(realization.get_id(), realization);
}

template<class realization_type>
realization_type Realization_Manager<realization_type>::get_realization(std::string id) const {
    return this->realizations.at(id);
}

template<class realization_type>
bool Realization_Manager<realization_type>::contains(std::string identifier) const {
    return this->realizations.find(identifier) > 0;
}

template<class realization_type>
int Realization_Manager<realization_type>::get_size() {
    return this->realizations.size();
}

template<class realization_type>
bool Realization_Manager<realization_type>::is_empty() {
    return this->realizations.empty();
}

template<class realization_type>
typename std::map<std::string, realization_type>::const_iterator Realization_Manager<realization_type>::begin() const {
    return this->realizations.cbegin();
}

template<class realization_type>
typename std::map<std::string, realization_type>::const_iterator Realization_Manager<realization_type>::end() const {
    return this->realizations.cend();
}*/
