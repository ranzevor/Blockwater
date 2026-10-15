"""Index physics-related consumers and native bindings in the pinned release."""
from __future__ import annotations
import hashlib
import io
import json
import re
import urllib.request
import zipfile
from pathlib import PurePosixPath
from audit_redcraft_release import URL, DIGEST, MAX_DOWNLOAD, inspect
from audit_redcraft_structure import mask, structure

TOKENS = ('kWeBlock', 'kEvHitActor', 'kEvExplosion', 'kColRegion',
          'ReadWorldEntities', 'WriteCollision', 'CopyHazards', 'CreateObject',
          'RequestModel', 'SetEntityCollision', 'CreateRemoteThread',
          'WriteProcessMemory', 'VirtualProtect', 'GetProcAddress', 'LoadLibrary')


def index_source(path: str, text: str) -> dict:
    clean = mask(text)
    functions = structure(text)
    references = []
    for line_number, line in enumerate(clean.splitlines(), 1):
        for token in TOKENS:
            if re.search(r'\b' + re.escape(token) + r'\w*\b', line):
                owners = [item['function'] for item in functions
                          if item['start_line'] <= line_number <= item['end_line']]
                references.append({'symbol': token, 'line': line_number, 'functions': owners})
    invocations = []
    for match in re.finditer(r'\binvoke\s*<[^>]+>\s*\(([^,)]*)', clean):
        line_number = clean.count('\n', 0, match.start()) + 1
        argument = match.group(1).strip()
        literal = re.fullmatch(r'0x[0-9a-fA-F]+[uUlL]*', argument) is not None
        invocations.append({'line': line_number, 'literal_hash': argument if literal else None,
                            'dynamic_hash': not literal})
    chosen = ('SendRegion', 'BuildAndSend', 'BuildWorldEntities', 'ConsumeRenderMessage',
              'ReadWorldEntities', 'WriteCollision', 'CopyHazards', 'Tick', 'DllMain', 'PatchSlot')
    return {'path': path, 'sha256': hashlib.sha256(text.encode('utf-8')).hexdigest(),
            'references': references, 'native_invocations': invocations,
            'selected_functions': [item for item in functions
                                   if item['function'].split('::')[-1] in chosen]}


def self_test() -> None:
    sample = 'void Tick() { invoke<int>(0x1234, 1); }\n// invoke<int>(0x9999, 2)\n'
    result = index_source('sample.cpp', sample)
    assert len(result['native_invocations']) == 1
    assert result['native_invocations'][0]['literal_hash'] == '0x1234'
    assert index_source('dynamic.cpp', 'void f() { invoke<int>(hash, 1); }')['native_invocations'][0]['dynamic_hash']
    assert not index_source('comment.cpp', '// CreateObject(x);\n')['references']
    assert not index_source('string.cpp', 'char const* x = "CreateObject(x)";')['references']


if __name__ == '__main__':
    self_test()
    request = urllib.request.Request(URL, headers={'User-Agent': 'Blockwater-release-audit'})
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read(MAX_DOWNLOAD + 1)
    if len(data) > MAX_DOWNLOAD or hashlib.sha256(data).hexdigest() != DIGEST:
        raise SystemExit('release size or SHA-256 mismatch')
    inspect(data)
    with zipfile.ZipFile(io.BytesIO(data)) as outer:
        inner_bytes = outer.read('RedCraft/RedCraft-source.zip')
    sources = []
    with zipfile.ZipFile(io.BytesIO(inner_bytes)) as inner:
        for item in inner.infolist():
            if PurePosixPath(item.filename).suffix.lower() in {'.cpp', '.h', '.fx'}:
                sources.append(index_source(item.filename, inner.read(item).decode('utf-8-sig')))
    bindings = [binding for source in sources for binding in source['native_invocations']]
    print(json.dumps({'archive_sha256': DIGEST, 'self_test': 'passed',
                      'source_files_indexed': len(sources),
                      'native_invocations': len(bindings),
                      'dynamic_native_hashes': sum(item['dynamic_hash'] for item in bindings),
                      'files': sources}, ensure_ascii=True, indent=2))
