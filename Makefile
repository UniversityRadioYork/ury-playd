# Replace with your favourite Markdown to HTML compiler.
MARKDOWN ?= hoedown

.POSIX:

all: index.html

index.html: index.header.html index.md index.footer.html
	$(MARKDOWN) index.md | cat index.header.html - index.footer.html > index.html

clean:
	rm index.html
