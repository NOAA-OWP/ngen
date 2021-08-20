
#ifndef NGEN_CONSTANTS_H
#define NGEN_CONSTANTS_H

// "Standard" Atmosphere pressure
#define STANDARD_ATMOSPHERIC_PRESSURE_PASCALS 101325
// Units for water specific weight value: Newtons / meters^3
#define WATER_SPECIFIC_WEIGHT 9810

/**
 * Sentinel for indicating something that could be handled locally or remotely is remote.
 *
 * A sentinel that indicates that some value that could potentially be the responsibility of local (in the context of
 * an MPI rank) entity is actually the responsibility of a remote entity in a different rank.  As such, generally the
 * value itself should not be directly used as it typically would.
 */
const int SENTINEL_IS_REMOTELY_HANDLED = -9999;

#endif //NGEN_CONSTANTS_H
