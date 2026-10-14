#!/usr/bin/env python3
"""Validate language catalogs and local Markdown links for a source checkout."""

from __future__ import annotations

import re
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent
KEY = re.compile(r"[a-z][a-z0-9]*(?:\.[a-z][a-z0-9_]*)+")
LINK = re.compile(r"(?<!!)\[[^\]\n]+\]\(([^)]+)\)")
PLACEHOLDER = re.compile(r"\{[a-zA-Z][a-zA-Z0-9_]*\}")


def catalog(locale: str) -> dict[str, str]:
    path = ROOT / "locales" / f"{locale}.lang"
    values: dict[str, str] = {}
    for number, raw in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise ValueError(f"{path.name}:{number}: missing '='")
        key, value = (section.strip() for section in line.split("=", 1))
        if not KEY.fullmatch(key) or not value or key in values:
            raise ValueError(f"{path.name}:{number}: invalid or duplicate message key")
        values[key] = value
    if not values:
        raise ValueError(f"{path.name}: empty catalog")
    return values


def check_local_links() -> int:
    checked = 0
    for path in ROOT.rglob("*.md"):
        if any(segment.startswith("build") for segment in path.relative_to(ROOT).parts):
            continue
        contents = path.read_text(encoding="utf-8")
        for match in LINK.finditer(contents):
            target = match.group(1).strip()
            if target.startswith(("https://", "http://", "mailto:", "#")):
                continue
            target = unquote(target.split("#", 1)[0])
            if not target:
                continue
            resolved = (path.parent / target).resolve()
            if not resolved.is_relative_to(ROOT) or not resolved.exists():
                raise ValueError(f"{path.relative_to(ROOT)}: invalid link to {target}")
            checked += 1
    return checked


def main() -> None:
    english = catalog("en-US")
    chinese = catalog("zh-CN")
    missing = sorted(english.keys() - chinese.keys())
    extra = sorted(chinese.keys() - english.keys())
    if missing or extra:
        raise ValueError(f"locale keys mismatch: missing zh-CN={missing}, extra zh-CN={extra}")
    for key, source in english.items():
        if set(PLACEHOLDER.findall(source)) != set(PLACEHOLDER.findall(chinese[key])):
            raise ValueError(f"placeholder mismatch: {key}")
    links = check_local_links()
    print(f"Validated {len(english)} message keys in 2 locales and {links} local documentation links")


if __name__ == "__main__":
    main()
