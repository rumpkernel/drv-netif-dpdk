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

( cd buildrump.sh && ./buildrump.sh -T ${RUMPTOOLS} -d ${RUMPDEST} \
    checkout fullbuild ) || die buildrump.sh failed
echo bar
( cd dpdk ; make T=$(uname -m)-default-linuxapp-gcc config && make ) \
    || die dpdk build failed
echo foo
( cd src && ${RUMPMAKE} dependall && ${RUMPMAKE} install ) \
    || dpdkif build failed

touch .build_done
