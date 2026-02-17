#ifndef NGEN_STATE_SAVE_UTILS_HPP
#define NGEN_STATE_SAVE_UTILS_HPP

namespace StateSaveNames {
    const auto CREATE = "serialization_create";
    const auto STATE = "serialization_state";
    const auto FREE = "serialization_free";
    const auto SIZE = "serialization_size";
    const auto RESET = "reset_time";
}

enum class State_Save_Direction {
    None = 0,
    Save,
    Load
};

enum class State_Save_Mechanism {
    None = 0,
    FilePerUnit
};

enum class State_Save_When {
    None = 0,
    EndOfRun,
    FirstOfMonth,
    StartOfRun
};

#endif
