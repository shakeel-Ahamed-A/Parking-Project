"""Read-only consistency checks. Run after tests and optional packaging."""
from pathlib import Path
import ast
import csv
import hashlib
import os
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SKETCH = 'firmware/smart_parking_barrier/smart_parking_barrier.ino'
EXCLUDED = {'.git', '.tools', 'build', 'dist', '__pycache__'}

def read(path):
    return (ROOT / path).read_text(encoding='utf8')

def require(condition, message):
    if not condition:
        raise SystemExit('FAIL: ' + message)

files = []
for directory, dirs, names in os.walk(ROOT):
    dirs[:] = [d for d in dirs if d not in EXCLUDED]
    files.extend(Path(directory)/name for name in names)
for path in files:
    if path.suffix == '.py':
        ast.parse(path.read_text(encoding='utf8'), filename=str(path))
    if path.suffix == '.md':
        body = path.read_text(encoding='utf8')
        require(len(re.findall(r'^```', body, re.M)) % 2 == 0,
                f'unbalanced fences: {path}')
        body = re.sub(r'```.*?```', '', body, flags=re.S)
        for target in re.findall(r'\[[^\]\n]+\]\(([^)\n]+)\)', body):
            if target.startswith(('https:', 'http:', 'mailto:', '#')):
                continue
            target = target.split('#')[0]
            require((path.parent / target).is_file(), f'broken local link: {path}: {target}')

tree = ast.parse(read('tests/run_tests.py'))
scenarios = next(ast.literal_eval(n.value) for n in tree.body
                 if isinstance(n, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == 'SCENARIOS' for t in n.targets))
require(len(set(scenarios)) == len(scenarios), 'duplicate scenario')
results = read('docs/software-results.txt')
require(re.findall(r'^PASS (\w+)', results, re.M) == scenarios, 'recorded tests differ from runner')
n = len(scenarios)
require(f'{n}/{n} software scenarios passed.' in results, 'result summary')
require(f'{n} software scenarios pass' in read('README.md'), 'README test count')
require(f'{n} / {n} PASS' in read('docs/validation.md'), 'validation test count')
rows = list(csv.DictReader(read('docs/test-results.csv').splitlines()))
require([r['test_id'] for r in rows] == [f'T{i:02}' for i in range(1, 15)], 'physical IDs')
require(all(r['status'] == 'NOT RUN' for r in rows), 'physical results require real evidence; current build unverified')
require(read('docs/testing.md').count('| NOT RUN |') == 14, 'physical table result count')

source = read(SKETCH)
pins = dict(re.findall(r'(\w+_PIN)\s*=\s*(A\d|\d+)', source))
expected = {'TRIG_PIN':'4', 'ECHO_PIN':'2', 'BEAM_PIN':'3', 'EMITTER_PIN':'5',
            'RED_PIN':'6', 'GREEN_PIN':'7', 'BUZZER_PIN':'8', 'SERVO_PIN':'9',
            'OPEN_LIMIT_PIN':'A0', 'CLOSED_LIMIT_PIN':'A1', 'MANUAL_PIN':'A2'}
require(pins == expected, 'review pin table after firmware pin changes')
wiring = read('docs/wiring.md')
for fragment in ['| D4 | HC-SR04 TRIG', '| D2 | HC-SR04 ECHO', '| D3 | IR receiver',
                 '| D5 → 1 kΩ | NPN base', '| D9 | Servo signal',
                 '| D6 → 330 Ω | Red', '| D7 → 330 Ω | Green',
                 '| D8 → 330 Ω | Passive piezo', '| A0 | OPEN', '| A1 | CLOSED', '| A2 | Hold-open']:
    require(fragment in wiring, 'wiring table mismatch: '+fragment)
states = set(re.search(r'enum class State[^\{]*\{([^}]+)', source)[1].replace('\n', '').replace(' ', '').split(','))
diagram = re.search(r'```mermaid\n(.*?)```', read('README.md'), re.S)[1]
diagram_states = set()
for left, right in re.findall(r'^\s*(\w+|\[\*\]) --> (\w+|\[\*\])', diagram, re.M):
    diagram_states.update([left, right])
diagram_states.discard('[*]')
require(states == diagram_states, 'FSM diagram names')
digest = hashlib.sha256(source.encode()).hexdigest()
require(digest in read('docs/validation.md'), 'firmware changed since validation report')
for paragraph in re.split(r'\n## .*\n', read('docs/submission.md'))[1:]:
    count = len(paragraph.split())
    require(80 <= count <= 120, f'submission length {count}')
workflow = read('.github/workflows/verify.yml')
for required in ['tests/firmware_tests.cpp', 'tests/run_tests.py', 'arduino:avr@1.8.6',
                 'Servo@1.3.0', 'firmware/smart_parking_barrier']:
    require(required in workflow, 'workflow target: '+required)
handout = ROOT/'dist/final-engineering-review.md'
if handout.exists():
    text = handout.read_text(encoding='utf8')
    require('```cpp\n'+source+'```' in text, 'handout firmware differs')
    require(len(re.findall(r'^## \d+\.', text, re.M)) == 17, 'handout section count')
archive = ROOT/'dist/smart-parking-barrier-source.zip'
if archive.exists():
    with zipfile.ZipFile(archive) as z:
        require(all(not EXCLUDED.intersection(Path(p).parts) for p in z.namelist()), 'archive exclusions')
        for path in files:
            name = 'smart-parking-barrier/'+path.relative_to(ROOT).as_posix()
            require(z.read(name) == path.read_bytes(), 'stale archive member: '+name)
print(f'PASS consistency: {n} recorded software scenarios; 14 physical NOT RUN; pins, states, local links, fences, Python syntax, workflow targets and source digest.')
print('External URLs, remote CI and hardware behaviour are not validated by this checker.')
