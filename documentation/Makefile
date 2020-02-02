
# stuff for LaTeX 2e itself
#
LATEX = /usr/local/bin/latex

# PDF macros
#
PDFLATEX = /usr/local/bin/pdflatex

# PostScript macros
#
DVIPS = /usr/local/bin/dvips

# TEXT macros
#
W3M = /usr/local/bin/w3m

# HTML macros
#
LATEX2HTML = /usr/local/bin/latex2html
# directories to contain HTML output
HTMLSPLITDIR = html-split
HTMLSINGLEDIR = html-single
# arguments for latex2html to generate a split document
HTMLSPLIT = -dir ${HTMLSPLITDIR} -mkdir -local_icons -white -noaddress -noinfo
# arguments for latex2html to generate a single document
HTMLSINGLE = -dir ${HTMLSINGLEDIR} -mkdir -white -noaddress -noinfo -split 0

# Which make to use?  make or gmake?
MAKE = make

all: txt html ps pdf dvi

txt: ecartis.txt

html: ecartis.html

ps: ecartis.ps

pdf: ecartis.pdf

dvi: ecartis.dvi

ecartis.pdf:	ecartis.dvi
	${MAKE} clean_pdf
	# must run twice to get TOC
	${PDFLATEX} ecartis
	${PDFLATEX} ecartis

ecartis.ps:	ecartis.dvi
	${DVIPS} -o ecartis.ps ecartis
	
ecartis.txt:	${HTMLSINGLEDIR}/ecartis.html
	${W3M} -dump ${HTMLSINGLEDIR}/ecartis.html > ecartis.txt

ecartis.html:	${HTMLSINGLEDIR}/ecartis.html

${HTMLSINGLEDIR}/ecartis.html:	ecartis.dvi
	${MAKE} distclean_html
	${LATEX2HTML} ${HTMLSPLIT} ecartis
	${LATEX2HTML} ${HTMLSINGLE} ecartis

ecartis:	ecartis.dvi

ecartis.dvi: 	ecartis.sty ecartis.tex
	# must run twice to get TOC
	${LATEX} ecartis
	${LATEX} ecartis

distclean:	clean distclean_dvi distclean_pdf distclean_ps distclean_txt \
			distclean_html

clean:	clean_dvi clean_pdf

distclean_dvi:	clean_dvi
	rm -Rf ecartis.dvi

clean_dvi:	
	rm -Rf ecartis.aux ecartis.log ecartis.toc 

distclean_pdf:
	rm -Rf ecartis.pdf

clean_pdf:
	rm -Rf ecartis.aux ecartis.log ecartis.out ecartis.toc 

distclean_ps:
	rm -Rf ecartis.ps

distclean_txt:
	rm -Rf ecartis.txt

distclean_html:
	rm -Rf ${HTMLSPLITDIR} ${HTMLSINGLEDIR}
