#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_targeted_corpus.sh}"
script_dir="$(cd "$(dirname "${script}")" && pwd)"
summary_script="${script_dir}/summarize_v6_targeted_corpus.py"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

build_dir="${tmp_dir}/build"
bin_dir="${tmp_dir}/bin"
log_file="${tmp_dir}/calls.log"
mkdir -p "${build_dir}" "${bin_dir}"

cat > "${bin_dir}/cmake" <<'CMAKE'
#!/usr/bin/env bash
set -euo pipefail
echo "fake cmake $*" >> "${FAKE_TARGETED_LOG}"
CMAKE

cat > "${build_dir}/rubik-cache-setup" <<'CACHE'
#!/usr/bin/env bash
set -euo pipefail
echo "cache_setup,status,Ready"
echo "cache_setup,effective_profile,large-local"
echo "cache_setup,payload_bytes,1392639935"
echo "cache_setup,cache_warm,true"
echo "cache_setup,bytes_missing,0"
echo "cache_setup,message,dry run: cache warm"
CACHE

cat > "${build_dir}/rubik-bench" <<'BENCH'
#!/usr/bin/env bash
set -euo pipefail

seed=""
start_index=""
count=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --random-seed)
            seed="$2"
            shift 2
            ;;
        --random-start-index)
            start_index="$2"
            shift 2
            ;;
        --random-count)
            count="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [[ -z "${seed}" || -z "${start_index}" || -z "${count}" ]]; then
    echo "fake bench missing random options" >&2
    exit 1
fi

echo "case,case_depth,scramble,status,optimal,moves,initial_lower_bound,elapsed_ms,nodes_expanded,nodes_per_ms,max_depth,timeout_ms,nodes_by_depth,solution,optimal_move_ordering,root_ordering_profile"
for ((offset = 0; offset < count; offset += 1)); do
    case_index=$((start_index + offset))
    case "${case_index}" in
        1|4)
            profile="root_ordering_mode=default;scheduler=adaptive;adaptive_decision=root;adaptive_reason=conservative_root;adaptive_lb=8;adaptive_max_depth=15;adaptive_threads=16;adaptive_strong_min_count=11;adaptive_first_diff=1"
            ;;
        2)
            profile="root_ordering_mode=default;scheduler=adaptive;adaptive_decision=root;adaptive_reason=conservative_root;adaptive_lb=8;adaptive_max_depth=15;adaptive_threads=16;adaptive_strong_min_count=7;adaptive_first_diff=1"
            ;;
        *)
            profile="root_ordering_mode=default;scheduler=adaptive;adaptive_decision=deep_split;adaptive_reason=lb9_mid_strong_min;adaptive_lb=9;adaptive_max_depth=15;adaptive_threads=16;adaptive_strong_min_count=4;adaptive_first_diff=0"
            ;;
    esac
    echo "random_${seed}_${case_index},15,R,Optimal,true,15,8,$((1000 + case_index)),20000,20.0,15,30000,1|2,R,auto_strong_bound,${profile}"
done
BENCH

cat > "${tmp_dir}/fake_sweep.sh" <<'SWEEP'
#!/usr/bin/env bash
set -euo pipefail

output_dir=""
corpus_file=""
candidates=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        --corpus-file)
            corpus_file="$2"
            shift 2
            ;;
        --candidates)
            candidates="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [[ -z "${output_dir}" || -z "${corpus_file}" || "${candidates}" != "phase2_tiebreak" ]]; then
    echo "fake sweep missing required values" >&2
    exit 1
fi

grep -q "hardening,42,1,15,1,conservative_root" "${corpus_file}"
grep -q "hardening,42,2,15,1,conservative_root" "${corpus_file}"
grep -q "hardening,42,4,15,1,conservative_root" "${corpus_file}"
if grep -q "hardening,42,3,15,1,conservative_root" "${corpus_file}"; then
    echo "non-target profile entered corpus" >&2
    exit 1
fi

mkdir -p "${output_dir}"
cat > "${output_dir}/summary.csv" <<'CSV'
candidate,common_cases,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_max_elapsed_ms,candidate_max_elapsed_ms,max_elapsed_delta_ms,baseline_nodes,candidate_nodes,nodes_delta,winner,comparison_file
phase2_tiebreak,2,2000,1900,-100,-5.00,1001,950,-51,40000,39000,-1000,candidate,comparison.csv
CSV
mkdir -p "${output_dir}/phase2_tiebreak"
cat > "${output_dir}/phase2_tiebreak/comparison.csv" <<'CSV'
case_key,common_cases,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_nodes,candidate_nodes,nodes_delta,baseline_wall_ms,candidate_wall_ms,wall_delta_ms,winner,baseline_ordering,candidate_ordering,baseline_reason,candidate_reason
hardening:depth15:seed42:random_42_1,,1001,950,-51,-5.09,20000,19000,-1000,0,0,0,candidate,default,phase2_tiebreak,conservative_root,conservative_root
hardening:depth15:seed42:random_42_2,,1002,1050,48,4.79,20000,20100,100,0,0,0,baseline,default,phase2_tiebreak,conservative_root,conservative_root
hardening:depth15:seed42:random_42_4,,1004,960,-44,-4.38,20000,19100,-900,0,0,0,candidate,default,phase2_tiebreak,conservative_root,conservative_root
__summary__,3,3007,2960,-47,-1.56,60000,58200,-1800,0,0,0,candidate,,,,
CSV
SWEEP

chmod +x "${bin_dir}/cmake" "${build_dir}/rubik-cache-setup" "${build_dir}/rubik-bench" "${tmp_dir}/fake_sweep.sh"

FAKE_TARGETED_LOG="${log_file}" PATH="${bin_dir}:${PATH}" "${script}" \
    --build-dir "${build_dir}" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --seeds 42 \
    --random-count 3 \
    --random-start-indices 1,4 \
    --target-profiles 8:7:1,8:11:1 \
    --min-target-cases 3 \
    --sweep-script "${tmp_dir}/fake_sweep.sh" \
    --summary-script "${summary_script}" \
    > "${tmp_dir}/run.out" 2>&1

grep -q "fake cmake --build ${build_dir} --target rubik-bench rubik-cache-setup" "${log_file}"
grep -q "42,1,random_42_1,8,11,1,1001,20000" "${tmp_dir}/out/targeted_cases.csv"
grep -q "42,2,random_42_2,8,7,1,1002,20000" "${tmp_dir}/out/targeted_cases.csv"
grep -q "42,4,random_42_4,8,11,1,1004,20000" "${tmp_dir}/out/targeted_cases.csv"
test -f "${tmp_dir}/out/ordering-sweep/summary.csv"
grep -q "8:11:1,8,11,1,hardening:depth15:seed42:random_42_1" "${tmp_dir}/out/case_summary.csv"
grep -q "8:7:1,8,7,1,1,0,1" "${tmp_dir}/out/profile_summary.csv"
grep -q "8:11:1,8,11,1,2,2,0" "${tmp_dir}/out/profile_summary.csv"
