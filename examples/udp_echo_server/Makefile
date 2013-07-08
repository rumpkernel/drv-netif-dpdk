# If you're using anything but the default, change this to
# point to your installation destination directory.
RD=../../../buildrump.sh/rump

all: udp_echo_server

udp_echo_server: udp_echo_server.c
	cc -g $< -o $@ -I${RD}/include -L${RD}/lib -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,-R${RD}/lib -Wl,--no-as-needed -lrumpnet_dpdkif -lrumpnet_config -lrumpdev_bpf -lrumpnet_netinet -lrumpnet_net -lrumpnet -lrump -lrumpuser -lpthread -ldl -lethdev -lrte_eal -lrte_hash -lrte_lpm -lrte_malloc -lrte_mbuf -lrte_mempool -lrte_pmd_igb -lrte_pmd_ixgbe -lrte_ring -lrte_timer

clean:
	rm -f udp_echo_server *core
