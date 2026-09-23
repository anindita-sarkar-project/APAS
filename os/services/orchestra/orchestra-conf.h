/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 *         Orchestra configuration
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef ORCHESTRA_CONF_H_
#define ORCHESTRA_CONF_H_

#ifdef ORCHESTRA_CONF_RULES
#define ORCHESTRA_RULES ORCHESTRA_CONF_RULES
#else /* ORCHESTRA_CONF_RULES */
/* A default configuration with:
 * - a sender-based slotframe for EB transmission
 * - a sender-based or receiver-based slotframe for unicast to RPL parents and children
 * - a common shared slotframe for any other traffic (mostly broadcast)
 *  */
#define ORCHESTRA_RULES { &eb_per_time_source, \
                          &unicast_per_neighbor_rpl_ns, \
                          &default_common }
/* Example configuration for RPL storing mode: */
/* #define ORCHESTRA_RULES { &eb_per_time_source, \
                             &unicast_per_neighbor_rpl_storing, \
                             &default_common } */

#endif /* ORCHESTRA_CONF_RULES */

/* Length of the various slotframes. Tune to balance network capacity,
 * contention, energy, latency. */
#ifdef ORCHESTRA_CONF_EBSF_PERIOD
#define ORCHESTRA_EBSF_PERIOD                     ORCHESTRA_CONF_EBSF_PERIOD
#else /* ORCHESTRA_CONF_EBSF_PERIOD */
#define ORCHESTRA_EBSF_PERIOD                     397
#endif /* ORCHESTRA_CONF_EBSF_PERIOD */

#ifdef ORCHESTRA_CONF_COMMON_SHARED_PERIOD
#define ORCHESTRA_COMMON_SHARED_PERIOD            ORCHESTRA_CONF_COMMON_SHARED_PERIOD
#else /* ORCHESTRA_CONF_COMMON_SHARED_PERIOD */
#define ORCHESTRA_COMMON_SHARED_PERIOD            31
#endif /* ORCHESTRA_CONF_COMMON_SHARED_PERIOD */

#ifdef ORCHESTRA_CONF_UNICAST_PERIOD
#define ORCHESTRA_UNICAST_PERIOD                  ORCHESTRA_CONF_UNICAST_PERIOD
#else /* ORCHESTRA_CONF_UNICAST_PERIOD */
#define ORCHESTRA_UNICAST_PERIOD                  17
#endif /* ORCHESTRA_CONF_UNICAST_PERIOD */

/* Slotframe size for the root rule. Usually this should be shorter than the unicast slotframe size,
   as the root node receives more traffic than the other nodes in the network. */
#ifdef ORCHESTRA_CONF_ROOT_PERIOD
#define ORCHESTRA_ROOT_PERIOD                     ORCHESTRA_CONF_ROOT_PERIOD
#else /* ORCHESTRA_CONF_ROOT_PERIOD */
#define ORCHESTRA_ROOT_PERIOD                     7
#endif /* ORCHESTRA_CONF_ROOT_PERIOD */

/* Is the per-neighbor unicast slotframe sender-based (if not, it is receiver-based).
 * Note: sender-based works only with RPL storing mode as it relies on DAO and
 * routing entries to keep track of children and parents. */
#ifdef ORCHESTRA_CONF_UNICAST_SENDER_BASED
#define ORCHESTRA_UNICAST_SENDER_BASED            ORCHESTRA_CONF_UNICAST_SENDER_BASED
#else /* ORCHESTRA_CONF_UNICAST_SENDER_BASED */
#define ORCHESTRA_UNICAST_SENDER_BASED            0
#endif /* ORCHESTRA_CONF_UNICAST_SENDER_BASED */

/* The hash function used to assign timeslot to a given node (based on its link-layer address).
 * For rules with multiple channel offsets, it is also used to select the channel offset. */
#ifdef ORCHESTRA_CONF_LINKADDR_HASH
#define ORCHESTRA_LINKADDR_HASH                   ORCHESTRA_CONF_LINKADDR_HASH
#else /* ORCHESTRA_CONF_LINKADDR_HASH */
#define ORCHESTRA_LINKADDR_HASH(addr)             ((addr != NULL) ? (addr)->u8[LINKADDR_SIZE - 1] : -1)
#endif /* ORCHESTRA_CONF_LINKADDR_HASH */

/* The hash function used to assign timeslot for a pair of given nodes.
 * The value of 264 is a good choice for the default slotframe size 17. It also a good choice for
 * most other slotframe sizes: there are no prime numbers between 2 and 101 (inclusive) that
 * produce modulo 1 when used to divide 264. This ensures that for any a1, a2 this is true:
 * `ORCHESTRA_LINKADDR_HASH2(a1, a2) != ORCHESTRA_LINKADDR_HASH2(a2, a1)` */
#ifdef ORCHESTRA_CONF_LINKADDR_HASH2
#define ORCHESTRA_LINKADDR_HASH2                  ORCHESTRA_CONF_LINKADDR_HASH2
#else /* ORCHESTRA_CONF_LINKADDR_HASH2 */
#define ORCHESTRA_LINKADDR_HASH2(addr1, addr2)    ((addr1)->u8[LINKADDR_SIZE - 1] + 264 * (addr2)->u8[LINKADDR_SIZE - 1])
#endif /* ORCHESTRA_CONF_LINKADDR_HASH2 */

/* The hash function used to assign a rotating cell to a (parent, child, grandparent)
 * triple, used by the implicit-ack tree rule for its 3-node "relay" cells. Extends
 * ORCHESTRA_LINKADDR_HASH2's base-264 positional encoding one term further: 264^2 =
 * 69696. Explicit uint32_t arithmetic avoids int-promotion overflow on 16-bit-int
 * platforms (264*255 already exceeds INT16_MAX). */
#ifdef ORCHESTRA_CONF_LINKADDR_HASH3
#define ORCHESTRA_LINKADDR_HASH3                  ORCHESTRA_CONF_LINKADDR_HASH3
#else /* ORCHESTRA_CONF_LINKADDR_HASH3 */
#define ORCHESTRA_LINKADDR_HASH3(addr1, addr2, addr3) \
  ((uint32_t)(addr1)->u8[LINKADDR_SIZE - 1] \
   + 264UL   * (uint32_t)(addr2)->u8[LINKADDR_SIZE - 1] \
   + 69696UL * (uint32_t)(addr3)->u8[LINKADDR_SIZE - 1])
#endif /* ORCHESTRA_CONF_LINKADDR_HASH3 */

/* The maximum hash */
#ifdef ORCHESTRA_CONF_MAX_HASH
#define ORCHESTRA_MAX_HASH                        ORCHESTRA_CONF_MAX_HASH
#else /* ORCHESTRA_CONF_MAX_HASH */
#define ORCHESTRA_MAX_HASH                        0x7fff
#endif /* ORCHESTRA_CONF_MAX_HASH */

/* Is the "hash" function collision-free? (e.g. it maps to unique node-ids) */
#ifdef ORCHESTRA_CONF_COLLISION_FREE_HASH
#define ORCHESTRA_COLLISION_FREE_HASH             ORCHESTRA_CONF_COLLISION_FREE_HASH
#else /* ORCHESTRA_CONF_COLLISION_FREE_HASH */
#define ORCHESTRA_COLLISION_FREE_HASH             0 /* Set to 1 if ORCHESTRA_LINKADDR_HASH returns unique hashes */
#endif /* ORCHESTRA_CONF_COLLISION_FREE_HASH */

/* Channel offset for the default common rule, default 0 */
#ifdef ORCHESTRA_CONF_DEFAULT_COMMON_CHANNEL_OFFSET
#define ORCHESTRA_DEFAULT_COMMON_CHANNEL_OFFSET   ORCHESTRA_CONF_DEFAULT_COMMON_CHANNEL_OFFSET
#else
#define ORCHESTRA_DEFAULT_COMMON_CHANNEL_OFFSET   0
#endif

/* Min channel offset for the unicast rules; the default min/max range is [2, sizeof(HS)-2].
   If the HS has less then 3 channels [1, 1] is used instead.
*/
#ifdef ORCHESTRA_CONF_UNICAST_MIN_CHANNEL_OFFSET
#define ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET       ORCHESTRA_CONF_UNICAST_MIN_CHANNEL_OFFSET
#else
#define ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET       (sizeof(TSCH_DEFAULT_HOPPING_SEQUENCE) > 2 ? 2 : 1)
#endif

/* Max channel offset for the unicast rules */
#ifdef ORCHESTRA_CONF_UNICAST_MAX_CHANNEL_OFFSET
#define ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET       ORCHESTRA_CONF_UNICAST_MAX_CHANNEL_OFFSET
#else
#define ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET       \
  (MAX(ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET, sizeof(TSCH_DEFAULT_HOPPING_SEQUENCE) - 1))
#endif

/* Channel offsets for the EB rule, default: 1 */
#ifdef ORCHESTRA_CONF_EB_MIN_CHANNEL_OFFSET
#define ORCHESTRA_EB_MIN_CHANNEL_OFFSET ORCHESTRA_CONF_EB_MIN_CHANNEL_OFFSET
#else
#define ORCHESTRA_EB_MIN_CHANNEL_OFFSET 1
#endif

#ifdef ORCHESTRA_CONF_EB_MAX_CHANNEL_OFFSET
#define ORCHESTRA_EB_MAX_CHANNEL_OFFSET ORCHESTRA_CONF_EB_MAX_CHANNEL_OFFSET
#else
#define ORCHESTRA_EB_MAX_CHANNEL_OFFSET 1
#endif

/* Slotframe size for the implicit-ack tree rule's root-adjacent (regular-timing,
 * explicit-ack) cells. Nodes one hop from the root use only this slotframe. */
#ifdef ORCHESTRA_CONF_IA_ROOT_PERIOD
#define ORCHESTRA_IA_ROOT_PERIOD ORCHESTRA_CONF_IA_ROOT_PERIOD
#else
#define ORCHESTRA_IA_ROOT_PERIOD 7
#endif

/* Number of parallel Rx cells (update_relay_rx_shared()) a node listens on
 * for its children's UPLINK traffic, and correspondingly how many candidate
 * Tx positions each child picks deterministically between (uplink_shard_
 * for()). Raising this from 1 (matching stock Orchestra's own single shared
 * cell) to 2 was tried, on the theory that splitting a parent's children
 * into two separate CSMA-contention groups would cut the "duplicate
 * retransmission" waste a single fully-shared cell causes (confirmed
 * present: "drop dup ll" log lines, a sender retrying a packet that had
 * already gotten through because contention delayed it past its own
 * confirmation deadline). That theory only paid off partially: 2 shards did
 * lower RDC at 25 nodes, but regressed PDR at every scale tested (8/25/49
 * nodes: 90.7/70.4/51.0% at 1 shard vs 87.9/45.2/35.3% at 2), worst at the
 * larger grids -- doubling this cell's count roughly doubles how many
 * positions it occupies in the *network-wide* hash address space shared
 * with every other node's own copy of it, and that added cross-node
 * collision risk apparently outweighs the within-parent contention it was
 * meant to relieve, especially since most parents only have 2-4 children in
 * practice -- too small a group for a fixed 2-way split to reliably balance.
 * Left at 1 (see git history for the sharding mechanism itself, still
 * intact and selectable via ORCHESTRA_CONF_IA_UPLINK_SHARDS for further
 * experimentation, e.g. if child counts were much larger). */
#ifdef ORCHESTRA_CONF_IA_UPLINK_SHARDS
#define ORCHESTRA_IA_UPLINK_SHARDS ORCHESTRA_CONF_IA_UPLINK_SHARDS
#else
#define ORCHESTRA_IA_UPLINK_SHARDS 1
#endif

/* Number of simultaneous positions each child's own RELAY_TX cell occupies
 * within its own dedicated relay_tx_sf slotframe (all installed and active
 * every cycle, unlike ORCHESTRA_IA_UPLINK_SHARDS above which is a *choice*
 * of one-of-N made once per node): with 1 (the default), RELAY_TX[child]
 * gets exactly one relay opportunity per TSCH_IA_SFS_SIZE-slot cycle, a
 * hard throughput ceiling on that specific child's entire subtree combined
 * (see update_child_links()'s and orchestra_ia_overhear()'s own comments --
 * this queue is shared by the child's own traffic and everything relayed
 * through it from deeper descendants). Raising this gives the same queue
 * more chances per cycle to drain, at the cost of occupying that many more
 * positions in the network-wide hash address space -- the identical
 * tradeoff ORCHESTRA_IA_UPLINK_SHARDS already documents above (and which
 * regressed PDR there); measure before assuming this one helps instead of
 * hurting. c->relay_tx (the existing, single-link field) is always shard
 * 0; additional shards live in c->relay_tx_extra[] (see struct ia_child).
 * SELF_OVERHEAR listens at every shard simultaneously so this needs no
 * selection logic on the sending side at all -- see update_self_overhear()'s
 * comment for why. */
#ifdef ORCHESTRA_CONF_IA_RELAY_TX_SHARDS
#define ORCHESTRA_IA_RELAY_TX_SHARDS ORCHESTRA_CONF_IA_RELAY_TX_SHARDS
#else
#define ORCHESTRA_IA_RELAY_TX_SHARDS 1
#endif

/* Max number of children a single node tracks for the implicit-ack tree rule's
 * per-child RELAY_RX/RELAY_TX cells. */
#ifdef ORCHESTRA_CONF_IA_MAX_CHILDREN
#define ORCHESTRA_IA_MAX_CHILDREN ORCHESTRA_CONF_IA_MAX_CHILDREN
#else
#define ORCHESTRA_IA_MAX_CHILDREN 8
#endif

/* Thresholds, in packets queued toward our parent (the raw value reported by
 * tsch_queue_nbr_packet_count(), 0..TSCH_QUEUE_NUM_PER_NEIGHBOR), on our
 * PARENT's own reported queue depth (orchestra_ia_parent_congestion_input(),
 * carried on every EB from our time source) used by
 * orchestra_ia_confirmation_cycles() to pick our own implicit-ack
 * confirmation deadline: below LOW, our parent isn't backed up so we use the
 * shortest deadline (1 cycle -- confirmed best at small/shallow scale);
 * above HIGH, our parent is genuinely struggling to relay so we give it more
 * time before declaring a timeout (3 cycles -- confirmed best at larger,
 * more-converged scale); in between, 2 cycles. This replaces three prior
 * *local* adaptive-timeout attempts (plain latency EWMA, EWMA + naive
 * timeout feedback, direct timeout-rate EWMA), all of which were confirmed
 * (via IATRACE logging) to fail for a structural reason: a node's own
 * confirm/timeout history only reflects its own self-originated traffic --
 * a relayed child's frame resolves synchronously at Tx time regardless (see
 * the slotframe-tag gate in tsch_tx_slot(), tsch-slot-operation.c), so the
 * harm from a too-short local deadline is exported one hop up (an extra
 * retry lands on the PARENT's queue) where the child's own history can never
 * see it. Using the parent's own self-reported depth instead is a real,
 * cross-boundary signal rather than a local proxy. */
#ifdef ORCHESTRA_CONF_IA_CONGESTION_LOW_THRESHOLD
#define ORCHESTRA_IA_CONGESTION_LOW_THRESHOLD ORCHESTRA_CONF_IA_CONGESTION_LOW_THRESHOLD
#else
#define ORCHESTRA_IA_CONGESTION_LOW_THRESHOLD 4
#endif
#ifdef ORCHESTRA_CONF_IA_CONGESTION_HIGH_THRESHOLD
#define ORCHESTRA_IA_CONGESTION_HIGH_THRESHOLD ORCHESTRA_CONF_IA_CONGESTION_HIGH_THRESHOLD
#else
#define ORCHESTRA_IA_CONGESTION_HIGH_THRESHOLD 16
#endif

/* Master on/off switch for the congestion-driven confirmation-cycles logic
 * above (orchestra_ia_confirmation_cycles()). When 0, that function always
 * returns the plain fixed TSCH_IA_CONFIRMATION_TIMEOUT_CYCLES constant --
 * i.e. the plain fixed-cycles configuration from earlier in this session,
 * before the congestion signal existed. All of the EB congestion-signaling
 * plumbing (ie_ia_parent_congestion, TSCH_CALLBACK_IA_CONGESTION/
 * TSCH_CALLBACK_IA_PARENT_CONGESTION) keeps running either way -- this only
 * gates whether orchestra_ia_confirmation_cycles() acts on it. */
#ifdef ORCHESTRA_CONF_IA_CONGESTION_ADAPTIVE
#define ORCHESTRA_IA_CONGESTION_ADAPTIVE ORCHESTRA_CONF_IA_CONGESTION_ADAPTIVE
#else
#define ORCHESTRA_IA_CONGESTION_ADAPTIVE 1
#endif

#endif /* ORCHESTRA_CONF_H_ */
