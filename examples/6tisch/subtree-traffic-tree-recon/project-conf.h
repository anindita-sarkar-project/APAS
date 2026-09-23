/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden.
 * All rights reserved. See child-grandchild-tree/project-conf.h for the
 * full license text (identical).
 */

/*
 * LABELED RECONSTRUCTION -- does not reproduce the paper's published 8/25
 * -node baseline figures (86.3%/41.7% PDR). This dir + its topology .csc
 * files + os/services/orchestra-recon/ were regenerated Aug 21, two weeks
 * after paper3.tex (Aug 7) was finalized; the original code/topology that
 * produced 86.3%/41.7% no longer exists anywhere. Verified via debug
 * instrumentation: this reconstruction's own_shard tier reaches 4 (max)
 * at 8 nodes within ~4 simulated minutes, contradicting the paper's own
 * description ("n_C rarely if ever exceeds its first tier" at 8 nodes) --
 * a real topology/behavior mismatch, not a code bug in the tiering logic
 * itself (verified line-by-line against the paper's algorithm). Treat
 * every number produced from this directory (and any mitigation-attempt-
 * *-recon/ dir built on it) as ITS OWN new result set, not an extension
 * of the paper's published Table~tab:attempts or Table~tab:mainresults.
 *
 * Isolated from both ../subtree-traffic-tree/ (the original, untouched
 * reconstruction dir) and ../child-grandchild-tree/ (the working PA3
 * approach) -- neither is modified by anything in this directory.
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
