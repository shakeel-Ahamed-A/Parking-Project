"""Package maintained sources and a review handout. Run from any directory."""
from pathlib import Path
import os
import re
import zipfile

ROOT=Path(__file__).resolve().parents[1]
DIST=ROOT/'dist'
DIST.mkdir(exist_ok=True)
SKETCH='firmware/smart_parking_barrier/smart_parking_barrier.ino'
EXCLUDED={'.git','.tools','build','dist','__pycache__'}

def read(path):
    return (ROOT/path).read_text(encoding='utf8')

def source_files():
    for directory,dirs,files in os.walk(ROOT):
        dirs[:]=[d for d in dirs if d not in EXCLUDED]
        for name in files:
            yield Path(directory)/name

def doc(path):
    def link(m):
        target=m[2]
        if target.startswith(('https:','http:','#','mailto:')):return m[0]
        file,sep,anchor=target.partition('#')
        rel=Path(os.path.relpath((ROOT/path).parent/file,DIST)).as_posix()
        return f'[{m[1]}]({rel}{sep}{anchor})'
    return re.sub(r'\[([^\]\n]+)\]\(([^)\n]+)\)',link,read(path))

readme=read('README.md')
diagram='```mermaid'+readme.split('```mermaid',1)[1].split('```',1)[0]+'```'
listing='```text\nsmart-parking-barrier/\n'+'\n'.join('  '+p.relative_to(ROOT).as_posix() for p in sorted(source_files()))+'\n```'
sections=[
 ('FINAL VERDICT','C — advanced student implementation. Physical validation and evidence remain pending; D is not justified yet.'),
 ('CRITICAL ISSUES FOUND',doc('docs/review.md')),
 ('CHANGES MADE','The five reproduced defects are corrected, with additional stale-data, motion and timing-boundary protection. Periodic diagnostic blanking and latched waiting faults were removed. Documentation is consolidated; all physical results remain NOT RUN.'),
 ('FINAL ARCHITECTURE',doc('docs/scenario-traces.md')),
 ('FINAL HARDWARE LIST',doc('hardware/bom.md')),
 ('FINAL WIRING',doc('docs/wiring.md')),
 ('FINAL FSM',doc('docs/architecture.md')+'\n\n'+diagram),
 ('FINAL FIRMWARE STATUS','Complete maintained Uno sketch:\n\n```cpp\n'+read(SKETCH)+'```'),
 ('SOFTWARE TEST RESULTS',doc('docs/validation.md')),
 ('PHYSICAL TESTS STILL REQUIRED',doc('docs/testing.md')),
 ('FINAL TABLETOP GEOMETRY',doc('docs/geometry.md')),
 ('FINAL BUILD ORDER',doc('docs/construction.md')),
 ('FINAL README STATUS',doc('README.md')+'\n\n'+listing),
 ('FINAL DEMO PLAN',doc('docs/demo.md')+'\n\n'+doc('docs/evidence.md')),
 ('FINAL INTERVIEW DEFENCE',doc('docs/interview.md')),
 ('FINAL SUBMISSION TEXT',doc('docs/submission.md')),
 ('EXACT PRE-SUBMISSION CHECKLIST',doc('docs/checklist.md')),
]
text='# Smart Parking Barrier — Final Engineering Review\n\n'
text+='Verdict: C, advanced student design/software; physical evidence pending. The repository remains the source of truth; this handout is generated outside version control.\n\n'
for n,(title,body) in enumerate(sections,1):
    body=re.sub(r'^(#{1,3}) ',lambda m:'#'*(len(m[1])+2)+' ',body,flags=re.M)
    text+=f'## {n}. {title}\n\n{body.strip()}\n\n'
handout=DIST/'final-engineering-review.md'
handout.write_text(text.rstrip()+'\n',encoding='utf8',newline='\n')
archive=DIST/'smart-parking-barrier-source.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for path in sorted(source_files()):
        z.write(path,Path('smart-parking-barrier')/path.relative_to(ROOT))
print(f'Review handout: {handout}\nSource archive: {archive}')
