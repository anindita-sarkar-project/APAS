/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden.
 * All rights reserved. See child-grandchild-tree/project-conf.h for the
 * full license text (identical).
 */

/*
 * "Subtree" column reproduction (rejected baseline) -- see
 * orchestra-rule-subtree-traffic.c's own header comment for the design
 * and why it's kept around. Every parameter below matches
 * child-grandchild-tree-vanilla/project-conf.h exactly (same slotframe,
 * same shard cap, same everything) except which rule struct is linked in
 * (see this dir's Makefile) -- so the ONLY thing that differs between a
 * Table 2 run in this directory and one in ../child-grandchild-tree-vanilla/
 * is which quantity sizes a node's own shard count.
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define TSCH_CONF_AUTOSTART 0
#define PARENT_SWITCH_THRESHOLD_CONF 192
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3
#define TSCH_SCHEDULE_CONF_MAX_SLOTFRAMES 4
#define CG_CONF_MAX_CHILDREN 10
#define TSCH_CONF_WITH_IMPLICIT_ACK 1
#define TSCH_CONF_IA_SFS_SIZE 101

#define TSCH_CALLBACK_IMPLICIT_ACK_ACTIVE    st_implicit_ack_active
#define TSCH_CALLBACK_IA_OVERHEAR            st_overhear
#define TSCH_CALLBACK_IA_OWN_PARENT          st_get_own_parent
#define TSCH_CALLBACK_IA_PARENT_EB           st_parent_eb_input
#define TSCH_CALLBACK_IA_CONGESTION          st_get_own_congestion
#define TSCH_CALLBACK_IA_PARENT_CONGESTION   st_parent_congestion_input
#define TSCH_CALLBACK_IA_CONFIRMATION_CYCLES st_confirmation_cycles
#define TSCH_CALLBACK_IA_OWN_CHILDREN        st_get_own_children
#define TSCH_CALLBACK_IA_CHILD_EB            st_child_eb_input
#define TSCH_CALLBACK_NEW_ASFN               st_new_asfn

#define RPL_CONF_MOP RPL_MOP_STORING_NO_MULTICAST
#define TSCH_CONF_MAC_MAX_FRAME_RETRIES 7
#define QUEUEBUF_CONF_NUM 128
#define ORCHESTRA_CONF_COMMON_SHARED_PERIOD 11
#define ENERGEST_CONF_ON 1

#define LOG_CONF_LEVEL_RPL     LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_TCPIP   LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6    LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC     LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER  LOG_LEVEL_WARN

#define TSCH_LOG_CONF_PER_SLOT 1

#endif /* PROJECT_CONF_H_ */
