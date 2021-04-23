#include "GiuhJsonReader.h"

using namespace giuh;

std::shared_ptr<giuh_kernel_impl> GiuhJsonReader::build_giuh_kernel(std::string catchment_id, std::string comid,
                                                                    ptree catchment_data_node) {
    // Get times and freqs and convert to vectors
    std::vector<double> cdf_times;
    std::vector<double> cumulative_freqs = extract_cumulative_frequency_ordinates(catchment_data_node);

    // TODO: account for error condition of unmatching JSON structure for times
    for (ptree::value_type times : catchment_data_node.get_child("CDF.Time")) {
        cdf_times.push_back(times.second.get_value<double>());
    }

    return std::make_shared<giuh_kernel_impl>(giuh_kernel_impl(catchment_id, comid, cdf_times, cumulative_freqs));
}

std::vector<double> GiuhJsonReader::extract_cumulative_frequency_ordinates(std::string catchment_id) {
    std::string associated_comid = get_associated_comid(std::move(catchment_id));
    std::unique_ptr<ptree> data_node_ptr = find_data_node_for_comid(associated_comid);
    return extract_cumulative_frequency_ordinates(*data_node_ptr);
}

std::vector<double> GiuhJsonReader::extract_cumulative_frequency_ordinates(ptree catchment_data_node) {
    std::vector<double> cumulative_freqs;

    // TODO: account for error condition of unmatching JSON structure for freqs
    std::string freq_node_name;
    if (catchment_data_node.get_child_optional("CDF.CumulativeFreq") != boost::none) {
        freq_node_name = "CDF.CumulativeFreq";
    }
    // TODO: later apply logic to be able to handle if there is a 'CDF.minHydro.Runoff', though that may also belong
    //  elsewhere since those are not cumulative frequencies.
    else {
        throw std::runtime_error("Unable to find GIUH cumulative frequencies data node in parsed GIUH JSON");
    }

    for (ptree::value_type freqs : catchment_data_node.get_child(freq_node_name)) {
        cumulative_freqs.push_back(freqs.second.get_value<double>());
    }
    double eps = 0.0001;
    if (cumulative_freqs.back() > 1.0 + eps || cumulative_freqs.back() < 1.0 - eps) {
        throw std::runtime_error(
                "GIUH cumulative frequencies do not have 1 as final value " + std::to_string(cumulative_freqs.back()) +
                "\n");
    }
    return cumulative_freqs;
}

std::unique_ptr<ptree> GiuhJsonReader::find_data_node_for_comid(std::string comid) {
    // Traverse tree until finding node with correct id
    for (ptree::iterator pos = data_json_tree->begin(); pos != data_json_tree->end(); ++pos) {
        // TODO: confirm this logic is correct, with respect to iterator members and what they retrieve
        // When the right node is found, use it to build a referenced kernel, and return the reference
        auto first = pos->first;
        if (pos->first == comid) {
            return std::make_unique<ptree>(pos->second);
        }
    }
    return nullptr;
}

std::string GiuhJsonReader::get_associated_comid(std::string catchment_id) {
    return id_map != nullptr && id_map->count(catchment_id) == 1 ? id_map->at(catchment_id) : "";
}

std::shared_ptr<giuh_kernel_impl> GiuhJsonReader::get_giuh_kernel_for_id(std::string catchment_id) {
    if (!is_data_json_file_readable()) {
        return nullptr;
    }
    std::string comid = get_associated_comid(catchment_id);
    if (comid == "") {
        return nullptr;
    }
    // Traverse tree until finding node with correct id
    std::unique_ptr<ptree> data_node = find_data_node_for_comid(comid);
    return data_node == nullptr ? nullptr : build_giuh_kernel(catchment_id, comid, *data_node);
}

bool GiuhJsonReader::is_giuh_kernel_for_id_exists(std::string catchment_id) {
    if (!is_data_json_file_readable() || !is_id_map_json_file_readable()) {
        return false;
    }
    std::string comid = get_associated_comid(catchment_id);
    if (comid == "") {
        return false;
    }
    // Traverse tree until finding node with correct id
    for (ptree::iterator pos = data_json_tree->begin(); pos != data_json_tree->end(); ++pos) {
        // TODO: confirm this logic is correct, with respect to iterator members and what they retrieve
        // When the right node is found, use it to build a referenced kernel, and return the reference
        if (pos->first == comid) {
            return true;
        }
    }
    return false;
}

bool GiuhJsonReader::is_data_json_file_readable() {
    return data_json_file_readable;
}

bool GiuhJsonReader::is_id_map_json_file_readable() {
    return id_map_json_file_readable;
}
