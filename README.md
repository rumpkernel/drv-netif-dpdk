TCP/IP stack for DPDK
=====================

The hypercall driver in this repository attaches a userspace
[rump kernel](http://www.netbsd.org/docs/rump/) TCP/IP
stack to a network interface card accessed via the Intel Data Plane
Development Kit [DPDK](http://dpdk.org/).

in rough diagram form:

	------------------------------
	|    application process     |
	||--------------------------||
	|| rump kernel (TCP/IP etc.)||
	||--------------------------||
	|| dpdk-rumptcpip (dpdkif)  ||
	||--------------------------||
	|| DPDK                     ||
	|----------------------------|
	-----------------------------|


Status
------

The driver has been tested to work and is able to exchange TCP traffic
with an Internet peer.  Testing has been done at least with DPDK versions
1.2, 1.3 and 1.4.

Future plans include benchmarking, performance optimization, and
improved configurability.

A [wiki page](https://github.com/anttikantee/dpdk-rumptcpip/wiki/Optimizing-performance) documents some ideas for potential performance optimizations.


Instructions
------------

To use, in addition to a working DPDK installation you need the rump
kernel TCP/IP stack components.  The easiest way to obtain the rump
kernel components is to use
[buildrump.sh](https://github.com/anttikantee/buildrump.sh).  It is
recommended to pull and build the latest buildrump.sh after pulling a
new version of this repository.

The procedure is follows:

* clone the buildrump.sh git repo run a build there
* in `src/libdpdkif` of this repo, edit the parameters at the top of
  rumpcomp_user.c, e.g. the interface port to be used.
* still in `src/libdpdkif`, run `rumpmake dependall && rumpmake install`.
  (note, you must have `rumpmake` in your path.  See
  [here](https://github.com/anttikantee/buildrump.sh#tips-for-advanced-users)
  for more information on rumpmake)

For more information on how to use the resulting userspace TCP/IP stack,
see e.g. the [buildrump.sh repo](https://github.com/anttikantee/buildrump.sh)
or the [page on rump kernels](http://www.netbsd.org/docs/rump/).


Support
-------

If you need commercial support e.g. for development or integration, contact
Antti Kantee <pooka@iki.fi> (Fixup Software Ltd.)

For free support, please use the public
[issue tracker](https://github.com/anttikantee/dpdk-rumptcpip/issues)
offered by GitHub.
