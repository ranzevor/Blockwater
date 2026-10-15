"""Report licensing scope and call structure from a verified archive."""
from __future__ import annotations
import hashlib
import io
import json
import re
import urllib.request
import zipfile
from pathlib import PurePosixPath
from audit_redcraft_release import URL, DIGEST, MAX_DOWNLOAD, inspect

TOKEN = re.compile(r'R"(?P<delimiter>[^\s()\\]{0,16})\(.*?\)(?P=delimiter)"|//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
DEFINITION = re.compile(r'^\s*(?:(?:static|inline|constexpr|virtual|extern|BOOL|DWORD|unsigned|const)\s+)*(?:[\w:*&<>]+\s+)+(?P<name>[A-Za-z_~]\w*(?:::[A-Za-z_~]\w*)*)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?\{', re.M)
CALL = re.compile(r'\b([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)(?:\s*<[^;{}()]*>)?\s*\(')
KEYWORDS = {'if', 'for', 'while', 'switch', 'catch', 'sizeof', 'alignof', 'return', 'decltype', 'static_cast', 'reinterpret_cast', 'const_cast'}

def mask(text: str) -> str:
    return TOKEN.sub(lambda match: ''.join('\n' if char == '\n' else ' ' for char in match.group()), text)

def structure(text: str) -> list[dict]:
    clean = mask(text)
    result = []
    for match in DEFINITION.finditer(clean):
        start = match.end() - 1
        depth = 1
        end = start + 1
        while end < len(clean) and depth:
            depth += (clean[end] == '{') - (clean[end] == '}')
            end += 1
        if depth:
            raise ValueError('unbalanced function body')
        body = clean[start:end]
        calls = sorted(set(CALL.findall(body)) - KEYWORDS)
        result.append({'function': match.group('name'), 'start_line': text.count('\n', 0, match.start()) + 1, 'end_line': text.count('\n', 0, end) + 1, 'calls': calls, 'native_hashes': sorted(set(re.findall(r'0x[0-9A-Fa-f]{16}\b', body))), 'control_counts': {name: len(re.findall(r'\b' + name + r'\b', body)) for name in ['if', 'switch', 'for', 'while', 'return']}})
    return result

if __name__ == '__main__':
    request = urllib.request.Request(URL, headers={'User-Agent': 'Blockwater-release-audit'})
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read(MAX_DOWNLOAD + 1)
    if len(data) > MAX_DOWNLOAD or hashlib.sha256(data).hexdigest() != DIGEST:
        raise SystemExit('release size or SHA-256 mismatch')
    inspect(data)
    outer = zipfile.ZipFile(io.BytesIO(data))
    inner = zipfile.ZipFile(io.BytesIO(outer.read('RedCraft/RedCraft-source.zip')))
    report = {'archive_sha256': DIGEST, 'license_scope_text': [], 'readme_evidence': [], 'call_structure': [], 'build': {}, 'native_object_creation_hash_present': False}
    texts = {'RedCraft/README.txt': outer.read('RedCraft/README.txt').decode('utf-8-sig')}
    for item in inner.infolist():
        if not item.is_dir() and PurePosixPath(item.filename).suffix.lower() in {'.cpp', '.h', '.fx', '.md', '.txt'}:
            texts['source!' + item.filename] = inner.read(item).decode('utf-8-sig')
    for path, text in texts.items():
        lower = path.lower()
        if 'notices' in lower or 'license' in lower:
            report['license_scope_text'].append({'path': path, 'text': text})
        if 'readme' in lower:
            license_paragraphs = [paragraph for paragraph in text.split('\n\n') if re.search(r'licen[cs]|permission|redistribut|copyright|\bMIT\b|\bGPL\b|rights reserved', paragraph, re.I)]
            excerpts = []
            for number, line in enumerate(text.splitlines(), 1):
                if re.search(r'block.*(?:collis|horse|wagon|NPC)|(?:collis|horse|wagon|NPC).*block|not.*support|known.*limit|ReShade|DX12|DirectX|SkyCraft', line, re.I):
                    excerpts.append({'line': number, 'words': line.split()[:24]})
            report['readme_evidence'].append({'path': path, 'license_paragraphs': license_paragraphs, 'feature_excerpts': excerpts[:8], 'urls': re.findall(r'https?://[^\s)>]+', text), 'versions': sorted(set(re.findall(r'(?<![\d.])\d+(?:\.\d+){1,3}(?![\d.])', text)))})
        if lower.endswith(('redcraft.cpp', 'collision.cpp', 'link.cpp', 'reshadebridge.cpp', 'renderer.cpp')):
            clean = mask(text)
            all_calls = sorted(set(CALL.findall(clean)) - KEYWORDS)
            report['call_structure'].append({'path': path, 'functions': structure(text), 'native_invocations': len(re.findall(r'\binvoke\s*<', clean)), 'all_call_symbols': all_calls if lower.endswith(('redcraft.cpp', 'collision.cpp')) else None, 'object_terms': sorted(set(re.findall(r'\b\w*(?:[Cc]reate[Oo]bject|[Rr]equest[Mm]odel|[Bb]lock[Pp]rop|[Cc]ollision[Bb]ody|[Pp]hysics[Mm]esh|[Ss]et[Ee]ntity[Cc]ollision)\w*\b', clean))), 'native_hashes': sorted(set(re.findall(r'0x[0-9a-fA-F]{16}\b', clean)))})
            if '509d5878eb39e842' in clean.lower():
                report['native_object_creation_hash_present'] = True
        if lower.endswith('cmakelists.txt'):
            report['build'] = {'source_files': re.findall(r'\bsrc/[\w./-]+', text), 'target_libraries': re.findall(r'target_link_libraries\([^)]*\)', text, re.S), 'tests_registered': bool(re.search(r'\badd_test\s*\(', text)), 'standard': re.findall(r'CXX_STANDARD\s+([0-9]+)', text)}
    print('FOCUSED_AUDIT_BEGIN')
    print(json.dumps(report, ensure_ascii=True, indent=2))
    print('FOCUSED_AUDIT_END')
