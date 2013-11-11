# If you're using anything but the default, change this to
# point to your installation destination directory.
RD=../../rump

INCS=-I${RD}/include

LIBDIR=-L${RD}/lib -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,-R${RD}/lib 

NETLIBS=-lrumpnet_dpdkif -lrumpnet_config -lrumpdev_bpf -lrumpnet_netinet -lrumpnet_net -lrumpnet -lrump -lrumpuser -lethdev -lrte_eal -lrte_hash -lrte_lpm -lrte_malloc -lrte_mbuf -lrte_mempool -lrte_pmd_ixgbe -lrte_pmd_e1000 -lrte_pmd_ring -lrte_pmd_virtio -lrte_ring -lrte_timer

LIBS=-lpthread -lrt -ldl
