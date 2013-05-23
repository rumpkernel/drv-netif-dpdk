TCP/IP stack for DPDK
=====================

The hypercall driver in this repository attaches a rump kernel TCP/IP
stack to a network interface card accessed via the Intel Data Plane
Development Kit [DPDK](http://dpdk.org/).

To use, in addition to a working DPDK installation you need the rump
kernel TCP/IP stack components.  The easiest way to obtain them is to use
[buildrump.sh](https://github.com/anttikantee/buildrump.sh).  The
procedure is follows.  Clone the buildrump.sh git repo.
Run `./buildrump.sh checkout`.  Edit the parameters at the top of
rumpcomp_user.c (e.g. interface port to be used) and both that and the
Makefile in this repo to buildrump.sh `src/sys/rump/net/lib/libvirtif`.
Run `./buildrump.sh`.

For more information on how to use the resulting userspace TCP/IP stack,
see e.g. the [buildrump.sh repo](https://github.com/anttikantee/buildrump.sh)
or the [page on rump kernels](http://www.netbsd.org/docs/rump/).

  - pooka@iki.fi
