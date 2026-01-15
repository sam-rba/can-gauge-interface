TEX = pdflatex

SRC = report.tex ../references.bib \
	  $(wildcard *.png) $(wildcard *.jpg) pcb_pours*.pdf schematic*.pdf

report.pdf: ${SRC}
	${TEX} report
	biber report
	${TEX} report
	${TEX} report

clean:
	rm -f report.pdf *.aux *.bbl *.bcf *.blg *.log *.out *.run.xml *.toc
