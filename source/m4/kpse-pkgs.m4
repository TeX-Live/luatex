# Private macros for the teTeX / TeX Live (TL) tree.
# Copyright (C) 2009 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# serial 0

## ------------------------------ ##
## Define lists of sub-packages.  ##
## ------------------------------ ##

# Hopefully these lists are defined here and nowhere else.
# Note: directories in these lists need not exist.

# KPSE_LIBS_PKGS()
# ----------------
# Define the list of library sub-packages, i.e., subdirs 'libs/*'.
# Each library must precede required other libraries (if any).
AC_DEFUN([KPSE_LIBS_PKGS],
[m4_define([kpse_libs_pkgs], [
teckit
graphite
xpdf
gd
freetype2
freetype
t1lib
obsdcompat
libpng
zlib
])]) # KPSE_LIBS_PKGS

# KPSE_ALL_SYSTEM_FLAGS()
# -----------------------
# Generate flags for all potential system libraries available,
# only used at top-level.
AC_DEFUN([KPSE_ALL_SYSTEM_FLAGS],
[AC_REQUIRE([KPSE_LIBS_PREPARE])[]dnl
AC_REQUIRE([KPSE_ZLIB_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_LIBPNG_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_T1LIB_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_FREETYPE_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_FREETYPE2_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_GD_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_GRAPHITE_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_TECKIT_SYSTEM_FLAGS])[]dnl
AC_REQUIRE([KPSE_KPATHSEA_SYSTEM_FLAGS])[]dnl
]) # KPSE_ALL_SYSTEM_FLAGS

# KPSE_UTILS_PKGS()
# -----------------
# Define the list of utility sub-packages, i.e., subdirs 'utils/*'.
AC_DEFUN([KPSE_UTILS_PKGS],
[m4_define([kpse_utils_pkgs], [
dialog
pdfopen
ps2eps
psutils
t1utils
tpic2pdftex
vlna
])]) # KPSE_UTILS_PKGS

# KPSE_TEXK_PKGS()
# ----------------
# Define the list of TeXk sub-packages, i.e., subdirs 'texk/*',
# excluding 'texk/kpathsea'.
AC_DEFUN([KPSE_TEXK_PKGS],
[m4_define([kpse_texk_pkgs], [
web2c
one
afm2pl
bibtex8
cjkutils
detex
devnag
dtl
dvi2tty
dvidvi
dviljk
dvipdfm
dvipdfmx
dvipng
dvipos
dvipsk
gsftopk
lacheck
lcdf-typetools
makeindexk
musixflx
ps2pkm
seetexk
tex4htk
ttf2pk
ttfdump
xdv2pdf
xdvik
xdvipdfmx
tetex
texlive
])]) # KPSE_TEXK_PKGS



## ------------------------------- ##
##  Loop over one of these lists.  ##
## ------------------------------- ##

# KPSE_FOR_PKGS(LIST, ACTION)
# ---------------------------
# Run the shell code ACTION for each element Kpse_Pkg in kpse_LIST_pkgs,
# with Kpse_pkg a sanitized version of Kpse_Pkg for shell variables and
# Kpse_PKG a sanitized (upper case) version for make variables.
AC_DEFUN([KPSE_FOR_PKGS],
[AC_REQUIRE([KPSE_LIBS_PKGS])AC_REQUIRE([KPSE_UTILS_PKGS])AC_REQUIRE([KPSE_TEXK_PKGS])[]dnl
AC_FOREACH([Kpse_Pkg], kpse_$1_pkgs,
[m4_pushdef([Kpse_pkg], AS_TR_SH(Kpse_Pkg))[]dnl
m4_pushdef([Kpse_PKG], AS_TR_CPP(Kpse_Pkg))[]dnl
$2
m4_popdef([Kpse_pkg])m4_popdef([Kpse_PKG])[]dnl
])]) # KPSE_FOR_PKGS
