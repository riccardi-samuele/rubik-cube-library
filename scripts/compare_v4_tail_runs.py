#!/usr/bin/env python3
import argparse
import csv
import sys
from pathlib import Path


USAGE = "Usage: scripts/compare_v4_tail_runs.py [options]"


def parse_args():
    value_options = {"--baseline", "--candidate", "--output"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/compare_v4_tail_runs.py",
        usage="%(prog)s [options]",
        add_help=False,
    )
    parser.add_argument("--baseline", default="")
    parser.add_argument("--candidate", default="")
    parser.add_argument("--output", default="")
    parser.add_argument("-h", "--help", action="store_true")

    args, unknown = parser.parse_known_args()
    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --baseline FILE    baseline summary CSV\n"
            "  --candidate FILE   candidate summary CSV\n"
            "  --output FILE      write comparison CSV to file instead of stdout\n"
            "  -h, --help         show this help"
        )
        sys.exit(0)
    if unknown or not args.baseline or not args.candidate:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if args.baseline.startswith("-") or args.candidate.startswith("-") or args.output.startswith("-"):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return Path(args.baseline), Path(args.candidate), Path(args.output) if args.output else None


def int_value(row, key):
    try:
        return int(row[key])
    except (KeyError, TypeError, ValueError):
        return 0


def row_seed(row):
    if "seed" in row and row["seed"]:
        return row["seed"]
    case_name = row.get("case_name", "")
    pieces = case_name.split("_")
    if len(pieces) >= 3 and pieces[0] == "random" and pieces[1].isdigit():
        return pieces[1]
    return ""


def load_rows(path):
    if not path.is_file():
        print(f"comparison failed: input file not found: {path}", file=sys.stderr)
        sys.exit(1)

    rows = {}
    order = []
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            seed = row_seed(row)
            if not seed or seed == "__summary__":
                continue
            rows[seed] = row
            order.append(seed)
    return rows, order


def percent(delta, baseline):
    if baseline == 0:
        return "0.00"
    return f"{delta * 100.0 / baseline:.2f}"


def winner(delta):
    if delta < 0:
        return "candidate"
    if delta > 0:
        return "baseline"
    return "tie"


def compare(baseline_path, candidate_path):
    baseline_rows, baseline_order = load_rows(baseline_path)
    candidate_rows, _ = load_rows(candidate_path)

    rows = []
    for seed in baseline_order:
        if seed not in candidate_rows:
            continue
        baseline = baseline_rows[seed]
        candidate = candidate_rows[seed]
        baseline_elapsed = int_value(baseline, "elapsed_ms")
        candidate_elapsed = int_value(candidate, "elapsed_ms")
        baseline_nodes = int_value(baseline, "nodes_expanded")
        candidate_nodes = int_value(candidate, "nodes_expanded")
        elapsed_delta = candidate_elapsed - baseline_elapsed
        nodes_delta = candidate_nodes - baseline_nodes
        rows.append({
            "seed": seed,
            "baseline_elapsed_ms": baseline_elapsed,
            "candidate_elapsed_ms": candidate_elapsed,
            "elapsed_delta_ms": elapsed_delta,
            "elapsed_delta_percent": percent(elapsed_delta, baseline_elapsed),
            "baseline_nodes": baseline_nodes,
            "candidate_nodes": candidate_nodes,
            "nodes_delta": nodes_delta,
            "winner": winner(elapsed_delta),
        })

    if not rows:
        print("comparison failed: no common seeds", file=sys.stderr)
        sys.exit(1)

    baseline_elapsed_avg = sum(row["baseline_elapsed_ms"] for row in rows) // len(rows)
    candidate_elapsed_avg = sum(row["candidate_elapsed_ms"] for row in rows) // len(rows)
    elapsed_delta_avg = candidate_elapsed_avg - baseline_elapsed_avg
    baseline_nodes_total = sum(row["baseline_nodes"] for row in rows)
    candidate_nodes_total = sum(row["candidate_nodes"] for row in rows)
    rows.append({
        "seed": "__summary__",
        "baseline_elapsed_ms": baseline_elapsed_avg,
        "candidate_elapsed_ms": candidate_elapsed_avg,
        "elapsed_delta_ms": elapsed_delta_avg,
        "elapsed_delta_percent": percent(elapsed_delta_avg, baseline_elapsed_avg),
        "baseline_nodes": baseline_nodes_total,
        "candidate_nodes": candidate_nodes_total,
        "nodes_delta": candidate_nodes_total - baseline_nodes_total,
        "winner": winner(elapsed_delta_avg),
    })
    return rows


def emit(rows, output):
    fieldnames = [
        "seed",
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "elapsed_delta_ms",
        "elapsed_delta_percent",
        "baseline_nodes",
        "candidate_nodes",
        "nodes_delta",
        "winner",
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
    print(f"v4 tail comparison: {output}")


def main():
    baseline, candidate, output = parse_args()
    emit(compare(baseline, candidate), output)


if __name__ == "__main__":
    main()
