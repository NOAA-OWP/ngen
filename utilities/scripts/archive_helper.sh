#!/bin/bash

#huc01_demo
#huc01_simple_conf

DATA_BASE_DIR="data"

EXAMPLE[1]="huc01_examples"
EXAMPLE_SUBDIRS[1]="huc01_demo huc01_simple_conf"

EXAMPLE[2]="sugar_creek"
EXAMPLE_SUBDIRS[2]="sugar_creek sugar_creek_simple"

MODE_ARCHIVE_KEY="archive"
MODE_EXTRACT_KEY="extract"


#tar -rf "${ARCHIVE_1}.tar" data/config/realization_config/huc01_demo

build_find_names_clause()
{
    local _STR=""

    [ ${#} -eq 0 ] && echo "Error: can't build find names clause without any names" && exit 1

    for n in ${@}; do
        if [[ ${_STR} == "" ]]; then
            _STR="( -name ${n}"
        else
            _STR="${_STR} -o -name ${n}"
        fi
        shift
    done

    _STR="${_STR} )"
    echo "${_STR}"
    #\( -name huc01_demo -o -name huc01_simple_conf \)
}

build_archive()
{
    local _ARCHIVE_NAME="${1:?}"
    local _FIND_NAMES="$(build_find_names_clause ${@:2})"

    #find ${DATA_BASE_DIR} -type d ${_FIND_NAMES}
    if [ -e ${_ARCHIVE_NAME} ]; then
        find ${DATA_BASE_DIR} -type d ${_FIND_NAMES} -exec bash -c "tar -uf ${_ARCHIVE_NAME} {}" \; -exec bash -c "find {} -type f -exec rm \{\} \;" \;
        #find ${DATA_BASE_DIR} -type d ${_FIND_NAMES}
    else
        find ${DATA_BASE_DIR} -type d ${_FIND_NAMES} -exec bash -c "tar -rf ${_ARCHIVE_NAME} {}" \; -exec bash -c "find {} -type f -exec rm \{\} \;" \;
    fi
}

extract_archive()
{
    if [ ! -d ${DATA_BASE_DIR} ]; then
        echo "Error: you do not appear to be in the top-level directory; move there before extracting" >&2
        exit 1
    elif [ ! -e ${1:?No archive argument supplied to extract} ]; then
        echo "Error: archive file '${1}' does not exist" >&2
        exit 1
    fi
    # TODO: account for compressed later
    tar xf ${1}
}

#build_find_names_clause huc01_demo huc01_simple_conf
#build_find_names_clause "huc01_demo huc01_simple_conf"
#build_archive build_find_names_clause huc01_demo huc01_simple_conf

do_list()
{
    for ex in "${EXAMPLE[@]}"; do
        echo "${ex}"
    done
}

usage()
{
    echo "TODO"
}

while [ ${#} -gt 0 ]; do
    case "${1}" in
        --archive-output-dir)
            ARCHIVE_OUTPUT_DIR="${2}"
            shift
            ;;
        --compress|-c)
            DO_COMPRESS="true"
            ;;
        --example-number|-n)
            EXAMPLE_NUMBER=${2}
            shift
            ;;
        --archive|-a)
            MODE="${MODE_ARCHIVE_KEY}"
            ;;
        --extract|-e)
            MODE="${MODE_EXTRACT_KEY}"
            ;;
        --list-examples|-l)
            do_list
            exit
            ;;
        *)
            usage
            exit 1
            ;;
    esac
    shift
done

# Set defaults
[ -z "${MODE:-}" ] && MODE=${MODE_ARCHIVE_KEY}
[ -z "${EXAMPLE_NUMBER:-}" ] && EXAMPLE_NUMBER=1
[ -z "${ARCHIVE_DIR:-}" ] && ARCHIVE_DIR="."

[ ! -d ${ARCHIVE_DIR} ] && echo "Error: supplied directory for archive file does not exist" && exit 1

ARCHIVE_NAME="${ARCHIVE_DIR}/${EXAMPLE[EXAMPLE_NUMBER]}.tar"

if [ "${MODE}" == "${MODE_ARCHIVE_KEY}" ]; then
    build_archive ${ARCHIVE_NAME} ${EXAMPLE_SUBDIRS[EXAMPLE_NUMBER]}
else
    extract_archive ${ARCHIVE_NAME}
fi
