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

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

/* MRHOF's default (96, "Eq ETX of 0.75") is deliberately more aggressive
 * than RFC6719's own recommended 192 -- confirmed in testing as a major
 * contributor to this scheme's disassociation cascade at 25/49-node grid
 * scale (129 parent switches logged in one 60-minute, 24-node run): every
 * switch relocates every one of this node's own autonomous cells that are
 * keyed off its parent's address (UPLINK/ROOT_ADJACENT under the pairwise
 * hash, RELAY_RX/RELAY_TX on the parent's side either way), so reducing
 * switch frequency at the RPL layer helps this scheme specifically more
 * than it helps stock Orchestra's own, much smaller, per-node cell budget.
 * Reverting to RFC6719's own recommended value rather than picking an
 * arbitrary number. */
#define PARENT_SWITCH_THRESHOLD_CONF 192

/* 6TiSCH minimal schedule length, used only until the implicit-ack tree
 * rule's own slotframes take over scheduling of unicast/data traffic. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3

/* The implicit-ack tree rule uses sf_short + sf_root, plus one dedicated
 * relay_tx slotframe per direct child (up to ORCHESTRA_CONF_IA_MAX_CHILDREN
 * below -- each child's RELAY_TX must live in its own slotframe, separate
 * from our own UPLINK's sf_short and from every other child's, or the
 * shared per-neighbor Tx queue can hand a relayed frame to the wrong cell;
 * see update_child_links()'s comment in orchestra-rule-implicit-ack.c), on
 * top of one slotframe each for the EB and default-common rules: 4 fixed +
 * up to 6 relay_tx = 10. */
#define TSCH_SCHEDULE_CONF_MAX_SLOTFRAMES 10

/* Bumped from 4 for the 25/49-node grid topologies: a grid node has up to 4
 * orthogonal neighbors in radio range, any of which could pick it as RPL
 * parent, plus headroom for transient churn during convergence. */
#define ORCHESTRA_CONF_IA_MAX_CHILDREN 6

/* Bumped from 1 (the library default, see orchestra-conf.h) to give the
 * failure-triggered UPLINK rehash (uplink_shard_for()'s comment,
 * orchestra-rule-implicit-ack.c) an actual second shard to move to -- with
 * only 1 shard there is no alternative parent-side Rx cell for a rehash to
 * select, making it a no-op. This is a targeted, unilateral alternative to
 * the unconditional per-ASFN rotation tried and reverted above: only
 * UPLINK moves, and only once a persistent (not merely contended) failure
 * streak is actually observed. */
#define ORCHESTRA_CONF_IA_UPLINK_SHARDS 2

/* ORCHESTRA_IA_RELAY_TX_SHARDS (orchestra-conf.h): tried at 2 to relieve the
 * RELAY_TX[child] throughput ceiling exposed by ia_overhear_source_is_own()
 * (orchestra-rule-implicit-ack.c) -- see that constant's own comment for the
 * reasoning -- and reverted immediately: measured at 25 nodes/SF=101, it
 * made PDR *worse* (64.2%->27.9%), not better, the same network-wide
 * collision-pressure cost ORCHESTRA_IA_UPLINK_SHARDS's own comment already
 * documents (doubling a cell's count roughly doubles how many positions it
 * occupies in the shared hash address space) outweighing the within-queue
 * throughput gain here too. Left at the default (1) pending a genuinely
 * different lever; the shard mechanism itself (update_child_links()'s and
 * update_self_overhear()'s extra-shard loops) is left in place and correct,
 * just unused at this value. */

/* ASFN rotation was tried again this session specifically to fix the
 * permanent-collision failure mode this comment documents below, and
 * reverted again -- see ia_hash1_shard()'s comment in orchestra-rule-
 * implicit-ack.c for the measured 8/25/60-node data. It resolved the
 * targeted mechanism (RDC and "not able to re-synchronize" counts both
 * improved substantially at short TSCH_IA_SFS_SIZE) but cost more than it
 * saved once the resulting extra parent-switch churn was accounted for, and
 * regressed this file's own SFS_SIZE=101 configuration below. The
 * birthday-paradox math and historical context below (why 17 is risky, why
 * 31/101 were tried) still explains *why* short values are collision-prone
 * in the first place; only "does a collision ever go away" was retested.
 *
 * Since ASFN rotation was removed, every autonomous cell's (timeslot,
 * channel_offset) is now a permanent, one-time hash -- any two cells (across
 * any two nodes, not just our own) that happen to land on the same pair
 * collide every single occurrence, for as long as both nodes keep their
 * current addresses, instead of the collision self-resolving as ASFN used to
 * rotate it away. The default 4-channel hopping sequence only gives
 * TSCH_IA_SFS_SIZE(17) * 4 = 68 distinct (timeslot, channel) combinations to
 * spread an 8-node tree's ~30 autonomous cells across -- a birthday-paradox
 * collision is close to certain (>99% chance of at least one, by
 * n^2/(2M) with n~=30, M~=68).
 *
 * Switching to the 16-channel hopping sequence (TSCH_HOPPING_SEQUENCE_16_16)
 * was tried twice and reverted both times. The second attempt tried to
 * decouple EB reception from the wider channel set by pinning
 * TSCH_CONF_JOIN_HOPPING_SEQUENCE to the single physical channel EBs use
 * (ORCHESTRA_EB_MIN/MAX_CHANNEL_OFFSET's default channel_offset=1) -- this
 * looked promising (get_node_channel_offset() really does return a fixed
 * channel_offset independent of hopping sequence length) but was wrong about
 * what channel_offset means: 802.15.4e TSCH channel hopping rotates the
 * *actual* physical channel every occurrence via
 * hopping_sequence[(channel_offset + ASN) % length] -- channel_offset is a
 * fixed rotation *phase*, not a fixed physical channel. So EB's true channel
 * still cycled through all 16 entries over time even with channel_offset
 * pinned; a join scan parked on the one physical channel that phase
 * happened to resolve to at one particular ASN only ever catches ~1-in-16
 * occurrences instead of every one. Confirmed in testing: root-adjacent
 * nodes (which sync directly off the root's own coordinator EB, sent before
 * this rotation compounds across a relayed hop) still associated fine, but
 * every node one hop further out never associated at all in a 45-minute run
 * (0/5 vs the expected 5/5) -- a strictly worse failure mode than the first
 * attempt's slow-but-eventual joining. There's no cheap way to decouple join-
 * scan cost from data-cell channel diversity under the standard hopping
 * formula; enlarging TSCH_IA_SFS_SIZE remains the only lever that widens the
 * (timeslot, channel) address space without touching channel scanning/
 * joining at all -- it only affects already-associated nodes' own autonomous
 * cells -- at the cost of a proportionally longer cycle (and thus higher
 * latency) between any single cell's own occurrences. 31 (nearly double 17)
 * eliminated collisions almost entirely for the first two hops (99%+
 * delivery, zero RPL warnings, one disassociation in 15 minutes, down from
 * 5-8) -- strong confirmation of the collision hypothesis. 23 was tried as a
 * smaller compromise (all 8 nodes stay active within 15 minutes, but with
 * more residual collisions -- node3 back down to 18%) and rejected in favor
 * of 31 plus a longer test run instead, since deep nodes not finishing
 * convergence in 15 minutes is a matter of needing more real time with a
 * proportionally slower cycle, not a hard block.
 *
 * Larger values (127, 255) were tried against 25/49-node grids while this
 * had to stay pinned to stock Orchestra's own slot budget for a fair
 * same-budget comparison: 127 gave a real PDR gain at that scale (25-node
 * 41.0%->50.2%, 49-node 21.8%->33.2%), 255 regressed catastrophically (see
 * QUEUEBUF_CONF_NUM's comment -- the confirmation deadline scales with this
 * constant, and at 255 it grew long enough relative to the app's send
 * interval that genuinely-successful relays looked timed out). Given
 * implicit-ack's per-node cell count is structurally larger than stock
 * Orchestra's (own traffic + confirmation-listen + 2 cells per child, vs
 * stock's 1 cell per node) and there's no way to close that gap while also
 * holding this constant equal, matching stock's own slot budget number was
 * abandoned as the fairness criterion in favor of matching its *PDR* --
 * raised to 127, then set to 101 (a comparable, prime-numbered slotframe
 * length applied identically to stock Orchestra's own ORCHESTRA_CONF_
 * UNICAST_PERIOD in orchestra-baseline/project-conf.h, restoring "same
 * slotframe length for every scheme" as the fairness criterion, just at a
 * new common value rather than the original 17/31).
 *
 * Post-origin-fix addendum (ia_overhear_source_is_own(), orchestra-rule-
 * implicit-ack.c): that fix trades false confirmations (silent packet loss)
 * for honest retries, which costs real throughput at 150-node scale under
 * this file's matched SFS_SIZE=101 (PDR 15.8%->2.8%, paper Section
 * sec:falseneg). Re-tried 167 and 397 at 150 nodes only (unmatched to
 * Orchestra's own period, so not adopted as this file's default): 167 gave a
 * real recovery (PDR 14.2%, RDC 1.79%, actually beating Orchestra's own
 * matched-budget throughput there), but 397 reversed almost all of it (PDR
 * 3.2%, RDC 21.91%) -- the same confirmation-deadline-scales-with-SFS_SIZE
 * queuebuf pressure documented above for 255, just triggered again at a
 * larger value now that the origin fix removed the false-confirmation
 * "shortcut" that used to keep packets from staying pending as long. 167 is
 * a genuine sweet spot at this one scale, not a new safe default -- see the
 * paper's Section sec:sfrecovery for the full writeup. */
#define TSCH_CONF_IA_SFS_SIZE 101

/* A longer SFS_SIZE (above) was expected to just mean "slower, otherwise
 * equivalent" -- but a 45-minute run at SFS_SIZE=31 revealed a genuinely
 * different failure: node3's global queuebuf pool (QUEUEBUF_CONF_NUM, 64 by
 * default on this platform) hit 0 free buffers around 11 minutes in and
 * never recovered for the remaining 34 minutes, permanently blocking even
 * its own EB transmission (which cascaded into node4 never being able to
 * re-associate again). This does not happen at SFS_SIZE=17 or 23 (confirmed
 * absent from every prior test run's logs) -- it is specific to the larger
 * size: the implicit-ack confirmation deadline is TSCH_IA_CONFIRMATION_
 * TIMEOUT_CYCLES * TSCH_IA_SFS_SIZE, so packets now stay "in flight"
 * (queued, unconfirmed) proportionally longer before they can be retried or
 * dropped -- meaning more packets need to be queued *simultaneously* to
 * sustain the same throughput, and the buffer pool sized for the faster
 * (17-cycle) case is no longer enough. Doubling it gives the larger cycle
 * the same effective in-flight headroom the smaller one had.
 *
 * Raising this further (128->192) was tried once SFS_SIZE=101 comparisons
 * showed real, measured queuebuf exhaustion at 25-node scale ("can't send
 * packet ... queue 128/128", ~2000 occurrences in one 60-minute run) --
 * reverted immediately: tsch-conf.h rounds QUEUEBUF_CONF_NUM up to the next
 * power of two for TSCH_QUEUE_NUM_PER_NEIGHBOR, and above 128 that becomes
 * 256, which overflows the uint8_t ringbufindex_init() takes it in
 * (tsch-queue.c) -- a hard compile-time ceiling, not a tunable one. 128 is
 * the largest safe value here. See TSCH_CONF_IA_CONFIRMATION_TIMEOUT_
 * CYCLES's comment for the other half of this fix instead: shortening the
 * in-flight window itself, rather than trying to grow the buffer that has
 * to absorb it past where it can go. */
#define QUEUEBUF_CONF_NUM 128

/* TSCH_CONF_IA_CONFIRMATION_TIMEOUT_CYCLES (a fixed compile-time deadline of
 * N*TSCH_IA_SFS_SIZE ASN ticks) was swept across 1/2/3 cycles at 8/25/49/60
 * nodes:
 *
 *   cycles   8-node   25-node   49-node   60-node
 *      1      95.1%     77.4%     49.7%     29.6%
 *      2      91.4%     75.2%     46.0%     25.0%
 *      3      89.4%     53.1%     43.8%     47.6%
 *
 * At <=49 nodes, lower is strictly better, and 1 is the best of the three
 * tried -- diagnostic counters (IATRACE armed/timeout, "drop dup ll") show
 * this isn't free (duplicate-retransmit waste does rise as cycles drops),
 * but the reduced wait time wins by a wider margin than that cost at this
 * scale. At 60 nodes both 1 and 2 collapse to roughly the same bad outcome
 * (~25-30%, vs. 3's 47.6%) -- confirmed as the *same* failure mode: the
 * per-child dedicated relay cells this scheme relies on need more than 1-2
 * cycles to get a turn on a busy relay node once enough traffic converges
 * there, and cutting the deadline just marks in-progress relays as failed
 * early, piling more retries onto the exact nodes that are already the
 * bottleneck.
 *
 * A per-node adaptive replacement (TSCH_CALLBACK_IA_CONFIRMATION_CYCLES,
 * each node estimating its own deadline from its own observed confirm/
 * timeout history, TCP-RTO-style) was designed and tested through three
 * iterations, all reverted: every version landed at essentially the same
 * ~25-27% PDR at 60 nodes as the plain fixed 2-cycle deadline, despite
 * internally behaving very differently (confirmed via IATRACE armed/
 * deadline logging that the final version really was escalating individual
 * nodes' cycles values, not stuck). Root cause: this deadline only governs
 * a node's own self-originated uplink traffic -- a relayed child's frame is
 * resolved synchronously at radio-Tx-success time regardless of it (see
 * tsch_tx_slot()'s own comment) -- so when a node's own deadline is too
 * short, the resulting extra retry lands as load on its *parent's* relay
 * queue, not its own. A node's own confirmation history can't see
 * congestion it causes one hop away, which is exactly where the 60-node
 * bottleneck lives -- a purely local per-node signal is the wrong tool for
 * a problem that's inherently about parent-child interaction.
 *
 * Replacement mechanism now in place: TSCH_CALLBACK_IA_CONFIRMATION_CYCLES
 * (orchestra_ia_confirmation_cycles(), orchestra-rule-implicit-ack.c) is
 * driven by an actual congestion signal crossing the parent-child boundary,
 * not a local counter -- every EB now also carries the sender's own current
 * queue depth toward *its* parent (ie_ia_parent_congestion, frame802154e-
 * ie.h/.c; TSCH_CALLBACK_IA_CONGESTION/TSCH_CALLBACK_IA_PARENT_CONGESTION,
 * tsch.h), and each node picks 1/2/3 cycles by thresholding its PARENT's
 * self-reported depth (ORCHESTRA_CONF_IA_CONGESTION_LOW/HIGH_THRESHOLD,
 * orchestra-conf.h, default 4/16) rather than anything it observed itself.
 * This constant is now only the static fallback used if
 * TSCH_CALLBACK_IA_CONFIRMATION_CYCLES is ever undefined (e.g. a build
 * without BUILD_WITH_ORCHESTRA's default wiring) -- kept at 1, the best
 * single fixed value for <=49 nodes, purely as that fallback. This constant
 * is always a multiple of TSCH_IA_SFS_SIZE (see its use in tsch-slot-
 * operation.c), never a fixed absolute time -- "N cycles" and "N slotframe
 * lengths" are the same thing by construction.
 *
 * Measured results (8/25/49/60 nodes), one single build, no scale-specific
 * config, against the best fixed value found for each scale:
 *
 *   scale   best fixed cycles   congestion-driven (this mechanism)
 *      8       95.1% (c=1)        95.1%  (matches)
 *     25       77.4% (c=1)        79.8%  (beats it)
 *     49       49.7% (c=1)        42.8%  (below, see below)
 *     60       47.6% (c=3)        46.5%  (essentially matches)
 *
 * First pass at 49/60 nodes was much worse (38.5%/25.7%) and traced to a
 * real bug, not an inherent cost of the mechanism: tsch_queue_add_packet()
 * (tsch-queue.c) never initialized a freshly allocated packet's ia_pending
 * field, and memb_alloc() doesn't zero its blocks -- a brand-new,
 * never-transmitted packet could inherit a stale ia_pending=1 left by
 * whatever packet previously occupied that pool slot, get permanently
 * skipped by the queue's scan-forward logic (mistaken for "already sent,
 * awaiting confirmation"), and sit until the timeout sweep eventually
 * force-cleared it -- confirmed via "IATRACE timeout ... transmissions=0"
 * (a timeout fired on a packet that was never actually radio-transmitted)
 * and via specific nodes stuck at 0% delivery for ~90% of the run. This hit
 * high-churn (high-congestion) neighbors hardest, and got much worse once
 * deadlines could exceed 1 cycle -- exactly why it only surfaced now. Fixed
 * by explicitly zeroing ia_pending in tsch_queue_add_packet(); the numbers
 * above are post-fix. The remaining 49-node gap (42.8% vs. 49.7%) looks
 * like a smaller, genuine cost: the most heavily-escalated nodes (e.g. one
 * node at 86.5% escalation rate) still measurably underperform their own
 * fixed-cycles=1 baseline, consistent with "wait longer" competing with
 * queue throughput even once correctly informed and bug-free.
 *
 * Reverted to the plain fixed-cycles version (ORCHESTRA_CONF_IA_CONGESTION_
 * ADAPTIVE 0, orchestra-conf.h): this was the specific configuration that
 * measured >45% (49.7%) PDR at 49 nodes, the best of any single value tried
 * at that scale. The congestion-driven mechanism's plumbing (EB signaling,
 * thresholds) is left in place and can be re-enabled by flipping that one
 * knob back to 1.
 *
 * Above sweep was all at one TSCH_IA_SFS_SIZE; re-checked at SFS_SIZE=167,
 * 8 nodes, after digging into an unexplained node-to-node confirm-rate
 * spread (45%-94.5%) that two other hypotheses (UPLINK/SELF_OVERHEAR slot
 * ordering; chain depth/aggregate relayed volume) both failed to explain --
 * cycles=2 there raised confirm rate 35.3%->77.1% and PDR 67.6%->79.5% with
 * RDC unchanged, a real fix for that specific case. But it is not a better
 * default: the exact same change measured on SFS_SIZE=101 (this file's own
 * value) *regressed* 8-node PDR from 88.3% to 55.2% -- confirming the
 * "wait longer costs queue throughput" tradeoff above is real and cuts the
 * other way once the deadline was already long enough. Net: the right
 * number of cycles depends on SFS_SIZE, not just node count, and 1 remains
 * correct for this file's SFS_SIZE=101. */
#define TSCH_CONF_IA_CONFIRMATION_TIMEOUT_CYCLES 1
#define ORCHESTRA_CONF_IA_CONGESTION_ADAPTIVE 0

/* Raising TSCH_MAC_MAX_FRAME_RETRIES from its default (7, unchanged from
 * stock Orchestra's own default) was tried and reverted. Motivation: an
 * 8-node diagnostic (cycles=1) showed loss concentrated almost entirely on
 * one deep/weak-link node (23 of 29 total lost packets), zero
 * disassociation, no collision blowup -- consistent with the multiplicative
 * per-hop loss this scheme was already known to suffer, so giving that node
 * more chances seemed like a free win. It was: 8-node PDR rose to 96.4%.
 * But at 25 nodes the *same* change collapsed PDR from 77.4% to 40.5%
 * (disassociation events also rose, 12->24) -- a struggling packet held
 * onto for more attempts also holds its queuebuf slot for longer, and at
 * 25-node scale the queuebuf pool is already under real, measured pressure
 * (see QUEUEBUF_CONF_NUM's own comment on "can't send packet ... 128/128");
 * more patience for one packet just delays freeing that slot for the next
 * one, worsening the exact bottleneck that was already tight. The same
 * "helps when there's slack, hurts once the network is already
 * resource-constrained" pattern as TSCH_CONF_IA_CONFIRMATION_TIMEOUT_
 * CYCLES above, on a different resource (queuebuf slots instead of relay
 * cell turns) -- and since 25 nodes is squarely within this file's own
 * <=49-node target range, this was a net loss for that goal, not a
 * scale-specific tradeoff worth keeping.
 *
 * Reducing it instead (the opposite direction, 7->3, i.e. 8->4 total
 * attempts) was also tried and also reverted -- the "both should improve"
 * reasoning (fewer attempts frees the queuebuf slot sooner *and* costs less
 * radio-on time) turned out to only half-hold in testing:
 *
 *   scale     PDR (7 vs. 3)      RDC (7 vs. 3)
 *   8-node    95.1% -> 94.3%     5.84% -> 7.11%  (both worse)
 *   25-node   77.4% -> 67.8%     7.58% -> 6.16%  (RDC better, PDR worse)
 *   49-node   49.7% -> 42.8%     8.70% -> 9.84%  (both worse)
 *
 * PDR regressed at every scale, and RDC only improved at 25 nodes. The
 * asymmetry the "should help" reasoning missed: most packets already
 * succeed within the first 1-2 attempts (especially at cycles=1, see
 * above), so only a small fraction of all traffic was ever reaching
 * attempts 5-8 in the first place -- cutting them away saves little
 * aggregate radio time (small fraction of traffic affected) while
 * permanently losing exactly the packets that needed those later attempts
 * to eventually succeed (a real, full-sized PDR cost, not a small one).
 * Left at the library default (7) -- neither direction from it was a net
 * win once actually measured. */
#define TSCH_CONF_MAC_MAX_FRAME_RETRIES 7

/* CSMA backoff on shared cells (UPLINK/RELAY_RX/RELAY_TX all use LINK_
 * OPTION_SHARED): after a collision, the next attempt is delayed by a
 * random number of *occurrences of that same cell* (not raw timeslots) --
 * 0 to 2^backoff_exponent-1, starting at TSCH_MAC_MIN_BE and growing by 1
 * per further collision up to TSCH_MAC_MAX_BE. Capping MAX_BE from its
 * default (5, up to 31 skipped cycles after repeated collisions -- our
 * shared cells only recur once per TSCH_IA_SFS_SIZE-slot cycle, so this can
 * impose a very long worst-case wait) down to 3 was tried and reverted --
 * the same "give up on patience sooner" pattern as the other two knobs
 * above, on a third, previously-untried mechanism (radio-level collision
 * backoff, rather than missed-confirmation or retry-count):
 *
 *   scale     PDR (5 vs. 3)      RDC (5 vs. 3)
 *   8-node    95.1% -> 95.3%     5.84% -> 5.02%  (both better)
 *   25-node   77.4% -> 74.0%     7.58% -> 7.92%  (both worse)
 *   49-node   49.7% -> 38.7%     8.70% -> 6.72%  (PDR much worse, RDC better)
 *
 * A clean win only at 8 nodes; a real PDR cost by 25, a severe one by 49.
 * Three different mechanisms (confirmation deadline, retry budget, CSMA
 * backoff cap) have now all shown the identical shape: shortening how long
 * a node waits before giving up always helps the least-contended case and
 * always costs real delivery once there's genuine contention to resolve.
 * Left at the library default (5). */

/* EB slotframe length: back to the library default of 397 (was shortened to
 * 167 for a time -- see git history) to isolate whether that shortening, or
 * removing sf_child_eb/sf_control_uplink, was actually responsible for this
 * session's PDR recovery. */
#define ORCHESTRA_CONF_EBSF_PERIOD 397

/* Radio duty cycle measurement (node.c's periodic RDC report), for comparing
 * energy cost against stock Orchestra scheduling. */
#define ENERGEST_CONF_ON 1

#define LOG_CONF_LEVEL_RPL     LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_TCPIP   LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6    LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC     LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER  LOG_LEVEL_WARN

#define TSCH_LOG_CONF_PER_SLOT 1

#endif /* PROJECT_CONF_H_ */
