#!/bin/sh

# Updates the Doxygen documentation and man pages for the playd GitHub Pages
# distribution.
#
# Requires commit rights on the upstream `gh-pages` branch.

BRANCH=$(git symbolic-ref HEAD | cut -d'/' -f3)

git checkout gh-pages && \
	git rm -rf doxygen && \
	mv doc/html doxygen && \
	mv ${MAN_HTML} man.html && \
	git add doxygen man.html && \
	git commit -m "[Auto] Update autodocs on gh-pages." && \
	git push

git checkout ${BRANCH}
