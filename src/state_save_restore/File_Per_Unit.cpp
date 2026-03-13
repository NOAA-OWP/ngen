#include <state_save_restore/File_Per_Unit.hpp>
#include <utilities/Logger.hpp>

#if __has_include(<filesystem>) && __cpp_lib_filesystem >= 201703L
  #include <filesystem>
  using namespace std::filesystem;
  #warning "Using STD Filesystem"
#elif __has_include(<experimental/filesystem>) && defined(__cpp_lib_filesystem)
  #include <experimental/filesystem>
  using namespace std::experimental::filesystem;
  #warning "Using Filesystem TS"
#elif __has_include(<boost/filesystem.hpp>)
  #include <boost/filesystem.hpp>
  using namespace boost::filesystem;
  #warning "Using Boost.Filesystem"
#else
  #error "No Filesystem library implementation available"
#endif

#include <fstream>
#include <iomanip>

namespace unit_saving_utils {
    std::string format_epoch(State_Saver::snapshot_time_t epoch)
    {
        time_t t = std::chrono::system_clock::to_time_t(epoch);
        std::tm tm = *std::gmtime(&t);

        std::stringstream tss;
        tss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return tss.str();
    }
}

// This class is only declared and defined here, in the .cpp file,
// because it is strictly an implementation detail of the top-level
// File_Per_Unit_Saver class
class File_Per_Unit_Snapshot_Saver : public State_Snapshot_Saver
{
    friend class File_Per_Unit_Saver;

    public:
    File_Per_Unit_Snapshot_Saver() = delete;
    File_Per_Unit_Snapshot_Saver(path base_path, State_Saver::State_Durability durability);
    ~File_Per_Unit_Snapshot_Saver();

public:
    void save_unit(std::string const& unit_name, boost::span<char const> data) override;
    void finish_saving() override;

private:
    path dir_path_;
};

File_Per_Unit_Saver::File_Per_Unit_Saver(std::string base_path)
    : base_path_(std::move(base_path))
{
    auto dir_path = path(base_path_);
    create_directories(dir_path);
}

File_Per_Unit_Saver::~File_Per_Unit_Saver() = default;

std::shared_ptr<State_Snapshot_Saver> File_Per_Unit_Saver::initialize_snapshot(State_Durability durability) {
    // TODO
    return std::make_shared<File_Per_Unit_Snapshot_Saver>(path(this->base_path_), durability);
}

std::shared_ptr<State_Snapshot_Saver> File_Per_Unit_Saver::initialize_checkpoint_snapshot(snapshot_time_t epoch, State_Durability durability)
{
    path checkpoint_path = path(this->base_path_) / unit_saving_utils::format_epoch(epoch);
    create_directory(checkpoint_path);
    return std::make_shared<File_Per_Unit_Snapshot_Saver>(checkpoint_path, durability);
}

void File_Per_Unit_Saver::finalize()
{
    // nothing to be done
}

File_Per_Unit_Snapshot_Saver::File_Per_Unit_Snapshot_Saver(path base_path, State_Saver::State_Durability durability)
    : State_Snapshot_Saver(durability)
    , dir_path_(base_path)
{
    create_directory(dir_path_);
}

File_Per_Unit_Snapshot_Saver::~File_Per_Unit_Snapshot_Saver() = default;

void File_Per_Unit_Snapshot_Saver::save_unit(std::string const& unit_name, boost::span<char const> data)
{
    auto file_path = dir_path_ / unit_name;
    try {
        std::ofstream stream(file_path.string(), std::ios_base::out | std::ios_base::binary);
        stream.write(data.data(), data.size());
        stream.close();
    } catch (std::exception &e) {
        LOG("Failed to write state save data for unit '" + unit_name + "' in file '" + file_path.string() + "'", LogLevel::WARNING);
        throw;
    }
}

void File_Per_Unit_Snapshot_Saver::finish_saving()
{
    if (durability_ == State_Saver::State_Durability::strict) {
        // fsync() or whatever
    }
}


// This class is only declared and defined here, in the .cpp file,
// because it is strictly an implementation detail of the top-level
// File_Per_Unit_Saver class
class File_Per_Unit_Snapshot_Loader : public State_Snapshot_Loader
{
    friend class State_Snapshot_Loader;
public:
    File_Per_Unit_Snapshot_Loader() = default;
    File_Per_Unit_Snapshot_Loader(path dir_path);
    ~File_Per_Unit_Snapshot_Loader() override = default;

    bool has_unit(const std::string &unit_name) override;

    /**
     * Load data from whatever source and store it in the `data` vector.
     * 
     * @param data The location where the loaded data will be stored. This will be resized to the amount of data loaded.
     */
    void load_unit(const std::string &unit_name, std::vector<char> &data) override;

    /**
     * Execute logic to complete the saving process
     *
     * Data may be flushed here, and delayed errors may be detected
     * and reported here. With relaxed durability, error reports may
     * not come until the parent State_Saver::finalize() call is made,
     * or ever.
     */
    void finish_saving() override { };

private:
    path dir_path_;
    std::vector<char> data_;
};

File_Per_Unit_Snapshot_Loader::File_Per_Unit_Snapshot_Loader(path dir_path)
    : dir_path_(dir_path)
{

}

bool File_Per_Unit_Snapshot_Loader::has_unit(const std::string &unit_name) {
    auto file_path = dir_path_ / unit_name;
    return exists(file_path.string());
}

void File_Per_Unit_Snapshot_Loader::load_unit(std::string const& unit_name, std::vector<char> &data) {
    auto file_path = dir_path_ / unit_name;
    std::uintmax_t size;
    try {
        size = file_size(file_path.string());
    } catch (std::exception &e) {
        LOG("Failed to read state save data size for unit '" + unit_name + "' in file '" + file_path.string() + "'", LogLevel::WARNING);
        throw;
    }
    std::ifstream stream(file_path.string(), std::ios_base::binary);
    if (!stream) {
        LOG("Failed to open state save data for unit '" + unit_name + "' in file '" + file_path.string() + "'", LogLevel::WARNING);
        throw;
    }
    try {
        data.resize(size);
        stream.read(data.data(), size);
    } catch (std::exception &e) {
        LOG("Failed to read state save data for unit '" + unit_name + "' in file '" + file_path.string() + "'", LogLevel::WARNING);
        throw;
    }
}

File_Per_Unit_Loader::File_Per_Unit_Loader(std::string dir_path)
    : dir_path_(dir_path)
{

}

std::shared_ptr<State_Snapshot_Loader> File_Per_Unit_Loader::initialize_snapshot()
{
    return std::make_shared<File_Per_Unit_Snapshot_Loader>(path(dir_path_));
}

std::shared_ptr<State_Snapshot_Loader> File_Per_Unit_Loader::initialize_checkpoint_snapshot(State_Saver::snapshot_time_t epoch)
{
    path checkpoint_path = path(dir_path_) / unit_saving_utils::format_epoch(epoch);;
    return std::make_shared<File_Per_Unit_Snapshot_Loader>(checkpoint_path);
}

