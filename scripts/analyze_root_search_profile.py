#!/usr/bin/env python3
import argparse
import csv
import sys
from pathlib import Path


USAGE = "Usage: scripts/analyze_root_search_profile.py [options]"


def parse_args():
    value_options = {"--input-dir", "--output", "--limit"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/analyze_root_search_profile.py",
        usage="%(prog)s [options]",
        add_help=False,
    )
    parser.add_argument("--input-dir", default="benchmark-results")
    parser.add_argument("--output", default="")
    parser.add_argument("--limit", default="0")
    parser.add_argument("-h", "--help", action="store_true")

    args, unknown = parser.parse_known_args()
    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --input-dir DIR    directory containing rubik-bench CSV output, default: benchmark-results\n"
            "  --output FILE      write CSV output to FILE instead of stdout\n"
            "  --limit N          maximum number of root rows to emit, default: all\n"
            "  -h, --help         show this help"
        )
        sys.exit(0)
    if unknown:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if args.input_dir.startswith("-") or args.output.startswith("-") or args.limit.startswith("-"):
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    try:
        limit = int(args.limit)
    except ValueError:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if limit < 0:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    return Path(args.input_dir), Path(args.output) if args.output else None, limit


def profile_value(profile, key):
    prefix = key + "="
    for part in profile.split(";"):
        if part.startswith(prefix):
            return part[len(prefix):]
    return ""


def parse_root_search(profile):
    root_search = profile_value(profile, "root_search")
    if not root_search:
        return []

    entries = []
    for index, token in enumerate(root_search.split("|"), start=1):
        pieces = token.split(":")
        if len(pieces) not in {3, 4}:
            continue
        move, outcome, nodes = pieces[:3]
        elapsed_ms = pieces[3] if len(pieces) == 4 else ""
        entries.append((index, move, outcome, nodes, elapsed_ms))
    return entries


def case_rows(path):
    benchmark = ""
    with path.open(newline="") as handle:
        for row in csv.reader(handle):
            if len(row) >= 3 and row[0] == "benchmark" and row[1] == "name":
                benchmark = row[2]
                continue
            if len(row) < 16 or row[0] in {"case", "summary", "slowest", "benchmark", "diagnostic_optimal_bounds"}:
                continue
            profile = row[15]
            root_rows = parse_root_search(profile)
            if not root_rows:
                continue
            solution_rank = profile_value(profile, "solution_rank")
            for root_rank, move, outcome, nodes, elapsed_ms in root_rows:
                yield {
                    "source_file": path.name,
                    "benchmark": benchmark,
                    "case_name": row[0],
                    "elapsed_ms": row[7],
                    "total_nodes": row[8],
                    "solution_rank": solution_rank,
                    "root_rank": str(root_rank),
                    "move": move,
                    "outcome": outcome,
                    "nodes_expanded": nodes,
                    "root_elapsed_ms": elapsed_ms,
                    "before_solution": "true" if solution_rank and root_rank < int(solution_rank) else "false",
                }


def collect_rows(input_dir):
    if not input_dir.is_dir():
        print(f"root search analysis failed: input directory not found: {input_dir}", file=sys.stderr)
        sys.exit(1)

    rows = []
    for path in sorted(input_dir.rglob("*.csv")):
        rows.extend(case_rows(path))
    return rows


def emit(rows, output):
    fieldnames = [
        "source_file",
        "benchmark",
        "case_name",
        "elapsed_ms",
        "total_nodes",
        "solution_rank",
        "root_rank",
        "move",
        "outcome",
        "nodes_expanded",
        "root_elapsed_ms",
        "before_solution",
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
    print(f"root search analysis: {output}")


def main():
    input_dir, output, limit = parse_args()
    rows = collect_rows(input_dir)
    if limit:
        rows = rows[:limit]
    emit(rows, output)


if __name__ == "__main__":
    main()
