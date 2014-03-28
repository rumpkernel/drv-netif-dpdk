/*
 * Configurables.  Adjust these to be appropriate for your system.
 */

/* change blacklist parameters (-b) if necessary
 * If you have more than one interface, you will likely want to blacklist
 * at least one of them.
 */
static const char *ealargs[] = {
	"if_dpdk",
	"-b 00:00:03.0",
	"-c 1",
	"-n 1",
};

/* change PORTID to the one your want to use */
#define IF_PORTID 0

/* change to the init method of your NIC driver */
#ifndef PMD_INIT
#define PMD_INIT rte_pmd_init_all
#endif

/* Receive packets in bursts of 16 per read */
#define MAX_PKT_BURST 16
