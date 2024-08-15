#!/bin/bash

NGEN_BASE="/ngen-app/ngen"
NGEN_EXE_PATH="$NGEN_BASE/cmake_build/ngen"
NGEN_DATA_PATH="$NGEN_BASE/data"
NGEN_EXTERN_PATH="$NGEN_BASE/extern"
RUN_PATH="/ngen-app/run"
OUTPUT_PATH="/ngencerf/data/ngen-run-logs"



function exit_script () {
    if [ -n "$2" ] ; then
        echo "$2"
    fi
    popd &> /dev/null
    exit $1
}

timestamp=$(date --utc  +"%Y-%m-%dT%H:%M:%S")

# create temp /run folder
temp_run_path="$RUN_PATH/ngen_$timestamp/"
echo "DEBUG: creating folder [$temp_run_path]"
mkdir --parents "$temp_run_path" || \
    exit_script 1 "ERROR: unable to create temp ngen run directory [$temp_run_path]"

pushd "$temp_run_path" > /dev/null

# create sym-links
echo "DEBUG: creating sym-links in folder [$temp_run_path]"
ln --symbolic "$NGEN_EXE_PATH" || \
    exit_script 1 "ERROR: unable to create sym-link for [$NGEN_EXE_PATH] in directory [$temp_run_path]"
ln --symbolic "$NGEN_DATA_PATH" || \
    exit_script 1 "ERROR: unable to create sym-link for [$NGEN_DATA_PATH] in directory [$temp_run_path]"
ln --symbolic "$NGEN_EXTERN_PATH" || \
    exit_script 1 "ERROR: unable to create sym-link for [$NGEN_EXTERN_PATH] in directory [$temp_run_path]"

# run ngen and tee output to log file
echo "DEBUG: Running ngen. Command line: [\"./ngen $@ \"]"
./ngen $@ |& tee ngen_output.log

# push results to output folder
output_dir="$OUTPUT_PATH/ngen_$timestamp"
echo "DEBUG: creating folder [$output_dir]"
mkdir --parents "$output_dir" || \
    exit_script 1 "ERROR: unable to create ngen run output directory [$output_dir]"
echo "DEBUG: Saving output data and logs to [$output_dir]"
if [ -n $(ls *.log *.csv &> /dev/null) ] ; then
    rsync --archive --ignore-missing-args *.csv *.log "$output_dir" || \
        exit_script 1 "ERROR: unable to copy output data and logs to [$output_dir]"
fi

# perform cleanup
echo "DEBUG: deleting folder [$temp_run_path]"
rm --force --recursive "$temp_run_path" || \
    echo "WARN: unable to delete temp ngen run directory [$temp_run_path]"

exit_script 0 "INFO: NGEN run complete, data saved to [$output_dir]"

