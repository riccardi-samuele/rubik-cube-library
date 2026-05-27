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
    parser.add_argument("--summary", action="store_true")
    parser.add_argument("-h", "--help", action="store_true")

    args, unknown = parser.parse_known_args()
    if args.help:
        print(
            f"{USAGE}\n\n"
            "Options:\n"
            "  --input-dir DIR    directory containing rubik-bench CSV output, default: benchmark-results\n"
            "  --output FILE      write CSV output to FILE instead of stdout\n"
            "  --limit N          maximum number of root rows to emit, default: all\n"
            "  --summary          emit one aggregate row per benchmark case\n"
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
    return Path(args.input_dir), Path(args.output) if args.output else None, limit, args.summary


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


def parse_root_bound_diagnostics(profile):
    root_bound_diagnostics = profile_value(profile, "root_bound_diagnostics")
    if not root_bound_diagnostics:
        return {}

    entries = {}
    for index, token in enumerate(root_bound_diagnostics.split("|"), start=1):
        pieces = token.split(":")
        if len(pieces) != 8:
            continue
        entries[index] = {
            "cheap_node_prunes": pieces[2],
            "three_phase_node_checks": pieces[3],
            "three_phase_node_prunes": pieces[4],
            "cheap_candidate_prunes": pieces[5],
            "three_phase_candidate_checks": pieces[6],
            "three_phase_candidate_prunes": pieces[7],
        }
    return entries


def int_value(value):
    try:
        return int(value)
    except (TypeError, ValueError):
        return None


def integer_ratio(numerator, denominator, scale=1):
    numerator_value = int_value(numerator)
    denominator_value = int_value(denominator)
    if numerator_value is None or denominator_value is None or denominator_value <= 0:
        return ""
    return str((numerator_value * scale) // denominator_value)


def file_wall_elapsed_ms(path):
    with path.open(newline="") as handle:
        for row in csv.reader(handle):
            if len(row) >= 3 and row[0] == "benchmark" and row[1] == "wall_elapsed_ms":
                return row[2]
    return ""


def cache_setup_values(input_dir):
    path = input_dir / "cache_setup.csv"
    values = {}
    if not path.is_file():
        return values
    with path.open(newline="") as handle:
        for row in csv.reader(handle):
            if len(row) >= 3 and row[0] == "cache_setup":
                values[row[1]] = row[2]
    return values


def case_rows(path):
    benchmark = ""
    wall_elapsed_ms = file_wall_elapsed_ms(path)
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
            root_bound_diagnostics = parse_root_bound_diagnostics(profile)
            solution_rank = profile_value(profile, "solution_rank")
            for root_rank, move, outcome, nodes, elapsed_ms in root_rows:
                diagnostics = root_bound_diagnostics.get(root_rank, {})
                yield {
                    "source_file": path.name,
                    "benchmark": benchmark,
                    "case_name": row[0],
                    "elapsed_ms": row[7],
                    "wall_elapsed_ms": wall_elapsed_ms,
                    "total_nodes": row[8],
                    "solution_rank": solution_rank,
                    "root_rank": str(root_rank),
                    "move": move,
                    "outcome": outcome,
                    "nodes_expanded": nodes,
                    "root_elapsed_ms": elapsed_ms,
                    "root_nodes_per_ms": integer_ratio(nodes, elapsed_ms),
                    "cheap_node_prunes": diagnostics.get("cheap_node_prunes", ""),
                    "three_phase_node_checks": diagnostics.get("three_phase_node_checks", ""),
                    "three_phase_node_prunes": diagnostics.get("three_phase_node_prunes", ""),
                    "cheap_candidate_prunes": diagnostics.get("cheap_candidate_prunes", ""),
                    "three_phase_candidate_checks": diagnostics.get("three_phase_candidate_checks", ""),
                    "three_phase_candidate_prunes": diagnostics.get("three_phase_candidate_prunes", ""),
                    "cheap_candidate_prunes_per_node_ppm": integer_ratio(
                        diagnostics.get("cheap_candidate_prunes", ""),
                        nodes,
                        1_000_000),
                    "three_phase_candidate_prune_rate_ppm": integer_ratio(
                        diagnostics.get("three_phase_candidate_prunes", ""),
                        diagnostics.get("three_phase_candidate_checks", ""),
                        1_000_000),
                    "before_solution": "true" if solution_rank and root_rank < int(solution_rank) else "false",
                }


def collect_rows(input_dir):
    if not input_dir.is_dir():
        print(f"root search analysis failed: input directory not found: {input_dir}", file=sys.stderr)
        sys.exit(1)

    cache_setup = cache_setup_values(input_dir)
    rows = []
    for path in sorted(input_dir.rglob("*.csv")):
        for row in case_rows(path):
            row["cache_setup_status"] = cache_setup.get("status", "")
            row["cache_setup_elapsed_ms"] = cache_setup.get("elapsed_ms", "")
            row["cache_warm"] = cache_setup.get("cache_warm", "")
            rows.append(row)
    return rows


def summarize_rows(rows):
    groups = {}
    for row in rows:
        key = (row["source_file"], row["benchmark"], row["case_name"])
        group = groups.setdefault(key, {
            "source_file": row["source_file"],
            "benchmark": row["benchmark"],
            "case_name": row["case_name"],
            "roots": 0,
            "before_solution_roots": 0,
            "total_root_nodes": 0,
            "total_root_elapsed_ms": 0,
            "solver_elapsed_ms": "",
            "wall_elapsed_ms": "",
            "cache_setup_status": "",
            "cache_setup_elapsed_ms": "",
            "cache_warm": "",
            "cheap_candidate_prunes": 0,
            "three_phase_candidate_checks": 0,
            "three_phase_candidate_prunes": 0,
            "solution_root_elapsed_ms": "",
            "max_root_elapsed_ms": 0,
        })
        nodes = int_value(row["nodes_expanded"]) or 0
        elapsed = int_value(row["root_elapsed_ms"]) or 0
        cheap_prunes = int_value(row["cheap_candidate_prunes"]) or 0
        three_phase_checks = int_value(row["three_phase_candidate_checks"]) or 0
        three_phase_prunes = int_value(row["three_phase_candidate_prunes"]) or 0
        group["roots"] += 1
        group["before_solution_roots"] += 1 if row["before_solution"] == "true" else 0
        group["total_root_nodes"] += nodes
        group["total_root_elapsed_ms"] += elapsed
        if not group["solver_elapsed_ms"]:
            group["solver_elapsed_ms"] = row["elapsed_ms"]
        if not group["wall_elapsed_ms"]:
            group["wall_elapsed_ms"] = row["wall_elapsed_ms"]
        if not group["cache_setup_status"]:
            group["cache_setup_status"] = row["cache_setup_status"]
        if not group["cache_setup_elapsed_ms"]:
            group["cache_setup_elapsed_ms"] = row["cache_setup_elapsed_ms"]
        if not group["cache_warm"]:
            group["cache_warm"] = row["cache_warm"]
        group["cheap_candidate_prunes"] += cheap_prunes
        group["three_phase_candidate_checks"] += three_phase_checks
        group["three_phase_candidate_prunes"] += three_phase_prunes
        group["max_root_elapsed_ms"] = max(group["max_root_elapsed_ms"], elapsed)
        if row["outcome"] == "found":
            group["solution_root_elapsed_ms"] = row["root_elapsed_ms"]

    result = []
    for group in groups.values():
        total_nodes = str(group["total_root_nodes"])
        total_elapsed = str(group["total_root_elapsed_ms"])
        solver_elapsed = group["solver_elapsed_ms"]
        solver_elapsed_value = int_value(solver_elapsed)
        wall_elapsed = group["wall_elapsed_ms"]
        wall_elapsed_value = int_value(wall_elapsed)
        wall_overhead = ""
        if wall_elapsed_value is not None and solver_elapsed_value is not None:
            wall_overhead = str(wall_elapsed_value - solver_elapsed_value)
        three_phase_checks = str(group["three_phase_candidate_checks"])
        result.append({
            "source_file": group["source_file"],
            "benchmark": group["benchmark"],
            "case_name": group["case_name"],
            "roots": str(group["roots"]),
            "before_solution_roots": str(group["before_solution_roots"]),
            "total_root_nodes": total_nodes,
            "total_root_elapsed_ms": total_elapsed,
            "solver_elapsed_ms": solver_elapsed,
            "wall_elapsed_ms": wall_elapsed,
            "wall_overhead_ms": wall_overhead,
            "cache_setup_status": group["cache_setup_status"],
            "cache_setup_elapsed_ms": group["cache_setup_elapsed_ms"],
            "cache_warm": group["cache_warm"],
            "root_nodes_per_ms": integer_ratio(total_nodes, total_elapsed),
            "cheap_candidate_prunes_per_node_ppm": integer_ratio(
                str(group["cheap_candidate_prunes"]),
                total_nodes,
                1_000_000),
            "three_phase_candidate_prune_rate_ppm": integer_ratio(
                str(group["three_phase_candidate_prunes"]),
                three_phase_checks,
                1_000_000),
            "solution_root_elapsed_ms": group["solution_root_elapsed_ms"],
            "max_root_elapsed_ms": str(group["max_root_elapsed_ms"]),
        })
    return result


def emit(rows, output):
    fieldnames = [
        "source_file",
        "benchmark",
        "case_name",
        "elapsed_ms",
        "wall_elapsed_ms",
        "cache_setup_status",
        "cache_setup_elapsed_ms",
        "cache_warm",
        "total_nodes",
        "solution_rank",
        "root_rank",
        "move",
        "outcome",
        "nodes_expanded",
        "root_elapsed_ms",
        "root_nodes_per_ms",
        "cheap_node_prunes",
        "three_phase_node_checks",
        "three_phase_node_prunes",
        "cheap_candidate_prunes",
        "three_phase_candidate_checks",
        "three_phase_candidate_prunes",
        "cheap_candidate_prunes_per_node_ppm",
        "three_phase_candidate_prune_rate_ppm",
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


def emit_summary(rows, output):
    fieldnames = [
        "source_file",
        "benchmark",
        "case_name",
        "roots",
        "before_solution_roots",
        "total_root_nodes",
        "total_root_elapsed_ms",
        "solver_elapsed_ms",
        "wall_elapsed_ms",
        "wall_overhead_ms",
        "cache_setup_status",
        "cache_setup_elapsed_ms",
        "cache_warm",
        "root_nodes_per_ms",
        "cheap_candidate_prunes_per_node_ppm",
        "three_phase_candidate_prune_rate_ppm",
        "solution_root_elapsed_ms",
        "max_root_elapsed_ms",
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
    print(f"root search summary: {output}")


def main():
    input_dir, output, limit, summary = parse_args()
    rows = collect_rows(input_dir)
    if summary:
        rows = summarize_rows(rows)
    if limit:
        rows = rows[:limit]
    if summary:
        emit_summary(rows, output)
    else:
        emit(rows, output)


if __name__ == "__main__":
    main()
