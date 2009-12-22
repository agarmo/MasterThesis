PDF = pdflatex
BIBTEX = bibtex

FLAGS = 


projectplan : 
	cd projectplan
	make projectplan
	mv index.pdf ../projectplan.pdf
	cd ..


clean_prjectplan :
	cd projectplan
	make clean
	cd ..




