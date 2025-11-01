datafmt.pdf: datafmt.ms refs.refer
	dformat $< | refer -P -p refs.refer | pic | tbl | groff -ms -Tpdf >$@
