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

    //auto saving_config = *maybe;
    for (const auto& saving_config : *maybe) {
        try {
            auto& subtree = saving_config.second;
            auto direction = subtree.get<std::string>("direction");
            auto what = subtree.get<std::string>("label");
            auto where = subtree.get<std::string>("path");
            auto how = subtree.get<std::string>("type");
            auto when = subtree.get<std::string>("when");

            instance i{direction, what, where, how, when};
            instances_.push_back(i);
        } catch (...) {
            LOG("Bad state saving config", LogLevel::WARNING);
            throw;
        }
    }

    LOG("State saving configured", LogLevel::INFO);
}

int State_Save_Config::end_of_run() const {
    for (size_t i = 0; i < instances_.size(); ++i) {
        auto &instance = instances_[i];
        if (instance.timing_ == State_Save_When::EndOfRun && instance.direction_ == State_Save_Direction::Save) {
            return i;
        }
    }
    return -1;
}

bool State_Save_Config::has_end_of_run() const {
    return this->end_of_run() >= 0;
}

std::shared_ptr<State_Saver> State_Save_Config::end_of_run_saver() const {
    int index = this->end_of_run();
    if (index >= 0) {
        const auto& i = instances_[index];
        if (i.mechanism_ == State_Save_Mechanism::FilePerUnit) {
            return std::make_shared<File_Per_Unit_Saver>(i.path_);
        } else {
            Logger::logMsgAndThrowError("State_Save_Config: Saving mechanism " + i.mechanism_string() + " is not supported for end of run saving.");
        }
    }
    auto error = "State_Save_Config: No end of run was defined in the realization config.";
    LOG(LogLevel::SEVERE, error);
    throw std::runtime_error(error);
}

int State_Save_Config::cold_start() const {
    for (size_t i = 0; i < instances_.size(); ++i) {
        const auto& instance = instances_[i];
        if (instance.timing_ == State_Save_When::StartOfRun && instance.direction_ == State_Save_Direction::Load) {
            return i;
        }
    }
    return -1;
}

bool State_Save_Config::has_cold_start() const {
    return this->cold_start() >= 0;
}

std::shared_ptr<State_Loader> State_Save_Config::cold_start_loader() const {
    int index = this->cold_start();
    if (index >= 0) {
        const auto& i = instances_[index];
        if (i.mechanism_ == State_Save_Mechanism::FilePerUnit) {
            return std::make_shared<File_Per_Unit_Loader>(i.path_);
        } else {
            Logger::logMsgAndThrowError("State_Save_Config: Saving mechanism " + i.mechanism_string() + " is not supported for end of run saving.");
        }
    }
    auto error = "State_Save_Config: No configuration was found for loading a cold start.";
    LOG(LogLevel::SEVERE, error);
    throw std::runtime_error(error);
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
