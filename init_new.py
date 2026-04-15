#!/usr/bin/env python3
"""
init_new: Create a typed folder with a hashed subfolder,
and inside it create a markdown file named after a sanitized heading.

Rules:
- Spaces -> '_'
- Invalid filename characters -> '?'
"""

import hashlib
import sys
import re
from pathlib import Path

HASH_LENGTH = 8


def compute_hash(text: str) -> str:
    return hashlib.md5(text.encode("utf-8")).hexdigest()[:HASH_LENGTH]

def main():
    if len(sys.argv) < 3:
        print("Usage: ./init_new.py <type> 'Your heading text here'")
        sys.exit(1)

    entry_type = sys.argv[1].strip().lower()
    heading = " ".join(sys.argv[2:]).strip()

    if not entry_type.isidentifier():
        print(f"Invalid type '{entry_type}'. Use a simple name like 'todo' or 'note'.")
        sys.exit(1)

    short_hash = compute_hash(heading)

    base_path = Path(entry_type)
    folder_path = base_path / short_hash
    folder_path.mkdir(parents=True, exist_ok=True)

    # sanitized filename
    file_path = folder_path / f"{entry_type}.md"

    if file_path.exists():
        print(f"File '{file_path}' already exists")
    else:
        file_path.write_text(f"# {heading}\n\n", encoding="utf-8")
        print(f"Created '{file_path}'")

    print(f"Directory structure ensured: '{folder_path}'")


if __name__ == "__main__":
    main()