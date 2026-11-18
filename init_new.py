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
from pathlib import Path

HASH_LENGTH = 8


def compute_hash(text: str) -> str:
    return hashlib.md5(text.encode("utf-8")).hexdigest()[:HASH_LENGTH]


def ensure_entry_folder(
    entry_type: str, heading: str, base_dir: Path = Path(".")
) -> tuple[Path, bool]:
    """Ensure <base_dir>/<entry_type>/<hash(heading)>/<entry_type>.md exists.

    Returns (file_path, created) where created is False if the file was
    already there (left untouched) and True if this call wrote it.
    """
    if not entry_type.isidentifier():
        raise ValueError(f"Invalid type '{entry_type}'. Use a simple name like 'todo' or 'note'.")

    short_hash = compute_hash(heading)
    folder_path = base_dir / entry_type / short_hash
    folder_path.mkdir(parents=True, exist_ok=True)

    file_path = folder_path / f"{entry_type}.md"
    created = not file_path.exists()
    if created:
        escaped_heading = (
            heading
            .replace("|", r"\|")
            .replace("<", r"&lt;")
            .replace(">", r"&gt;")
        )        
        file_path.write_text(
            f"# {escaped_heading}"
            f"\n\n```#{short_hash}```",
            encoding="utf-8")

    return file_path, created


def main():
    if len(sys.argv) < 3:
        print("Usage: ./init_new.py <type> 'Your heading text here'")
        sys.exit(1)

    entry_type = sys.argv[1].strip().lower()
    heading = " ".join(sys.argv[2:]).strip()

    try:
        file_path, created = ensure_entry_folder(entry_type, heading)
    except ValueError as e:
        print(e)
        sys.exit(1)

    if created:
        print(f"Created '{file_path}'")
    else:
        print(f"File '{file_path}' already exists")

    print(f"Directory structure ensured: '{file_path.parent}'")


if __name__ == "__main__":
    main()
