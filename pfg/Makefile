all: relatorio.pdf

relatorio.pdf: relatorio.md img/*.png chicago-author-date.csl
	pandoc -N --filter pandoc-citeproc --csl chicago-author-date.csl -V geometry:margin=1in relatorio.md -o relatorio.pdf

img/linux.png: scripts/make_results.py
	scripts/make_results.py

clean:
	rm -f relatorio.pdf
