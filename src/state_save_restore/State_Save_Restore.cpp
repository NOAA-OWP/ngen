#include <state_save_restore/State_Save_Restore.hpp>

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
