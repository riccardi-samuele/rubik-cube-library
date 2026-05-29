#!/usr/bin/env python3
import argparse
import csv
import re
import sys
from pathlib import Path


USAGE = "Usage: scripts/compare_v6_latency.py [options]"


def parse_args():
    value_options = {"--baseline-dir", "--candidate-dir", "--output"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/compare_v6_latency.py",
        usage="%(prog)s [options]",
        add_help=False,
        allow_abbrev=False,
    )
    parser.add_argument("--baseline-dir", default="")
    parser.add_argument("--candidate-dir", default="")
    parser.add_argument("--output", default="")
    parser.add_argument("-h", "--help", action="store_true")
    args, unknown = parser.parse_known_args()

    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --baseline-dir DIR   baseline V6 benchmark output directory\n"
            "  --candidate-dir DIR  candidate V6 benchmark output directory\n"
            "  --output FILE        write comparison CSV to file instead of stdout\n"
            "  -h, --help           show this help"
        )
        sys.exit(0)
    if unknown or not args.baseline_dir or not args.candidate_dir:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if args.baseline_dir.startswith("-") or args.candidate_dir.startswith("-") or args.output.startswith("-"):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return (
        Path(args.baseline_dir),
        Path(args.candidate_dir),
        Path(args.output) if args.output else None,
    )


def int_value(value):
    try:
        return int(value)
    except (TypeError, ValueError):
        return 0


def percent(delta, baseline):
    if baseline == 0:
        return "0.00"
    return f"{delta * 100.0 / baseline:.2f}"


def winner(delta):
    return "candidate" if delta < 0 else "baseline"


def percentile(values, fraction):
    if not values:
        return 0
    ordered = sorted(values)
    index = int((len(ordered) - 1) * fraction)
    return ordered[index]


def profile_value(profile, key):
    prefix = f"{key}="
    for piece in profile.split(";"):
        if piece.startswith(prefix):
            return piece[len(prefix):]
    return ""


def case_key(path, row):
    match = re.search(r"(tail|hardening).*depth_(\d+)_seed_(\d+)", path.name)
    if match is None:
        return f"{path.name}:{row.get('case_name', '')}"
    suite, depth, seed = match.groups()
    return f"{suite}:depth{depth}:seed{seed}:{row.get('case_name', '')}"


def load_run(root):
    if not root.is_dir():
        print(f"comparison failed: input directory not found: {root}", file=sys.stderr)
        sys.exit(1)

    rows = {}
    order = []
    for path in sorted(root.rglob("warm_*.csv")):
        if path.name.endswith("_summary.csv"):
            continue
        wall_elapsed_ms = 0
        file_rows = []
        with path.open(newline="") as handle:
            reader = csv.reader(handle)
            header = []
            for raw in reader:
                if not raw:
                    continue
                if raw[0] == "benchmark" and len(raw) >= 3 and raw[1] == "wall_elapsed_ms":
                    wall_elapsed_ms = int_value(raw[2])
                    continue
                if raw[0] == "case":
                    header = raw
                    header[0] = "case_name"
                    continue
                if raw[0].startswith("random_") and header:
                    row = dict(zip(header, raw))
                    file_rows.append(row)
        for row in file_rows:
            key = case_key(path, row)
            profile = row.get("root_ordering_profile", "")
            rows[key] = {
                "case_key": key,
                "elapsed_ms": int_value(row.get("elapsed_ms")),
                "nodes": int_value(row.get("nodes_expanded")),
                "wall_ms": wall_elapsed_ms,
                "status": row.get("status", ""),
                "move_count": int_value(row.get("moves")),
                "ordering": profile_value(profile, "root_ordering_mode"),
                "adaptive_reason": profile_value(profile, "adaptive_reason"),
            }
            order.append(key)
    return rows, order


def compare(baseline_dir, candidate_dir):
    baseline_rows, baseline_order = load_run(baseline_dir)
    candidate_rows, _ = load_run(candidate_dir)

    rows = []
    for key in baseline_order:
        if key not in candidate_rows:
            continue
        baseline = baseline_rows[key]
        candidate = candidate_rows[key]
        elapsed_delta = candidate["elapsed_ms"] - baseline["elapsed_ms"]
        nodes_delta = candidate["nodes"] - baseline["nodes"]
        wall_delta = candidate["wall_ms"] - baseline["wall_ms"]
        rows.append({
            "case_key": key,
            "common_cases": "",
            "baseline_elapsed_ms": baseline["elapsed_ms"],
            "candidate_elapsed_ms": candidate["elapsed_ms"],
            "elapsed_delta_ms": elapsed_delta,
            "elapsed_delta_percent": percent(elapsed_delta, baseline["elapsed_ms"]),
            "baseline_nodes": baseline["nodes"],
            "candidate_nodes": candidate["nodes"],
            "nodes_delta": nodes_delta,
            "baseline_wall_ms": baseline["wall_ms"],
            "candidate_wall_ms": candidate["wall_ms"],
            "wall_delta_ms": wall_delta,
            "winner": winner(elapsed_delta),
            "baseline_ordering": baseline["ordering"],
            "candidate_ordering": candidate["ordering"],
            "baseline_reason": baseline["adaptive_reason"],
            "candidate_reason": candidate["adaptive_reason"],
        })

    if not rows:
        print("comparison failed: no common cases", file=sys.stderr)
        sys.exit(1)

    baseline_elapsed = sum(row["baseline_elapsed_ms"] for row in rows)
    candidate_elapsed = sum(row["candidate_elapsed_ms"] for row in rows)
    elapsed_delta = candidate_elapsed - baseline_elapsed
    baseline_p50 = percentile([row["baseline_elapsed_ms"] for row in rows], 0.50)
    candidate_p50 = percentile([row["candidate_elapsed_ms"] for row in rows], 0.50)
    baseline_p90 = percentile([row["baseline_elapsed_ms"] for row in rows], 0.90)
    candidate_p90 = percentile([row["candidate_elapsed_ms"] for row in rows], 0.90)
    baseline_p95 = percentile([row["baseline_elapsed_ms"] for row in rows], 0.95)
    candidate_p95 = percentile([row["candidate_elapsed_ms"] for row in rows], 0.95)
    baseline_p99 = percentile([row["baseline_elapsed_ms"] for row in rows], 0.99)
    candidate_p99 = percentile([row["candidate_elapsed_ms"] for row in rows], 0.99)
    baseline_max_elapsed = max(row["baseline_elapsed_ms"] for row in rows)
    candidate_max_elapsed = max(row["candidate_elapsed_ms"] for row in rows)
    baseline_nodes = sum(row["baseline_nodes"] for row in rows)
    candidate_nodes = sum(row["candidate_nodes"] for row in rows)
    baseline_wall = max(row["baseline_wall_ms"] for row in rows)
    candidate_wall = max(row["candidate_wall_ms"] for row in rows)
    rows.append({
        "case_key": "__summary__",
        "common_cases": len(rows),
        "baseline_elapsed_ms": baseline_elapsed,
        "candidate_elapsed_ms": candidate_elapsed,
        "elapsed_delta_ms": elapsed_delta,
        "elapsed_delta_percent": percent(elapsed_delta, baseline_elapsed),
        "baseline_p50_ms": baseline_p50,
        "candidate_p50_ms": candidate_p50,
        "p50_delta_ms": candidate_p50 - baseline_p50,
        "baseline_p90_ms": baseline_p90,
        "candidate_p90_ms": candidate_p90,
        "p90_delta_ms": candidate_p90 - baseline_p90,
        "baseline_p95_ms": baseline_p95,
        "candidate_p95_ms": candidate_p95,
        "p95_delta_ms": candidate_p95 - baseline_p95,
        "baseline_p99_ms": baseline_p99,
        "candidate_p99_ms": candidate_p99,
        "p99_delta_ms": candidate_p99 - baseline_p99,
        "baseline_max_elapsed_ms": baseline_max_elapsed,
        "candidate_max_elapsed_ms": candidate_max_elapsed,
        "max_elapsed_delta_ms": candidate_max_elapsed - baseline_max_elapsed,
        "baseline_nodes": baseline_nodes,
        "candidate_nodes": candidate_nodes,
        "nodes_delta": candidate_nodes - baseline_nodes,
        "baseline_wall_ms": baseline_wall,
        "candidate_wall_ms": candidate_wall,
        "wall_delta_ms": candidate_wall - baseline_wall,
        "winner": winner(elapsed_delta),
        "baseline_ordering": "",
        "candidate_ordering": "",
        "baseline_reason": "",
        "candidate_reason": "",
    })
    return rows


def emit(rows, output):
    fieldnames = [
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
    ]
    if output is None:
        writer = csv.DictWriter(sys.stdout, fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
        return

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(f"v6 latency comparison: {output}")


def main():
    baseline_dir, candidate_dir, output = parse_args()
    emit(compare(baseline_dir, candidate_dir), output)


if __name__ == "__main__":
    main()
