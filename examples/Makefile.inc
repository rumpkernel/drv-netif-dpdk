BINDIR=		/usr/bin

LDFLAGS+=	-L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--no-as-needed

LDADD+=		-Wl,--whole-archive -lrumpnet_dpdkif -lrumpnet_config -lrumpdev_bpf -lrumpnet_netinet6 -lrumpnet_netinet -lrumpnet_net -lrumpnet -lrump -lrumpuser -lethdev -lrte_eal -lrte_hash -lrte_lpm -lrte_malloc -lrte_mbuf -lrte_mempool -lrte_pmd_ixgbe -lrte_pmd_e1000 -lrte_pmd_ring -lrte_pmd_virtio -lrte_pmd_vmxnet3 -lrte_ring -lrte_timer -Wl,--no-whole-archive
