PDF = pdflatex

BIB = bibtex


all: thesis.aux
	$(PDF) thesis.tex


bib: thesis.aux
	$(BIB) thesis.aux

thesis.aux:
	$(PDF) thesis.tex

clean:
	rm -f *.aux *.toc *.log *.pdf *.glo *.nlo *.out *.blg *.brf *.lot *.lof


