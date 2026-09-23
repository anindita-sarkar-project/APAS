#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() -- matches
 * implicit-ack-tree's project-conf.h for a fair comparison. */
#define TSCH_CONF_AUTOSTART 0

/* Matches implicit-ack-tree's QUEUEBUF_CONF_NUM so buffer capacity isn't a
 * confounding variable between the two comparisons. */
#define QUEUEBUF_CONF_NUM 128

/* Matches implicit-ack-tree's TSCH_CONF_IA_SFS_SIZE (101) -- same slotframe
 * length for both schemes' own unicast/data slotframe, so slot budget isn't
 * a confounding variable between the two comparisons either. Stock
 * Orchestra's own default (17) is deliberately much smaller than this; this
 * override exists purely to hold the comparison's slot-budget variable
 * fixed, not because 17 was ever a problem for stock Orchestra itself. */
#define ORCHESTRA_CONF_UNICAST_PERIOD 101

/* Radio duty cycle measurement (node.c's periodic RDC report). */
#define ENERGEST_CONF_ON 1

#define LOG_CONF_LEVEL_RPL     LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_TCPIP   LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6    LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC     LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER  LOG_LEVEL_WARN

#define TSCH_LOG_CONF_PER_SLOT 1

#endif /* PROJECT_CONF_H_ */
