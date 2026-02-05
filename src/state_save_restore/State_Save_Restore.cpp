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

    bool hot_start = false;
    for (const auto& saving_config : *maybe) {
        try {
            auto& subtree = saving_config.second;
            auto direction = subtree.get<std::string>("direction");
            auto what = subtree.get<std::string>("label");
            auto where = subtree.get<std::string>("path");
            auto how = subtree.get<std::string>("type");
            auto when = subtree.get<std::string>("when");

            instance i{direction, what, where, how, when};
            if (i.timing_ == State_Save_When::StartOfRun && i.direction_ == State_Save_Direction::Load) {
                if (hot_start)
                    throw std::runtime_error("Only one hot start state saving configuration is allowed.");
                hot_start = true;
            }
            instances_.push_back(i);
        } catch (std::exception &e) {
            LOG("Bad state saving config: " + std::string(e.what()), LogLevel::WARNING);
            throw;
        }
    }

    LOG("State saving configured", LogLevel::INFO);
}

std::vector<std::pair<std::string, std::shared_ptr<State_Loader>>> State_Save_Config::start_of_run_loaders() const {
    std::vector<std::pair<std::string, std::shared_ptr<State_Loader>>> loaders;
    for (const auto &i : this->instances_) {
        if (i.timing_ == State_Save_When::StartOfRun && i.direction_ == State_Save_Direction::Load) {
            if (i.mechanism_ == State_Save_Mechanism::FilePerUnit) {
                auto loader = std::make_shared<File_Per_Unit_Loader>(i.path_);
                auto pair = std::make_pair(i.label_, loader);
                loaders.push_back(pair);
            } else {
                LOG(LogLevel::WARNING, "State_Save_Config: Loading mechanism " + i.mechanism_string() + " is not supported for start of run loading.");
            }
        }
    }
    return loaders;
}

std::vector<std::pair<std::string, std::shared_ptr<State_Saver>>> State_Save_Config::end_of_run_savers() const {
    std::vector<std::pair<std::string, std::shared_ptr<State_Saver>>> savers;
    for (const auto &i : this->instances_) {
        if (i.timing_ == State_Save_When::EndOfRun && i.direction_ == State_Save_Direction::Save) {
            if (i.mechanism_ == State_Save_Mechanism::FilePerUnit) {
                auto saver = std::make_shared<File_Per_Unit_Saver>(i.path_);
                auto pair = std::make_pair(i.label_, saver);
                savers.push_back(pair);
            } else {
                LOG(LogLevel::WARNING, "State_Save_Config: Saving mechanism " + i.mechanism_string() + " is not supported for start of run saving.");
            }
        }
    }
    return savers;
}

std::unique_ptr<State_Loader> State_Save_Config::hot_start() const {
    for (const auto &i : this->instances_) {
        if (i.direction_ == State_Save_Direction::Load && i.timing_ == State_Save_When::StartOfRun) {
            if (i.mechanism_ == State_Save_Mechanism::FilePerUnit) {
                return std::make_unique<File_Per_Unit_Loader>(i.path_);
            } else {
                LOG(LogLevel::WARNING, "State_Save_Config: Saving mechanism " + i.mechanism_string() + " is not supported for start of run saving.");
            }
        }
    }
    return std::unique_ptr<State_Loader>();
}

State_Save_Config::instance::instance(std::string const& direction, std::string const& label, std::string const& path, std::string const& mechanism, std::string const& timing)
    : label_(label)
    , path_(path)
{
    if (direction == "save") {
        direction_ = State_Save_Direction::Save;
    } else if (direction == "load") {
        direction_ = State_Save_Direction::Load;
    } else {
        Logger::logMsgAndThrowError("Unrecognized state saving direction '" + direction + "'");
    }

    if (mechanism == "FilePerUnit") {
        mechanism_ = State_Save_Mechanism::FilePerUnit;
    } else {
        Logger::logMsgAndThrowError("Unrecognized state saving mechanism '" + mechanism + "'");
    }

    if (timing == "EndOfRun") {
        timing_ = State_Save_When::EndOfRun;
    } else if (timing == "FirstOfMonth") {
        timing_ = State_Save_When::FirstOfMonth;
    } else if (timing == "StartOfRun") {
        timing_ = State_Save_When::StartOfRun;
    } else {
        Logger::logMsgAndThrowError("Unrecognized state saving timing '" + timing + "'");
    }
}

std::string State_Save_Config::instance::instance::mechanism_string() const {
    switch (mechanism_) {
        case State_Save_Mechanism::None:
            return "None";
        case State_Save_Mechanism::FilePerUnit:
            return "FilePerUnit";
        default:
            return "Other";
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
