
AM_CFLAGS = @LIBPNG_CFLAGS@ @LIBWEBP_CFLAGS@ @LIBLZMA_CFLAGS@ \
	@LIBSPATIALITE_CFLAGS@ @LIBCAIRO_CFLAGS@ @LIBCURL_CFLAGS@ \
	@LIBXML2_CFLAGS@ @LIBXML2_CFLAGS@

AM_CPPFLAGS = @CFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/headers

lib_LTLIBRARIES = librasterlite2.la 

librasterlite2_la_SOURCES = rasterlite2.c rl2raw.c rl2codec.c \
	rl2jpeg.c rl2png.c rl2gif.c rl2webp.c rl2tiff.c rl2wms.c \
	rl2paint.c

librasterlite2_la_LDFLAGS = -version-info 1:0:0 -no-undefined

librasterlite2_la_LIBADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ \
	@LIBLZMA_LIBS@ @LIBSPATIALITE_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
