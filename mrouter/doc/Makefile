# $Id: Makefile,v 1.3 2016/11/15 04:25:41 stevew Exp $
#
# To fix mathend000# bug: edit share/latex2html/versions/math.pl, set
# $mathend_mark = ""; 

MANUAL = mrmanual

manual::
	sed -e "s/@RELEASE@/`../version`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < $(MANUAL).sed > $(MANUAL).tex
	latex $(MANUAL).tex
	rm -f $(MANUAL).tex

foo::
	sed -e "s/@RELEASE@/`../version`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < $(MANUAL).sed > $(MANUAL).tex

index::
	makeindex $(MANUAL).idx

html::
	sed -e "s/@RELEASE@/`../version`/" \
  -e "s/@DATE@/`date +'%B %-d, %Y'`/" < $(MANUAL).sed > $(MANUAL).tex
	latex2html -image_type png -local_icons -no_math -html_version \
 3.2,math $(MANUAL)
	files=`grep -l "This page intentionally" $(MANUAL)/*.html`; \
	for a in $$files; do \
	    mv  $$a zzztmp; \
	    sed -e "s/0*This page intentionally left blank.//" < zzztmp > $$a; \
	    unlink zzztmp; \
	done
	tar czf $(MANUAL)-html-`../version`.tar.gz $(MANUAL)
	rm -f $(MANUAL).tex

ps::
	dvips -o $(MANUAL)-`../version`.ps $(MANUAL).dvi
	gzip $(MANUAL)-`../version`.ps

pdf::
	dvipdf $(MANUAL).dvi $(MANUAL)-`../version`.pdf

clean::
	-@rm -f *.aux *.log *.toc *.ilg *.ind *.idx

realclean::
	-@$(MAKE) clean
	-@rm -rf $(MANUAL) $(MANUAL).dvi $(MANUAL)-*.pdf $(MANUAL)-*.ps.gz \
  $(MANUAL)-*.tar.gz

