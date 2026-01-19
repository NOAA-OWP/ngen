#ifndef NGEN_FILE_PER_UNIT_HPP
#define NGEN_FILE_PER_UNIT_HPP

#include <state_save_restore/State_Save_Restore.hpp>

class File_Per_Unit_Saver : public State_Saver
{
public:
    File_Per_Unit_Saver(std::string base_path);
    ~File_Per_Unit_Saver();

    std::shared_ptr<State_Snapshot_Saver> initialize_snapshot(snapshot_time_t epoch, State_Durability durability) override;

    void finalize() override;

private:
    std::string base_path_;
};


class File_Per_Unit_Loader : public State_Loader
{
public:
    File_Per_Unit_Loader(std::string dir_path);
    ~File_Per_Unit_Loader() = default;

    void finalize() override { };

    std::shared_ptr<State_Snapshot_Loader> initialize_snapshot(State_Saver::snapshot_time_t epoch) override;
private:
    std::string dir_path_;
};

#endif // NGEN_FILE_PER_UNIT_HPP
