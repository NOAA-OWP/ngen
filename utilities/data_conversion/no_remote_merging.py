from pathlib import Path
import argparse
import os
import polars as pl
from collections import defaultdict
from functools import partial
import multiprocessing as mp

def sum_csv_files(file_pattern: str):
    # Scan CSV files and assign column names
    df = pl.scan_csv(file_pattern, has_header=False, new_columns=["index", "timestamp", "value"])
    # Group by timestamp and sum the values
    df = df.group_by("index").agg(
        pl.col("timestamp").first().str.strip_chars().alias("timestamp"),
        pl.col("value").str.strip_chars().cast(pl.Float64).sum().alias("total_value"),
    )

    return df.sort("index")

def process_pair(output_path:Path, tup):
    nexus, count = tup
    if count > 1:
            df = sum_csv_files(output_path / f"{nexus}_rank_*.csv")
            df.collect().write_csv(output_path / f"{nexus}_output.csv", include_header=False)
            for file in output_path.glob(f"{nexus}_rank_*.csv"):
                file.unlink()
            # use sed to add the spaces in the csv files
            output_file = output_path / f"{nexus}_output.csv"
            os.system(f"sed -i 's/,/, /g' {output_file.absolute()}")


def merge_outputs(output_path: Path) -> None:
    # get all the file names in the folder
    output_files = list(output_path.glob("*_rank_*.csv"))
    # print(output_files)
    # parse out nexus id from the file names
    total_files = len(output_files)
    # sort the files

    nexus_counts = defaultdict(int)

    for file in output_files:
        nexus_id = file.stem.split("_")[0]
        nexus_counts[nexus_id] += 1

    for file in output_files:
        nexus_id = file.stem.split("_")[0]
        if nexus_counts[nexus_id] == 1:
            os.rename(file, output_path / f"{nexus_id}_output.csv")

    partial_process = partial(process_pair, output_path)
    with mp.Pool() as p:
        p.map(partial_process,nexus_counts.items())



if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Merge the per rank outputs created by running ngen with round robin partitioning."
    )
    parser.add_argument("output_path", type=Path, help="ngen output folder")

    args = parser.parse_args()

    merge_outputs(args.output_path)
