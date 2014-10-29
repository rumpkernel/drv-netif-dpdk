#!/bin/sh

die ()
{

	echo ERROR: $*
	exit 1
}

: ${RTE_SDK:=$(pwd)/dpdk}
: ${RTE_TARGET:=build}
export RTE_SDK
export RTE_TARGET

RUMPTOOLS=$(pwd)/rumptools
RUMPMAKE=${RUMPTOOLS}/rumpmake
RUMPDEST=$(pwd)/rump

set -e
( cd buildrump.sh && ./buildrump.sh -T ${RUMPTOOLS} -d ${RUMPDEST} -q \
    checkout fullbuild || die buildrump.sh failed )
( cd dpdk ; make T=$(uname -m)-native-linuxapp-gcc config && make \
    || die dpdk build failed )
( cd src && ${RUMPMAKE} dependall && ${RUMPMAKE} install \
    || die dpdkif build failed )
( cd examples && ${RUMPMAKE} dependall && ${RUMPMAKE} install \
    || die examples build failed )

touch .build_done
