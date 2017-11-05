AM_CFLAGS = $(VARNISHAPI_CFLAGS)

# Modules

vmod_LTLIBRARIES = \
	libvmod_esiextra.la

libvmod_esiextra_la_LDFLAGS = $(VMOD_LDFLAGS)
libvmod_esiextra_la_SOURCES = vmod_esiextra.c
nodist_libvmod_esiextra_la_SOURCES = \
	vcc_esiextra_if.c \
	vcc_esiextra_if.h \
	vfp_bodyhash.c \
	vfp_bodyhash.h

@BUILD_VMOD_ESIEXTRA@

# Test suite

AM_TESTS_ENVIRONMENT = \
	PATH="$(abs_builddir):$(VARNISH_TEST_PATH):$(PATH)" \
	LD_LIBRARY_PATH="$(VARNISH_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = varnishtest -v
AM_VTC_LOG_FLAGS = \
	-p vcl_path="$(abs_top_srcdir)/vcl" \
	-p vmod_path="$(abs_builddir)/.libs:$(vmoddir)"

TESTS = \
	vtc/vmod_esiextra.vtc

# Documentation

dist_doc_DATA = \
	vmod_esiextra.vcc \
	$(TESTS)

dist_man_MANS = \
	vmod_esiextra.3


.rst.1:
	$(AM_V_GEN) $(RST2MAN) $< $@
