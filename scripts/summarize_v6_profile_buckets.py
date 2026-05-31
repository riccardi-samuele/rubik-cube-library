#!/usr/bin/env python3
import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path


USAGE = "Usage: scripts/summarize_v6_profile_buckets.py [options]"


def parse_args():
    value_options = {"--targeted-cases", "--output"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/summarize_v6_profile_buckets.py",
        usage="%(prog)s [options]",
        add_help=False,
        allow_abbrev=False,
    )
    parser.add_argument("--targeted-cases", default="")
    parser.add_argument("--output", default="")
    parser.add_argument("-h", "--help", action="store_true")
    args, unknown = parser.parse_known_args()

    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --targeted-cases FILE  targeted_cases.csv from the targeted corpus runner\n"
            "  --output FILE          write bucket CSV to file instead of stdout\n"
            "  -h, --help             show this help"
        )
        sys.exit(0)

    if unknown or not args.targeted_cases:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if args.targeted_cases.startswith("-") or args.output.startswith("-"):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return Path(args.targeted_cases), Path(args.output) if args.output else None


def int_value(value, field, row_index):
    try:
        return int(value)
    except (TypeError, ValueError):
        print(
            f"profile bucket summary failed: invalid integer in {field} at row {row_index}",
            file=sys.stderr,
        )
        sys.exit(1)


def load_csv(path):
    if not path.is_file():
        print(f"profile bucket summary failed: file not found: {path}", file=sys.stderr)
        sys.exit(1)
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


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


def profile_key(row):
    return (
        f"{row['adaptive_lb']}:"
        f"{row['adaptive_strong_min_count']}:"
        f"{row['adaptive_first_diff']}"
    )


def bucket_key(adaptive_lb, strong_bucket, first_diff):
    return f"lb{adaptive_lb}_s{strong_bucket}_fd{first_diff}"


def summarize(targeted_cases_path):
    target_rows = load_csv(targeted_cases_path)
    if not target_rows:
        print("profile bucket summary failed: no targeted cases", file=sys.stderr)
        sys.exit(1)

    grouped = defaultdict(list)
    for row_index, row in enumerate(target_rows, start=2):
        adaptive_lb = int_value(row.get("adaptive_lb"), "adaptive_lb", row_index)
        strong_min_count = int_value(
            row.get("adaptive_strong_min_count"),
            "adaptive_strong_min_count",
            row_index,
        )
        first_diff = int_value(row.get("adaptive_first_diff"), "adaptive_first_diff", row_index)
        strong_bucket = strong_min_bucket(strong_min_count)
        grouped[bucket_key(adaptive_lb, strong_bucket, first_diff)].append({
            "adaptive_lb": adaptive_lb,
            "strong_min_bucket": strong_bucket,
            "adaptive_first_diff": first_diff,
            "profile": profile_key(row),
            "elapsed_ms": int_value(row.get("elapsed_ms"), "elapsed_ms", row_index),
            "nodes_expanded": int_value(row.get("nodes_expanded"), "nodes_expanded", row_index),
        })

    bucket_rows = []
    for bucket in sorted(grouped):
        rows = grouped[bucket]
        elapsed = sum(row["elapsed_ms"] for row in rows)
        nodes = sum(row["nodes_expanded"] for row in rows)
        profiles = sorted({row["profile"] for row in rows})
        first = rows[0]
        bucket_rows.append({
            "bucket": bucket,
            "adaptive_lb": str(first["adaptive_lb"]),
            "strong_min_bucket": first["strong_min_bucket"],
            "adaptive_first_diff": str(first["adaptive_first_diff"]),
            "cases": str(len(rows)),
            "total_discovery_elapsed_ms": str(elapsed),
            "avg_discovery_elapsed_ms": f"{elapsed / len(rows):.2f}",
            "total_discovery_nodes": str(nodes),
            "avg_discovery_nodes": f"{nodes / len(rows):.2f}",
            "profiles": "|".join(profiles),
        })

    return bucket_rows


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
    targeted_cases, output = parse_args()
    fields = [
        "bucket",
        "adaptive_lb",
        "strong_min_bucket",
        "adaptive_first_diff",
        "cases",
        "total_discovery_elapsed_ms",
        "avg_discovery_elapsed_ms",
        "total_discovery_nodes",
        "avg_discovery_nodes",
        "profiles",
    ]
    write_rows(output, summarize(targeted_cases), fields)


if __name__ == "__main__":
    main()
