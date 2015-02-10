# $Id$
#

all: SMART-userman.pdf SMART-userman-private.pdf

TEXSRC= SMARTman.tex overview.tex language.tex petri-nets.tex stategen.tex


include figures.makefile
include examples.makefile

figures.makefile: $(TEXSRC)
	cat $(TEXSRC) | awk -F'[{}]' 'BEGIN{print "FIGS = "};/includegraphics/{print $$2}' | tr '\n' ' ' | xargs > figures.makefile

examples.makefile: $(TEXSRC)
	cat $(TEXSRC) | awk -F'[{}]' 'BEGIN{print "EXSM = "};/lstinputlisting/{print $$2}' | tr '\n' ' ' | xargs > examples.makefile

SMART-userman.pdf: $(TEXSRC) $(FIGS) $(EXSM)
	echo "EXTERNAL" | pdflatex SMARTman 
	# bibtex SMARTman 
	# makeindex SMARTman
	# echo "EXTERNAL" | pdflatex SMARTman
	# echo "EXTERNAL" | pdflatex SMARTman
	mv SMARTman.pdf SMART-userman.pdf
	

SMART-userman-private.pdf: $(TEXSRC) $(FIGS) $(EXSM)
	echo "INTERNAL" | pdflatex SMARTman 
	# bibtex SMARTman 
	# makeindex SMARTman
	# echo "EXTERNAL" | pdflatex SMARTman
	# echo "EXTERNAL" | pdflatex SMARTman
	mv SMARTman.pdf SMART-userman-private.pdf

#
#  Generic rules
#
	
%.pdf: %.dot
	dot -Tpdf $< -o $@

%.eps: %.obj
	tgif -color -print -eps -ofigures $<

%.pdf: %.eps
	epstopdf $<

