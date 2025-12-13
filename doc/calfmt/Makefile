calfmt.pdf: calfmt.ms sources.refer
	dformat $< | refer -P -p sources.refer | pic | tbl | groff -ms -Tpdf >$@
