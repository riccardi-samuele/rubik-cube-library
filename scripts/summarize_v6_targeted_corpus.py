#!/usr/bin/env python3
import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path


USAGE = "Usage: scripts/summarize_v6_targeted_corpus.py [options]"


def parse_args():
    value_options = {"--targeted-cases", "--comparison", "--case-output", "--profile-output"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/summarize_v6_targeted_corpus.py",
        usage="%(prog)s [options]",
        add_help=False,
        allow_abbrev=False,
    )
    parser.add_argument("--targeted-cases", default="")
    parser.add_argument("--comparison", default="")
    parser.add_argument("--case-output", default="")
    parser.add_argument("--profile-output", default="")
    parser.add_argument("-h", "--help", action="store_true")
    args, unknown = parser.parse_known_args()

    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --targeted-cases FILE  targeted_cases.csv from the targeted corpus runner\n"
            "  --comparison FILE      phase2_tiebreak comparison.csv from the ordering sweep\n"
            "  --case-output FILE     write joined per-case CSV to file\n"
            "  --profile-output FILE  write grouped profile CSV to file\n"
            "  -h, --help             show this help"
        )
        sys.exit(0)

    if unknown or not args.targeted_cases or not args.comparison:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if (
        args.targeted_cases.startswith("-")
        or args.comparison.startswith("-")
        or args.case_output.startswith("-")
        or args.profile_output.startswith("-")
    ):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return (
        Path(args.targeted_cases),
        Path(args.comparison),
        Path(args.case_output) if args.case_output else None,
        Path(args.profile_output) if args.profile_output else None,
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
        print(f"targeted corpus summary failed: file not found: {path}", file=sys.stderr)
        sys.exit(1)
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


def case_key(row):
    return f"hardening:depth15:seed{row['seed']}:{row['case_name']}"


def summarize(targeted_cases_path, comparison_path):
    target_rows = load_csv(targeted_cases_path)
    comparison_rows = [
        row for row in load_csv(comparison_path)
        if row.get("case_key") and row.get("case_key") != "__summary__"
    ]
    comparison_by_key = {row["case_key"]: row for row in comparison_rows}

    case_rows = []
    for target in target_rows:
        key = case_key(target)
        comparison = comparison_by_key.get(key)
        if comparison is None:
            continue
        profile = (
            f"{target['adaptive_lb']}:"
            f"{target['adaptive_strong_min_count']}:"
            f"{target['adaptive_first_diff']}"
        )
        elapsed_delta = int_value(comparison.get("elapsed_delta_ms"))
        nodes_delta = int_value(comparison.get("nodes_delta"))
        case_rows.append({
            "profile": profile,
            "adaptive_lb": target["adaptive_lb"],
            "adaptive_strong_min_count": target["adaptive_strong_min_count"],
            "adaptive_first_diff": target["adaptive_first_diff"],
            "case_key": key,
            "baseline_elapsed_ms": comparison.get("baseline_elapsed_ms", "0"),
            "candidate_elapsed_ms": comparison.get("candidate_elapsed_ms", "0"),
            "elapsed_delta_ms": str(elapsed_delta),
            "elapsed_delta_percent": comparison.get("elapsed_delta_percent", "0.00"),
            "baseline_nodes": comparison.get("baseline_nodes", "0"),
            "candidate_nodes": comparison.get("candidate_nodes", "0"),
            "nodes_delta": str(nodes_delta),
            "winner": comparison.get("winner", ""),
        })

    if not case_rows:
        print("targeted corpus summary failed: no matching comparison cases", file=sys.stderr)
        sys.exit(1)

    grouped = defaultdict(list)
    for row in case_rows:
        grouped[row["profile"]].append(row)

    profile_rows = []
    for profile in sorted(grouped):
        rows = grouped[profile]
        baseline_elapsed = sum(int_value(row["baseline_elapsed_ms"]) for row in rows)
        candidate_elapsed = sum(int_value(row["candidate_elapsed_ms"]) for row in rows)
        elapsed_delta = candidate_elapsed - baseline_elapsed
        baseline_nodes = sum(int_value(row["baseline_nodes"]) for row in rows)
        candidate_nodes = sum(int_value(row["candidate_nodes"]) for row in rows)
        nodes_delta = candidate_nodes - baseline_nodes
        wins = sum(1 for row in rows if row["winner"] == "candidate")
        losses = sum(1 for row in rows if row["winner"] == "baseline")
        first = rows[0]
        profile_rows.append({
            "profile": profile,
            "adaptive_lb": first["adaptive_lb"],
            "adaptive_strong_min_count": first["adaptive_strong_min_count"],
            "adaptive_first_diff": first["adaptive_first_diff"],
            "cases": str(len(rows)),
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

    return case_rows, profile_rows


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
    targeted_cases, comparison, case_output, profile_output = parse_args()
    case_rows, profile_rows = summarize(targeted_cases, comparison)
    case_fields = [
        "profile",
        "adaptive_lb",
        "adaptive_strong_min_count",
        "adaptive_first_diff",
        "case_key",
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "elapsed_delta_ms",
        "elapsed_delta_percent",
        "baseline_nodes",
        "candidate_nodes",
        "nodes_delta",
        "winner",
    ]
    profile_fields = [
        "profile",
        "adaptive_lb",
        "adaptive_strong_min_count",
        "adaptive_first_diff",
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
    if profile_output is None:
        writer = csv.DictWriter(sys.stdout, fieldnames=profile_fields)
        writer.writeheader()
        writer.writerows(profile_rows)
    else:
        write_rows(profile_output, profile_rows, profile_fields)


if __name__ == "__main__":
    main()
