"""Write a concise GitHub Actions summary from NanoCoverage test artifacts."""

import os
from pathlib import Path
import xml.etree.ElementTree as ET


def main():
    log = Path(os.environ["TEST_LOG"])
    xml = Path("demo/reports/report_1/results.xml")
    lcov = Path("demo/coverage-report/lcov.info")
    artifact_url = os.environ.get("ARTIFACT_URL", "")
    lines = []

    output = log.read_text(errors="replace") if log.is_file() else ""
    lines.append(f"- C++: {'passed' if '[OK] C++ Tests Passed' in output else 'not confirmed'}")

    if xml.is_file():
        root = ET.parse(xml).getroot()
        total = int(root.get("tests", "0"))
        skipped = int(root.get("skipped", "0"))
        failures = int(root.get("failures", "0"))
        errors = sum(int(s.get("errors", "0")) for s in root.findall("testsuite"))
        flaky = int(root.get("flaky", "0"))
        lines.append(
            f"- GdUnit4: {total - skipped}/{total} executed, "
            f"{failures} failures, {errors} errors, {flaky} flaky, {skipped} skipped"
        )
    else:
        lines.append("- GdUnit4: report unavailable")

    if lcov.is_file():
        total_lines = covered_lines = 0
        for line in lcov.read_text(errors="replace").splitlines():
            if line.startswith("LF:"):
                total_lines += int(line[3:])
            elif line.startswith("LH:"):
                covered_lines += int(line[3:])
        if total_lines:
            lines.append(
                f"- GDScript coverage (all instrumented files): "
                f"{covered_lines}/{total_lines} lines ({100 * covered_lines / total_lines:.1f}%)"
            )
        else:
            lines.append("- GDScript coverage: no line records")
    else:
        lines.append("- GDScript coverage: report unavailable")

    if artifact_url:
        lines.append(f"- [Detailed reports]({artifact_url})")

    with Path(os.environ["GITHUB_STEP_SUMMARY"]).open("a") as summary:
        summary.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
