/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         Smoke-test node for the autonomous implicit-ack tree scheduler
 *         (os/services/orchestra/orchestra-rule-implicit-ack.c). Node 1 is the
 *         RPL/TSCH root; every other node joins the tree and periodically
 *         sends a UDP packet to the root, exercising ROOT_ADJACENT and, for
 *         nodes 2+ hops from the root, UPLINK/RELAY_RX/RELAY_TX cell
 *         installation and rotation.
 *
 * \author Alakesh Kalita
 */

#include "contiki.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "sys/energest.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"
#include <string.h>

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PORT 8765
#define SEND_INTERVAL (30 * CLOCK_SECOND)
#define RDC_REPORT_INTERVAL (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
static void
print_rdc(void)
{
  static uint64_t last_transmit, last_listen, last_total;
  uint64_t cpu, lpm, transmit, listen, total;
  uint64_t d_transmit, d_listen, d_total;

  energest_flush();

  cpu = energest_type_time(ENERGEST_TYPE_CPU);
  lpm = energest_type_time(ENERGEST_TYPE_LPM);
  transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  listen = energest_type_time(ENERGEST_TYPE_LISTEN);
  total = cpu + lpm;

  d_transmit = transmit - last_transmit;
  d_listen = listen - last_listen;
  d_total = total - last_total;

  if(d_total > 0) {
    LOG_INFO("RDC tx %lu%% rx %lu%% total %lu%% (all-time tx %lu%% rx %lu%%)\n",
              (unsigned long)((100ULL * d_transmit) / d_total),
              (unsigned long)((100ULL * d_listen) / d_total),
              (unsigned long)((100ULL * (d_transmit + d_listen)) / d_total),
              (unsigned long)(total > 0 ? (100ULL * transmit) / total : 0),
              (unsigned long)(total > 0 ? (100ULL * listen) / total : 0));
  }

  last_transmit = transmit;
  last_listen = listen;
  last_total = total;
}

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "Implicit-ack tree test node");
PROCESS(rdc_process, "RDC reporting");
AUTOSTART_PROCESSES(&node_process, &rdc_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
                const uint8_t *data, uint16_t datalen)
{
  unsigned count = 0;
  if(datalen == sizeof(count)) {
    memcpy(&count, data, sizeof(count));
  }
  LOG_INFO("received %u bytes (seq %u) from ", datalen, count);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer et;
  static unsigned count;
  static uip_ipaddr_t dst;

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

#if CONTIKI_TARGET_COOJA
  if(node_id == 1) {
    NETSTACK_ROUTING.root_start();
  }
#endif
  NETSTACK_MAC.on();

#if CONTIKI_TARGET_COOJA
  if(node_id == 1) {
    /* The root only receives application data in this test; it must not also
     * run the periodic sender below, or it would just loop traffic back to
     * itself and never actually exercise the network. */
    PROCESS_WAIT_EVENT_UNTIL(0);
  }
#endif

  etimer_set(&et, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD_UNTIL(etimer_expired(&et));
    etimer_reset(&et);

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dst)) {
      LOG_INFO("sending %u to root\n", count);
      simple_udp_sendto(&udp_conn, &count, sizeof(count), &dst);
      count++;
    } else {
      LOG_INFO("not reachable yet\n");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rdc_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, RDC_REPORT_INTERVAL);
  while(1) {
    PROCESS_YIELD_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
    print_rdc();
  }

  PROCESS_END();
}
