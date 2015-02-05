# $Id$
#

all: SMART-userman.pdf SMART-userman-private.pdf

TEXSRC= SMARTman.tex overview.tex language.tex


include figures.makefile

figures.makefile: $(TEXSRC)
	cat $(TEXSRC) | awk -F'[{}]' 'BEGIN{print "FIGS = "};/includegraphics/{print $$2}' | tr '\n' ' ' | xargs > figures.makefile

SMART-userman.pdf: $(TEXSRC) $(FIGS)
	echo "EXTERNAL" | pdflatex SMARTman 
	# bibtex SMARTman 
	# makeindex SMARTman
	# echo "EXTERNAL" | pdflatex SMARTman
	# echo "EXTERNAL" | pdflatex SMARTman
	mv SMARTman.pdf SMART-userman.pdf
	

SMART-userman-private.pdf: $(TEXSRC) $(FIGS)
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
	tgif -color -print -eps -o. $<

%.pdf: %.eps
	epstopdf $<

