#

all: SMART-userman.pdf SMART-devman.pdf

bbl: SMART-userman.bbl SMART-devman.bbl

TEXSRC= defines.tex main.tex overview.tex language.tex petri-nets.tex \
        stategen.tex modelchecking.tex numerical-markov.tex examples.tex


include figures.makefile
include examples.makefile

figures.makefile: $(TEXSRC)
	cat $(TEXSRC) | awk -F'[{}]' 'BEGIN{print "FIGS = "};/includegraphics/{print $$2}' | tr '\n' ' ' | xargs > figures.makefile

examples.makefile: $(TEXSRC)
	cat $(TEXSRC) | awk -F'[{}]' 'BEGIN{print "EXSM = "};/lstinputlisting/{print $$2}' | tr '\n' ' ' | xargs > examples.makefile

SMART-userman.pdf: SMART-userman.tex SMART-userman.idx $(TEXSRC) $(FIGS) $(EXSM)
	makeindex SMART-userman
	pdflatex SMART-userman
	pdflatex SMART-userman
	
SMART-userman.bbl: ALL.bib
	pdflatex SMART-userman
	bibtex SMART-userman

SMART-devman.pdf: SMART-devman.tex SMART-devman.idx $(TEXSRC) $(FIGS) $(EXSM)
	makeindex SMART-devman
	pdflatex SMART-devman
	pdflatex SMART-devman
	
SMART-devman.bbl: ALL.bib
	pdflatex SMART-devman
	bibtex SMART-devman

#
#  Generic rules
#
	
%.pdf: %.dot
	dot -Tpdf $< -o $@

%.eps: %.obj
	tgif -color -print -eps -ofigures $<

%.pdf: %.eps
	epstopdf $<

%.idx: %.tex
	pdflatex $<
