import json
from pathlib import Path
import sqlite3
import argparse
import multiprocessing


def get_cat_to_nex_flowpairs(hydrofabric: Path) -> list:
    with sqlite3.connect(f"{hydrofabric}") as conn:
        cursor = conn.cursor()
        cursor.execute("SELECT divide_id, toid FROM divides")
        cat_to_nex_pairs = cursor.fetchall()
    return cat_to_nex_pairs


def create_partitions(hydrofabric: Path, output_path: Path, num_partitions: int = None) -> None:
    if num_partitions is None:
        num_partitions = multiprocessing.cpu_count()

    cat_to_nex_pairs = get_cat_to_nex_flowpairs(hydrofabric)

    # sort the cat nex tuples by the nex id
    cat_to_nex_pairs = sorted(cat_to_nex_pairs, key=lambda x: x[1])

    num_partitions = min(num_partitions, len(cat_to_nex_pairs))

    cats = set([cat for cat, _ in cat_to_nex_pairs])
    nexs = set([nex for _, nex in cat_to_nex_pairs])
    print(f"Number of partitions: {num_partitions}")
    print(f"Number of cats: {len(cats)}")
    print(f"Number of nexus: {len(nexs)}")

    partitions = []
    for i in range(num_partitions):
        part = {}
        part["id"] = i
        part["cat-ids"] = []
        part["nex-ids"] = []
        part["remote-connections"] = []
        partitions.append(part)

    for i, (cat_id, nex_id) in enumerate(cat_to_nex_pairs):
        print(i)
        part_id = i % num_partitions
        partitions[part_id]["cat-ids"].append(cat_id)
        partitions[part_id]["nex-ids"].append(nex_id)

    with open(output_path, "w") as f:
        f.write(json.dumps({"partitions": partitions}, indent=4))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create partitions from hydrofabric data.")
    parser.add_argument("hydrofabric", type=Path, help="Path to the hydrofabric SQLite file.")
    parser.add_argument("output_path", type=Path, help="Path to the output JSON file.")
    parser.add_argument(
        "-n", "--num_partitions", type=int, default=None, help="Number of partitions to create."
    )

    args = parser.parse_args()

    create_partitions(args.hydrofabric, args.output_path, args.num_partitions)
