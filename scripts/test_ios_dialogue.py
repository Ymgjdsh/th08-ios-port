"""Verify the production lookup against private, extracted retail msg*.dat.

Run on a Mac with clang++ after a simulator launch with
TH08_IOS_DUMP_MESSAGES=1. The input directory is the app container's
Library/Application Support/th08/dialogue-audit. No game data is committed.
"""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

from generate_ios_dialogue_i18n import cpp_bytes


def retail_lines(path):
    data = path.read_bytes()
    count, = struct.unpack_from('<I', data)
    assert 0 < count < 128, path
    offsets = struct.unpack_from('<' + 'I' * count, data, 4)
    for script, start in enumerate(offsets):
        assert 4 + count * 4 <= start < len(data), (path, script, start)
        pos = start
        line = 0
        pending = True
        while pos + 4 <= len(data):
            time, opcode, size = struct.unpack_from('<HBB', data, pos)
            assert pos + 4 + size <= len(data), (path, pos)
            args = data[pos + 4:pos + 4 + size]
            if opcode == 0:
                break
            if opcode in (0xF, 0x11, 4, 0x17):
                pending = True
            if opcode in (3, 0x10, 0x13, 0x14):
                if opcode == 3:
                    _, line = struct.unpack_from('<hh', args)
                    args = args[4:]
                elif opcode == 0x13:
                    line = 0
                elif opcode == 0x14:
                    line = 1
                elif pending:
                    line = 0
                text = bytes(b ^ 0x77 for b in args).split(b'\0', 1)[0]
                if text.strip():
                    yield path.name, script, time, max(0, min(line, 1)), text
                if opcode == 0x10:
                    line += 1
                    pending = False
            pos += size + 4


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--messages', type=Path, required=True)
    parser.add_argument('--cxx', default='clang++')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    tables = root / 'resources/localization/th08'
    files = sorted(args.messages.glob('msg*.dat'))
    assert len(files) == 32, 'Expected all 32 retail message files'
    cases = [case for path in files for case in retail_lines(path)]
    known = {}
    for path in sorted((tables / 'ja').glob('*.jdiff')):
        data = json.loads(path.read_text(encoding='utf-8'))
        for script, entries in data.items():
            for key, entry in entries.items():
                if '_h' not in key:
                    for text in entry['lines']:
                        known.setdefault((path.name[:-6], int(script)), set()).add(text.encode('cp932'))
    headers = {}
    aliases = {}
    for path in files:
        data = path.read_bytes()
        count, = struct.unpack_from('<I', data)
        offsets = struct.unpack_from('<' + 'I' * count, data, 4)
        headers[path.name] = data[:4 + count * 4]
        aliases[path.name] = [offsets.index(offset) for offset in offsets]
    absent = [case for case in cases if case[4] not in known.get(
        (case[0], aliases[case[0]][case[1]]), set())]
    for file, script, time, line, text in absent:
        print('ABSENT', file, script, time, line, text.decode('cp932'))

    with tempfile.TemporaryDirectory(prefix='th08-dialogue-test-') as directory:
        temp = Path(directory)
        subprocess.run(['python3', str(root / 'scripts/generate_ios_dialogue_i18n.py'),
                        '--root', str(tables), '--output', str(temp / 'ios_dialogue_i18n.hpp')], check=True)
        # Compile the real production implementation. Only its platform
        # language provider is stubbed; lookup and data remain unchanged.
        for name in ('ios_localization.cpp', 'ios_localization.hpp'):
            shutil.copyfile(root / 'src/modern/ios' / name, temp / name)
        (temp / 'ios_touch.hpp').write_text('''#pragma once
namespace th08 { namespace modern { namespace ios {
enum IosLanguage { IOS_LANGUAGE_JP, IOS_LANGUAGE_ZH, IOS_LANGUAGE_EN };
IosLanguage GetLanguage();
}}}
''')
        rows = []
        for file, script, time, line, text in cases:
            header = headers[file]
            literal = '"' + ''.join('\\x%02x' % b for b in header) + '"'
            rows.append('{%s,%d,%d,%d,%s,%s,%d}' % (cpp_bytes(file, 'ascii'), script, time, line,
                                             cpp_bytes(text.decode('cp932'), 'cp932'), literal, len(header)))
        harness = '''#include "ios_localization.hpp"
#include <stdio.h>
#include <string.h>
using namespace th08::modern::ios;
static IosLanguage language = IOS_LANGUAGE_ZH;
namespace th08 { namespace modern { namespace ios {
IosLanguage GetLanguage() { return language; }
}}}
struct Case { const char *file; int script, time, line; const char *text; const char *header; size_t size; };
static const Case cases[] = { CASES };
int main() {
 int failed = 0;
 for (int lang = 1; lang <= 2; ++lang) {
  language = static_cast<IosLanguage>(lang);
  for (unsigned i=0; i<sizeof(cases)/sizeof(cases[0]); ++i) {
   const Case &c=cases[i]; LocalizedDialogueText result;
   SetDialogueMessageFile(c.file,c.header,c.size);
   if (!FindLocalizedDialogue(c.script,c.time,c.text,c.line,&result) || !result.lines[0][0] ||
       result.originalLine != c.line) {
    printf("MISS lang=%d %s script=%d time=%d line=%d text=",lang,c.file,c.script,c.time,c.line);
    for (const unsigned char *p=(const unsigned char*)c.text; *p; ++p) printf("%02x",*p);
    puts(""); ++failed;
   }
  }
 }
 LocalizedDialogueText result;
 language=IOS_LANGUAGE_ZH;
 SetDialogueMessageFile("msg1a.dat");
 if (!FindLocalizedDialogue(0,124,WRIGGLE_JA,1,&result) ||
     strcmp(result.lines[1],WRIGGLE_ZH)!=0 || result.originalLine!=1 || result.originalLineCount!=2) ++failed;
 SetDialogueMessageFile("msg1a.dat");
 if (FindLocalizedDialogue(0,63,"not a real dialogue",0,&result)) ++failed;
 ResetLocalizationRuntime();
 if (FindLocalizedDialogue(0,60,cases[0].text,0,&result)) ++failed;
 language=IOS_LANGUAGE_JP;
 SetDialogueMessageFile(cases[0].file);
 if (FindLocalizedDialogue(cases[0].script,cases[0].time,cases[0].text,0,&result)) ++failed;
 printf("Retail lines=%u, languages=2, failures=%d\\n",(unsigned)(sizeof(cases)/sizeof(cases[0])),failed);
 return failed ? 1 : 0;
}
'''.replace('CASES', ',\n'.join(rows)).replace(
            'WRIGGLE_JA', cpp_bytes('私が居たでしょ！', 'cp932')).replace(
            'WRIGGLE_ZH', cpp_bytes('我可是在的啊！', 'utf-8'))
        (temp / 'test.cpp').write_text(harness, encoding='utf-8')
        subprocess.run([args.cxx, '-std=c++98', '-O2', str(temp / 'ios_localization.cpp'),
                        str(temp / 'test.cpp'), '-o', str(temp / 'test')], check=True)
        subprocess.run([str(temp / 'test')], check=True)


if __name__ == '__main__':
    main()
