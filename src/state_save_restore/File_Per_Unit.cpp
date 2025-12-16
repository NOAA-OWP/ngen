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

// This class is only declared and defined here, in the .cpp file,
// because it is strictly an implementation detail of the top-level
// File_Per_Unit_Saver class
class File_Per_Unit_Snapshot_Saver : public State_Snapshot_Saver
{
    friend class File_Per_Unit_Saver;

    public:
    File_Per_Unit_Snapshot_Saver() = delete;
    File_Per_Unit_Snapshot_Saver(path base_path, State_Saver::snapshot_time_t epoch, State_Saver::State_Durability durability);
    ~File_Per_Unit_Snapshot_Saver();

public:
    void save_unit(std::string const& unit_name, boost::span<char const> data) override;
    void finish_saving() override;

private:
    std::string format_epoch(State_Saver::snapshot_time_t epoch);

    path dir_path_;
};

File_Per_Unit_Saver::File_Per_Unit_Saver(std::string base_path)
    : base_path_(std::move(base_path))
{
    auto dir_path = path(base_path_);
    create_directories(dir_path);
}

File_Per_Unit_Saver::~File_Per_Unit_Saver() = default;

std::shared_ptr<State_Snapshot_Saver> File_Per_Unit_Saver::initialize_snapshot(snapshot_time_t epoch, State_Durability durability)
{
    return std::make_shared<File_Per_Unit_Snapshot_Saver>(base_path_, epoch, durability);
}

void File_Per_Unit_Saver::finalize()
{
    // nothing to be done
}

File_Per_Unit_Snapshot_Saver::File_Per_Unit_Snapshot_Saver(path base_path, State_Saver::snapshot_time_t epoch, State_Saver::State_Durability durability)
    : State_Snapshot_Saver(epoch, durability)
    , dir_path_(base_path / format_epoch(epoch))
{
    create_directory(dir_path_);
}

File_Per_Unit_Snapshot_Saver::~File_Per_Unit_Snapshot_Saver() = default;

std::string File_Per_Unit_Snapshot_Saver::format_epoch(State_Saver::snapshot_time_t epoch)
{
    time_t t = std::chrono::system_clock::to_time_t(epoch);
    std::tm tm = *std::gmtime(&t);

    std::stringstream tss;
    tss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return tss.str();
}

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
