#!/bin/sh
# Stands in for the partitionGenerator program, which has been removed from ngen.
cat >&2 <<'MESSAGE'
partitionGenerator has been removed from ngen. The partitions it made required
remote nexus connections between MPI ranks, which ngen no longer supports.

Instead, either:
  - Let ngen partition the hydrofabric automatically: run it under MPI with no
    partition config path, or with '-' in its place.
  - Use a separate tool that writes a partition config with no remote
    connections, in which every nexus is in the same partition as all of the
    catchments that drain to it.

See doc/DISTRIBUTED_PROCESSING.md in the ngen source for details.
MESSAGE
exit 1
