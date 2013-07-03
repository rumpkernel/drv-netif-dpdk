TCP/IP stack for DPDK
=====================

The hypercall driver in this repository attaches a
[rump kernel](http://www.netbsd.org/docs/rump/) TCP/IP
stack to a network interface card accessed via the Intel Data Plane
Development Kit [DPDK](http://dpdk.org/).


Status
------

The driver has been tested to work and is able to exchange TCP traffic
with an Internet peer.

Future plans include benchmarking, performance optimization, and
improved configurability.

A [wiki page](https://github.com/anttikantee/dpdk-rumptcpip/wiki/Optimizing-performance) documents some ideas for potential performance optimizations.


Instructions
------------

To use, in addition to a working DPDK installation you need the rump
kernel TCP/IP stack components.  The easiest way to obtain the rump
kernel components is to use
[buildrump.sh](https://github.com/anttikantee/buildrump.sh).  The
procedure is follows.

* clone the buildrump.sh git repo and run `./buildrump.sh checkout fullbuild`
* in `src/libdpdkif` of this repo, edit the parameters at the top of
  rumpcomp_user.c, e.g. the interface port to be used.
* still in `src/libdpdkif`, run `rumpmake dependall && rumpmake install`.
  (note, you must have `rumpmake` in your path.  See the buildrump.sh
  repo for more information on rumpmake)

For more information on how to use the resulting userspace TCP/IP stack,
see e.g. the [buildrump.sh repo](https://github.com/anttikantee/buildrump.sh)
or the [page on rump kernels](http://www.netbsd.org/docs/rump/).
