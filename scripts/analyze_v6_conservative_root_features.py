#!/usr/bin/env python3
import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path


USAGE = "Usage: scripts/analyze_v6_conservative_root_features.py [options]"


def parse_args():
    value_options = {"--run-dir", "--case-output", "--feature-output"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/analyze_v6_conservative_root_features.py",
        usage="%(prog)s [options]",
        add_help=False,
        allow_abbrev=False,
    )
    parser.add_argument("--run-dir", action="append", default=[])
    parser.add_argument("--case-output", default="")
    parser.add_argument("--feature-output", default="")
    parser.add_argument("-h", "--help", action="store_true")
    args, unknown = parser.parse_known_args()

    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --run-dir DIR       directory containing targeted_cases.csv and case_summary.csv\n"
            "  --case-output FILE  write per-case feature CSV\n"
            "  --feature-output FILE  write grouped feature CSV\n"
            "  -h, --help          show this help"
        )
        sys.exit(0)

    if unknown or not args.run_dir:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if args.case_output.startswith("-") or args.feature_output.startswith("-"):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return (
        [Path(run_dir) for run_dir in args.run_dir],
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
        print(f"conservative-root feature analysis failed: file not found: {path}", file=sys.stderr)
        sys.exit(1)
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


def parse_profile(profile):
    values = {}
    for piece in profile.split(";"):
        if "=" not in piece:
            continue
        key, value = piece.split("=", 1)
        values[key] = value
    return values


def strong_min_bucket(value):
    if value <= 4:
        return "0-4"
    if value <= 8:
        return "5-8"
    if value <= 12:
        return "9-12"
    if value <= 16:
        return "13-16"
    return "17+"


def rank_bucket(rank):
    if rank <= 3:
        return "1-3"
    if rank <= 6:
        return "4-6"
    if rank <= 9:
        return "7-9"
    return "10+"


def dominant_share_bucket(share):
    if share < 0:
        return "unknown"
    if share >= 60:
        return "high"
    if share >= 35:
        return "medium"
    return "low"


def case_key(row):
    return f"hardening:depth15:seed{row['seed']}:{row['case_name']}"


def bucket_name(lb, strong_min_count, first_diff):
    return f"lb{lb}_s{strong_min_bucket(strong_min_count)}_fd{first_diff}"


def move_from_order_entry(entry):
    return entry.split("/", 1)[0]


def solution_order_rank(root_order, solution_first):
    if not root_order or not solution_first:
        return 0
    for index, entry in enumerate(root_order.split("|"), start=1):
        if move_from_order_entry(entry) == solution_first:
            return index
    return 0


def parse_root_search(root_search):
    rows = []
    if not root_search:
        return rows
    for entry in root_search.split("|"):
        pieces = entry.split(":")
        if len(pieces) < 4:
            continue
        rows.append({
            "move": pieces[0],
            "status": pieces[1],
            "nodes": int_value(pieces[2]),
            "elapsed": int_value(pieces[3]),
        })
    return rows


def root_search_features(root_search, solution_first):
    search_rows = parse_root_search(root_search)
    if not search_rows:
        return {
            "solution_root_status": "unknown",
            "dominant_child_share_percent": "-1",
            "dominant_child_share_bucket": "unknown",
            "dominant_child_move": "unknown",
        }

    total_nodes = sum(row["nodes"] for row in search_rows)
    dominant = max(search_rows, key=lambda row: row["nodes"])
    share = round(dominant["nodes"] * 100.0 / total_nodes) if total_nodes else 0
    solution_status = "missing"
    for row in search_rows:
        if row["move"] == solution_first:
            solution_status = row["status"]
            break

    return {
        "solution_root_status": solution_status,
        "dominant_child_share_percent": str(share),
        "dominant_child_share_bucket": dominant_share_bucket(share),
        "dominant_child_move": dominant["move"],
    }


def summarize(rows):
    grouped = defaultdict(list)
    for row in rows:
        grouped[("bucket", row["bucket"])].append(row)
        grouped[("profile", row["profile"])].append(row)
        grouped[("solution_rank_bucket", row["solution_rank_bucket"])].append(row)
        grouped[("solution_matches_base_first", row["solution_matches_base_first"])].append(row)
        grouped[("solution_matches_strong_first", row["solution_matches_strong_first"])].append(row)
        grouped[("solution_order_rank_bucket", row["solution_order_rank_bucket"])].append(row)
        grouped[("solution_root_status", row["solution_root_status"])].append(row)
        grouped[("dominant_child_share_bucket", row["dominant_child_share_bucket"])].append(row)
        grouped[("dominant_child_move", row["dominant_child_move"])].append(row)

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
        summary_rows.append({
            "feature": feature,
            "feature_value": value,
            "cases": str(len(feature_rows)),
            "candidate_wins": str(wins),
            "candidate_losses": str(losses),
            "baseline_elapsed_ms": str(baseline_elapsed),
            "candidate_elapsed_ms": str(candidate_elapsed),
            "elapsed_delta_ms": str(elapsed_delta),
            "elapsed_delta_percent": percent(elapsed_delta, baseline_elapsed),
            "baseline_nodes": str(baseline_nodes),
            "candidate_nodes": str(candidate_nodes),
            "nodes_delta": str(nodes_delta),
            "winner": "candidate" if elapsed_delta < 0 else "baseline",
        })
    return summary_rows


def analyze(run_dirs):
    case_rows = []
    for run_dir in run_dirs:
        targeted_rows = load_csv(run_dir / "targeted_cases.csv")
        comparison_rows = load_csv(run_dir / "case_summary.csv")
        targeted_by_key = {case_key(row): row for row in targeted_rows}

        for comparison in comparison_rows:
            key = comparison["case_key"]
            target = targeted_by_key.get(key)
            if target is None:
                continue
            profile_values = parse_profile(target.get("profile", ""))
            lb = int_value(target["adaptive_lb"])
            strong_min_count = int_value(target["adaptive_strong_min_count"])
            first_diff = int_value(target["adaptive_first_diff"])
            solution_rank = int_value(profile_values.get("solution_rank"))
            solution_first = profile_values.get("solution_first", "")
            base_first = profile_values.get("base_first", "")
            strong_first = profile_values.get("strong_first", "")
            root_features = root_search_features(profile_values.get("root_search", ""), solution_first)
            order_rank = solution_order_rank(profile_values.get("root_order", ""), solution_first)
            case_rows.append({
                "case_key": key,
                "profile": comparison["profile"],
                "bucket": bucket_name(lb, strong_min_count, first_diff),
                "solution_rank": str(solution_rank),
                "solution_rank_bucket": rank_bucket(solution_rank),
                "solution_matches_base_first": "1" if solution_first == base_first else "0",
                "solution_matches_strong_first": "1" if solution_first == strong_first else "0",
                "solution_order_rank": str(order_rank),
                "solution_order_rank_bucket": rank_bucket(order_rank),
                "solution_root_status": root_features["solution_root_status"],
                "dominant_child_share_percent": root_features["dominant_child_share_percent"],
                "dominant_child_share_bucket": root_features["dominant_child_share_bucket"],
                "dominant_child_move": root_features["dominant_child_move"],
                "baseline_elapsed_ms": comparison["baseline_elapsed_ms"],
                "candidate_elapsed_ms": comparison["candidate_elapsed_ms"],
                "elapsed_delta_ms": comparison["elapsed_delta_ms"],
                "elapsed_delta_percent": comparison["elapsed_delta_percent"],
                "baseline_nodes": comparison["baseline_nodes"],
                "candidate_nodes": comparison["candidate_nodes"],
                "nodes_delta": comparison["nodes_delta"],
                "winner": comparison["winner"],
            })

    if not case_rows:
        print("conservative-root feature analysis failed: no joined cases", file=sys.stderr)
        sys.exit(1)

    return case_rows, summarize(case_rows)


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
    run_dirs, case_output, feature_output = parse_args()
    case_rows, feature_rows = analyze(run_dirs)
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
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "elapsed_delta_ms",
        "elapsed_delta_percent",
        "baseline_nodes",
        "candidate_nodes",
        "nodes_delta",
        "winner",
    ]
    write_rows(case_output, case_rows, case_fields)
    write_rows(feature_output, feature_rows, feature_fields)


if __name__ == "__main__":
    main()
