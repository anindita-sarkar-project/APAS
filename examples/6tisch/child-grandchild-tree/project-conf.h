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

/*
 * This example ("child-grandchild-tree") is a parent/child hash-based cell
 * taxonomy, with implicit acknowledgment and the short/regular dual timeslot
 * template both removed -- every cell here is a completely standard TSCH
 * cell (regular timeslot, explicit ACK, TSCH's own normal retry/backoff).
 * See orchestra-rule-child-grandchild.c's own header comment for the full
 * design and for exactly how TSCH_WITH_IMPLICIT_ACK=1 below is reused
 * purely for its children-list Information Element without pulling in any
 * of the implicit-ack behavior that name otherwise implies.
 *
 * Cell sizing rule: every node allocates cells with its own parent sized by
 * (its own direct RPL child count) + 1 for its own traffic -- nothing
 * deeper than direct children is ever tracked or signaled. This replaced an
 * earlier design that instead estimated each relay relationship's traffic
 * from the RPL routing table (a deeper, whole-subtree traffic proxy that
 * turned out to change too fast for the one-hop EB signal carrying it to
 * keep up with at scale) -- see the rule file's own header comment for the
 * full history.
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

/* RFC6719's own recommended hysteresis rather than MRHOF's more aggressive
 * default, since every RPL parent switch relocates this scheme's own
 * parent-keyed autonomous cells too (uplink/root-adjacent). */
#define PARENT_SWITCH_THRESHOLD_CONF 192

/* 6TiSCH minimal schedule length, used only until this rule's own
 * slotframes take over scheduling of unicast/data traffic. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3

/* sf_short + sf_root, plus one slotframe each for the EB and default-common
 * rules. No per-child slotframes needed: every child's relay_rx cells live
 * in sf_short at their own hash-derived position (HASH2(self, child)),
 * distinct from every other child's by construction, with collisions
 * tolerated the same way stock Orchestra already tolerates them. */
#define TSCH_SCHEDULE_CONF_MAX_SLOTFRAMES 4

/* Bumped from the library default (4): a grid node has up to 4 orthogonal
 * neighbors in radio range, any of which could pick it as RPL parent, plus
 * headroom for transient churn during convergence. Raised further from 6 to
 * 10 after a code review found alloc_child() (orchestra-rule-child-
 * grandchild.c) silently drops any child beyond this cap -- permanent,
 * unmitigated 100% loss for that child and its whole subtree, with only a
 * LOG_ERR as any trace -- and this evaluation now runs topologies dense and
 * deep enough (Section~sec:extendedscale, 200-300 nodes) that the risk of a
 * single node attracting more than 6 direct RPL children is no longer
 * negligible. 10 is not a structural fix (the cap is still fixed-size), just
 * a wider margin; see child_added()'s own comment for the defensive check
 * added alongside this. */
#define CG_CONF_MAX_CHILDREN 10

/* Reused purely for its children-list Enhanced Beacon Information Element
 * (MLME_SHORT_IE_TSCH_IA_CHILDREN, frame802154e-ie.c) -- the only piece of
 * orchestra-rule-implicit-ack.c's shared infrastructure this example needs.
 * See orchestra-rule-child-grandchild.c's header comment for the full
 * argument that this does NOT, by itself, enable any implicit-ack behavior:
 * every one of the other TSCH_CALLBACK_IA_* hooks this flag auto-wires is
 * explicitly redirected below to this file's own harmless stubs, and
 * TSCH_CALLBACK_IMPLICIT_ACK_ACTIVE always returning 0 is what actually
 * keeps tsch-slot-operation.c's deferred-confirmation/short-timing paths
 * permanently dormant regardless of this flag's value. */
#define TSCH_CONF_WITH_IMPLICIT_ACK 1

/* Governing slotframe length for sf_short (uplink + every child's relay_rx).
 * Matched to the same value used throughout the implicit-ack-tree-multicell
 * evaluation for a fair, same-budget comparison. Also directly compared
 * against 167 across all 6 scales: 167 gave slightly lower RDC (~0.2-1.0
 * points, longer slotframe means fewer active slots per second for the same
 * cell count) but slightly worse PDR at every scale from 25 nodes up
 * (e.g. 150 nodes: 98.5%->98.1%) and higher latency throughout (longer
 * average wait to reach one's own cell) -- not a beneficial trade given 101
 * already keeps PDR at or above 98.5% and RDC at 3-4% everywhere. Kept at
 * 101. */
#define TSCH_CONF_IA_SFS_SIZE 101

/* Comparison variant, off by default: EWMA-smooth a child's EB-reported
 * shard count instead of adopting it directly (see orchestra-rule-child-
 * grandchild.c's own header comment and cg_child_eb_input() for the
 * mechanism). Asymmetric fast-attack/slow-decay: 0 = adopt an increase
 * immediately (matching direct adoption for the direction that has a
 * correctness cost), 2 = alpha=1/4 smoothing on a decrease (safe to delay). */
/* #define CG_CONF_EWMA_SHARD_COUNT 1 */
/* #define CG_CONF_EWMA_ATTACK_SHIFT 0 */
/* #define CG_CONF_EWMA_DECAY_SHIFT 2 */

/* Idle reclaim, ENABLED (see orchestra-rule-child-grandchild.c's own
 * header comment on CG_IDLE_RECLAIM for the mechanism and the bounded,
 * self-correcting mismatch window it accepts). This watches whether a
 * child has actually been heard from lately -- independent of what it
 * self-reports in its EB -- and shrinks it back to its guaranteed-minimum
 * 1 shard after sustained silence, freeing capacity for others. It never
 * removes the child outright; that stays RPL's job via child_removed().
 * Both this and TSCH_CALLBACK_UNICAST_DATA_INPUT below must be enabled
 * together, or the build fails to link. An earlier version of this tapped
 * TSCH_CALLBACK_DO_NACK instead; live Cooja testing found that hook fires
 * on TSCH's own empty keepalive frames too, which a genuinely idle child
 * sends *more* of, not less -- defeating idle detection entirely. Fixed
 * by switching to this hook (fires after packetbuf is populated, so a
 * zero-length keepalive can be told apart from real traffic). */
#define CG_CONF_IDLE_RECLAIM 1
/* #define CG_CONF_IDLE_CHECK_PERIOD (CLOCK_SECOND * 120) */
/* #define CG_CONF_IDLE_EWMA_DECAY_SHIFT 2 */
/* #define CG_CONF_IDLE_RECLAIM_THRESHOLD 64 */

/* Load bonus, off by default (see orchestra-rule-child-grandchild.c's own
 * header comment on CG_LOAD_BONUS). Lets a node ask for one extra shard
 * beyond its structural (child-count) minimum when its OWN recent traffic
 * toward its OWN parent stays genuinely high -- separate from, and safe
 * against, the signaling-lag defect this whole design exists to avoid,
 * because it only ever measures this one node's own local activity, never
 * a whole-subtree aggregate. Uncomment to enable and test. */
#define CG_CONF_LOAD_BONUS 1
/* #define CG_CONF_LOAD_CHECK_PERIOD (CLOCK_SECOND * 60) */
/* #define CG_CONF_LOAD_SATURATE_COUNT 4 */
/* #define CG_CONF_LOAD_EWMA_ATTACK_SHIFT 1 */
/* #define CG_CONF_LOAD_EWMA_DECAY_SHIFT 3 */
/* #define CG_CONF_LOAD_BUSY_THRESHOLD 160 */
#if defined(CG_CONF_IDLE_RECLAIM) && CG_CONF_IDLE_RECLAIM
#define TSCH_CALLBACK_UNICAST_DATA_INPUT cg_data_input
#endif

/* Rank-aware cap, ENABLED (see orchestra-rule-child-grandchild.c's own
 * header comment on CG_RANK_CAP). Direct mitigation for the 300-node
 * shared-uplink-saturation collapse documented in the companion paper's
 * extended-scale check (Table~extendedscale): nodes within a couple of
 * hops of the root get a larger shard cap than everyone else, using RPL
 * rank they already compute for routing -- no new signaling of any kind.
 * CG_CONF_RANK_CAP_THRESHOLD assumes the library default
 * RPL_MIN_HOPRANKINC (256); retune if your deployment overrides it. */
#define CG_CONF_RANK_CAP 1
/* #define CG_CONF_RANK_CAP_THRESHOLD 512 */
/* #define CG_CONF_RELAY_SHARDS_NEAR_ROOT 6 */
/* #define CG_CONF_RELAY_SHARDS_FAR 4 */

/* Hold-before-use, ENABLED (see orchestra-rule-child-grandchild.c's own
 * header comment on CG_HOLD_BEFORE_USE). A newly-grown shard is only used
 * once this node's own EB has actually stated the new count at least
 * once -- closes the last gap against the original design proposal, at
 * the cost of a short, bounded delay (roughly one EB period, ~12-16s
 * measured) before a genuine increase takes effect. */
#define CG_CONF_HOLD_BEFORE_USE 1

/* Root-adjacent cells' own (shorter) slotframe period, library default. */
/* ORCHESTRA_CONF_IA_ROOT_PERIOD left at its library default (7). */

/* Redirect every TSCH_CALLBACK_IA_* hook to this rule's own functions,
 * rather than orchestra-rule-implicit-ack.c's -- that file is not compiled
 * into this example, so leaving these to tsch.h's own TSCH_WITH_IMPLICIT_ACK
 * auto-defaults (which point at orchestra_ia_* names) would fail to link.
 * Only cg_get_own_children/cg_child_eb_input do real work; every other name
 * here is a harmless stub (see orchestra-rule-child-grandchild.c).
 * TSCH_CALLBACK_UNICAST_DATA_INPUT is wired above, conditionally on
 * CG_CONF_IDLE_RECLAIM -- this scheme doesn't need it to tell relayed
 * traffic apart from self-originated traffic (both use the same uplink
 * cells), but idle-reclaim does need it to tell real traffic apart from
 * empty TSCH keepalives. */
#define TSCH_CALLBACK_IMPLICIT_ACK_ACTIVE    cg_implicit_ack_active
#define TSCH_CALLBACK_IA_OVERHEAR            cg_overhear
#define TSCH_CALLBACK_IA_OWN_PARENT          cg_get_own_parent
#define TSCH_CALLBACK_IA_PARENT_EB           cg_parent_eb_input
#define TSCH_CALLBACK_IA_CONGESTION          cg_get_own_congestion
#define TSCH_CALLBACK_IA_PARENT_CONGESTION   cg_parent_congestion_input
#define TSCH_CALLBACK_IA_CONFIRMATION_CYCLES cg_confirmation_cycles
#define TSCH_CALLBACK_IA_OWN_CHILDREN        cg_get_own_children
#define TSCH_CALLBACK_IA_CHILD_EB            cg_child_eb_input
#define TSCH_CALLBACK_NEW_ASFN               cg_new_asfn

/* RPL storing mode: required for nbr_routes-based child_added/removed --
 * this is how this rule learns who its own direct RPL children are at all,
 * independent of the (now-removed) routing-table-traversal traffic
 * estimate that used to also depend on storing mode. */
#define RPL_CONF_MOP RPL_MOP_STORING_NO_MULTICAST

/* Library default (7); no implicit-ack-specific reason to change it here. */
#define TSCH_CONF_MAC_MAX_FRAME_RETRIES 7

/* QUEUEBUF_CONF_NUM: kept at the same value validated for
 * implicit-ack-tree-multicell at this same TSCH_CONF_IA_SFS_SIZE. */
#define QUEUEBUF_CONF_NUM 128

/* Shortened from the library default (31): the same bootstrap-time DAO/
 * default_common contention livelock found and fixed for
 * implicit-ack-tree-multicell applies identically here -- a node cannot
 * install its own uplink cell until its first DAO gets a link-layer ACK,
 * and until then must contend with every other simultaneously-bootstrapping
 * node for this one shared cell. */
#define ORCHESTRA_CONF_COMMON_SHARED_PERIOD 11

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
