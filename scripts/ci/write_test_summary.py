#!/usr/bin/env python3

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def _text(element: ET.Element | None) -> str:
    if element is None:
        return ""
    return "".join(element.itertext()).strip()


def _escape_cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ").strip()


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: write_test_summary.py <junit-xml>", file=sys.stderr)
        return 2

    report_path = Path(sys.argv[1])
    print("## Test Summary")
    print()

    if not report_path.is_file():
        print(f"Test report not found: `{report_path}`")
        return 0

    root = ET.parse(report_path).getroot()
    suites = root.findall("testsuite")
    if not suites and root.tag == "testsuite":
        suites = [root]

    total_tests = 0
    total_failures = 0
    total_errors = 0
    total_skipped = 0
    total_time = 0.0
    failed_cases: list[tuple[str, str, str]] = []

    for suite in suites:
        total_tests += int(suite.attrib.get("tests", "0"))
        total_failures += int(suite.attrib.get("failures", "0"))
        total_errors += int(suite.attrib.get("errors", "0"))
        total_skipped += int(suite.attrib.get("skipped", "0"))
        total_time += float(suite.attrib.get("time", "0") or 0.0)

        for case in suite.findall("testcase"):
            failure = case.find("failure")
            error = case.find("error")
            skipped = case.find("skipped")
            if failure is not None or error is not None:
                outcome = "failed" if failure is not None else "error"
                details = _text(failure if failure is not None else error)
                failed_cases.append((
                    case.attrib.get("name", "unknown"),
                    outcome,
                    details or "No failure details available.",
                ))
            elif skipped is not None:
                continue

    passed = total_tests - total_failures - total_errors - total_skipped
    status = "PASS" if total_failures == 0 and total_errors == 0 else "FAIL"

    print(f"**Overall Status:** `{status}`")
    print()
    print("| Metric | Value |")
    print("| --- | ---: |")
    print(f"| Total | {total_tests} |")
    print(f"| Passed | {passed} |")
    print(f"| Failed | {total_failures} |")
    print(f"| Errors | {total_errors} |")
    print(f"| Skipped | {total_skipped} |")
    print(f"| Duration | {total_time:.3f}s |")
    print()

    if failed_cases:
        print("### Failed Cases")
        print()
        print("| Test | Outcome |")
        print("| --- | --- |")
        for name, outcome, _ in failed_cases:
            print(f"| {_escape_cell(name)} | {outcome} |")
        print()

        for name, outcome, details in failed_cases:
            print(f"<details><summary>{_escape_cell(name)} ({outcome})</summary>")
            print()
            print("```text")
            print(details)
            print("```")
            print("</details>")
            print()
    else:
        print("All tests passed.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
