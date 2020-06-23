#ifndef NGEN_HYMODERRORCODES_H
#define NGEN_HYMODERRORCODES_H

/**
 * Enum for codes representing conditions resulting from running Hymod model.
 */
enum HymodErrorCodes
{

    NO_ERROR = 0,               //!< Model run was successful and without error.
    MASS_BALANCE_ERROR = 100    //!< Model run results for a time step did not appropriately conserve mass.
};

#endif //NGEN_HYMODERRORCODES_H
