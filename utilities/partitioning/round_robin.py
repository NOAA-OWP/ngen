import argparse
import json
import multiprocessing
import sqlite3
from pathlib import Path
from typing import List, Tuple


def get_cat_to_nex_flowpairs(hydrofabric: Path) -> List[Tuple]:
    sql_query = "SELECT divide_id, toid FROM divides"
    try:
        con = sqlite3.connect(str(hydrofabric.absolute()))
        edges = con.execute(sql_query).fetchall()
        con.close()
    except sqlite3.Error as e:
        print(f"SQLite error: {e}")
        raise
    unique_edges = list(set(edges))
    return unique_edges


def create_partitions(
    geopackage_path: Path, num_partitions: int = None, output_folder: Path = None
) -> int:
    """
    The partitioning algorithm is as follows:
    1. Get the list of catchments and their corresponding nexus
    2. loop over the pairs adding them evenly across partitions
    This breaks connectivity, uses no mpi communication and will likely cause corruption of nexus output files.
    If reading outputs from catchment files in t-route, this partitioning allows for much better work distribution
    of smaller networks.
    2x the number of cpus as partitions can give faster total execution time if partition compute time is still uneven.
    """

    if output_folder is None:
        output_folder = Path.cwd()
    else:
        output_folder = Path(output_folder)

    if num_partitions is None:
        num_partitions = multiprocessing.cpu_count()

    cat_to_nex_pairs = get_cat_to_nex_flowpairs(geopackage_path)
    num_cats = len(cat_to_nex_pairs)

    num_partitions = min(num_partitions, num_cats)

    partitions = []
    for i in range(num_partitions):
        part = {}
        part["id"] = i
        part["cat-ids"] = []
        part["nex-ids"] = []
        part["remote-connections"] = []
        partitions.append(part)

    i = 0
    while True:
        cat, nex = cat_to_nex_pairs.pop(0)
        partitions[i]["cat-ids"].append(cat)
        partitions[i]["nex-ids"].append(nex)
        i += 1
        if i >= num_partitions:
            i = 0
        if len(cat_to_nex_pairs) == 0:
            break
    with open(output_folder / f"partitions_{num_partitions}.json", "w") as f:
        f.write(json.dumps({"partitions": partitions}, indent=4))

    return num_partitions


def main():
    parser = argparse.ArgumentParser(description="Create partitions for hydrofabric.")
    parser.add_argument("path", type=str, help="The file path to the hydrofabric.")
    parser.add_argument("num_partitions", type=int, help="The desired number of partitions.")
    parser.add_argument("output_folder", type=str, help="The path of the folder output in.")
    args = parser.parse_args()

    paths = Path(args.path)

    actual_num_partitions = create_partitions(paths, args.num_partitions, args.output_folder)
    print(f"Actual number of partitions: \n{actual_num_partitions}")


if __name__ == "__main__":
    main()
