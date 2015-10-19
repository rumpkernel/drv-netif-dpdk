TCP/IP stack for DPDK [![Build Status](https://travis-ci.org/rumpkernel/drv-netif-dpdk.png?branch=master)](https://travis-ci.org/rumpkernel/drv-netif-dpdk)
=====================

drv-netif-dpdk is a [DPDK](http://dpdk.org) network interface for [rump
kernels](http://rumpkernel.org).  The combined result is a userspace
TCP/IP stack doing packet I/O via DPDK.

See [the wiki page](http://wiki.rumpkernel.org/Repo:-drv-netif-dpdk) for more
information and instructions.

Note: you need expert level knowledge for using the DPDK driver and
especially for tuning it to achieve high performance.  It is unlikely
that you will find anyone in the rump kernel open source community
willing to do that consulting for free.
