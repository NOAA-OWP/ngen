#include <state_save_restore/State_Save_Restore.hpp>
#include <state_save_restore/File_Per_Unit.hpp>

#include <utilities/Logger.hpp>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

#include <string>

State_Save_Config::State_Save_Config(boost::property_tree::ptree const& tree)
{
    auto maybe = tree.get_child_optional("state_saving");

    // Default initialization will represent the "not enabled" case
    if (!maybe) {
        LOG("State saving not configured", LogLevel::INFO);
        return;
    }

    auto subtree = *maybe;

    try {
        auto single_what = subtree.get<std::string>("label");
        auto single_where = subtree.get<std::string>("path");
        auto single_how = subtree.get<std::string>("type");
        auto single_when = subtree.get<std::string>("when");

        instance i{single_what, single_where, single_how, single_when};
        instances_.push_back(i);
    } catch (...) {
        LOG("Bad state saving config", LogLevel::WARNING);
        throw;
    }

    LOG("State saving configured", LogLevel::INFO);
}

bool State_Save_Config::has_end_of_run() const {
    for (const auto& i : instances_)
        if (i.timing_ == "EndOfRun")
            return true;
    return false;
}

std::shared_ptr<State_Saver> State_Save_Config::end_of_run_saver() const {
    for (const auto& i : instances_) {
        if (i.timing_ == "EndOfRun") {
            if (i.mechanism_ == "FilePerUnit") {
                return std::make_shared<File_Per_Unit_Saver>(i.path_);
            } else {
                Logger::logMsgAndThrowError("State_Save_Config: Saving mechanism " + i.mechanism_ + " is not supported for end of run saving.");
            }
        }
    }
    Logger::logMsgAndThrowError("State_Save_Config: No end of run was defined in the realization config.");
}

State_Save_Config::instance::instance(std::string const& label, std::string const& path, std::string const& mechanism, std::string const& timing)
    : label_(label)
    , path_(path)
    , mechanism_(mechanism)
    , timing_(timing)
{
    if (mechanism_ == "FilePerUnit") {
        // nothing to do here
    } else {
        Logger::logMsgAndThrowError("Unrecognized state saving mechanism '" + mechanism_ + "'");
    }

    if (timing_ == "EndOfRun") {
        // nothing to do here
    } else if (timing_ == "FirstOfMonth") {
        // nothing to do here
    } else {
        Logger::logMsgAndThrowError("Unrecognized state saving timing '" + timing_ + "'");
    }
}

State_Snapshot_Saver::State_Snapshot_Saver(State_Saver::snapshot_time_t epoch, State_Saver::State_Durability durability)
    : epoch_(epoch)
    , durability_(durability)
{

}

State_Saver::snapshot_time_t State_Saver::snapshot_time_now() {
#if __cplusplus < 201703L // C++ < 17
    auto now = std::chrono::system_clock::now();
    return std::chrono::time_point_cast<std::chrono::seconds>(now);
#else
    return std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
#endif
}
