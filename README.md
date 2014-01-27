TCP/IP stack for DPDK [![Build Status](https://travis-ci.org/rumpkernel/dpdk-rumptcpip.png?branch=master)](https://travis-ci.org/rumpkernel/dpdk-rumptcpip)
=====================

The hypercall driver in this repository attaches a userspace
[rump kernel](http://rumpkernel.org/) TCP/IP
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
with an Internet peer.  Future plans include benchmarking,
performance optimization, and improved configurability.  A
[wiki page](https://github.com/rumpkernel/dpdk-rumptcpip/wiki/Optimizing-performance)
documents some ideas for potential performance optimizations.


Instructions
------------

The procedure is follows:

* `git submodule update --init`
* `./buildrump.sh/buildrump.sh -s rumpsrc -T rumptools`
* in `src/libdpdkif` of this repo, edit the parameters at the top of
  rumpcomp_user.c, e.g. the interface port to be used.
* still in `src/libdpdkif`, run: `../../rumptools/rumpmake dependall &&
  ../../rumptools/rumpmake install`.  Note that you need `RTE_SDK`
  and `RTE_TARGET` set in the normal DPDK manner (consult DPDK docs).

You can now link and use the DPDK interface driver (`librumpnet_dpdkif`)
into applications.

DPDK is provided as a git submodule mostly for autobuild purposes.
You can also use DPDK from other sources/installations (via `RTE_SDK`).
It is advisable -- even if not always strictly necessary -- to use a
DPDK revision at least as recent as the submodule.  The DPDK submodule
revision can be obtained by going to the `dpdk` subdirectory and running
`git describe`.

For more information on how to use the resulting userspace TCP/IP stack,
see e.g. the [buildrump.sh repo](https://github.com/rumpkernel/buildrump.sh)
or http://rumpkernel.org/.  To portably configure the TCP/IP stack,
using [rumprun](https://github.com/rumpkernel/rumprun/) is recommended.


Support
-------

For free support, use the rump kernel mailing
list at rumpkernel-users@lists.sourceforge.net
([subscribe](https://lists.sourceforge.net/lists/listinfo/rumpkernel-users)
before posting).  You can also ask simple questions on irc: __#rumpkernel__
on __irc.freenode.net__.  Please do not send private email.

If you need commercial support e.g. for development or integration,
or want to pay for private email support, contact
[Fixup Software Ltd.](http://www.fixup.fi/).
