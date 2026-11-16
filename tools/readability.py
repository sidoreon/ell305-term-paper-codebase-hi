"""readability.py : compute FKGL, Gunning Fog, SMOG, Coleman-Liau and ARI on
the body text of the paper.  Usage: python3 readability.py paper.tex
Strips LaTeX markup, listings, tables, figures, maths and captions from the
main matter (from the Introduction chapter to \\appendix), then applies the
standard formulae with a rule-based syllable counter."""
import re, sys, math, json

def strip_latex(tex):
    # the body is the last Introduction chapter (earlier matches are inside embedded sources)
    start = tex.rfind('\\chapter{' + 'Introduction}'); end = tex.find('\\appendix', start)
    body = tex[start:end] if start >= 0 and end > start else tex
    for env in ('lstlisting', 'table', 'figure', 'tikzpicture', 'ganttchart', 'equation', 'align\\*', 'align', 'tabular', 'prediction', 'takeaway', 'subfigure'):
        body = re.sub(r'\\begin\{' + env + r'\}.*?\\end\{' + env + r'\}', ' ', body, flags=re.S)
    body = re.sub(r'\\\[.*?\\\]', ' ', body, flags=re.S)
    body = re.sub(r'\$[^$]*\$', ' X ', body)
    body = re.sub(r'%.*', ' ', body)
    body = re.sub(r'\\(chapter|section|subsection|caption|label|index|cite[pt]?|ref|src|code|texttt|textbf|textit|emph|gls)\*?\{[^{}]*\}', ' ', body)
    body = re.sub(r'\\(item|nosep|meas|simu|cited|clearpage|centering|small|scriptsize)\b', ' ', body)
    body = re.sub(r'\\[A-Za-z]+\*?(\[[^\]]*\])?', ' ', body)
    body = re.sub(r'[{}\[\]]', ' ', body)
    body = re.sub(r'\s+', ' ', body)
    return body

def syllables(word):
    w = word.lower()
    w = re.sub(r'[^a-z]', '', w)
    if not w: return 0
    if len(w) <= 3: return 1
    w = re.sub(r'(?:[^laeiouy]es|ed|[^laeiouy]e)$', '', w)
    w = re.sub(r'^y', '', w)
    groups = re.findall(r'[aeiouy]+', w)
    return max(1, len(groups))

def indices(text):
    sentences = [s for s in re.split(r'(?<=[.!?])\s+', text) if len(s.split()) > 1]
    words = [w for w in re.findall(r"[A-Za-z][A-Za-z'-]*", text)]
    n_s, n_w = max(1, len(sentences)), max(1, len(words))
    syl = [syllables(w) for w in words]
    n_syl = sum(syl); poly = sum(1 for s in syl if s >= 3)
    letters = sum(len(re.sub(r'[^A-Za-z]', '', w)) for w in words)
    chars = sum(len(w) for w in words)
    fkgl = 0.39 * n_w / n_s + 11.8 * n_syl / n_w - 15.59
    fog = 0.4 * (n_w / n_s + 100.0 * poly / n_w)
    smog = 1.043 * math.sqrt(poly * 30.0 / n_s) + 3.1291
    cli = 0.0588 * (100.0 * letters / n_w) - 0.296 * (100.0 * n_s / n_w) - 15.8
    ari = 4.71 * chars / n_w + 0.5 * n_w / n_s - 21.43
    return dict(sentences=n_s, words=n_w, syllables=n_syl, polysyllables=poly, letters=letters,
                words_per_sentence=n_w / n_s, syllables_per_word=n_syl / n_w,
                FKGL=fkgl, Fog=fog, SMOG=smog, CLI=cli, ARI=ari)

if __name__ == '__main__':
    tex = open(sys.argv[1], encoding='utf-8').read()
    r = indices(strip_latex(tex))
    print(json.dumps(r, indent=1))
