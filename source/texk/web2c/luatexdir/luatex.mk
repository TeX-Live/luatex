# Makefile fragment for pdfeTeX and web2c. --infovore@xs4all.nl. Public domain.
# This fragment contains the parts of the makefile that are most likely to
# differ between releases of luatex
# $Id: luatex.mk 2071 2009-03-20 23:24:45Z hhenkel $

# We build luatex
luatex = @LTEX@ luatex
luatexdir = luatexdir

# libz
ZLIBDIR=../../libs/zlib
ZLIBSRCDIR=$(srcdir)/$(ZLIBDIR)
# libpng
LIBPNGDIR=../../libs/libpng
LIBPNGSRCDIR=$(srcdir)/$(LIBPNGDIR)
# libxpdf
LIBXPDFDIR=../../libs/xpdf
LIBXPDFSRCDIR=$(srcdir)/$(LIBXPDFDIR)
# libobsdcompa
LIBOBSDCOMPATDIR=../../libs/obsdcompat
LIBOBSDCOMPATFSRCDIR=$(srcdir)/$(LIBOBSDCOMPATDIR)

SVN_REV := $(shell $(srcdir)/$(luatexdir)/get_svnversion.sh $(srcdir))

XCPPFLAGS=-I$(LIBOBSDCOMPATDIR) -I$(LIBOBSDCOMPATDIR)/.. -I$(LIBOBSDCOMPATFSRCDIR) -I$(LIBOBSDCOMPATFSRCDIR)/..  -I$(ZLIBSRCDIR) -I$(LIBPNGSRCDIR) -I$(LIBXPDFSRCDIR) -Dextra_version_info=`date +-%Y%m%d%H` -DSVN_REV=\"$(SVN_REV)\"

Makefile: $(srcdir)/$(luatexdir)/luatex.mk

luatex_bin = luatex
linux_build_dir = $(HOME)/luatex/build/linux/texk/web2c


# Extract luatex version
$(luatexdir)/luatex.version: $(srcdir)/$(luatexdir)/luatex.web
	test -d $(luatexdir) || mkdir $(luatexdir)
	grep '^@d luatex_version_string==' $(srcdir)/$(luatexdir)/luatex.web \
	  | sed "s/^.*=='//;s/' .*$$//" \
	  >$(luatexdir)/luatex.version

# The C sources.
luatex_c = luatexini.c luatex0.c
luatex_o = luatexini.o luatex0.o luatexextra.o luatex-pool.o $(luatex_o-with_synctex)

# Making luatex
luatex: luatexd.h $(luatex_o) $(luatexextra_o) $(luatexlibsdep)
	@CXXHACKLINK@ $(luatex_o) $(luatexextra_o) $(luatexlibs) $(socketlibs) @SOCKETHACK@ @CXXHACKLDLIBS@ @CXXLDEXTRA@

# C file dependencies.
$(luatex_c) luatexcoerce.h luatexd.h: luatex.p $(web2c_texmf) $(srcdir)/$(luatexdir)/luatex.defines $(srcdir)/$(luatexdir)/luatex.h
	$(web2c) luatex
#    $(luatexd.h-with_synctex)
luatexextra.c: luatexd.h $(luatexdir)/luatexextra.h lib/texmfmp.c
	test -d $(luatexdir) || mkdir $(luatexdir)
	sed s/TEX-OR-MF-OR-MP/luatex/ $(srcdir)/lib/texmfmp.c >$@
$(luatexdir)/luatexextra.h: $(luatexdir)/luatexextra.in $(luatexdir)/luatex.version
	test -d $(luatexdir) || mkdir $(luatexdir)
	sed -e s/LUATEX-VERSION/`cat $(luatexdir)/luatex.version`/ \
	  $(srcdir)/$(luatexdir)/luatexextra.in >$@
luatex-pool.c: luatex.pool
	$(makecpool) luatex.pool luatexdir/ptexlib.h >$@ || rm -f $@

# luatangle we need a private version of tangle

luatangle: luatangle.o
	$(kpathsea_link) luatangle.o $(LOADLIBES)

luatangle.c luatangle.h: luatangle.p
	$(web2c) luatangle

luatangle.p: tangle $(srcdir)/$(luatexdir)/luatangle.web $(srcdir)/$(luatexdir)/luatangle.ch
	$(TANGLE) $(srcdir)/$(luatexdir)/luatangle.web $(srcdir)/$(luatexdir)/luatangle.ch

# Tangling
luatex.p luatex.pool: luatangle $(srcdir)/$(luatexdir)/luatex.web $(srcdir)/$(luatexdir)/luatex.ch
	$(native)/luatangle --underlines $(srcdir)/$(luatexdir)/luatex.web $(srcdir)/$(luatexdir)/luatex.ch

#   Sources for luatex.ch:
#luatex_ch_srcs = $(srcdir)/$(luatexdir)/luatex.web \
#  $(srcdir)/$(luatexdir)/lua.ch \
#  $(srcdir)/$(luatexdir)/lua.ch0

#   Rules:
#luatex.ch: $(TIE) $(luatex_ch_srcs)
#	$(TIE) -c luatex.ch $(luatex_ch_srcs)

# for developing only
#luatex-org.web: $(TIE) $(luatex_ch_srcs_org)
#	$(TIE) -m $@ $(luatex_ch_srcs_org)
luatex-all.web: $(TIE) $(srcdir)/$(luatexdir)/luatex.web $(srcdir)/$(luatexdir)/luatex.ch
	$(TIE) -m $@ $(srcdir)/$(luatexdir)/luatex.web $(srcdir)/$(luatexdir)/luatex.ch
#luatex-all.tex: luatex-all.web
#	$(WEAVE) luatex-all.web
#	echo -e '1s/ webmac/ pdfwebmac/\nw\nq' | ed $@ >/dev/null 2>&1
#luatex-all.pdf: luatex-all.tex
#	$(luatex) luatex-all.tex

check: @LTEX@ luatex-check
luatex-check: luatex luatex.fmt

clean:: luatex-clean
luatex-clean:
	$(LIBTOOL) --mode=clean $(RM) luatex
	rm -f $(luatex_o) $(luatex_c) luatexextra.c luatexcoerce.h
	rm -f $(luatexdir)/luatexextra.h
	rm -f luatexd.h luatex.p luatex.pool luatex.ch strpool.c
	rm -f luatex.fmt luatex.log

# Dumps
all_luafmts = @FMU@ luatex.fmt $(luafmts)

dumps: @LTEX@ luafmts
luafmts: $(all_luafmts)

luafmtdir = $(web2cdir)/luatex
$(luafmtdir)::
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(luafmtdir)

luatex.fmt: luatex
	$(dumpenv) $(MAKE) progname=luatex files="etex.src plain.tex cmr10.tfm" prereq-check
	$(dumpenv) ./luatex --progname=luatex --jobname=luatex --ini \*\\pdfoutput=1\\input etex.src \\dump </dev/null

# 
# Installation.
install-luatex: install-luatex-exec

install-programs: @LTEX@ install-luatex-exec
install-luatex-exec: $(luatex) $(bindir)
	for p in luatex; do $(INSTALL_LIBTOOL_PROG) $$p $(bindir); done

# 
# luatex binaries archive
luatexbin:
	$(MAKE) $(luatex_bin)

luatex-cross:
	$(MAKE) luatex-web2c-cross
	$(MAKE) luatexbin

luatex-web2c-cross: $(web2c_programs)
	@if test ! -x $(linux_build_dir)/luatangle; then echo Error: linux_build_dir not ready; exit -1; fi
	rm -f web2c/fixwrites web2c/splitup web2c/web2c
	cp -f $(linux_build_dir)/web2c/fixwrites web2c
	cp -f $(linux_build_dir)/web2c/splitup web2c
	cp -f $(linux_build_dir)/web2c/web2c web2c
	touch web2c/fixwrites web2c/splitup web2c/web2c
	$(MAKE) tangleboot && rm -f tangleboot && \
	cp -f $(linux_build_dir)/tangleboot .  && touch tangleboot
	$(MAKE) ctangleboot && rm -f ctangleboot && \
	cp -f $(linux_build_dir)/ctangleboot .  && touch ctangleboot
	$(MAKE) ctangle && rm -f ctangle && \
	cp -f $(linux_build_dir)/ctangle .  && touch ctangle
	$(MAKE) tie && rm -f tie && \
	cp -f $(linux_build_dir)/tie .  && touch tie
	$(MAKE) luatangle && rm -f luatangle && \
	cp -f $(linux_build_dir)/luatangle .  && touch luatangle

# vim: set noexpandtab
# end of luatex.mk
