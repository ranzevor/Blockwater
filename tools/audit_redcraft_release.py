"""Inspect a digest-pinned release archive without executing or publishing its code."""
from __future__ import annotations
import hashlib
import io
import json
import re
import stat
import urllib.request
import zipfile
from pathlib import PurePosixPath

URL = "https://github.com/Keel62155/Minecraft-X-Games/releases/download/3.0.0-RedCraft/RedCraft.zip"
DIGEST = "f003eb89654b7d3ac09c6fd211cf9902c46c7c68d4f18156bcfb9b63038901d1"
MAX_DOWNLOAD = 2 * 1024 * 1024
MAX_EXPANDED = 16 * 1024 * 1024
SOURCE_SUFFIXES = {".cpp", ".cc", ".c", ".h", ".hpp", ".java", ".cs", ".fx", ".hlsl", ".glsl", ".py", ".lua"}
SYMBOLS = re.compile(r"\b(?:CREATE_OBJECT\w*|DELETE_ENTITY|SET_ENTITY_COLLISION|FREEZE_ENTITY_POSITION|ADD_\w*EXPLOSION|APPLY_DAMAGE_TO_PED|SET_ENTITY_HEALTH|GET_ENTITY_HEALTH|SHOOT_SINGLE_BULLET_BETWEEN_COORDS|START_\w*SHAPE_TEST\w*|GET_SHAPE_TEST_RESULT|GET_GROUND_Z_FOR_3D_COORD|GET_FINAL_RENDERED_CAM\w*|NETWORK_IS_\w+|CreateFileMapping\w*|OpenFileMapping\w*|MapViewOfFile|CreateCommittedResource\w*|CreateSharedHandle|OpenSharedHandle\w*|CreateGraphicsPipelineState|Present|CreateSwapChain\w*|D3D11\w*|D3D12\w*|DXGI\w*|MinHook|MH_\w+|glReadPixels|glFenceSync|vk\w+)\b")
FEATURES = {
    "collision": r"collis|hitbox|\bsolid\b|shape.?test",
    "blocks": r"\bblocks?\b|voxel|placed|destroyed",
    "combat": r"damage|health|combat|projectile|\bTNT\b|creeper|explosion",
    "render": r"depth|shader|render|texture|mesh|vertex|indexbuffer|swap.?chain|reproject",
    "persistence": r"save|persist|ofstream|ifstream|fwrite|fread",
    "lifecycle": r"heartbeat|epoch|session|disconnect|timeout|multiplayer",
    "limits": r"capacity|MAX_|kMax|limit|budget|clamp",
    "tests": r"assert|test_|CTest|add_test|pytest|unittest",
}

def inspect(data: bytes) -> dict:
    report = {"archive_sha256": hashlib.sha256(data).hexdigest(), "archive_bytes": len(data), "files": [], "licenses": [], "source_index": [], "text_index": []}
    total = 0
    count = 0
    def walk(blob: bytes, prefix: str, depth: int) -> None:
        nonlocal total, count
        if depth > 2:
            raise ValueError("archive nesting exceeds inspection budget")
        with zipfile.ZipFile(io.BytesIO(blob)) as archive:
            for item in archive.infolist():
                path = PurePosixPath(item.filename.replace("\\", "/"))
                if path.is_absolute() or ".." in path.parts or (path.parts and ":" in path.parts[0]):
                    raise ValueError("unsafe archive path")
                if stat.S_ISLNK(item.external_attr >> 16) or item.flag_bits & 1:
                    raise ValueError("unsupported archive member")
                if item.is_dir():
                    continue
                total += item.file_size
                count += 1
                if total > MAX_EXPANDED or count > 2000:
                    raise ValueError("archive exceeds inspection budget")
                body = archive.read(item)
                name = prefix + path.as_posix()
                report["files"].append({"path": name, "bytes": len(body), "sha256": hashlib.sha256(body).hexdigest()})
                if path.suffix.lower() == ".zip":
                    walk(body, name + "!", depth + 1)
                    continue
                if len(body) > 2 * 1024 * 1024:
                    continue
                try:
                    text = body.decode("utf-8-sig")
                except UnicodeDecodeError:
                    continue
                lines = text.splitlines()
                license_file = bool(re.search(r"(^|[_.-])(licen[cs]e|copying|notice)([_.-]|$)", path.name, re.I))
                if license_file:
                    report["licenses"].append({"path": name, "text": text[:16000], "complete": len(text) <= 16000})
                if path.suffix.lower() in SOURCE_SUFFIXES:
                    symbols = {}
                    feature_lines = {key: [] for key in FEATURES}
                    for line_number, line in enumerate(lines, 1):
                        for symbol in set(SYMBOLS.findall(line)):
                            symbols.setdefault(symbol, []).append(line_number)
                        for feature, pattern in FEATURES.items():
                            if re.search(pattern, line, re.I):
                                feature_lines[feature].append(line_number)
                    constants = re.findall(r"\b((?:k[A-Z]|[A-Z_]{3})\w*)\s*=\s*([-+]?\d+(?:\.\d+)?)(?:[uUlLfF]*\b)", text)
                    includes = re.findall(r"^\s*#include\s+[<\"]([^>\"]+)[>\"]", text, re.M)
                    definitions = re.findall(r"^\s*(?:(?:static|inline|constexpr|virtual|extern)\s+)*(?:[\w:*&<>]+\s+)+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?\{", text, re.M)
                    report["source_index"].append({"path": name, "lines": len(lines), "includes": includes, "functions": definitions, "symbols": symbols, "numeric_constants": constants, "feature_lines": feature_lines, "license_markers": {key: len(re.findall(pattern, text, re.I)) for key, pattern in {"mit_grant": "Permission is hereby granted", "spdx": "SPDX-License-Identifier", "copyright": "Copyright", "all_rights_reserved": "all rights reserved"}.items()}})
                elif path.suffix.lower() in {".md", ".txt", ".ini", ".cmake", ".json", ".def", ".bat", ".ps1"}:
                    report["text_index"].append({"path": name, "lines": len(lines), "headings": re.findall(r"^#{1,4}\s+(.+)$", text, re.M), "versions": sorted(set(re.findall(r"\b\d+\.\d+(?:\.\d+){0,2}\b", text))), "license_markers": {key: len(re.findall(pattern, text, re.I)) for key, pattern in {"mit_grant": "Permission is hereby granted", "spdx": "SPDX-License-Identifier", "copyright": "Copyright", "all_rights_reserved": "all rights reserved"}.items()}})
    walk(data, "", 0)
    report["expanded_bytes"] = total
    return report

if __name__ == "__main__":
    request = urllib.request.Request(URL, headers={"User-Agent": "Blockwater-release-audit"})
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read(MAX_DOWNLOAD + 1)
    if len(data) > MAX_DOWNLOAD or hashlib.sha256(data).hexdigest() != DIGEST:
        raise SystemExit("release size or SHA-256 mismatch")
    result = inspect(data)
    print("AUDIT_REPORT_BEGIN")
    print(json.dumps(result, ensure_ascii=True, indent=2))
    print("AUDIT_REPORT_END")
