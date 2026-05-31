#!/usr/bin/env python3
import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path


USAGE = "Usage: scripts/analyze_v6_ordering_candidate_features.py [options]"

FEATURE_FIELDS = [
    "profile",
    "bucket",
    "solution_rank_bucket",
    "solution_matches_base_first",
    "solution_matches_strong_first",
    "solution_order_rank_bucket",
    "solution_root_status",
    "dominant_child_share_bucket",
    "dominant_child_move",
]


def parse_args():
    value_options = {"--comparison", "--features", "--case-output", "--feature-output"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/analyze_v6_ordering_candidate_features.py",
        usage="%(prog)s [options]",
        add_help=False,
        allow_abbrev=False,
    )
    parser.add_argument("--comparison", default="")
    parser.add_argument("--features", default="")
    parser.add_argument("--case-output", default="")
    parser.add_argument("--feature-output", default="")
    parser.add_argument("-h", "--help", action="store_true")
    args, unknown = parser.parse_known_args()

    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --comparison FILE      ordering comparison.csv from compare_v6_latency.py\n"
            "  --features FILE        discovery_case_features.csv from feature mining\n"
            "  --case-output FILE     write joined per-case CSV\n"
            "  --feature-output FILE  write grouped feature CSV\n"
            "  -h, --help             show this help"
        )
        sys.exit(0)

    if unknown or not args.comparison or not args.features:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if args.case_output.startswith("-") or args.feature_output.startswith("-"):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return (
        Path(args.comparison),
        Path(args.features),
        Path(args.case_output) if args.case_output else None,
        Path(args.feature_output) if args.feature_output else None,
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


def load_csv(path):
    if not path.is_file():
        print(f"ordering candidate feature analysis failed: file not found: {path}", file=sys.stderr)
        sys.exit(1)
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


def comparison_case_rows(rows):
    return [row for row in rows if row.get("case_key") and row["case_key"] != "__summary__"]


def feature_map(rows):
    by_key = {}
    for row in rows:
        key = row.get("case_key", "")
        if key:
            by_key[key] = row
    return by_key


def merged_case_rows(comparison_rows, features_by_key):
    rows = []
    missing = []
    for comparison in comparison_case_rows(comparison_rows):
        key = comparison["case_key"]
        features = features_by_key.get(key)
        if features is None:
            missing.append(key)
            continue

        merged = {
            "case_key": key,
            "baseline_elapsed_ms": comparison.get("baseline_elapsed_ms", "0"),
            "candidate_elapsed_ms": comparison.get("candidate_elapsed_ms", "0"),
            "elapsed_delta_ms": comparison.get("elapsed_delta_ms", "0"),
            "elapsed_delta_percent": comparison.get("elapsed_delta_percent", "0.00"),
            "baseline_nodes": comparison.get("baseline_nodes", "0"),
            "candidate_nodes": comparison.get("candidate_nodes", "0"),
            "nodes_delta": comparison.get("nodes_delta", "0"),
            "winner": comparison.get("winner", ""),
        }
        for field in [
            "profile",
            "bucket",
            "solution_rank",
            "solution_rank_bucket",
            "solution_matches_base_first",
            "solution_matches_strong_first",
            "solution_order_rank",
            "solution_order_rank_bucket",
            "solution_root_status",
            "dominant_child_share_percent",
            "dominant_child_share_bucket",
            "dominant_child_move",
        ]:
            merged[field] = features.get(field, "")
        rows.append(merged)

    if missing:
        print(
            "ordering candidate feature analysis failed: missing feature rows for "
            + ", ".join(missing[:10]),
            file=sys.stderr,
        )
        sys.exit(1)
    if not rows:
        print("ordering candidate feature analysis failed: no joined cases", file=sys.stderr)
        sys.exit(1)
    return rows


def winner_for_delta(delta):
    if delta < 0:
        return "candidate"
    if delta > 0:
        return "baseline"
    return "tie"


def summarize(rows):
    grouped = defaultdict(list)
    for row in rows:
        for field in FEATURE_FIELDS:
            grouped[(field, row[field])].append(row)

    summary_rows = []
    for (feature, value) in sorted(grouped):
        feature_rows = grouped[(feature, value)]
        baseline_elapsed = sum(int_value(row["baseline_elapsed_ms"]) for row in feature_rows)
        candidate_elapsed = sum(int_value(row["candidate_elapsed_ms"]) for row in feature_rows)
        elapsed_delta = candidate_elapsed - baseline_elapsed
        baseline_nodes = sum(int_value(row["baseline_nodes"]) for row in feature_rows)
        candidate_nodes = sum(int_value(row["candidate_nodes"]) for row in feature_rows)
        nodes_delta = candidate_nodes - baseline_nodes
        wins = sum(1 for row in feature_rows if row["winner"] == "candidate")
        losses = sum(1 for row in feature_rows if row["winner"] == "baseline")
        ties = len(feature_rows) - wins - losses
        best = min(feature_rows, key=lambda row: int_value(row["elapsed_delta_ms"]))
        worst = max(feature_rows, key=lambda row: int_value(row["elapsed_delta_ms"]))
        summary_rows.append({
            "feature": feature,
            "feature_value": value,
            "cases": str(len(feature_rows)),
            "candidate_wins": str(wins),
            "candidate_losses": str(losses),
            "candidate_ties": str(ties),
            "baseline_elapsed_ms": str(baseline_elapsed),
            "candidate_elapsed_ms": str(candidate_elapsed),
            "elapsed_delta_ms": str(elapsed_delta),
            "elapsed_delta_percent": percent(elapsed_delta, baseline_elapsed),
            "baseline_nodes": str(baseline_nodes),
            "candidate_nodes": str(candidate_nodes),
            "nodes_delta": str(nodes_delta),
            "winner": winner_for_delta(elapsed_delta),
            "best_case_key": best["case_key"],
            "best_elapsed_delta_ms": best["elapsed_delta_ms"],
            "worst_case_key": worst["case_key"],
            "worst_elapsed_delta_ms": worst["elapsed_delta_ms"],
        })
    return summary_rows


def write_rows(path, rows, fields):
    if path is None:
        writer = csv.DictWriter(sys.stdout, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    comparison_path, features_path, case_output, feature_output = parse_args()
    case_rows = merged_case_rows(load_csv(comparison_path), feature_map(load_csv(features_path)))
    feature_rows = summarize(case_rows)

    case_fields = [
        "case_key",
        "profile",
        "bucket",
        "solution_rank",
        "solution_rank_bucket",
        "solution_matches_base_first",
        "solution_matches_strong_first",
        "solution_order_rank",
        "solution_order_rank_bucket",
        "solution_root_status",
        "dominant_child_share_percent",
        "dominant_child_share_bucket",
        "dominant_child_move",
        "elapsed_delta_ms",
        "elapsed_delta_percent",
        "nodes_delta",
        "winner",
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "baseline_nodes",
        "candidate_nodes",
    ]
    feature_fields = [
        "feature",
        "feature_value",
        "cases",
        "candidate_wins",
        "candidate_losses",
        "candidate_ties",
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "elapsed_delta_ms",
        "elapsed_delta_percent",
        "baseline_nodes",
        "candidate_nodes",
        "nodes_delta",
        "winner",
        "best_case_key",
        "best_elapsed_delta_ms",
        "worst_case_key",
        "worst_elapsed_delta_ms",
    ]
    write_rows(case_output, case_rows, case_fields)
    write_rows(feature_output, feature_rows, feature_fields)


if __name__ == "__main__":
    main()
