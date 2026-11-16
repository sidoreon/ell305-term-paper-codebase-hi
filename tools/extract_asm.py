"""extract_asm.py : pull one function out of a compiler .s file and strip the
assembler directives, so that the listing in the paper shows only the
instructions.  Usage: python3 extract_asm.py file.s symbol"""
import re, sys

def extract(path, sym):
    out = []; inside = False
    for line in open(path):
        s = line.rstrip('\n')
        if re.match(rf'^_?{re.escape(sym)}:', s):
            inside = True; out.append(sym + ':'); continue
        if inside:
            if re.match(r'^\s*\.cfi_endproc', s) or re.match(r'^\s*\.size', s) or re.match(r'^LFE', s) or re.match(r'^\.Lfunc_end', s):
                break
            if re.match(r'^\s*\.', s) or re.match(r'^\s*#', s) or re.match(r'^LFB|^LCFI', s) or s.strip() == '':
                continue
            s = re.sub(r'\s*//.*$', '', s)
            s = re.sub(r'\s*#\s*[@%].*$', '', s)
            out.append(s.replace('\t', ' ', 1).replace('\t', ' '))
    return '\n'.join(out)

if __name__ == '__main__':
    print(extract(sys.argv[1], sys.argv[2]))
