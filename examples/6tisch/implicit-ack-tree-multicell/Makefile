CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI=../../..

MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.identify-target

MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra

# RPL storing mode: required, the implicit-ack tree rule needs to know its
# direct children (via child_added/child_removed), which only fire when
# nbr_routes (storing-mode routes) is compiled in.
CFLAGS += -DRPL_CONF_MOP=RPL_MOP_STORING_NO_MULTICAST

CFLAGS += -DTSCH_CONF_WITH_IMPLICIT_ACK=1
# eb_fixed_shared (not the stock eb_per_time_source): every node's EB Tx/Rx
# uses one fixed, network-wide-common slot instead of a slot re-derived as
# hash(current time source address) -- see orchestra-rule-eb-fixed.c's file
# header for the disassociation cascade this replaces (a parent switch used
# to relocate the EB-listening slot, and repeated switches could relocate it
# faster than a full re-lock ever completed, running out the desync timeout).
CFLAGS += -DORCHESTRA_CONF_RULES="{&eb_fixed_shared,&implicit_ack_tree,&default_common}"

include $(CONTIKI)/Makefile.include
