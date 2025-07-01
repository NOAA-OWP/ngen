##
# @file partition_validator.py
#
# @brief This program validates an input partition file for use in the ngen framework
#
# @section TODO
# - Update remote sections to show full communication path source->nexus->destination
# - Update validate of remote sections to use full paths when present
#
# @section authors
# - Created 10/26/2021 donald.w.johnson@noaa.gov

import json
import argparse


def test_catchments_ids(data: dict):
    """This function check the catchment ids stored in each partition of data to ensure that catchments are all in only
        one partition

        @param data A dictionary containing a list of partition records stored with the key 'partitions'

        """
    cat_ids = {}

    # create a set of each partitions catchment_ids
    for partition in data['partitions']:
        cat_ids[partition["id"]] = set(partition["cat-ids"])

    cat_id_conflicts = 0

    # make sure that each catchment has a unique set of catchment_ids
    for key1, catchment_set1 in cat_ids.items():
        for key2, catchment_set2 in cat_ids.items():
            if key1 != key2:
                inter = cachment_set1.intersection(catchment_set2)
                if len(inter) > 0:
                    cat_id_conflicts = cat_id_conflicts + 1
                    print("partition " + str(key1) + " and partition " + str(key2) + " both contain" + str(inter))

    return cat_id_conflicts


def test_nexus_ids(data: dict):
    """This function check the nexus ids in the recorded partitions to determine the set of remote nexi

        @param data A dictionary containing a list of partition records stored with the key 'partitions'

        """
    nex_ids = {}

    # create a set of each partitions catchment_ids
    for partition in data['partitions']:
        nex_ids[partition["id"]] = set(partition["nex-ids"])

    remote_nexus_ids = set()

    # find the remote nexi by finding all nexus ids that are more than one partition
    for key1, catchment_set1 in nex_ids.items():
        for key2, catchment_set2 in nex_ids.items():
            if key1 != key2:
                inter = cachment_set1.intersection(catchment_set2)
                if len(inter) > 0:
                    remote_nexus_ids = remote_nexus_ids.union(inter);

    return remote_nexus_ids


def test_remote_connections(data: dict):
    """This function checks to ensure that there exist a minal set of reciving remote connection sections, such
       that MPI send and recives will pair properly

        @param data A dictionary containing a list of partition records stored with the key 'partitions'

        TODO update logic when partition files encode full communication path in recmote sections

        """
    partitions = data['partitions']

    mpi_rank = -1;
    err_count = 0
    for partition in partitions:
        mpi_rank = mpi_rank + 1
        for rc in partition['remote-connections']:

            # get references about the current remote connection
            target_rank = rc["mpi-rank"]
            nex_id = rc["nex-id"]
            cat_id = rc["cat-id"]
            cat_dir = rc["cat-direction"]

            # currently only check send from upstream to downstream
            if cat_dir == "orig_cat-to-nex":
                continue

            try:
                # get a reference to the target partition based on the specified mpi rank
                target_part = next(x for x in partitions if x["id"] == target_rank)

                # get the list of remote connections
                target_connections = target_part['remote-connections']

                # find the matching remote connection
                matches = [x for x in target_connections
                           if x["nex-id"] == nex_id and
                           x["mpi-rank"] == mpi_rank and
                           cat_dir != x["cat-direction"]]
                if len(matches) > 0:
                    # any number of matching records will ensure the needed receive is generated
                    pass
                else:
                    # we did not find any matching record this is an un matched send
                    err_count = err_count + 1
                    print("No Match")
                    print("src = " + str(rc) + " partition-id = " + str(mpi_rank))
                    print(target_connections)

            except StopIteration:
                # if we get this exception then the there was a remote connection to an mpi rank that was not found
                print("Could not find partition with id = " + str(target_rank))
                err_count = err_count + 1

    # return the number of remote connections that did not validate
    return err_count


def main():
    """
    Main program

    Handle command line
    Load datafile
    run tests
    """

    #setup the arg parser
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--partitions', type=argparse.FileType('r'))
    args = parser.parse_args()

    # an input datafile is required
    if args.partitions is None:
        parser.error("--partitions is required.")

    # load the json data file
    data = json.load(args.partitions)

    # test for duplicate catchment ids
    num_cat_conflict = test_catchments_ids(data);
    if num_cat_conflict == 0:
        print("No catchment id duplications found")

    # test nexus ids and find remote nexus ids
    remote_nexus_ids = test_nexus_ids(data)
    if len(remote_nexus_ids) <= 20:
        print("Remote nexus ids = " + str(remote_nexus_ids))

    # test remote connections
    rc_errors = test_remote_connections(data)
    if rc_errors > 0:
        print("There where " + str(rc_errors) + "errors in the remote connections")
    else:
        print("No errors found in remote connections")


if __name__ == "__main__":
    main()
