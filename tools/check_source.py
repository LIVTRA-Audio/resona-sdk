#!/usr/bin/env python3
"""Read-only, offline checks for the standalone SDK source tree (Python 3)."""
import argparse
import json
import os
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
GENERATED = {'build', 'target', 'dist', '__pycache__', 'node_modules'}
FORBIDDEN = {
    '.o', '.obj', '.a', '.lib', '.so', '.dylib', '.dll', '.exe', '.elf', '.map',
    '.rpk', '.rmod', '.itb', '.img', '.bin', '.fmu', '.ufw', '.ead', '.nam',
    '.key', '.pem', '.der', '.p12', '.pfx', '.pyc', '.pyo',
}
PRIVATE_KEY = re.compile(r'-----BEGIN (?:[A-Z0-9]+ )*PRIVATE KEY-----')
LOCAL_PATH = re.compile('/' + 'Users/' + r'|/' + 'home/' + r'|[A-Za-z]:\\Users\\')
LINK = re.compile(r'\[[^\]]*\]\(([^\s)]+)(?:\s+"[^"]*")?\)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--terms-file', type=Path,
                        help='external private name policy: one forbidden term per line')
    args = parser.parse_args()
    silicon = None
    if args.terms_file:
        try:
            terms = [line.strip() for line in args.terms_file.read_text(encoding='utf-8').splitlines()
                     if line.strip() and not line.lstrip().startswith('#')]
            if not terms:
                parser.error('external name policy is empty')
            silicon = re.compile(r'(?<![a-z0-9])(?:' + '|'.join(map(re.escape, terms)) + r')(?![a-z0-9])', re.I)
        except OSError:
            parser.error('cannot read external name policy')
    failures = []
    files = []

    def fail(path, reason):
        failures.append(f'{path.relative_to(ROOT)}: {reason}')

    def local_file(owner, base, value):
        if not isinstance(value, str):
            fail(owner, 'non-string source reference')
            return
        target = (base / value).resolve()
        if ROOT != target and ROOT not in target.parents:
            fail(owner, 'reference leaves standalone SDK')
        elif not target.is_file():
            fail(owner, f'missing local file: {value}')

    for directory, dirs, names in os.walk(ROOT, followlinks=False):
        base = Path(directory)
        for name in list(dirs):
            path = base / name
            if name == '.git':
                dirs.remove(name)  # Future repository metadata is not source.
            elif path.is_symlink() or name in GENERATED or name == 'firmware':
                fail(path, 'symlink, generated directory or firmware directory')
                dirs.remove(name)
        for name in names:
            path = base / name
            if path.is_symlink():
                fail(path, 'symlink is not a standalone source file')
                continue
            if (path.suffix.lower() in FORBIDDEN or name in {'.DS_Store', 'Thumbs.db', '.env'}
                    or name.startswith('.env.')):
                fail(path, 'generated, model, credential or machine-local file')
                continue
            files.append(path)

    for name in ('LICENSE', 'THIRD_PARTY_NOTICES.md', '.gitignore', 'README.md',
                 'CONTRIBUTING.md', 'include/resona/module.h', 'include/resona/platform.h'):
        if not (ROOT / name).is_file():
            failures.append(f'{name}: required distribution file missing')

    header = ROOT / 'include/resona/pack.h'
    if header.is_file() and not re.search(
            r"#define\s+RESONA_PACK_TARGET_BOX\s+RESONA_PACK_TAG\(\s*'R'\s*,\s*'B'\s*,\s*'X'\s*,\s*'1'\s*\)",
            header.read_text(encoding='utf-8')):
        fail(header, 'expected the product-specific RBX1 target definition')

    for path in files:
        try:
            raw = path.read_bytes()
            if b'\x00' in raw:
                fail(path, 'binary data is not part of this source distribution')
                continue
            text = raw.decode('utf-8')
        except (OSError, UnicodeError):
            fail(path, 'cannot read as UTF-8 source')
            continue
        if PRIVATE_KEY.search(text):
            fail(path, 'private-key marker')
        if LOCAL_PATH.search(text):
            fail(path, 'machine-specific absolute path')
        # Attribution must remain accurate even if future notices name a backend.
        if not path.name.startswith(('LICENSE', 'COPYING', 'NOTICE', 'THIRD_PARTY_NOTICES')):
            if silicon and silicon.search(text):
                fail(path, 'silicon-specific name in public SDK surface')
        if path.suffix == '.md':
            for match in LINK.finditer(text):
                value = match.group(1).strip('<>')
                link = urlsplit(value)
                if link.scheme or link.netloc or not link.path:
                    continue
                local_file(path, path.parent, unquote(link.path))
        if path.name in ('resona.project.json', 'template.json'):
            try:
                manifest = json.loads(text)
                if 'from' in manifest:
                    value = manifest['from']
                    if not isinstance(value, str):
                        raise ValueError('template from must be a string')
                    local_file(path, ROOT, value + '/resona.project.json')
                else:
                    for source in manifest.get('sources', []):
                        local_file(path, path.parent, source)
                if 'open' in manifest:
                    base = ROOT / manifest['from'] if 'from' in manifest else path.parent
                    local_file(path, base, manifest['open'])
            except (ValueError, TypeError) as error:
                fail(path, f'invalid manifest: {error}')

    if failures:
        print('\n'.join(sorted(failures)), file=sys.stderr)
        return 1
    policy = 'external name policy checked' if silicon else 'name policy not supplied'
    print(f'SDK source check passed ({len(files)} files; {policy}; no Git, network or device operations).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
