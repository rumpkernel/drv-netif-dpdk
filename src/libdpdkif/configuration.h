/*
 * Configurables.  Adjust these to be appropriate for your system.
 */

/* change blacklist parameters (-b) if necessary
 * If you have more than one interface, you will likely want to blacklist
 * at least one of them.
 */
static const char *ealargs[] = {
	"if_dpdk",
#if 0
	"-b 00:00:03.0",
#endif
	"-c 1",
	"-n 1",
};

/* Receive packets in bursts of 16 per read */
#define MAX_PKT_BURST 16
