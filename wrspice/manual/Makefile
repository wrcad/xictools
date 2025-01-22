# $Id: Makefile,v 1.9 2014/09/29 01:15:33 stevew Exp $
#
# To fix mathend000# bug: edit share/latex2html/versions/math.pl, set
# $mathend_mark = ""; 

# Instructions:
# You need latex, and latex2html if you want the HTML manual, dvips for
# PostScript, and dvipdf for PDF.
#
# make import   // this creates the tex files
#
# make manual
# make manual
# make index
# make manual   // run these as listed to create the dvi file.
#
# make html     // for HTML manual
# make ps       // for PostScript manual
# make pdf      // for PDF manual

import::
	@../../mozy/bin/hlp2latex -p ../help -d WRspice -d OpenAccess .

manual::
	@sed -e "s/@RELEASE@/`../release.sh wrspice`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < wrsmanual.sed > wrsmanual.tex
	latex wrsmanual.tex
	-@rm wrsmanual.tex

index::
	makeindex wrsmanual.idx

html::
	@sed -e "s/@RELEASE@/`../release.sh wrspice`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < wrsmanual.sed > wrsmanual.tex
	latex2html -image_type png -local_icons -no_math -html_version \
  3.2,math wrsmanual
	@files=`grep -l "This page intentionally" wrsmanual/*.html`; \
	for a in $$files; do \
	    mv  $$a zzztmp; \
	    sed -e "s/0*This page intentionally left blank.//" < zzztmp > $$a; \
	    unlink zzztmp; \
	done
	@tar czf wrsmanual-html-`../release.sh wrspice`.tar.gz wrsmanual
	-@rm wrsmanual.tex

ps::
	dvips -o wrsmanual-`../release.sh wrspice`.ps wrsmanual.dvi
	-@rm -f wrsmanual-`../release.sh wrspice`.ps.gz
	@gzip wrsmanual-`../release.sh wrspice`.ps

pdf::
	dvipdf wrsmanual.dvi wrsmanual-`../release.sh wrspice`.pdf

clean distclean::
	-@rm -rf *.log *.aux *.ilg *.ind *.dvi wrsmanual *.gz *.pdf *.tex

