#!/usr/bin/env python3
import argparse
import csv
import sys
from pathlib import Path


USAGE = "Usage: scripts/aggregate_v6_replay_comparisons.py [options]"
OUTPUT_FIELDS = [
    "case_key",
    "replays",
    "baseline_elapsed_ms",
    "candidate_elapsed_ms",
    "elapsed_delta_ms",
    "elapsed_delta_percent",
    "baseline_wins",
    "candidate_wins",
    "neutral_replays",
    "min_elapsed_delta_ms",
    "max_elapsed_delta_ms",
    "delta_spread_ms",
    "baseline_nodes",
    "candidate_nodes",
    "nodes_delta",
    "winner",
    "stability",
]


def parse_args():
    value_options = {"--comparison", "--output"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/aggregate_v6_replay_comparisons.py",
        usage="%(prog)s [options]",
        add_help=False,
        allow_abbrev=False,
    )
    parser.add_argument("--comparison", action="append", default=[])
    parser.add_argument("--output", default="")
    parser.add_argument("-h", "--help", action="store_true")
    args, unknown = parser.parse_known_args()

    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --comparison FILE  comparison.csv from compare_v6_latency.py; repeat for each replay\n"
            "  --output FILE      write aggregate CSV to file instead of stdout\n"
            "  -h, --help         show this help"
        )
        sys.exit(0)
    if unknown or len(args.comparison) < 1:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if args.output.startswith("-") or any(value.startswith("-") for value in args.comparison):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return [Path(value) for value in args.comparison], Path(args.output) if args.output else None


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


def stability(baseline_wins, candidate_wins, neutral_replays):
    active = sum(1 for value in (baseline_wins, candidate_wins, neutral_replays) if value > 0)
    if active != 1:
        return "mixed"
    if baseline_wins > 0:
        return "stable_baseline"
    if candidate_wins > 0:
        return "stable_candidate"
    return "neutral"


def require_columns(fieldnames, path):
    required = {
        "case_key",
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "elapsed_delta_ms",
        "baseline_nodes",
        "candidate_nodes",
        "nodes_delta",
    }
    missing = sorted(required.difference(fieldnames or []))
    if missing:
        print(f"replay aggregation failed: missing columns in {path}: {','.join(missing)}", file=sys.stderr)
        sys.exit(1)


def load_comparison(path):
    if not path.is_file():
        print(f"replay aggregation failed: comparison file not found: {path}", file=sys.stderr)
        sys.exit(1)

    rows = {}
    order = []
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        require_columns(reader.fieldnames, path)
        for row in reader:
            case_key = row.get("case_key", "")
            if not case_key or case_key == "__summary__":
                continue
            rows[case_key] = {
                "baseline_elapsed_ms": int_value(row.get("baseline_elapsed_ms")),
                "candidate_elapsed_ms": int_value(row.get("candidate_elapsed_ms")),
                "elapsed_delta_ms": int_value(row.get("elapsed_delta_ms")),
                "baseline_nodes": int_value(row.get("baseline_nodes")),
                "candidate_nodes": int_value(row.get("candidate_nodes")),
                "nodes_delta": int_value(row.get("nodes_delta")),
            }
            order.append(case_key)
    if not rows:
        print(f"replay aggregation failed: no case rows in {path}", file=sys.stderr)
        sys.exit(1)
    return rows, order


def aggregate(comparison_files):
    first_rows, order = load_comparison(comparison_files[0])
    all_rows = [first_rows]
    expected_keys = set(first_rows)
    for path in comparison_files[1:]:
        rows, _ = load_comparison(path)
        if set(rows) != expected_keys:
            print(f"replay aggregation failed: comparison case set differs: {path}", file=sys.stderr)
            sys.exit(1)
        all_rows.append(rows)

    output_rows = []
    for case_key in order:
        output_rows.append(aggregate_case(case_key, [rows[case_key] for rows in all_rows]))
    output_rows.append(aggregate_summary(output_rows, len(all_rows)))
    return output_rows


def aggregate_case(case_key, rows):
    baseline_elapsed = sum(row["baseline_elapsed_ms"] for row in rows)
    candidate_elapsed = sum(row["candidate_elapsed_ms"] for row in rows)
    elapsed_delta = sum(row["elapsed_delta_ms"] for row in rows)
    deltas = [row["elapsed_delta_ms"] for row in rows]
    baseline_nodes = sum(row["baseline_nodes"] for row in rows)
    candidate_nodes = sum(row["candidate_nodes"] for row in rows)
    baseline_wins = sum(1 for delta in deltas if delta > 0)
    candidate_wins = sum(1 for delta in deltas if delta < 0)
    neutral_replays = sum(1 for delta in deltas if delta == 0)
    min_delta = min(deltas)
    max_delta = max(deltas)
    return {
        "case_key": case_key,
        "replays": len(rows),
        "baseline_elapsed_ms": baseline_elapsed,
        "candidate_elapsed_ms": candidate_elapsed,
        "elapsed_delta_ms": elapsed_delta,
        "elapsed_delta_percent": percent(elapsed_delta, baseline_elapsed),
        "baseline_wins": baseline_wins,
        "candidate_wins": candidate_wins,
        "neutral_replays": neutral_replays,
        "min_elapsed_delta_ms": min_delta,
        "max_elapsed_delta_ms": max_delta,
        "delta_spread_ms": max_delta - min_delta,
        "baseline_nodes": baseline_nodes,
        "candidate_nodes": candidate_nodes,
        "nodes_delta": candidate_nodes - baseline_nodes,
        "winner": winner(elapsed_delta),
        "stability": stability(baseline_wins, candidate_wins, neutral_replays),
    }


def aggregate_summary(rows, replay_count):
    baseline_elapsed = sum(row["baseline_elapsed_ms"] for row in rows)
    candidate_elapsed = sum(row["candidate_elapsed_ms"] for row in rows)
    elapsed_delta = sum(row["elapsed_delta_ms"] for row in rows)
    min_delta = min(row["min_elapsed_delta_ms"] for row in rows)
    max_delta = max(row["max_elapsed_delta_ms"] for row in rows)
    baseline_nodes = sum(row["baseline_nodes"] for row in rows)
    candidate_nodes = sum(row["candidate_nodes"] for row in rows)
    baseline_wins = sum(row["baseline_wins"] for row in rows)
    candidate_wins = sum(row["candidate_wins"] for row in rows)
    neutral_replays = sum(row["neutral_replays"] for row in rows)
    return {
        "case_key": "__summary__",
        "replays": replay_count,
        "baseline_elapsed_ms": baseline_elapsed,
        "candidate_elapsed_ms": candidate_elapsed,
        "elapsed_delta_ms": elapsed_delta,
        "elapsed_delta_percent": percent(elapsed_delta, baseline_elapsed),
        "baseline_wins": baseline_wins,
        "candidate_wins": candidate_wins,
        "neutral_replays": neutral_replays,
        "min_elapsed_delta_ms": min_delta,
        "max_elapsed_delta_ms": max_delta,
        "delta_spread_ms": max_delta - min_delta,
        "baseline_nodes": baseline_nodes,
        "candidate_nodes": candidate_nodes,
        "nodes_delta": candidate_nodes - baseline_nodes,
        "winner": winner(elapsed_delta),
        "stability": stability(baseline_wins, candidate_wins, neutral_replays),
    }


def write_rows(rows, output_file):
    if output_file is None:
        writer = csv.DictWriter(sys.stdout, fieldnames=OUTPUT_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
        return

    output_file.parent.mkdir(parents=True, exist_ok=True)
    with output_file.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=OUTPUT_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main():
    comparison_files, output_file = parse_args()
    write_rows(aggregate(comparison_files), output_file)


if __name__ == "__main__":
    main()
