# If you're using anything but the default, change this to
# point to your installation destination directory.
RD=../../rump

INCS=-I${RD}/include

LIBDIR=-L${RD}/lib -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,-R${RD}/lib 

LIBS=-lrumpnet_dpdkif -lrumpnet_config -lrumpdev_bpf -lrumpnet_netinet -lrumpnet_net -lrumpnet -lrump -lrumpuser -lpthread -ldl -lethdev -lrte_eal -lrte_hash -lrte_lpm -lrte_malloc -lrte_mbuf -lrte_mempool -lrte_pmd_igb -lrte_pmd_ixgbe -lrte_pmd_wm -lrte_ring -lrte_timer
