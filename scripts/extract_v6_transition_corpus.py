#!/usr/bin/env python3
import argparse
import csv
import re
import sys
from pathlib import Path


USAGE = "Usage: scripts/extract_v6_transition_corpus.py [options]"
CASE_KEY_RE = re.compile(
    r"^(?P<suite>tail|hardening):depth(?P<depth>[0-9]+):seed(?P<seed>[0-9]+):random_[0-9]+_(?P<start_index>[0-9]+)$"
)
OUTPUT_FIELDS = ["suite", "seed", "start_index", "depth", "count", "expected_reason"]


def parse_args():
    value_options = {"--comparison", "--baseline-ordering", "--candidate-ordering", "--output", "--expected-reason"}
    argv = sys.argv[1:]
    for index, token in enumerate(argv):
        if token in value_options and (index + 1 >= len(argv) or argv[index + 1].startswith("-")):
            print(USAGE, file=sys.stderr)
            sys.exit(2)

    parser = argparse.ArgumentParser(
        prog="scripts/extract_v6_transition_corpus.py",
        usage="%(prog)s [options]",
        add_help=False,
        allow_abbrev=False,
    )
    parser.add_argument("--comparison", default="")
    parser.add_argument("--baseline-ordering", default="")
    parser.add_argument("--candidate-ordering", default="")
    parser.add_argument("--output", default="")
    parser.add_argument("--expected-reason", default="conservative_root")
    parser.add_argument("-h", "--help", action="store_true")
    args, unknown = parser.parse_known_args()

    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --comparison FILE         comparison.csv from compare_v6_latency.py\n"
            "  --baseline-ordering MODE  baseline root ordering mode to match\n"
            "  --candidate-ordering MODE candidate root ordering mode to match\n"
            "  --output FILE             write corpus CSV to file instead of stdout\n"
            "  --expected-reason VALUE   expected adaptive reason, default: conservative_root\n"
            "  -h, --help                show this help"
        )
        sys.exit(0)
    if unknown or not args.comparison or not args.baseline_ordering or not args.candidate_ordering:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    if (
        args.comparison.startswith("-")
        or args.baseline_ordering.startswith("-")
        or args.candidate_ordering.startswith("-")
        or args.output.startswith("-")
        or args.expected_reason.startswith("-")
    ):
        print(USAGE, file=sys.stderr)
        sys.exit(2)

    return (
        Path(args.comparison),
        args.baseline_ordering,
        args.candidate_ordering,
        Path(args.output) if args.output else None,
        args.expected_reason,
    )


def require_columns(fieldnames):
    required = {"case_key", "baseline_ordering", "candidate_ordering", "baseline_reason"}
    missing = sorted(required.difference(fieldnames or []))
    if missing:
        print(f"transition corpus extraction failed: missing columns: {','.join(missing)}", file=sys.stderr)
        sys.exit(1)


def parse_case_key(case_key):
    match = CASE_KEY_RE.match(case_key)
    if match is None:
        print(f"transition corpus extraction failed: unsupported case key: {case_key}", file=sys.stderr)
        sys.exit(1)
    return match.groupdict()


def extract_rows(comparison_file, baseline_ordering, candidate_ordering, expected_reason):
    if not comparison_file.is_file():
        print(f"transition corpus extraction failed: comparison file not found: {comparison_file}", file=sys.stderr)
        sys.exit(1)

    rows = []
    seen = set()
    with comparison_file.open(newline="") as handle:
        reader = csv.DictReader(handle)
        require_columns(reader.fieldnames)
        for row in reader:
            case_key = row.get("case_key", "")
            if not case_key or case_key == "__summary__":
                continue
            if row.get("baseline_ordering", "") != baseline_ordering:
                continue
            if row.get("candidate_ordering", "") != candidate_ordering:
                continue
            if row.get("baseline_reason", "") != expected_reason:
                continue
            parsed = parse_case_key(case_key)
            key = (parsed["suite"], parsed["seed"], parsed["start_index"], parsed["depth"])
            if key in seen:
                continue
            seen.add(key)
            rows.append({
                "suite": parsed["suite"],
                "seed": parsed["seed"],
                "start_index": parsed["start_index"],
                "depth": parsed["depth"],
                "count": "1",
                "expected_reason": expected_reason,
            })

    if not rows:
        print("transition corpus extraction failed: no matching transition rows", file=sys.stderr)
        sys.exit(1)
    return rows


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
    comparison_file, baseline_ordering, candidate_ordering, output_file, expected_reason = parse_args()
    rows = extract_rows(comparison_file, baseline_ordering, candidate_ordering, expected_reason)
    write_rows(rows, output_file)


if __name__ == "__main__":
    main()
