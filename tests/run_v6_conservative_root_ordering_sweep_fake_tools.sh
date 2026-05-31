#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_ordering_sweep.sh}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

probe_script="${tmp_dir}/fake_probe.sh"
compare_script="${tmp_dir}/fake_compare.py"
log_file="${tmp_dir}/calls.log"

cat > "${probe_script}" <<'PROBE'
#!/usr/bin/env bash
set -euo pipefail

output_dir=""
candidate_env=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        --candidate-env)
            candidate_env="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [[ -z "${output_dir}" || -z "${candidate_env}" ]]; then
    echo "fake probe missing required values" >&2
    exit 1
fi

mkdir -p "${output_dir}/default" "${output_dir}/candidate"
printf '%s\n' "${candidate_env}" >> "${FAKE_SWEEP_LOG}"
cat > "${output_dir}/cache_setup.csv" <<'CSV'
cache_setup,status,Ready
cache_setup,effective_profile,large-local
cache_setup,payload_bytes,1392639935
cache_setup,cache_warm,true
cache_setup,bytes_missing,0
cache_setup,message,dry run: cache warm
CSV
touch "${output_dir}/default/warm_fake.csv" "${output_dir}/candidate/warm_fake.csv"
PROBE

cat > "${compare_script}" <<'COMPARE'
#!/usr/bin/env python3
import csv
import sys
from pathlib import Path

candidate_dir = ""
output = ""
args = sys.argv[1:]
for index, token in enumerate(args):
    if token == "--candidate-dir":
        candidate_dir = args[index + 1]
    if token == "--output":
        output = args[index + 1]

if not candidate_dir or not output:
    print("fake compare missing values", file=sys.stderr)
    sys.exit(1)

name = Path(candidate_dir).parent.name
delta = {
    "reverse_tie": -100,
    "high_bound_first": 25,
    "phase2_tiebreak": -50,
    "positive_high_bound": -75,
}[name]
candidate_elapsed = 1000 + delta
nodes_delta = {
    "reverse_tie": -200,
    "high_bound_first": 50,
    "phase2_tiebreak": 0,
    "positive_high_bound": -125,
}[name]
candidate_nodes = 10000 + nodes_delta

Path(output).parent.mkdir(parents=True, exist_ok=True)
with open(output, "w", newline="") as handle:
    writer = csv.writer(handle)
    writer.writerow([
        "case_key",
        "common_cases",
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "elapsed_delta_ms",
        "elapsed_delta_percent",
        "baseline_p50_ms",
        "candidate_p50_ms",
        "p50_delta_ms",
        "baseline_p90_ms",
        "candidate_p90_ms",
        "p90_delta_ms",
        "baseline_p95_ms",
        "candidate_p95_ms",
        "p95_delta_ms",
        "baseline_p99_ms",
        "candidate_p99_ms",
        "p99_delta_ms",
        "baseline_max_elapsed_ms",
        "candidate_max_elapsed_ms",
        "max_elapsed_delta_ms",
        "baseline_nodes",
        "candidate_nodes",
        "nodes_delta",
        "baseline_wall_ms",
        "candidate_wall_ms",
        "wall_delta_ms",
        "winner",
        "baseline_ordering",
        "candidate_ordering",
        "baseline_reason",
        "candidate_reason",
    ])
    writer.writerow([
        "__summary__",
        5,
        1000,
        candidate_elapsed,
        delta,
        f"{delta / 10:.2f}",
        200,
        190,
        -10,
        300,
        290,
        -10,
        400,
        390,
        -10,
        500,
        490,
        -10,
        500,
        500 + max(delta, 0),
        max(delta, 0),
        10000,
        candidate_nodes,
        nodes_delta,
        600,
        600,
        0,
        "candidate" if delta < 0 else "baseline",
        "",
        "",
        "",
        "",
    ])
print(f"fake comparison: {output}")
COMPARE

chmod +x "${probe_script}" "${compare_script}"

FAKE_SWEEP_LOG="${log_file}" "${script}" \
    --build-dir "${tmp_dir}/build" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --corpus-file benchmarks/v6_conservative_root_corpus.csv \
    --probe-script "${probe_script}" \
    --compare-script "${compare_script}" \
    --candidates reverse_tie,high_bound_first,phase2_tiebreak,positive_high_bound \
    > "${tmp_dir}/run.out" 2>&1

grep -q "RUBIK_EXPERIMENTAL_ROOT_ORDERING=reverse_tie" "${log_file}"
grep -q "RUBIK_EXPERIMENTAL_ROOT_ORDERING=high_bound_first" "${log_file}"
grep -q "RUBIK_EXPERIMENTAL_ROOT_ORDERING=phase2_tiebreak" "${log_file}"
grep -q "RUBIK_EXPERIMENTAL_ROOT_ORDERING=positive_high_bound" "${log_file}"

summary="${tmp_dir}/out/summary.csv"
test -f "${summary}"
grep -q "reverse_tie,5,1000,900,-100,-10.00,500,500,0,10000,9800,-200,candidate" "${summary}"
grep -q "high_bound_first,5,1000,1025,25,2.50,500,525,25,10000,10050,50,baseline" "${summary}"
grep -q "phase2_tiebreak,5,1000,950,-50,-5.00,500,500,0,10000,10000,0,candidate" "${summary}"
grep -q "positive_high_bound,5,1000,925,-75,-7.50,500,500,0,10000,9875,-125,candidate" "${summary}"
