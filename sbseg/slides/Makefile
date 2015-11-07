all: slides.pdf

slides.pdf: slides.md
	pandoc -t beamer -V theme:Singapore --latex-engine=xelatex slides.md -o slides.pdf

clean:
	rm slides.pdf
