#!/usr/bin/env bash
# Reproduce PA3_IoTJournal.tex's Table 2 (tab:mainresults): builds and runs
# both designs -- the rejected "Subtree" baseline (subtree-traffic-tree/)
# and the adopted "Event-driven" design D.1, vanilla, matching the exact
# config that produced the published numbers (child-grandchild-tree-vanilla/)
# -- across all six topologies, 1 hour of simulated time each, exactly the
# methodology in the paper's Section "Metric Definitions".
#
# Usage:
#   ./run_table2.sh [output_dir]          # runs everything (~12 runs, ~4-5
#                                          # real minutes each = ~50-60 min)
#   ./run_table2.sh [output_dir] N        # only network size N, both designs
#
# Requires: java (with --enable-preview support -- this repo's own
# tools/cooja/build/libs/cooja-full.jar was built with Java preview
# features enabled), and a working `make TARGET=cooja` toolchain (already
# proven to work in this repo -- see any of the three example dirs below).
#
# Output: <output_dir>/<design>_n<N>/COOJA.testlog per run. Parse with:
#   python3 parse_table2.py <output_dir>/*
set -euo pipefail

# Locate the repo root robustly, regardless of where this script is run
# from or has been copied to: prefer the known install location, else
# search upward from this script's own location for the tools/cooja
# build marker, so a copy left in some other directory (e.g. inside
# another example dir) still finds everything it depends on.
KNOWN_ROOT="/home/winds-lab/contiki-ng_NACK"
if [ -f "$KNOWN_ROOT/tools/cooja/build/libs/cooja-full.jar" ] \
   && [ -d "$KNOWN_ROOT/examples/6tisch/subtree-traffic-tree" ]; then
  ROOT="$KNOWN_ROOT"
else
  d="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  ROOT=""
  while [ "$d" != "/" ]; do
    if [ -f "$d/tools/cooja/build/libs/cooja-full.jar" ]; then
      ROOT="$d"
      break
    fi
    d="$(dirname "$d")"
  done
  if [ -z "$ROOT" ]; then
    echo "ERROR: could not locate the contiki-ng_NACK repo root (looked for" >&2
    echo "tools/cooja/build/libs/cooja-full.jar walking up from this script's" >&2
    echo "own directory). Run this from inside the repo, or edit KNOWN_ROOT" >&2
    echo "at the top of this file to point at your checkout." >&2
    exit 1
  fi
fi
COOJA_JAR="$ROOT/tools/cooja/build/libs/cooja-full.jar"
DESIGN_BASE="$ROOT/examples/6tisch"

if [ ! -f "$COOJA_JAR" ]; then
  echo "ERROR: $COOJA_JAR not found. Build Cooja first:" >&2
  echo "  cd $ROOT/tools/cooja && ./gradlew build" >&2
  exit 1
fi

OUTDIR="${1:-./table2_results}"
ONLY_N="${2:-}"
mkdir -p "$OUTDIR"

declare -A TOPO_FILE=(
  [8]="rpl-tsch-cooja-8node_my_scriptcopy.csc"
  [25]="grid25_scriptcopy.csc"
  [49]="grid49_scriptcopy.csc"
  [60]="grid60_scriptcopy.csc"
  [100]="grid100_scriptcopy.csc"
  [150]="grid150_scriptcopy.csc"
)
DESIGNS=("subtree-traffic-tree" "child-grandchild-tree-vanilla")
SIZES=(8 25 49 60 100 150)
if [ -n "$ONLY_N" ]; then
  if [ -z "${TOPO_FILE[$ONLY_N]+x}" ]; then
    echo "ERROR: N=$ONLY_N is not one of the six topology sizes: ${!TOPO_FILE[@]}" >&2
    exit 1
  fi
  SIZES=("$ONLY_N")
fi

run_one() {
  local design="$1" n="$2"
  local dir="$DESIGN_BASE/$design"
  local csc="${TOPO_FILE[$n]}"
  local logdir="$OUTDIR/${design}_n${n}"

  if [ ! -d "$dir" ]; then
    echo "ERROR: $dir does not exist" >&2
    return 1
  fi
  if [ ! -f "$dir/$csc" ]; then
    echo "ERROR: $dir/$csc does not exist" >&2
    return 1
  fi

  mkdir -p "$logdir"
  echo "=== $design, N=$n : building ==="
  ( cd "$dir" && make TARGET=cooja clean >/dev/null 2>&1 )

  echo "=== $design, N=$n : running (1h simulated, expect ~4-5 real min) ==="
  if java --enable-preview -jar "$COOJA_JAR" \
      --no-gui --autostart --logdir="$logdir" \
      --contiki="$ROOT" \
      "$dir/$csc" > "$logdir/console.log" 2>&1; then
    :
  else
    # Cooja's own test-runner exits nonzero whenever the .csc script hits
    # TIMEOUT without an explicit pass condition -- expected for these
    # open-ended log-dumping scripts, not a real failure. A genuine crash
    # (Java exception, no COOJA.testlog produced) is still visible below.
    :
  fi
  if [ -f "$logdir/COOJA.testlog" ]; then
    echo "  -> $logdir/COOJA.testlog ($(du -h "$logdir/COOJA.testlog" | cut -f1))"
  else
    echo "  !! no COOJA.testlog produced -- check $logdir/console.log" >&2
  fi
}

for design in "${DESIGNS[@]}"; do
  for n in "${SIZES[@]}"; do
    run_one "$design" "$n"
  done
done

echo ""
echo "All runs complete. Parse with:"
echo "  python3 $DESIGN_BASE/parse_table2.py $OUTDIR"/*
