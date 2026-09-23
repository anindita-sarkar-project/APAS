CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI=../../..

MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.identify-target

MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra-recon

# Subtree-traffic-proportional ("Subtree" column, rejected baseline):
# orchestra-rule-subtree-traffic.c's struct -- see that file's own header
# comment for exactly what it reconstructs and why.
CFLAGS += -DORCHESTRA_CONF_RULES="{&eb_fixed_shared,&subtree_traffic_tree,&default_common}"

include $(CONTIKI)/Makefile.include
