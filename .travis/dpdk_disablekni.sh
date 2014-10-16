#!/bin/sh

patch -p0 <<EOF
--- dpdk/config/common_linuxapp
+++ dpdk/config/common_linuxapp
@@ -381,7 +381,7 @@ CONFIG_RTE_LIBRTE_PIPELINE=y
 #
 # Compile librte_kni
 #
-CONFIG_RTE_LIBRTE_KNI=y
+CONFIG_RTE_LIBRTE_KNI=n
 CONFIG_RTE_KNI_KO_DEBUG=n
 CONFIG_RTE_KNI_VHOST=n
 CONFIG_RTE_KNI_VHOST_MAX_CACHE_SIZE=1024
EOF
