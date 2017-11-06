ACLOCAL_AMFLAGS = -I m4 -I $(VARNISHAPI_DATAROOTDIR)/aclocal

DISTCHECK_CONFIGURE_FLAGS = RST2MAN=:

doc_DATA = README.rst LICENSE

SUBDIRS = src

EXTRA_DIST = libvmod-esiextra.spec LICENSE README.rst

README.rst: src/vmod_esiextra.man.rst
	cp src/vmod_esiextra.man.rst README.rst

src/vmod_esiextra.man.rst:
	make -C src vmod_esiextra.man.rst
