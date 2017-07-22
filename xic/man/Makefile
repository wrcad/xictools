# $Id: Makefile,v 1.9 2014/09/29 01:15:36 stevew Exp $
#
# To fix mathend000# bug: edit share/latex2html/versions/math.pl, set
# $mathend_mark = ""; 

manual::
	sed -e "s/@RELEASE@/`../version xic`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < xicmanual.sed > xicmanual.tex
	latex xicmanual.tex
	rm -f xicmanual.tex

foo::
	sed -e "s/@RELEASE@/`../version xic`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < xicmanual.sed > xicmanual.tex

index::
	makeindex xicmanual.idx

html::
	sed -e "s/@RELEASE@/`../version xic`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < xicmanual.sed > xicmanual.tex
	latex2html -image_type png -local_icons -no_math -html_version \
 3.2,math xicmanual
	files=`grep -l "This page intentionally" xicmanual/*.html`; \
	for a in $$files; do \
	    mv  $$a zzztmp; \
	    sed -e "s/0*This page intentionally left blank.//" < zzztmp > $$a; \
	    unlink zzztmp; \
	done
	tar czf xicmanual-html-`../version xic`.tar.gz xicmanual
	rm -f xicmanual.tex

ps::
	dvips -o xicmanual-`../version xic`.ps xicmanual.dvi
	gzip xicmanual-`../version xic`.ps

pdf::
	dvipdf xicmanual.dvi xicmanual-`../version xic`.pdf

clean distclean::
	rm -rf *.log *.aux *.ilg *.ind *.dvi xicmanual *.gz *.pdf

