"""Assemble the requested 17-part guide and clean source zip from existing files.

Run from the repository root. No network access, no generated test outcomes.
"""
from pathlib import Path
import csv
import os
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'docs' / 'complete-build-guide.md'

def read(name):
    return (ROOT / name).read_text(encoding='utf-8')

def chunk(text, start, end=None):
    result = text.split(start, 1)[1]
    return result.split(end, 1)[0].strip() if end else result.strip()

def links(text, source):
    def replace(match):
        target = match[2]
        if target.startswith(('http:', 'https:', '#', 'mailto:')):
            return match[0]
        path, sep, anchor = target.partition('#')
        resolved = (ROOT / source).parent / path
        relative = Path(os.path.relpath(resolved, DEST.parent)).as_posix()
        return f'[{match[1]}]({relative}{sep}{anchor})'
    return re.sub(r'\[([^\]\n]+)\]\(([^)\n]+)\)', replace, text)

arch = read('docs/architecture.md')
selected = chunk(arch, '## Selected architecture', '## Exact algorithm')
readme = read('README.md')
sections = [
    ('Task interpretation', chunk(arch, '## Task interpretation', '## Selected architecture')),
    ('Recommended final architecture', selected.split('| Option |')[0].strip()),
    ('Why this architecture', '| Option |' + selected.split('| Option |')[1].split('```mermaid')[0]
     + '\n\n' + chunk(arch, '## Safety boundary and residual risks', '## Optional features decision')
     + '\n\n' + chunk(arch, '## Optional features decision', '## Source references')
     + '\n\nSources:\n' + chunk(arch, '## Source references')),
    ('System block diagram', '```mermaid' + selected.split('```mermaid', 1)[1]),
    ('State machine + Mermaid diagram', chunk(arch, '## State machine', '## Safety boundary and residual risks')),
    ('BOM', links(read('hardware/bom.md'), 'hardware/bom.md')),
    ('Complete wiring', links(read('docs/wiring.md'), 'docs/wiring.md')),
    ('Complete final firmware', 'Target: Uno R3; this is the complete maintained sketch.\n\n```cpp\n'
     + read('firmware/smart_parking_barrier/smart_parking_barrier.ino') + '```'),
    ('Firmware logic explanation', chunk(arch, '## Exact algorithm', '## State machine')
     + '\n\nThe filter combines run-length confirmation with hysteresis. The latest reading must also be clear to permit closure. Interrupt capture removes the need for pulseIn; the only deliberate synchronous wait is the 10 microsecond trigger. Endpoint feedback, not Servo.read(), completes movement. Faults retain the open demand unless an actuator fault stops pulses.\n\n'
     + links(read('docs/validation.md'), 'docs/validation.md')),
    ('Physical prototype construction', links(read('docs/construction.md'), 'docs/construction.md')),
    ('Testing matrix', links(read('docs/testing.md'), 'docs/testing.md')),
    ('Debugging guide', links(read('docs/debugging.md'), 'docs/debugging.md')),
    ('Repository structure', chunk(readme, '## Project structure', '## Future improvements')
     + '\n\n' + read('docs/publishing.md')),
    ('Complete README.md', links(readme, 'README.md')),
    ('Demo/video plan', links(read('docs/demo.md'), 'docs/demo.md')),
    ('Final submission text', read('docs/submission.md')),
    ('Final pre-submission checklist', read('docs/checklist.md')),
]
guide = '# Smart Parking Barrier — Complete Build Guide\n\n'
guide += 'Prepared for Task 3. Software is verified; physical build and validation remain pending. This guide follows the requested 17-part order and includes the full firmware and README.\n\n'
for index, (title, body) in enumerate(sections, 1):
    # Keep copied subheadings below the numbered guide sections.
    body = re.sub(r'^(#{1,3}) ', lambda m: '#' * (len(m[1]) + 2) + ' ', body, flags=re.M)
    guide += f'## {index}. {title}\n\n{body.strip()}\n\n'
DEST.write_text(guide.rstrip() + '\n', encoding='utf-8')

results = ROOT / 'docs' / 'test-results.csv'
if not results.exists():
    with results.open('w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['test_id','status','date','firmware_commit','measured_values','evidence_file','notes'])
        for n in range(1, 26):
            writer.writerow([f'T{n:02}', 'NOT RUN', '', '', '', '', 'Physical test pending'])

dist = ROOT / 'dist'
dist.mkdir(exist_ok=True)
archive = dist / 'smart-parking-barrier-source.zip'
excluded = {'.git', '.tools', 'build', 'dist', '__pycache__'}
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for directory, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in excluded]
        for name in files:
            p = Path(directory) / name
            rel = p.relative_to(ROOT)
            z.write(p, Path('smart-parking-barrier') / rel)
print(f'Guide: {DEST}\nSource archive: {archive}')
