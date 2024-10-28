from pathlib import Path
import argparse
import os
import polars as pl
from collections import defaultdict


def sum_csv_files(file_pattern: str):
    # Scan CSV files and assign column names
    df = pl.scan_csv(file_pattern, has_header=False, new_columns=["index", "timestamp", "value"])
    # Group by timestamp and sum the values
    df = df.group_by("index").agg(
        pl.col("timestamp").first().str.strip_chars().alias("timestamp"),
        pl.col("value").str.strip_chars().cast(pl.Float64).sum().alias("total_value"),
    )

    return df.sort("index")


def merge_outputs(output_path: Path) -> None:
    # get all the file names in the folder
    output_files = list(output_path.glob("*_rank_*.csv"))
    # print(output_files)
    # parse out nexus id from the file names
    total_files = len(output_files)
    # sort the files

    nexuse_counts = defaultdict(int)

    for file in output_files:
        nexus_id = file.stem.split("_")[0]
        nexuse_counts[nexus_id] += 1

    for file in output_files:
        nexus_id = file.stem.split("_")[0]
        if nexuse_counts[nexus_id] == 1:
            os.rename(file, output_path / f"{nexus_id}_output.csv")

    for nexus, count in nexuse_counts.items():
        if count > 1:
            df = sum_csv_files(f"{output_path}/{nexus}_rank_*.csv")
            df.collect().write_csv(f"{output_path}/{nexus}_output.csv", include_header=False)
            for file in output_path.glob(f"{nexus}_rank_*.csv"):
                file.unlink()
            # use sed to add the spaces in the csv files
            os.system(f"sed -i 's/,/, /g' {output_path}/{nexus}_output.csv")

    # delete the files that were merged


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Merge the per rank outputs created by running ngen with round robin partitioning."
    )
    parser.add_argument("output_path", type=Path, help="ngen output folder")

    args = parser.parse_args()

    merge_outputs(args.output_path)
