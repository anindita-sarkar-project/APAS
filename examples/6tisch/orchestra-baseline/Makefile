CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI=../../..

MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.identify-target

MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra

# Same RPL storing-mode configuration as the implicit-ack-tree example, for a
# fair comparison -- only the scheduling rule/timing differs.
CFLAGS += -DRPL_CONF_MOP=RPL_MOP_STORING_NO_MULTICAST

# Stock Orchestra: standard eb_per_time_source + unicast_per_neighbor_rpl_storing
# + default_common, all regular (10ms) TSCH timing, explicit link-layer acks
# throughout -- no implicit-ack, no dual timing. This is the baseline the
# implicit-ack-tree example (same node.c, same topology) is compared against.
CFLAGS += -DORCHESTRA_CONF_RULES="{&eb_per_time_source,&unicast_per_neighbor_rpl_storing,&default_common}"

include $(CONTIKI)/Makefile.include
