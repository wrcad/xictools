# $Id: Makefile,v 1.9 2014/09/29 01:15:36 stevew Exp $
#
# To fix mathend000# bug: edit share/latex2html/versions/math.pl, set
# $mathend_mark = ""; 

manual::
	@sed -e "s/@RELEASE@/`../release.sh xic`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < xicmanual.sed > xicmanual.tex
	latex xicmanual.tex
	-@rm -f xicmanual.tex

index::
	makeindex xicmanual.idx

html::
	@sed -e "s/@RELEASE@/`../release.sh xic`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < xicmanual.sed > xicmanual.tex
	latex2html -image_type png -local_icons -no_math -html_version \
 3.2,math xicmanual
	@files=`grep -l "This page intentionally" xicmanual/*.html`; \
	for a in $$files; do \
	    mv  $$a zzztmp; \
	    sed -e "s/0*This page intentionally left blank.//" < zzztmp > $$a; \
	    unlink zzztmp; \
	done
	@tar czf xicmanual-html-`../release.sh xic`.tar.gz xicmanual
	-@rm -f xicmanual.tex

ps::
	dvips -o xicmanual-`../release.sh xic`.ps xicmanual.dvi
	-@rm -f xicmanual-`../release.sh xic`.ps.gz
	gzip xicmanual-`../release.sh xic`.ps

pdf::
	dvipdf xicmanual.dvi xicmanual-`../release.sh xic`.pdf

clean distclean::
	-@rm -rf *.log *.aux *.ilg *.ind *.dvi xicmanual.tex xicmanual \
  *.gz *.pdf

