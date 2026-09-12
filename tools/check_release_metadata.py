#!/usr/bin/env python3
"""Verify that release-facing version declarations agree with VERSION."""

from pathlib import Path
import argparse
import json
import re
import sys


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--product-manifest",
        type=Path,
        help="also verify a packaged product manifest against strata.lock",
    )
    args = parser.parse_args()

    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        print(f"VERSION is not a semantic version: {version!r}", file=sys.stderr)
        return 1

    tag = f"v{version}"
    declarations = {
        "openstrata.toml": rf'^version = "{re.escape(version)}"$',
        "plugins/vector-geojson/openstrata.plugin.yaml":
            rf'^plugin: \{{ name: vector-geojson, version: {re.escape(version)},',
        "plugins/vector-geojson/CMakeLists.txt":
            rf'^    VERSION {re.escape(version)}$',
        "CHANGELOG.md": rf'^## \[{re.escape(version)}\] - \d{{4}}-\d{{2}}-\d{{2}}$',
        f"docs/releases/{tag}.md": rf'^# {re.escape(tag)}$',
        "docs/releases/README.md":
            rf'^\| {re.escape(tag)} \| \d{{4}}-\d{{2}}-\d{{2}} \|.*\[{re.escape(tag)}\.md\]\({re.escape(tag)}\.md\)',
    }

    failures = []
    for relative_path, pattern in declarations.items():
        path = ROOT / relative_path
        if not path.is_file() or re.search(
            pattern, path.read_text(encoding="utf-8"), flags=re.MULTILINE
        ) is None:
            failures.append(relative_path)

    if args.product_manifest:
        manifest_path = args.product_manifest.resolve()
        try:
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            lock = json.loads((ROOT / "strata.lock").read_text(encoding="utf-8"))
            expected_digest = lock["runtime"]["digest"]
            runtime = manifest["provenance"]["runtime"]
            actual_digest = runtime["digest"]
            source = runtime.get("source")
        except (OSError, KeyError, TypeError, json.JSONDecodeError) as error:
            failures.append(f"{manifest_path} (unreadable or malformed: {error})")
        else:
            if actual_digest != expected_digest:
                failures.append(
                    f"{manifest_path} (runtime digest {actual_digest!r} "
                    f"does not match strata.lock {expected_digest!r})"
                )
            if source == "local":
                failures.append(f"{manifest_path} (runtime source is unmanaged local)")

    if failures:
        print(f"Release metadata does not match VERSION {version}:", file=sys.stderr)
        for relative_path in failures:
            print(f"- {relative_path}", file=sys.stderr)
        return 1

    print(f"Release metadata is consistent: {version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())