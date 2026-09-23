<img src="https://github.com/contiki-ng/contiki-ng.github.io/blob/master/images/logo/Contiki_logo_2RGB.png" alt="Logo" width="200">

# APAS: Autonomous Position-Aware Slot Allocation for 6TiSCH IoT Networks

[![license](https://img.shields.io/badge/license-3--clause%20bsd-brightgreen.svg)](LICENSE.md)
[![Built on Contiki-NG](https://img.shields.io/badge/built%20on-Contiki--NG-blue)](https://github.com/contiki-ng/contiki-ng)

This repository contains the reference implementation of **APAS**, an autonomous, position-aware slot allocation scheme for 6TiSCH networks, built on top of [Contiki-NG](https://github.com/contiki-ng/contiki-ng). APAS sizes a node's TSCH cells directly from its position in the RPL topology — its own direct child count and RPL rank — rather than from a fixed rule or from traffic observed after the fact.

> Anindita Sarkar and Alakesh Kalita, *"Autonomous Position-Aware Slot Allocation for 6TiSCH IoT Networks,"* submitted to **IEEE Internet of Things Journal**.

## Why APAS

Existing autonomous 6TiSCH schedulers fall into two camps: fixed allocators (Orchestra's ORB/OSB, ALICE) that never adapt to traffic, and traffic-adaptive allocators (A³) that only grow a node's capacity *after* enough traffic has already built up — a signaling-lag defect that gets worse as networks scale and traffic converges toward the root. Every node in a 6TiSCH network already knows its own RPL rank and its own direct child count locally, at no extra signaling cost, the moment it joins the network — before any of its traffic exists to be observed. APAS uses exactly that.

APAS combines four mechanisms:

| Mechanism | What it does |
|---|---|
| **Structural floor** (`K_S = min(m_S + 1, K_max)`) | Grants a node cells the instant a child joins, sized by its direct child count, before that child ever transmits. |
| **Asymmetric load estimation** | Lets a genuinely busy node earn one extra cell from its own recent traffic, using a fast-rise/slow-fall EWMA so a single busy window doesn't cost a still-needed cell later. |
| **Idle reclaim** | Shrinks a child's cells back to its guaranteed minimum after sustained silence, freeing capacity for other nodes without waiting for RPL to remove it. |
| **Rank-aware cap** | Gives nodes near the border router (BR) a larger cap (`K_max = 6` vs. `4`), since their traffic converges from many subtrees at once. |

## Repository layout

This is a full Contiki-NG checkout; the APAS-specific additions are:

```
In OS folder  # the APAS mechanism
examples/6tisch/
├── child-grandchild-tree/           # base example, all 4 mechanisms configurable via project-conf.h
├── apas-iotlab-m3/                  # real FIT IoT-LAB deployment, 60 nodes (Grenoble, m3 boards)
```




## Real hardware deployment (FIT IoT-LAB)

APAS is also validated on real hardware: FIT IoT-LAB, Grenoble site, m3 boards (STM32F103RE + AT86RF231), 4-channel hopping ({15, 20, 25, 26}), −17 dBm TX power. Build for the `iotlab` target with the ARM GCC 10.3-2021.10 toolchain (pinned in `tools/docker/Dockerfile`):

```bash
cd examples/6tisch/apas-iotlab-m3       # or apas-iotlab-m3-150node for the 150-node deployment
make TARGET=iotlab BOARD=m3
iotlab-experiment submit -n apas-node -d <duration> -l grenoble,m3,<node-range>,build/iotlab/m3/node.iotlab
iotlab-node -i <experiment-id> --flash build/iotlab/m3/node.iotlab -l grenoble,m3,<node-range>
```

Each node's real MAC address is mapped to a logical id (1 = root) via `deployment-map.c` and the `services/deployment` module — a node prints its own MAC and role over serial on boot and every 30 seconds thereafter, which is how `deployment-map.c` is populated with real hardware addresses.

## Comparison with existing schedulers

APAS is compared against ORB, OSB, ALICE, and A³ — see the paper for the full slotframe-length, traffic-rate, and node-count sweep. Averaged across all three sweep dimensions, APAS improves packet delivery ratio by 54.3%, reduces radio duty cycle and end-to-end latency by 43.9% and 67.4% respectively, and improves throughput by 70.8%, compared to the baseline average.


## About Contiki-NG

This repository is built on [Contiki-NG](https://github.com/contiki-ng/contiki-ng), an open-source, cross-platform operating system for Next-Generation IoT devices, focused on dependable low-power communication and standard protocols (IPv6/6LoWPAN, 6TiSCH, RPL, CoAP). Unless explicitly stated otherwise, sources in this repository are distributed under the terms of the [3-clause BSD license](LICENSE.md).

* Upstream repository: https://github.com/contiki-ng/contiki-ng
* Upstream documentation: https://docs.contiki-ng.org/
