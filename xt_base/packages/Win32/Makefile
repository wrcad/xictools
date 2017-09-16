
#VERSION = "2.24.10-20120208"
#VERSION = "2.24.25-20150430"
VERSION = "2.24.25-20160628"

dummy::
	@echo targets: Windows

Win32 pkg::
	files/setup.sh $(VERSION)

clean::
	-@rm -f *.iss *.exe
	-@rm -rf gtk2-bundle pkgs

lib:
	rm -rf gtk2-bundle pkgs
	files/getpkgs
	files/mkpkgs
	cp files/README windows_32
	cp files/gtkrc windows_32/etc/gtk-2.0
	mv windows_32 gtk2-bundle

# For package 20120208 from gtk.org:
#	@mkdir gtk2-bundle
#	cd gtk2-bundle; unzip ../gtk+-bundle*.zip; cd ..
