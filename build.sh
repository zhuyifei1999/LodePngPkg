#! /bin/bash

set -e

# https://stackoverflow.com/a/246128
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd "$DIR"

if [[ -z "$EDK2_DIR" ]]; then
    if [[ -d /usr/lib/edk2 ]]; then
        export EDK2_DIR=/usr/lib/edk2
    else
        echo 'Can not find EDK2 directory. Please clone https://github.com/tianocore/edk2 and set $EDK2_DIR.' >&2
        exit 1
    fi
fi

if ! [[ -e LodePngPkg/lodepng/lodepng.cpp ]]; then
    git submodule update --init
fi
if ! [[ -e LodePngPkg/lodepng.c ]]; then
    ln -sf lodepng/lodepng.cpp LodePngPkg/lodepng.c
fi

if ! [[ -x "${EDK2_DIR}/BaseTools/Source/C/bin/GenFw" ]]; then
    make -C "${EDK2_DIR}/BaseTools"
fi

mkdir -p Conf

export WORKSPACE="${PWD}"
export EDK_TOOLS_PATH="${EDK2_DIR}/BaseTools"

ARGS=("$@")
shift $#

source "${EDK2_DIR}/edksetup.sh"

ln -sf "${EDK2_DIR}/MdePkg" .

build -b RELEASE -p LodePngPkg/LodePngPkg.dsc "${ARGS[@]}"
