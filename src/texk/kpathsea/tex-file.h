/* tex-file.h: find files in a particular format.

   Copyright 1993, 1994, 1995, 1996, 2007, 2008 Karl Berry.
   Copyright 1998-2005 Olaf Weber.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef KPATHSEA_TEX_FILE_H
#define KPATHSEA_TEX_FILE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/c-vararg.h>
#include <kpathsea/types.h>


/* This initializes the fallback resolution list.  If ENVVAR
   is set, it is used; otherwise, the envvar `TEXSIZES' is looked at; if
   that's not set either, a compile-time default is used.  */
extern void kpse_init_fallback_resolutions P1H(string envvar);

/* If LEVEL is higher than `program_enabled_level' for FMT, set
   `program_enabled_p' to VALUE.  */
extern KPSEDLL void kpse_set_program_enabled P3H(kpse_file_format_type fmt,
                                         boolean value, kpse_src_type level);
/* Call kpse_set_program_enabled with VALUE and the format corresponding
   to FMTNAME.  */
extern KPSEDLL void kpse_maketex_option P2H(const_string fmtname,  boolean value);

/* Change the list of searched suffixes (alternate suffixes if alternate is
   true).  */
extern KPSEDLL void kpse_set_suffixes PVAR2H(kpse_file_format_type format,
					     boolean alternate);

/* Initialize the info for the given format.  This is called
   automatically by `kpse_find_file', but the glyph searching (for
   example) can't use that function, so make it available.  */
extern KPSEDLL const_string kpse_init_format P1H(kpse_file_format_type);

/* If FORMAT has a non-null `suffix' member, append it to NAME "."
   and call `kpse_path_search' with the result and the other arguments.
   If that fails, try just NAME.  */
extern KPSEDLL string kpse_find_file P3H(const_string name,
                            kpse_file_format_type format,  boolean must_exist);

/* Ditto, allowing ALL parameter and hence returning a NULL-terminated
   list of results.  */
extern KPSEDLL string *kpse_find_file_generic
  P4H(const_string name, kpse_file_format_type format,
      boolean must_exist, boolean all);

/* Here are some abbreviations.  */
#define kpse_find_mf(name)   kpse_find_file (name, kpse_mf_format, true)
#define kpse_find_mft(name)  kpse_find_file (name, kpse_mft_format, true)
#define kpse_find_pict(name) kpse_find_file (name, kpse_pict_format, true)
#define kpse_find_tex(name)  kpse_find_file (name, kpse_tex_format, true)
#define kpse_find_tfm(name)  kpse_find_file (name, kpse_tfm_format, true)
#define kpse_find_ofm(name)  kpse_find_file (name, kpse_ofm_format, true)

/* The `false' is correct for DVI translators, which should clearly not
   require vf files for every font (e.g., cmr10.vf).  But it's wrong for
   VF translators, such as vftovp.  */
#define kpse_find_vf(name) kpse_find_file (name, kpse_vf_format, false)
#define kpse_find_ovf(name) kpse_find_file (name, kpse_ovf_format, false)

/* Don't just look up the name, actually open the file.  */
extern KPSEDLL FILE *kpse_open_file P2H(const_string, kpse_file_format_type);

/* This function is used to set kpse_program_name (from progname.c) to
   a different value.  It will clear the path searching information, to
   ensure that the search paths are appropriate to the new name. */
extern KPSEDLL void kpse_reset_program_name P1H(const_string progname);

#endif /* not KPATHSEA_TEX_FILE_H */
