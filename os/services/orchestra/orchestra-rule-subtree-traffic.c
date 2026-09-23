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
 */

/**
 * \file
 *         RECONSTRUCTION, for comparison purposes only -- not the design
 *         this codebase actually ships. This is the "Subtree" baseline
 *         reported in the companion paper's Table~mainresults: it sizes a
 *         node's own uplink cell count from the number of entries in its
 *         OWN RPL storing-mode routing table (uip_ds6_route_num_routes()),
 *         i.e. the size of its entire subtree, not merely its direct
 *         child count. This is a MORE PRECISE traffic estimate than
 *         orchestra-rule-child-grandchild.c's own direct-child-count rule
 *         (CG_SUBTREE_TIER below), but the whole point of keeping this
 *         file around is that more precision is exactly what makes it
 *         fail at scale: the route count changes on every DAO/route
 *         change anywhere in the subtree behind a node, not just when
 *         that node's own direct children change, so it fluctuates far
 *         faster, at scale, than the one-hop EB channel signaling it can
 *         track. See the companion paper, Sec. "Why a Whole-Subtree
 *         Quantity Is the Wrong Thing to Signal", for the measured
 *         collapse this causes (86.3% to under 5% PDR between 8 and 49
 *         nodes) and orchestra-rule-child-grandchild.c for the design
 *         that replaced it.
 *
 *         Structurally this file is orchestra-rule-child-grandchild.c
 *         with exactly one change: cg_own_shard_count() below computes
 *         its tier from uip_ds6_route_num_routes() instead of from the
 *         local children[] list's length. Every other mechanism --
 *         cell placement (HASH2 + shard spreading), the children-list EB
 *         Information Element reuse, the bootstrap-default receive side,
 *         the periodic bootstrap-catch-up timer -- is unchanged and
 *         deliberately NOT updated with any of the newer, separately-
 *         evaluated additions (idle reclaim, load bonus, rank-aware cap,
 *         hold-before-use) that orchestra-rule-child-grandchild.c has
 *         picked up since: this file exists to reproduce the ORIGINAL,
 *         published Table~mainresults comparison, not to be a better
 *         version of the rejected design.
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"
#include "net/mac/tsch/tsch-roots.h"
#include "net/ipv6/uip-ds6-route.h"
#include "sys/ctimer.h"
#include "sys/log.h"

#define LOG_MODULE "Orchestra"
#define LOG_LEVEL  LOG_LEVEL_MAC

#if UIP_MAX_ROUTES != 0

#define CG_RELAY_SHARDS_MAX 4

#ifndef CG_CONF_MAX_CHILDREN
#define CG_MAX_CHILDREN 10
#else
#define CG_MAX_CHILDREN CG_CONF_MAX_CHILDREN
#endif

struct cg_child {
  linkaddr_t addr;
  uint8_t in_use;
  uint8_t shard_count;
  struct tsch_link *relay_rx[CG_RELAY_SHARDS_MAX];
  struct tsch_link *root_adjacent_rx[CG_RELAY_SHARDS_MAX];
};

static uint16_t slotframe_handle;
static struct tsch_slotframe *sf_short;
static struct tsch_slotframe *sf_root;

static struct tsch_link *l_uplink[CG_RELAY_SHARDS_MAX];
static struct tsch_link *l_root_adjacent[CG_RELAY_SHARDS_MAX];

static struct cg_child children[CG_MAX_CHILDREN];

/*---------------------------------------------------------------------------*/
static uint16_t
cg_slot(uint32_t hash, uint16_t period)
{
  return period > 0 ? (uint16_t)(hash % period) : 0;
}
/*---------------------------------------------------------------------------*/
static uint16_t
cg_channel(uint32_t hash)
{
  return tsch_hopping_sequence_length.val > 0 ? (uint16_t)(hash % tsch_hopping_sequence_length.val) : 0;
}
/*---------------------------------------------------------------------------*/
/* THE ONE SUBSTANTIVE DIFFERENCE FROM orchestra-rule-child-grandchild.c:
 * this node's own shard count is tiered from n_S = the number of entries
 * in its own RPL storing-mode routing table (every one of which is, by
 * construction of storing-mode DAO aggregation, a distinct destination
 * somewhere in this node's own subtree -- not merely its direct
 * children). Tiering per the companion paper: K(n)=1 for n<=0, 2 for
 * n<=2, 3 for n<=5, 4 (Kmax) otherwise. This is recomputed fresh on
 * every call -- unlike child count, which only changes on a
 * child_added()/child_removed() event, n_S can change on ANY route
 * change anywhere in the subtree, which is exactly the property that
 * makes this design fail at scale (see this file's own header). */
static uint8_t
cg_own_shard_count(void)
{
  int n = uip_ds6_route_num_routes();
  if(n <= 0) {
    return 1;
  } else if(n <= 2) {
    return 2;
  } else if(n <= 5) {
    return 3;
  }
  return CG_RELAY_SHARDS_MAX;
}
/*---------------------------------------------------------------------------*/
static struct cg_child *
find_child(const linkaddr_t *addr)
{
  int i;
  if(addr == NULL || linkaddr_cmp(addr, &linkaddr_null)) {
    return NULL;
  }
  for(i = 0; i < CG_MAX_CHILDREN; i++) {
    if(children[i].in_use && linkaddr_cmp(&children[i].addr, addr)) {
      return &children[i];
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static struct cg_child *
alloc_child(const linkaddr_t *addr)
{
  int i, j;
  for(i = 0; i < CG_MAX_CHILDREN; i++) {
    if(!children[i].in_use) {
      children[i].in_use = 1;
      linkaddr_copy(&children[i].addr, addr);
      children[i].shard_count = 0;
      for(j = 0; j < CG_RELAY_SHARDS_MAX; j++) {
        children[i].relay_rx[j] = NULL;
        children[i].root_adjacent_rx[j] = NULL;
      }
      return &children[i];
    }
  }
  LOG_ERR("cg: no free child slot for ");
  LOG_ERR_LLADDR(addr);
  LOG_ERR_("\n");
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
update_uplink(void)
{
  uint8_t want_shards;
  int k;
  if(sf_short == NULL || linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null)
     || tsch_roots_is_root(&orchestra_parent_linkaddr) || !orchestra_parent_knows_us) {
    for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
      if(l_uplink[k] != NULL) {
        tsch_schedule_remove_link(sf_short, l_uplink[k]);
        l_uplink[k] = NULL;
      }
    }
    return;
  }
  want_shards = cg_own_shard_count();
  {
    uint32_t h = ORCHESTRA_LINKADDR_HASH2(&orchestra_parent_linkaddr, &linkaddr_node_addr);
    for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
      if(k >= want_shards) {
        if(l_uplink[k] != NULL) {
          tsch_schedule_remove_link(sf_short, l_uplink[k]);
          l_uplink[k] = NULL;
        }
        continue;
      }
      {
        uint32_t kh = h + (uint32_t)k * 0x85EBCA6Bu;
        uint16_t ts = cg_slot(kh, TSCH_IA_SFS_SIZE);
        uint16_t ch = cg_channel(kh);
        if(l_uplink[k] == NULL) {
          l_uplink[k] = tsch_schedule_add_link(sf_short, LINK_OPTION_TX | LINK_OPTION_SHARED,
                                                LINK_TYPE_NORMAL, &orchestra_parent_linkaddr, ts, ch, 1);
        } else {
          linkaddr_copy(&l_uplink[k]->addr, &orchestra_parent_linkaddr);
          l_uplink[k]->timeslot = ts;
          l_uplink[k]->channel_offset = ch;
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
update_root_adjacent(void)
{
  uint8_t want_shards;
  int k;
  if(sf_root == NULL || linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null)
     || !tsch_roots_is_root(&orchestra_parent_linkaddr) || !orchestra_parent_knows_us) {
    for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
      if(l_root_adjacent[k] != NULL) {
        tsch_schedule_remove_link(sf_root, l_root_adjacent[k]);
        l_root_adjacent[k] = NULL;
      }
    }
    return;
  }
  want_shards = cg_own_shard_count();
  {
    uint32_t h = ORCHESTRA_LINKADDR_HASH2(&orchestra_parent_linkaddr, &linkaddr_node_addr);
    for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
      if(k >= want_shards) {
        if(l_root_adjacent[k] != NULL) {
          tsch_schedule_remove_link(sf_root, l_root_adjacent[k]);
          l_root_adjacent[k] = NULL;
        }
        continue;
      }
      {
        uint32_t kh = h + (uint32_t)k * 0x85EBCA6Bu;
        uint16_t ts = cg_slot(kh, ORCHESTRA_IA_ROOT_PERIOD);
        uint16_t ch = cg_channel(kh);
        if(l_root_adjacent[k] == NULL) {
          l_root_adjacent[k] = tsch_schedule_add_link(sf_root, LINK_OPTION_TX | LINK_OPTION_SHARED,
                                                       LINK_TYPE_NORMAL, &orchestra_parent_linkaddr, ts, ch, 1);
        } else {
          linkaddr_copy(&l_root_adjacent[k]->addr, &orchestra_parent_linkaddr);
          l_root_adjacent[k]->timeslot = ts;
          l_root_adjacent[k]->channel_offset = ch;
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
update_child_relay_rx(struct cg_child *c)
{
  int k;
  uint8_t want_shards = c->shard_count > 0 ? c->shard_count : 1;
  if(sf_short == NULL) {
    return;
  }
  {
    uint32_t h = ORCHESTRA_LINKADDR_HASH2(&linkaddr_node_addr, &c->addr);
    for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
      if(k >= want_shards) {
        if(c->relay_rx[k] != NULL) {
          tsch_schedule_remove_link(sf_short, c->relay_rx[k]);
          c->relay_rx[k] = NULL;
        }
        continue;
      }
      {
        uint32_t kh = h + (uint32_t)k * 0x85EBCA6Bu;
        uint16_t ts = cg_slot(kh, TSCH_IA_SFS_SIZE);
        uint16_t ch = cg_channel(kh);
        if(c->relay_rx[k] == NULL) {
          c->relay_rx[k] = tsch_schedule_add_link(sf_short, LINK_OPTION_RX, LINK_TYPE_NORMAL,
                                                   &c->addr, ts, ch, 1);
        } else {
          c->relay_rx[k]->timeslot = ts;
          c->relay_rx[k]->channel_offset = ch;
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
update_child_root_adjacent_rx(struct cg_child *c)
{
  int k;
  uint8_t want_shards = c->shard_count > 0 ? c->shard_count : 1;
  if(sf_root == NULL || !tsch_is_coordinator) {
    for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
      if(c->root_adjacent_rx[k] != NULL) {
        tsch_schedule_remove_link(sf_root, c->root_adjacent_rx[k]);
        c->root_adjacent_rx[k] = NULL;
      }
    }
    return;
  }
  {
    uint32_t h = ORCHESTRA_LINKADDR_HASH2(&linkaddr_node_addr, &c->addr);
    for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
      if(k >= want_shards) {
        if(c->root_adjacent_rx[k] != NULL) {
          tsch_schedule_remove_link(sf_root, c->root_adjacent_rx[k]);
          c->root_adjacent_rx[k] = NULL;
        }
        continue;
      }
      {
        uint32_t kh = h + (uint32_t)k * 0x85EBCA6Bu;
        uint16_t ts = cg_slot(kh, ORCHESTRA_IA_ROOT_PERIOD);
        uint16_t ch = cg_channel(kh);
        if(c->root_adjacent_rx[k] == NULL) {
          c->root_adjacent_rx[k] = tsch_schedule_add_link(sf_root, LINK_OPTION_RX, LINK_TYPE_NORMAL,
                                                           &c->addr, ts, ch, 1);
        } else {
          c->root_adjacent_rx[k]->timeslot = ts;
          c->root_adjacent_rx[k]->channel_offset = ch;
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
update_child(struct cg_child *c)
{
  if(c == NULL) {
    return;
  }
  if(tsch_is_coordinator) {
    update_child_root_adjacent_rx(c);
  } else {
    update_child_relay_rx(c);
  }
}
/*---------------------------------------------------------------------------*/
#define CG_BOOTSTRAP_CHECK_PERIOD (CLOCK_SECOND / 4)
static struct ctimer bootstrap_timer;
static uint8_t last_parent_knows_us;
/* Unlike orchestra-rule-child-grandchild.c, this design's own shard count
 * can change on ANY route change anywhere in the subtree, not just on a
 * local child_added()/child_removed() event -- so, in addition to the
 * same parent-switch catch-up child-grandchild-tree.c needs, this tick
 * also unconditionally re-applies update_uplink()/update_root_adjacent()
 * every period, since there is no single routing-layer callback that
 * fires on an arbitrary downstream route change the way there is for a
 * direct child. This is itself extra overhead (and extra schedule churn)
 * that orchestra-rule-child-grandchild.c's design does not pay. */
static void
bootstrap_tick(void *ptr)
{
  if(orchestra_parent_knows_us && !last_parent_knows_us) {
    update_uplink();
    update_root_adjacent();
  }
  last_parent_knows_us = orchestra_parent_knows_us;
  if(orchestra_parent_knows_us) {
    update_uplink();
    update_root_adjacent();
  }
  ctimer_set(&bootstrap_timer, CG_BOOTSTRAP_CHECK_PERIOD, bootstrap_tick, NULL);
}
/*---------------------------------------------------------------------------*/
static void
child_added(const linkaddr_t *addr)
{
  struct cg_child *c = find_child(addr);
  if(c == NULL) {
    c = alloc_child(addr);
  }
  if(c == NULL) {
    return;
  }
  update_child(c);
  update_uplink();
  update_root_adjacent();
}
/*---------------------------------------------------------------------------*/
static void
child_removed(const linkaddr_t *addr)
{
  struct cg_child *c = find_child(addr);
  int k;
  if(c == NULL) {
    return;
  }
  for(k = 0; k < CG_RELAY_SHARDS_MAX; k++) {
    if(c->relay_rx[k] != NULL) {
      tsch_schedule_remove_link(sf_short, c->relay_rx[k]);
      c->relay_rx[k] = NULL;
    }
    if(c->root_adjacent_rx[k] != NULL) {
      tsch_schedule_remove_link(sf_root, c->root_adjacent_rx[k]);
      c->root_adjacent_rx[k] = NULL;
    }
  }
  tsch_queue_free_packets_to(addr);
  c->in_use = 0;
  update_uplink();
  update_root_adjacent();
}
/*---------------------------------------------------------------------------*/
static void
reconcile_parent_address(const linkaddr_t *new_addr)
{
  if(!linkaddr_cmp(new_addr != NULL ? new_addr : &linkaddr_null, &orchestra_parent_linkaddr)) {
    const linkaddr_t *old_addr = &orchestra_parent_linkaddr;

    if(new_addr != NULL) {
      linkaddr_copy(&orchestra_parent_linkaddr, new_addr);
    } else {
      linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
    }
    tsch_queue_free_packets_to(old_addr);

    update_uplink();
    update_root_adjacent();
  }
}
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
  if(new != old) {
    reconcile_parent_address(tsch_queue_get_nbr_address(new));
  }
}
/*---------------------------------------------------------------------------*/
static void
root_node_updated(const linkaddr_t *root, uint8_t is_added)
{
  if(linkaddr_cmp(root, &linkaddr_node_addr)) {
    return;
  }
  if(!linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null)
     && linkaddr_cmp(root, &orchestra_parent_linkaddr)) {
    update_uplink();
    update_root_adjacent();
  }
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
  const linkaddr_t *dst = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
     && linkaddr_cmp(dst, &orchestra_parent_linkaddr)
     && !linkaddr_cmp(dst, &linkaddr_null)) {
    if(tsch_roots_is_root(&orchestra_parent_linkaddr)) {
      if(l_root_adjacent[0] != NULL) {
        *slotframe = sf_root->handle;
        *timeslot = 0xffff;
        *channel_offset = 0xffff;
        return 1;
      }
      return 0;
    }
    if(l_uplink[0] != NULL) {
      *slotframe = sf_short->handle;
      *timeslot = 0xffff;
      *channel_offset = 0xffff;
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  int i;
  slotframe_handle = sf_handle;
  for(i = 0; i < CG_MAX_CHILDREN; i++) {
    children[i].in_use = 0;
  }
  sf_short = tsch_schedule_add_slotframe(slotframe_handle, TSCH_IA_SFS_SIZE);
  sf_root = tsch_schedule_add_slotframe(slotframe_handle | 0x4000, ORCHESTRA_IA_ROOT_PERIOD);
  if(sf_short == NULL) {
    LOG_ERR("cg: failed to add the shared slotframe\n");
  }
  if(sf_root == NULL) {
    LOG_ERR("cg: failed to add the root-adjacent slotframe\n");
  }
  ctimer_set(&bootstrap_timer, CG_BOOTSTRAP_CHECK_PERIOD, bootstrap_tick, NULL);
}
/*---------------------------------------------------------------------------*/
uint8_t
st_get_own_children(linkaddr_t *out, uint8_t *out_shard_count, uint8_t max_children)
{
  int i;
  uint8_t n = 0;
  uint8_t own_shards = cg_own_shard_count();
  for(i = 0; i < CG_MAX_CHILDREN && n < max_children; i++) {
    if(children[i].in_use) {
      linkaddr_copy(&out[n], &children[i].addr);
      out_shard_count[n] = own_shards;
      n++;
    }
  }
  return n;
}
/*---------------------------------------------------------------------------*/
void
st_child_eb_input(const linkaddr_t *source, const linkaddr_t *children_list,
                  const uint8_t *children_shard_count, uint8_t num_children)
{
  struct cg_child *c = find_child(source);
  if(c == NULL || num_children == 0) {
    return;
  }
  {
    uint8_t reported = children_shard_count[0] > 0 ? children_shard_count[0] : 1;
    if(c->shard_count != reported) {
      c->shard_count = reported;
      update_child(c);
    }
  }
}
/*---------------------------------------------------------------------------*/
int
st_implicit_ack_active(const linkaddr_t *addr)
{
  return 0;
}
void
st_overhear(const linkaddr_t *source, const linkaddr_t *destination,
            struct tsch_link *link, const uint8_t *payload, uint16_t payload_len)
{
}
int
st_get_own_parent(linkaddr_t *out, uint8_t *out_is_root)
{
  return 0;
}
void
st_parent_eb_input(const linkaddr_t *grandparent, uint8_t grandparent_is_root)
{
}
uint8_t
st_get_own_congestion(void)
{
  return 0;
}
void
st_parent_congestion_input(uint8_t congestion)
{
}
uint8_t
st_confirmation_cycles(void)
{
  return 1;
}
void
st_new_asfn(uint32_t asfn)
{
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule subtree_traffic_tree = {
  init,
  new_time_source,
  select_packet,
  child_added,
  child_removed,
  NULL,
  root_node_updated,
  "subtree_traffic_tree",
  -1,
};

#endif /* UIP_MAX_ROUTES != 0 */
