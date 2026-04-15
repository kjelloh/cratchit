#!/usr/bin/env python3
"""
update_index: Scan a namespace folder for markdown entries and generate index.md.

Usage:
    ./update_index.py <namespace>

Example:
    ./update_index.py note
    ./update_index.py todo
"""

import sys
from pathlib import Path


def extract_heading(md_file: Path) -> str:
    """
    Read first line of markdown and extract heading text.
    Assumes format: '# Title'
    """
    with md_file.open("r", encoding="utf-8") as f:
        first_line = f.readline().strip()
    return first_line.lstrip("#").strip()


def main():
    if len(sys.argv) < 2:
        print("Usage: ./update_index.py <namespace>")
        sys.exit(1)

    namespace = sys.argv[1].strip().lower()

    if not namespace.isidentifier():
        print(f"Invalid namespace '{namespace}'")
        sys.exit(1)

    root = Path(namespace)

    if not root.exists():
        print(f"Namespace folder '{namespace}' does not exist.")
        sys.exit(1)

    index_file = root / "index.md"
    links = []

    # Scan: ./namespace/<hash>/<namespace>.md
    for folder in sorted(root.iterdir()):
        if not folder.is_dir():
            continue

        target_md = folder / f"{namespace}.md"
        if not target_md.exists():
            continue

        try:
            heading = extract_heading(target_md)
        except Exception:
            heading = target_md.stem

        rel_path = f"{folder.name}/{namespace}.md"
        links.append(f"[{heading}]({rel_path})")

    # Write index.md
    index_file.write_text("\n".join(links) + "\n", encoding="utf-8")

    print(f"Updated '{index_file}' with {len(links)} entries.")


if __name__ == "__main__":
    main()