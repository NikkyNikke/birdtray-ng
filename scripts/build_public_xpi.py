#!/usr/bin/env python3
"""Build the distributable XPI for Minimize on Close - Birdtray Wayland.

Packs the files from extension/ into a deterministic zip (fixed member
timestamps and permissions) so that the same source always yields the
same SHA-256 digest.
"""

import argparse
import hashlib
import json
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "extension"
DEFAULT_ID = "birdtray-close@nikky.st"
DEFAULT_OUTPUT = (
    ROOT / "artifacts" / "minimize-on-close-birdtray-wayland-158-2.0.3.2-public.xpi"
)

# Root-level XPI members, packed in this order.
FILES = [
    "manifest.json",
    "background.js",
    "experiment.js",
    "experiment.json",
    "icon.png",
    "LICENSE",
]

FIXED_DATE = (2020, 1, 1, 0, 0, 0)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--addon-id",
        default=DEFAULT_ID,
        help="gecko id written into the packed manifest (default: %(default)s)",
    )
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT))
    args = parser.parse_args()

    manifest = json.loads((SOURCE / "manifest.json").read_text(encoding="utf-8"))
    manifest["applications"]["gecko"]["id"] = args.addon_id
    manifest_bytes = (
        json.dumps(manifest, indent=2, ensure_ascii=True) + "\n"
    ).encode("utf-8")

    for name in FILES:
        if not (SOURCE / name).is_file():
            raise SystemExit(f"missing extension source file: {name}")

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in FILES:
            if name == "manifest.json":
                data = manifest_bytes
            else:
                data = (SOURCE / name).read_bytes()
            info = zipfile.ZipInfo(name, date_time=FIXED_DATE)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, data)

    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    print(f"{digest}  {output}")


if __name__ == "__main__":
    main()
