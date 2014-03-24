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

The driver has been tested to work on various Linux distributions
(e.g. Ubuntu and Void) and is able to exchange TCP traffic
with an Internet peer.  Future plans include benchmarking,
performance optimization, and improved configurability.  A
[wiki page](https://github.com/rumpkernel/dpdk-rumptcpip/wiki/Optimizing-performance)
documents some ideas for potential performance optimizations.


Instructions
------------

The simple version is as follows:

* run `git submodule update --init --recursive`
* edit `src/libdpdkif/configuration.h`
* run `make`

To test, try running `rump/bin/webbrowser`.

By default, the build will use the DPDK submodule in this repository.
You can also choose another DPDK installation, just set `$RTE_SDK` and
`$RTE_TARGET` differently.  You can now link and use the DPDK interface
driver (`librumpnet_dpdkif`, `-lrumpnet_dpdkif`) into applications.

Using the revision of DPDK included as a submodule is highly recommended.
It is possible to use another version of DPDK, but in that case be
aware that the combination may not be tested, and you should prepare to
debug and fix any resulting problems yourself.

For more information on how to use the resulting userspace TCP/IP stack,
see e.g. the [buildrump.sh repo](https://github.com/rumpkernel/buildrump.sh)
or http://rumpkernel.org/.  To portably configure the TCP/IP stack,
using [rumprun](https://github.com/rumpkernel/rumprun/) is recommended.
There are some very simple examples in the `examples` directory.  These
can be built using `rumpmake` (cf. above).

__NOTE__: To successfully use the TCP/IP stack, you must have the host correctly
configured for running DPDK.  This means you have to the appropriate host
kernel configurations set and the necessary kernel modules loaded.  These
steps are not documented here.  Consult DPDK documentation.


Support
-------

For free support, use the rump kernel mailing
list at rumpkernel-users@lists.sourceforge.net
(you can [subscribe here](https://lists.sourceforge.net/lists/listinfo/rumpkernel-users)).
You can also ask simple questions on irc: __#rumpkernel__
on __irc.freenode.net__.  Please do not send private email.

If you need commercial support e.g. for development or integration,
or want to pay for private email support, contact
[Fixup Software Ltd.](http://www.fixup.fi/).
