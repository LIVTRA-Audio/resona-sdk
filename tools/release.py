#!/usr/bin/env python3
"""Pack this SDK as resona-sdk-<version>.tar.gz plus its .sha256, for a GitHub release.

The archive is reproducible (sorted names, fixed times and owners) and holds one top-level
folder, resona-sdk-<version>/. It runs tools/check_source.py first and refuses to pack a
tree that fails it. Creator Lab downloads the archive, installs it into its SDK cache and
checks it against the sha256 a project pins in resona.sdk.lock (that pin hashes the
installed files, not this archive). Offline; standard library only.
"""
import argparse
import gzip
import hashlib
import io
import json
import subprocess
import sys
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKIP_DIRS = {'.git', '.github', 'build', 'target', 'dist', '__pycache__', 'node_modules'}
SKIP_FILES = {'.DS_Store', 'Thumbs.db'}


def files():
    for path in sorted(ROOT.rglob('*')):
        rel = path.relative_to(ROOT)
        if any(part in SKIP_DIRS for part in rel.parts) or path.name in SKIP_FILES:
            continue
        if path.is_symlink():
            sys.exit(f'refusing a symlink: {rel}')
        if path.is_file():
            yield rel


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT.parent / 'resona-sdk-releases',
                        help='output folder (outside the SDK, which must stay free of dist/)')
    parser.add_argument('--terms-file', type=Path, help='private name list for check_source.py')
    args = parser.parse_args()
    check = [sys.executable, str(ROOT / 'tools/check_source.py')]
    if args.terms_file:
        check += ['--terms-file', str(args.terms_file)]
    if subprocess.run(check).returncode:
        sys.exit('source check failed; nothing was packed')
    version = json.loads((ROOT / 'sdk.json').read_text(encoding='utf-8'))['version']
    name = f'resona-sdk-{version}'
    raw = io.BytesIO()
    with tarfile.open(fileobj=raw, mode='w', format=tarfile.PAX_FORMAT) as tar:
        for rel in files():
            data = (ROOT / rel).read_bytes()
            info = tarfile.TarInfo(f'{name}/{rel.as_posix()}')
            info.size = len(data)
            info.mode = 0o755 if rel.suffix in ('.py', '.sh') else 0o644
            info.mtime = 0
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            tar.addfile(info, io.BytesIO(data))
    args.out.mkdir(parents=True, exist_ok=True)
    archive = args.out / f'{name}.tar.gz'
    with open(archive, 'wb') as f, gzip.GzipFile(filename='', mode='wb', fileobj=f, mtime=0) as gz:
        gz.write(raw.getvalue())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (args.out / f'{name}.tar.gz.sha256').write_text(f'{digest}  {archive.name}\n', encoding='utf-8')
    print(f'{archive} ({archive.stat().st_size} bytes) sha256 {digest}')


if __name__ == '__main__':
    main()
