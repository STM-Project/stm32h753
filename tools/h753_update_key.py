"""Prepare a local update key header for an H753 application build."""
import os
from pathlib import Path
import re
import secrets
import sys


def generate_header(header: Path) -> None:
    key_path = Path(__file__).resolve().parents[1] / "Debug" / "bootloader-app" / "update-key"
    key_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        fd = os.open(key_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    except FileExistsError:
        pass
    else:
        with os.fdopen(fd, "w", encoding="ascii") as output:
            output.write(secrets.token_hex(16) + "\n")

    key = key_path.read_text(encoding="ascii").strip()
    if not re.fullmatch(r"[0-9a-f]{32}", key):
        raise ValueError("update-key must contain 32 lowercase hexadecimal characters")
    contents = f'#define H753_BOOT_UPDATE_KEY "{key}"\n'
    if not header.exists() or header.read_text(encoding="ascii") != contents:
        with os.fdopen(os.open(header, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600),
                       "w", encoding="ascii") as output:
            output.write(contents)


if __name__ == "__main__":
    generate_header(Path(sys.argv[1]))
