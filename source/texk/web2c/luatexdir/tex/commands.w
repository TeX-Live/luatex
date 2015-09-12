% commands.w
%
% Copyright 2009-2010 Taco Hoekwater <taco@@luatex.org>
%
% This file is part of LuaTeX.
%
% LuaTeX is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free
% Software Foundation; either version 2 of the License, or (at your
% option) any later version.
%
% LuaTeX is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.
%
% You should have received a copy of the GNU General Public License along
% with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

\def\eTeX{e-\TeX}

@ @c


#include "ptexlib.h"

@ The symbolic names for glue parameters are put into \TeX's hash table
by using the routine called |primitive|, defined below. Let us enter them
now, so that we don't have to list all those parameter names anywhere else.

@c
void initialize_commands(void)
{

    primitive_tex("lineskip", assign_glue_cmd, glue_base + line_skip_code,
                  glue_base);
    primitive_tex("baselineskip", assign_glue_cmd,
                  glue_base + baseline_skip_code, glue_base);
    primitive_tex("parskip", assign_glue_cmd, glue_base + par_skip_code,
                  glue_base);
    primitive_tex("abovedisplayskip", assign_glue_cmd,
                  glue_base + above_display_skip_code, glue_base);
    primitive_tex("belowdisplayskip", assign_glue_cmd,
                  glue_base + below_display_skip_code, glue_base);
    primitive_tex("abovedisplayshortskip", assign_glue_cmd,
                  glue_base + above_display_short_skip_code, glue_base);
    primitive_tex("belowdisplayshortskip", assign_glue_cmd,
                  glue_base + below_display_short_skip_code, glue_base);
    primitive_tex("leftskip", assign_glue_cmd, glue_base + left_skip_code,
                  glue_base);
    primitive_tex("rightskip", assign_glue_cmd, glue_base + right_skip_code,
                  glue_base);
    primitive_tex("topskip", assign_glue_cmd, glue_base + top_skip_code,
                  glue_base);
    primitive_tex("splittopskip", assign_glue_cmd,
                  glue_base + split_top_skip_code, glue_base);
    primitive_tex("tabskip", assign_glue_cmd, glue_base + tab_skip_code,
                  glue_base);
    primitive_tex("spaceskip", assign_glue_cmd, glue_base + space_skip_code,
                  glue_base);
    primitive_tex("xspaceskip", assign_glue_cmd, glue_base + xspace_skip_code,
                  glue_base);
    primitive_tex("parfillskip", assign_glue_cmd,
                  glue_base + par_fill_skip_code, glue_base);
    primitive_tex("thinmuskip", assign_mu_glue_cmd,
                  glue_base + thin_mu_skip_code, glue_base + thin_mu_skip_code);
    primitive_tex("medmuskip", assign_mu_glue_cmd, glue_base + med_mu_skip_code,
                  glue_base + thin_mu_skip_code);
    primitive_tex("thickmuskip", assign_mu_glue_cmd,
                  glue_base + thick_mu_skip_code,
                  glue_base + thin_mu_skip_code);
    primitive_tex("output", assign_toks_cmd, output_routine_loc, local_base);
    primitive_tex("everypar", assign_toks_cmd, every_par_loc, local_base);
    primitive_tex("everymath", assign_toks_cmd, every_math_loc, local_base);
    primitive_tex("everydisplay", assign_toks_cmd, every_display_loc,
                  local_base);
    primitive_tex("everyhbox", assign_toks_cmd, every_hbox_loc, local_base);
    primitive_tex("everyvbox", assign_toks_cmd, every_vbox_loc, local_base);
    primitive_tex("everyjob", assign_toks_cmd, every_job_loc, local_base);
    primitive_tex("everycr", assign_toks_cmd, every_cr_loc, local_base);
    primitive_tex("errhelp", assign_toks_cmd, err_help_loc, local_base);
    primitive_pdftex("pdfpagesattr", assign_toks_cmd, pdf_pages_attr_loc,
                     local_base);
    primitive_pdftex("pdfpageattr", assign_toks_cmd, pdf_page_attr_loc,
                     local_base);
    primitive_pdftex("pdfpageresources", assign_toks_cmd,
                     pdf_page_resources_loc, local_base);
    primitive_pdftex("pdfxformattr", assign_toks_cmd, pdf_xform_attr_loc,
                     local_base);
    primitive_pdftex("pdfxformresources", assign_toks_cmd,
                     pdf_xform_resources_loc, local_base);
    primitive_pdftex("pdfpkmode", assign_toks_cmd, pdf_pk_mode_loc, local_base);

    /* The integer parameter names must be entered into the hash table */
    primitive_tex("pretolerance", assign_int_cmd, int_base + pretolerance_code,
                  int_base);
    primitive_tex("tolerance", assign_int_cmd, int_base + tolerance_code,
                  int_base);
    primitive_tex("linepenalty", assign_int_cmd, int_base + line_penalty_code,
                  int_base);
    primitive_tex("hyphenpenalty", assign_int_cmd,
                  int_base + hyphen_penalty_code, int_base);
    primitive_tex("exhyphenpenalty", assign_int_cmd,
                  int_base + ex_hyphen_penalty_code, int_base);
    primitive_tex("clubpenalty", assign_int_cmd, int_base + club_penalty_code,
                  int_base);
    primitive_tex("widowpenalty", assign_int_cmd, int_base + widow_penalty_code,
                  int_base);
    primitive_tex("displaywidowpenalty", assign_int_cmd,
                  int_base + display_widow_penalty_code, int_base);
    primitive_tex("brokenpenalty", assign_int_cmd,
                  int_base + broken_penalty_code, int_base);
    primitive_tex("binoppenalty", assign_int_cmd,
                  int_base + bin_op_penalty_code, int_base);
    primitive_tex("relpenalty", assign_int_cmd, int_base + rel_penalty_code,
                  int_base);
    primitive_tex("predisplaypenalty", assign_int_cmd,
                  int_base + pre_display_penalty_code, int_base);
    primitive_tex("postdisplaypenalty", assign_int_cmd,
                  int_base + post_display_penalty_code, int_base);
    primitive_tex("interlinepenalty", assign_int_cmd,
                  int_base + inter_line_penalty_code, int_base);
    primitive_tex("doublehyphendemerits", assign_int_cmd,
                  int_base + double_hyphen_demerits_code, int_base);
    primitive_tex("finalhyphendemerits", assign_int_cmd,
                  int_base + final_hyphen_demerits_code, int_base);
    primitive_tex("adjdemerits", assign_int_cmd, int_base + adj_demerits_code,
                  int_base);
    primitive_tex("mag", assign_int_cmd, int_base + mag_code, int_base);
    primitive_tex("delimiterfactor", assign_int_cmd,
                  int_base + delimiter_factor_code, int_base);
    primitive_tex("looseness", assign_int_cmd, int_base + looseness_code,
                  int_base);
    primitive_tex("time", assign_int_cmd, int_base + time_code, int_base);
    primitive_tex("day", assign_int_cmd, int_base + day_code, int_base);
    primitive_tex("month", assign_int_cmd, int_base + month_code, int_base);
    primitive_tex("year", assign_int_cmd, int_base + year_code, int_base);
    primitive_tex("showboxbreadth", assign_int_cmd,
                  int_base + show_box_breadth_code, int_base);
    primitive_tex("showboxdepth", assign_int_cmd,
                  int_base + show_box_depth_code, int_base);
    primitive_tex("hbadness", assign_int_cmd, int_base + hbadness_code,
                  int_base);
    primitive_tex("vbadness", assign_int_cmd, int_base + vbadness_code,
                  int_base);
    primitive_tex("pausing", assign_int_cmd, int_base + pausing_code, int_base);
    primitive_tex("tracingonline", assign_int_cmd,
                  int_base + tracing_online_code, int_base);
    primitive_tex("tracingmacros", assign_int_cmd,
                  int_base + tracing_macros_code, int_base);
    primitive_tex("tracingstats", assign_int_cmd, int_base + tracing_stats_code,
                  int_base);
    primitive_tex("tracingparagraphs", assign_int_cmd,
                  int_base + tracing_paragraphs_code, int_base);
    primitive_tex("tracingpages", assign_int_cmd, int_base + tracing_pages_code,
                  int_base);
    primitive_tex("tracingoutput", assign_int_cmd,
                  int_base + tracing_output_code, int_base);
    primitive_tex("tracinglostchars", assign_int_cmd,
                  int_base + tracing_lost_chars_code, int_base);
    primitive_tex("tracingcommands", assign_int_cmd,
                  int_base + tracing_commands_code, int_base);
    primitive_tex("tracingrestores", assign_int_cmd,
                  int_base + tracing_restores_code, int_base);
    primitive_tex("uchyph", assign_int_cmd, int_base + uc_hyph_code, int_base);
    primitive_tex("outputpenalty", assign_int_cmd,
                  int_base + output_penalty_code, int_base);
    primitive_tex("maxdeadcycles", assign_int_cmd,
                  int_base + max_dead_cycles_code, int_base);
    primitive_tex("hangafter", assign_int_cmd, int_base + hang_after_code,
                  int_base);
    primitive_tex("floatingpenalty", assign_int_cmd,
                  int_base + floating_penalty_code, int_base);
    primitive_tex("globaldefs", assign_int_cmd, int_base + global_defs_code,
                  int_base);
    primitive_tex("fam", assign_int_cmd, int_base + cur_fam_code, int_base);
    primitive_tex("escapechar", assign_int_cmd, int_base + escape_char_code,
                  int_base);
    primitive_tex("defaulthyphenchar", assign_int_cmd,
                  int_base + default_hyphen_char_code, int_base);
    primitive_tex("defaultskewchar", assign_int_cmd,
                  int_base + default_skew_char_code, int_base);
    primitive_tex("endlinechar", assign_int_cmd, int_base + end_line_char_code,
                  int_base);
    primitive_tex("newlinechar", assign_int_cmd, int_base + new_line_char_code,
                  int_base);
    primitive_tex("language", assign_int_cmd, int_base + language_code,
                  int_base);
    primitive_tex("setlanguage", assign_int_cmd, int_base + cur_lang_code,
                  int_base);
    primitive_tex("exhyphenchar", assign_int_cmd,
                  int_base + ex_hyphen_char_code, int_base);
    primitive_tex("lefthyphenmin", assign_int_cmd,
                  int_base + left_hyphen_min_code, int_base);
    primitive_tex("righthyphenmin", assign_int_cmd,
                  int_base + right_hyphen_min_code, int_base);
    primitive_tex("holdinginserts", assign_int_cmd,
                  int_base + holding_inserts_code, int_base);
    primitive_tex("errorcontextlines", assign_int_cmd,
                  int_base + error_context_lines_code, int_base);
    primitive_luatex("luastartup", assign_int_cmd,
                     int_base + luastartup_id_code, int_base);
    primitive_luatex("nokerns", assign_int_cmd, int_base + disable_kern_code,
                     int_base);
    primitive_luatex("noligs", assign_int_cmd, int_base + disable_lig_code,
                     int_base);
    primitive_luatex("catcodetable", assign_int_cmd,
                     int_base + cat_code_table_code, int_base);
    primitive_luatex("outputbox", assign_int_cmd, int_base + output_box_code,
                     int_base);
    primitive_pdftex("pdfoutput", assign_int_cmd, int_base + pdf_output_code,
                     int_base);
    primitive_pdftex("pdfcompresslevel", assign_int_cmd,
                     int_base + pdf_compress_level_code, int_base);
    primitive_pdftex("pdfobjcompresslevel", assign_int_cmd,
                     int_base + pdf_objcompresslevel_code, int_base);
    primitive_pdftex("pdfdecimaldigits", assign_int_cmd,
                     int_base + pdf_decimal_digits_code, int_base);
    primitive_pdftex("pdfimageresolution", assign_int_cmd,
                     int_base + pdf_image_resolution_code, int_base);
    primitive_pdftex("pdfpkresolution", assign_int_cmd,
                     int_base + pdf_pk_resolution_code, int_base);
    primitive_pdftex("pdfuniqueresname", assign_int_cmd,
                     int_base + pdf_unique_resname_code, int_base);
    primitive_pdftex("pdfoptionpdfminorversion", assign_int_cmd,
                     int_base + pdf_minor_version_code, int_base);
    primitive_pdftex("pdfminorversion", assign_int_cmd,
                     int_base + pdf_minor_version_code, int_base);
    primitive_pdftex("pdfpagebox", assign_int_cmd, int_base + pdf_pagebox_code,
                     int_base);
    primitive_pdftex("pdfinclusionerrorlevel", assign_int_cmd,
                     int_base + pdf_inclusion_errorlevel_code, int_base);
    primitive_pdftex("pdfgamma", assign_int_cmd, int_base + pdf_gamma_code,
                     int_base);
    primitive_pdftex("pdfimagegamma", assign_int_cmd,
                     int_base + pdf_image_gamma_code, int_base);
    primitive_pdftex("pdfimagehicolor", assign_int_cmd,
                     int_base + pdf_image_hicolor_code, int_base);
    primitive_pdftex("pdfimageapplygamma", assign_int_cmd,
                     int_base + pdf_image_apply_gamma_code, int_base);
    primitive_pdftex("pdfadjustspacing", assign_int_cmd,
                     int_base + pdf_adjust_spacing_code, int_base);
    primitive_pdftex("pdfprotrudechars", assign_int_cmd,
                     int_base + pdf_protrude_chars_code, int_base);
    primitive_pdftex("pdftracingfonts", assign_int_cmd,
                     int_base + pdf_tracing_fonts_code, int_base);
    primitive_pdftex("pdfgentounicode", assign_int_cmd,
                     int_base + pdf_gen_tounicode_code, int_base);
    primitive_pdftex("pdfdraftmode", assign_int_cmd,
                     int_base + pdf_draftmode_code, int_base);
    primitive_pdftex("pdfinclusioncopyfonts", assign_int_cmd,
                     int_base + pdf_inclusion_copy_font_code, int_base);
    primitive_pdftex("pdfreplacefont", assign_int_cmd,
                     int_base + pdf_replace_font_code, int_base);
    primitive_tex("parindent", assign_dimen_cmd, dimen_base + par_indent_code,
                  dimen_base);
    primitive_tex("mathsurround", assign_dimen_cmd,
                  dimen_base + math_surround_code, dimen_base);
    primitive_tex("lineskiplimit", assign_dimen_cmd,
                  dimen_base + line_skip_limit_code, dimen_base);
    primitive_tex("hsize", assign_dimen_cmd, dimen_base + hsize_code,
                  dimen_base);
    primitive_tex("vsize", assign_dimen_cmd, dimen_base + vsize_code,
                  dimen_base);
    primitive_tex("maxdepth", assign_dimen_cmd, dimen_base + max_depth_code,
                  dimen_base);
    primitive_tex("splitmaxdepth", assign_dimen_cmd,
                  dimen_base + split_max_depth_code, dimen_base);
    primitive_tex("boxmaxdepth", assign_dimen_cmd,
                  dimen_base + box_max_depth_code, dimen_base);
    primitive_tex("hfuzz", assign_dimen_cmd, dimen_base + hfuzz_code,
                  dimen_base);
    primitive_tex("vfuzz", assign_dimen_cmd, dimen_base + vfuzz_code,
                  dimen_base);
    primitive_tex("delimitershortfall", assign_dimen_cmd,
                  dimen_base + delimiter_shortfall_code, dimen_base);
    primitive_tex("nulldelimiterspace", assign_dimen_cmd,
                  dimen_base + null_delimiter_space_code, dimen_base);
    primitive_tex("scriptspace", assign_dimen_cmd,
                  dimen_base + script_space_code, dimen_base);
    primitive_tex("predisplaysize", assign_dimen_cmd,
                  dimen_base + pre_display_size_code, dimen_base);
    primitive_tex("displaywidth", assign_dimen_cmd,
                  dimen_base + display_width_code, dimen_base);
    primitive_tex("displayindent", assign_dimen_cmd,
                  dimen_base + display_indent_code, dimen_base);
    primitive_tex("overfullrule", assign_dimen_cmd,
                  dimen_base + overfull_rule_code, dimen_base);
    primitive_tex("hangindent", assign_dimen_cmd, dimen_base + hang_indent_code,
                  dimen_base);
    primitive_tex("hoffset", assign_dimen_cmd, dimen_base + h_offset_code,
                  dimen_base);
    primitive_tex("voffset", assign_dimen_cmd, dimen_base + v_offset_code,
                  dimen_base);
    primitive_tex("emergencystretch", assign_dimen_cmd,
                  dimen_base + emergency_stretch_code, dimen_base);
    primitive_pdftex("pdfhorigin", assign_dimen_cmd,
                     dimen_base + pdf_h_origin_code, dimen_base);
    primitive_pdftex("pdfvorigin", assign_dimen_cmd,
                     dimen_base + pdf_v_origin_code, dimen_base);
    primitive_omega("pagewidth", assign_dimen_cmd, dimen_base + page_width_code,
                    dimen_base);
    primitive_pdftex("pdfpagewidth", assign_dimen_cmd,
                     dimen_base + page_width_code, dimen_base);
    primitive_omega("pageheight", assign_dimen_cmd,
                    dimen_base + page_height_code, dimen_base);
    primitive_pdftex("pdfpageheight", assign_dimen_cmd,
                     dimen_base + page_height_code, dimen_base);
    primitive_pdftex("pdflinkmargin", assign_dimen_cmd,
                     dimen_base + pdf_link_margin_code, dimen_base);
    primitive_pdftex("pdfdestmargin", assign_dimen_cmd,
                     dimen_base + pdf_dest_margin_code, dimen_base);
    primitive_pdftex("pdfthreadmargin", assign_dimen_cmd,
                     dimen_base + pdf_thread_margin_code, dimen_base);
    primitive_pdftex("pdffirstlineheight", assign_dimen_cmd,
                     dimen_base + pdf_first_line_height_code, dimen_base);
    primitive_pdftex("pdflastlinedepth", assign_dimen_cmd,
                     dimen_base + pdf_last_line_depth_code, dimen_base);
    primitive_pdftex("pdfeachlineheight", assign_dimen_cmd,
                     dimen_base + pdf_each_line_height_code, dimen_base);
    primitive_pdftex("pdfeachlinedepth", assign_dimen_cmd,
                     dimen_base + pdf_each_line_depth_code, dimen_base);
    primitive_pdftex("pdfignoreddimen", assign_dimen_cmd,
                     dimen_base + pdf_ignored_dimen_code, dimen_base);
    primitive_pdftex("pdfpxdimen", assign_dimen_cmd,
                     dimen_base + pdf_px_dimen_code, dimen_base);

    /* Many of \TeX's primitives need no |equiv|, since they are identifiable
       by their |eq_type| alone. These primitives are loaded into the hash table
       as follows: */

    primitive_tex(" ", ex_space_cmd, 0, 0);
    primitive_tex("/", ital_corr_cmd, 0, 0);
    primitive_tex("accent", accent_cmd, 0, 0);
    primitive_tex("advance", advance_cmd, 0, 0);
    primitive_tex("afterassignment", after_assignment_cmd, 0, 0);
    primitive_tex("aftergroup", after_group_cmd, 0, 0);
    primitive_tex("begingroup", begin_group_cmd, 0, 0);
    primitive_tex("char", char_num_cmd, 0, 0);
    primitive_tex("csname", cs_name_cmd, 0, 0);
    primitive_tex("delimiter", delim_num_cmd, 0, 0);
    primitive_umath("Udelimiter", delim_num_cmd, 1, 0);
    primitive_tex("divide", divide_cmd, 0, 0);
    primitive_tex("endcsname", end_cs_name_cmd, 0, 0);
    primitive_tex("endgroup", end_group_cmd, 0, 0);
    cs_text(frozen_end_group) = maketexstring("endgroup");
    eqtb[frozen_end_group] = eqtb[cur_val];
    primitive_tex("expandafter", expand_after_cmd, 0, 0);
    primitive_tex("font", def_font_cmd, 0, 0);
    primitive_pdftex("letterspacefont", letterspace_font_cmd, 0, 0);
    primitive_pdftex("pdfcopyfont", pdf_copy_font_cmd, 0, 0);
    primitive_tex("fontdimen", assign_font_dimen_cmd, 0, 0);
    primitive_tex("halign", halign_cmd, 0, 0);
    primitive_tex("hrule", hrule_cmd, 0, 0);
    primitive_tex("ignorespaces", ignore_spaces_cmd, 0, 0);
    primitive_tex("insert", insert_cmd, 0, 0);
    primitive_omega("leftghost", char_ghost_cmd, 0, 0);
    primitive_tex("mark", mark_cmd, 0, 0);
    primitive_tex("mathaccent", math_accent_cmd, 0, 0);
    primitive_umath("Umathaccent", math_accent_cmd, 1, 0);
    primitive_tex("mathchar", math_char_num_cmd, 0, 0);
    primitive_umath("Umathchar", math_char_num_cmd, 1, 0);
    primitive_umath("Umathcharnum", math_char_num_cmd, 2, 0);
    primitive_tex("mathchoice", math_choice_cmd, 0, 0);
    primitive_umath("Ustack", math_choice_cmd, 1, 0);
    primitive_tex("multiply", multiply_cmd, 0, 0);
    primitive_tex("noalign", no_align_cmd, 0, 0);
    primitive_tex("noboundary", no_boundary_cmd, 0, 0);
    primitive_tex("noexpand", no_expand_cmd, 0, 0);
    primitive_luatex("primitive", no_expand_cmd, 1, 0);
    primitive_pdftex("pdfprimitive", no_expand_cmd, 1, 0);
    primitive_tex("nonscript", non_script_cmd, 0, 0);
    primitive_tex("omit", omit_cmd, 0, 0);
    primitive_tex("parshape", set_tex_shape_cmd, par_shape_loc, par_shape_loc);
    primitive_tex("penalty", break_penalty_cmd, 0, 0);
    primitive_tex("prevgraf", set_prev_graf_cmd, 0, 0);
    primitive_tex("radical", radical_cmd, 0, 0);
    primitive_umath("Uradical", radical_cmd, 1, 0);
    primitive_umath("Uroot", radical_cmd, 2, 0);
    primitive_umath("Uunderdelimiter", radical_cmd, 3, 0);
    primitive_umath("Uoverdelimiter", radical_cmd, 4, 0);
    primitive_umath("Udelimiterunder", radical_cmd, 5, 0);
    primitive_umath("Udelimiterover", radical_cmd, 6, 0);
    primitive_tex("read", read_to_cs_cmd, 0, 0);
    primitive_tex("relax", relax_cmd, too_big_char, too_big_char);
    cs_text(frozen_relax) = maketexstring("relax");
    eqtb[frozen_relax] = eqtb[cur_val];
    primitive_omega("rightghost", char_ghost_cmd, 1, 0);
    primitive_tex("setbox", set_box_cmd, 0, 0);
    primitive_tex("the", the_cmd, 0, 0);
    primitive_tex("toks", toks_register_cmd, 0, 0);
    primitive_tex("vadjust", vadjust_cmd, 0, 0);
    primitive_tex("valign", valign_cmd, 0, 0);
    primitive_tex("vcenter", vcenter_cmd, 0, 0);
    primitive_tex("vrule", vrule_cmd, 0, 0);
    primitive_tex("par", par_end_cmd, too_big_char, too_big_char);      /* cf.\ |scan_file_name| */
    par_loc = cur_val;
    par_token = cs_token_flag + par_loc;
    @<Create a bunch of primitives@>;
    @<Create the math param primitives@>;
    @<Create another bunch of primitives@>;
}


@ These are in a separate module due to a CWEAVE limitation.

@<Create a bunch of primitives@>=
    /* The processing of \.{\\input} involves the |start_input| subroutine,
       which will be declared later; the processing of \.{\\endinput} is trivial. */
    primitive_tex("input", input_cmd, 0, 0);
    primitive_tex("endinput", input_cmd, 1, 0);
    primitive_tex("topmark", top_bot_mark_cmd, top_mark_code, 0);
    primitive_tex("firstmark", top_bot_mark_cmd, first_mark_code, 0);
    primitive_tex("botmark", top_bot_mark_cmd, bot_mark_code, 0);
    primitive_tex("splitfirstmark", top_bot_mark_cmd, split_first_mark_code, 0);
    primitive_tex("splitbotmark", top_bot_mark_cmd, split_bot_mark_code, 0);
    primitive_luatex("clearmarks", mark_cmd, clear_marks_code, 0);
    primitive_etex("marks", mark_cmd, marks_code, 0);
    primitive_etex("topmarks", top_bot_mark_cmd, top_mark_code + marks_code, 0);
    primitive_etex("firstmarks", top_bot_mark_cmd, first_mark_code + marks_code,
                   0);
    primitive_etex("botmarks", top_bot_mark_cmd, bot_mark_code + marks_code, 0);
    primitive_etex("splitfirstmarks", top_bot_mark_cmd,
                   split_first_mark_code + marks_code, 0);
    primitive_etex("splitbotmarks", top_bot_mark_cmd,
                   split_bot_mark_code + marks_code, 0);

    /* The hash table is initialized with `\.{\\count}', `\.{\\attribute}',
       `\.{\\dimen}', `\.{\\skip}', and `\.{\\muskip}' all having |register|
       as their command code; they are distinguished by the |chr_code|, which
       is either |int_val|, |attr_val|, |dimen_val|, |glue_val|, or |mu_val|.
     */
    primitive_tex("count", register_cmd, int_val_level, 0);
    primitive_luatex("attribute", register_cmd, attr_val_level, 0);
    primitive_tex("dimen", register_cmd, dimen_val_level, 0);
    primitive_tex("skip", register_cmd, glue_val_level, 0);
    primitive_tex("muskip", register_cmd, mu_val_level, 0);

    primitive_tex("spacefactor", set_aux_cmd, hmode, 0);
    primitive_tex("prevdepth", set_aux_cmd, vmode, 0);
    primitive_tex("deadcycles", set_page_int_cmd, 0, 0);
    primitive_tex("insertpenalties", set_page_int_cmd, 1, 0);
    primitive_tex("wd", set_box_dimen_cmd, width_offset, 0);
    primitive_tex("ht", set_box_dimen_cmd, height_offset, 0);
    primitive_tex("dp", set_box_dimen_cmd, depth_offset, 0);
    primitive_tex("lastpenalty", last_item_cmd, lastpenalty_code, 0);
    primitive_tex("lastkern", last_item_cmd, lastkern_code, 0);
    primitive_tex("lastskip", last_item_cmd, lastskip_code, 0);
    primitive_tex("inputlineno", last_item_cmd, input_line_no_code, 0);
    primitive_tex("badness", last_item_cmd, badness_code, 0);
    primitive_pdftex("pdftexversion", last_item_cmd, luatex_version_code, 0); /*OBSOLETE */
    primitive_luatex("luatexversion", last_item_cmd, luatex_version_code, 0);
    primitive_pdftex("pdflastobj", last_item_cmd, pdf_last_obj_code, 0);
    primitive_pdftex("pdflastxform", last_item_cmd, pdf_last_xform_code, 0);
    primitive_pdftex("pdflastximage", last_item_cmd, pdf_last_ximage_code, 0);
    primitive_pdftex("pdflastximagepages", last_item_cmd,
                     pdf_last_ximage_pages_code, 0);
    primitive_pdftex("pdflastannot", last_item_cmd, pdf_last_annot_code, 0);
    primitive_pdftex("pdflastxpos", last_item_cmd, pdf_last_x_pos_code, 0);
    primitive_pdftex("pdflastypos", last_item_cmd, pdf_last_y_pos_code, 0);
    primitive_pdftex("pdfretval", last_item_cmd, pdf_retval_code, 0);
    primitive_pdftex("pdflastximagecolordepth", last_item_cmd,
                     pdf_last_ximage_colordepth_code, 0);
    primitive_pdftex("pdfrandomseed", last_item_cmd, random_seed_code, 0);
    primitive_pdftex("pdflastlink", last_item_cmd, pdf_last_link_code, 0);

    primitive_tex("number", convert_cmd, number_code, 0);
    primitive_tex("romannumeral", convert_cmd, roman_numeral_code, 0);
    primitive_tex("string", convert_cmd, string_code, 0);
    primitive_tex("meaning", convert_cmd, meaning_code, 0);
    primitive_etex("eTeXVersion", convert_cmd, etex_code, 0);
    primitive_tex("fontname", convert_cmd, font_name_code, 0);
    primitive_luatex("fontid", convert_cmd, font_id_code, 0);
    primitive_pdftex("pdftexrevision", convert_cmd, luatex_revision_code, 0); /*OBSOLETE*/
    primitive_luatex("luatexrevision", convert_cmd, luatex_revision_code, 0);
    primitive_luatex("luatexdatestamp", convert_cmd, luatex_date_code, 0);
    primitive_pdftex("pdftexbanner", convert_cmd, luatex_banner_code, 0); /*OBSOLETE*/
    primitive_pdftex("luatexbanner", convert_cmd, luatex_banner_code, 0); /*will replace pdftexbanner*/
    primitive_pdftex("pdffontname", convert_cmd, pdf_font_name_code, 0);
    primitive_pdftex("pdffontobjnum", convert_cmd, pdf_font_objnum_code, 0);
    primitive_pdftex("pdffontsize", convert_cmd, pdf_font_size_code, 0);
    primitive_pdftex("pdfpageref", convert_cmd, pdf_page_ref_code, 0);
    primitive_pdftex("leftmarginkern", convert_cmd, left_margin_kern_code, 0);
    primitive_pdftex("rightmarginkern", convert_cmd, right_margin_kern_code, 0);
    primitive_pdftex("pdfxformname", convert_cmd, pdf_xform_name_code, 0);
    primitive_pdftex("pdfcreationdate", convert_cmd, pdf_creation_date_code, 0);
    primitive_pdftex("pdfuniformdeviate", convert_cmd, uniform_deviate_code, 0);
    primitive_pdftex("pdfcolorstackinit", convert_cmd, pdf_colorstack_init_code,
                     0);
    primitive_pdftex("pdfnormaldeviate", convert_cmd, normal_deviate_code, 0);
    primitive_core("directlua", convert_cmd, lua_code, 0);
    primitive_luatex("luafunction", convert_cmd, lua_function_code, 0);
    primitive_luatex("luaescapestring", convert_cmd, lua_escape_string_code, 0);
    primitive_luatex("mathstyle", convert_cmd, math_style_code, 0);
    primitive_pdftex("expanded", convert_cmd, expanded_code, 0);
    primitive_tex("jobname", convert_cmd, job_name_code, 0);
    primitive_luatex("formatname", convert_cmd, format_name_code, 0);
    primitive_pdftex("pdfinsertht", convert_cmd, pdf_insert_ht_code, 0);
    primitive_pdftex("pdfximagebbox", convert_cmd, pdf_ximage_bbox_code, 0);
    primitive_luatex("Uchar", convert_cmd, uchar_code, 0);

    primitive_tex("if", if_test_cmd, if_char_code, 0);
    primitive_tex("ifcat", if_test_cmd, if_cat_code, 0);
    primitive_tex("ifnum", if_test_cmd, if_int_code, 0);
    primitive_tex("ifdim", if_test_cmd, if_dim_code, 0);
    primitive_tex("ifodd", if_test_cmd, if_odd_code, 0);
    primitive_tex("ifvmode", if_test_cmd, if_vmode_code, 0);
    primitive_tex("ifhmode", if_test_cmd, if_hmode_code, 0);
    primitive_tex("ifmmode", if_test_cmd, if_mmode_code, 0);
    primitive_tex("ifinner", if_test_cmd, if_inner_code, 0);
    primitive_tex("ifvoid", if_test_cmd, if_void_code, 0);

    primitive_tex("ifhbox", if_test_cmd, if_hbox_code, 0);
    primitive_tex("ifvbox", if_test_cmd, if_vbox_code, 0);
    primitive_tex("ifx", if_test_cmd, ifx_code, 0);
    primitive_tex("ifeof", if_test_cmd, if_eof_code, 0);
    primitive_tex("iftrue", if_test_cmd, if_true_code, 0);
    primitive_tex("iffalse", if_test_cmd, if_false_code, 0);
    primitive_tex("ifcase", if_test_cmd, if_case_code, 0);
    primitive_luatex("ifprimitive", if_test_cmd, if_primitive_code, 0);
    primitive_pdftex("ifpdfprimitive", if_test_cmd, if_primitive_code, 0);

    primitive_tex("fi", fi_or_else_cmd, fi_code, 0);
    cs_text(frozen_fi) = maketexstring("fi");
    eqtb[frozen_fi] = eqtb[cur_val];
    primitive_tex("or", fi_or_else_cmd, or_code, 0);
    primitive_tex("else", fi_or_else_cmd, else_code, 0);

    /* \TeX\ always knows at least one font, namely the null font. It has no
       characters, and its seven parameters are all equal to zero. */
    primitive_tex("nullfont", set_font_cmd, null_font, 0);
    cs_text(frozen_null_font) = maketexstring("nullfont");
    eqtb[frozen_null_font] = eqtb[cur_val];

    primitive_tex("span", tab_mark_cmd, span_code, tab_mark_cmd_code);
    primitive_luatex("aligntab", tab_mark_cmd, tab_mark_cmd_code,
                     tab_mark_cmd_code);
    primitive_luatex("alignmark", mac_param_cmd, tab_mark_cmd_code,
                     tab_mark_cmd_code);
    primitive_tex("cr", car_ret_cmd, cr_code, cr_code);
    cs_text(frozen_cr) = maketexstring("cr");
    eqtb[frozen_cr] = eqtb[cur_val];
    primitive_tex("crcr", car_ret_cmd, cr_cr_code, cr_code);
    cs_text(frozen_end_template) = maketexstring("endtemplate");
    cs_text(frozen_endv) = maketexstring("endtemplate");
    set_eq_type(frozen_endv, endv_cmd);
    set_equiv(frozen_endv, null_list);
    set_eq_level(frozen_endv, level_one);
    eqtb[frozen_end_template] = eqtb[frozen_endv];
    set_eq_type(frozen_end_template, end_template_cmd);

    primitive_tex("pagegoal", set_page_dimen_cmd, 0, 0);
    primitive_tex("pagetotal", set_page_dimen_cmd, 1, 0);
    primitive_tex("pagestretch", set_page_dimen_cmd, 2, 0);
    primitive_tex("pagefilstretch", set_page_dimen_cmd, 3, 0);
    primitive_tex("pagefillstretch", set_page_dimen_cmd, 4, 0);
    primitive_tex("pagefilllstretch", set_page_dimen_cmd, 5, 0);
    primitive_tex("pageshrink", set_page_dimen_cmd, 6, 0);
    primitive_tex("pagedepth", set_page_dimen_cmd, 7, 0);

    /* Either \.{\\dump} or \.{\\end} will cause |main_control| to enter the
       endgame, since both of them have `|stop|' as their command code. */

    primitive_tex("end", stop_cmd, 0, 0);
    primitive_tex("dump", stop_cmd, 1, 0);

    primitive_tex("hskip", hskip_cmd, skip_code, 0);
    primitive_tex("hfil", hskip_cmd, fil_code, 0);
    primitive_tex("hfill", hskip_cmd, fill_code, 0);
    primitive_tex("hss", hskip_cmd, ss_code, 0);
    primitive_tex("hfilneg", hskip_cmd, fil_neg_code, 0);
    primitive_tex("vskip", vskip_cmd, skip_code, 0);
    primitive_tex("vfil", vskip_cmd, fil_code, 0);
    primitive_tex("vfill", vskip_cmd, fill_code, 0);
    primitive_tex("vss", vskip_cmd, ss_code, 0);
    primitive_tex("vfilneg", vskip_cmd, fil_neg_code, 0);
    primitive_tex("mskip", mskip_cmd, mskip_code, 0);
    primitive_tex("kern", kern_cmd, explicit, 0);
    primitive_tex("mkern", mkern_cmd, mu_glue, 0);
    primitive_tex("moveleft", hmove_cmd, 1, 0);
    primitive_tex("moveright", hmove_cmd, 0, 0);
    primitive_tex("raise", vmove_cmd, 1, 0);
    primitive_tex("lower", vmove_cmd, 0, 0);
    primitive_tex("box", make_box_cmd, box_code, 0);
    primitive_tex("copy", make_box_cmd, copy_code, 0);
    primitive_tex("lastbox", make_box_cmd, last_box_code, 0);
    primitive_tex("vsplit", make_box_cmd, vsplit_code, 0);
    primitive_tex("vtop", make_box_cmd, vtop_code, 0);
    primitive_tex("vbox", make_box_cmd, vtop_code + vmode, 0);
    primitive_tex("hbox", make_box_cmd, vtop_code + hmode, 0);
    primitive_tex("shipout", leader_ship_cmd, a_leaders - 1, 0);        /* |ship_out_flag=leader_flag-1| */
    primitive_tex("leaders", leader_ship_cmd, a_leaders, 0);
    primitive_tex("cleaders", leader_ship_cmd, c_leaders, 0);
    primitive_tex("xleaders", leader_ship_cmd, x_leaders, 0);
    primitive_luatex("gleaders", leader_ship_cmd, g_leaders, 0);
    primitive_aleph("boxdir", assign_box_dir_cmd, 0, 0);
    primitive_tex("indent", start_par_cmd, 1, 0);
    primitive_tex("noindent", start_par_cmd, 0, 0);
    primitive_pdftex("quitvmode", start_par_cmd, 2, 0);
    primitive_tex("unpenalty", remove_item_cmd, penalty_node, 0);
    primitive_tex("unkern", remove_item_cmd, kern_node, 0);
    primitive_tex("unskip", remove_item_cmd, glue_node, 0);
    primitive_tex("unhbox", un_hbox_cmd, box_code, 0);
    primitive_tex("unhcopy", un_hbox_cmd, copy_code, 0);
    primitive_tex("unvbox", un_vbox_cmd, box_code, 0);
    primitive_tex("unvcopy", un_vbox_cmd, copy_code, 0);
    primitive_tex("-", discretionary_cmd, explicit_disc, 0);
    primitive_tex("discretionary", discretionary_cmd, discretionary_disc, 0);
    primitive_omega("localleftbox", assign_local_box_cmd, 0, 0);
    primitive_omega("localrightbox", assign_local_box_cmd, 1, 0);

    primitive_umath("Ustartmath", math_shift_cs_cmd, text_style, 0);
    primitive_umath("Ustopmath", math_shift_cs_cmd, cramped_text_style, 0);
    primitive_umath("Ustartdisplaymath", math_shift_cs_cmd, display_style, 0);
    primitive_umath("Ustopdisplaymath", math_shift_cs_cmd,
                     cramped_display_style, 0);
    primitive_tex("eqno", eq_no_cmd, 0, 0);
    primitive_tex("leqno", eq_no_cmd, 1, 0);
    primitive_tex("mathord", math_comp_cmd, ord_noad_type, 0);
    primitive_tex("mathop", math_comp_cmd, op_noad_type_normal, 0);
    primitive_tex("mathbin", math_comp_cmd, bin_noad_type, 0);
    primitive_tex("mathrel", math_comp_cmd, rel_noad_type, 0);
    primitive_tex("mathopen", math_comp_cmd, open_noad_type, 0);
    primitive_tex("mathclose", math_comp_cmd, close_noad_type, 0);
    primitive_tex("mathpunct", math_comp_cmd, punct_noad_type, 0);
    primitive_tex("mathinner", math_comp_cmd, inner_noad_type, 0);
    primitive_tex("underline", math_comp_cmd, under_noad_type, 0);
    primitive_tex("overline", math_comp_cmd, over_noad_type, 0);
    primitive_tex("displaylimits", limit_switch_cmd, op_noad_type_normal, 0);
    primitive_tex("limits", limit_switch_cmd, op_noad_type_limits, 0);
    primitive_tex("nolimits", limit_switch_cmd, op_noad_type_no_limits, 0);
    primitive_tex("displaystyle", math_style_cmd, display_style, 0);
    primitive_tex("textstyle", math_style_cmd, text_style, 0);
    primitive_tex("scriptstyle", math_style_cmd, script_style, 0);
    primitive_tex("scriptscriptstyle", math_style_cmd, script_script_style, 0);
    primitive_luatex("crampeddisplaystyle", math_style_cmd,
                     cramped_display_style, 0);
    primitive_luatex("crampedtextstyle", math_style_cmd, cramped_text_style, 0);
    primitive_luatex("crampedscriptstyle", math_style_cmd, cramped_script_style,
                     0);
    primitive_luatex("crampedscriptscriptstyle", math_style_cmd,
                     cramped_script_script_style, 0);
    primitive_umath("Usuperscript", super_sub_script_cmd, sup_mark_cmd,
                     sup_mark_cmd);
    primitive_umath("Usubscript", super_sub_script_cmd, sub_mark_cmd,
                     sup_mark_cmd);
    primitive_tex("above", above_cmd, above_code, 0);
    primitive_tex("over", above_cmd, over_code, 0);
    primitive_tex("atop", above_cmd, atop_code, 0);
    primitive_tex("abovewithdelims", above_cmd, delimited_code + above_code, 0);
    primitive_tex("overwithdelims", above_cmd, delimited_code + over_code, 0);
    primitive_tex("atopwithdelims", above_cmd, delimited_code + atop_code, 0);
    primitive_tex("left", left_right_cmd, left_noad_side, 0);
    primitive_tex("right", left_right_cmd, right_noad_side, 0);
    primitive_tex("middle", left_right_cmd, middle_noad_side, 0);
    cs_text(frozen_right) = maketexstring("right");
    eqtb[frozen_right] = eqtb[cur_val];

    primitive_tex("long", prefix_cmd, 1, 0);
    primitive_tex("outer", prefix_cmd, 2, 0);
    primitive_tex("global", prefix_cmd, 4, 0);
    primitive_tex("def", def_cmd, 0, 0);
    primitive_tex("gdef", def_cmd, 1, 0);
    primitive_tex("edef", def_cmd, 2, 0);
    primitive_tex("xdef", def_cmd, 3, 0);
    primitive_tex("let", let_cmd, normal, 0);
    primitive_tex("futurelet", let_cmd, normal + 1, 0);
    primitive_tex("chardef", shorthand_def_cmd, char_def_code, 0);
    primitive_tex("mathchardef", shorthand_def_cmd, math_char_def_code, 0);
    primitive_umath("Umathchardef", shorthand_def_cmd, xmath_char_def_code, 0);
    primitive_umath("Umathcharnumdef", shorthand_def_cmd, umath_char_def_code, 0);
    primitive_tex("countdef", shorthand_def_cmd, count_def_code, 0);
    primitive_luatex("attributedef", shorthand_def_cmd, attribute_def_code, 0);
    primitive_tex("dimendef", shorthand_def_cmd, dimen_def_code, 0);
    primitive_tex("skipdef", shorthand_def_cmd, skip_def_code, 0);
    primitive_tex("muskipdef", shorthand_def_cmd, mu_skip_def_code, 0);
    primitive_tex("toksdef", shorthand_def_cmd, toks_def_code, 0);
    primitive_tex("catcode", def_char_code_cmd, cat_code_base, cat_code_base);
    primitive_tex("mathcode", def_char_code_cmd, math_code_base, cat_code_base);
    primitive_tex("lccode", def_char_code_cmd, lc_code_base, cat_code_base);
    primitive_tex("uccode", def_char_code_cmd, uc_code_base, cat_code_base);
    primitive_tex("sfcode", def_char_code_cmd, sf_code_base, cat_code_base);
    primitive_tex("delcode", def_del_code_cmd, del_code_base, del_code_base);
    primitive_tex("textfont", def_family_cmd, text_size, 0);
    primitive_tex("scriptfont", def_family_cmd, script_size, 0);
    primitive_tex("scriptscriptfont", def_family_cmd, script_script_size, 0);
    primitive_umath("Umathquad", set_math_param_cmd, math_param_quad, 0);
    primitive_umath("Umathaxis", set_math_param_cmd, math_param_axis, 0);


@ These are in a separate module due to a CWEAVE limitation.

@<Create the math param primitives@>=
    primitive_umath("Umathoperatorsize", set_math_param_cmd,
                     math_param_operator_size, 0);
    primitive_umath("Umathoverbarkern", set_math_param_cmd,
                     math_param_overbar_kern, 0);
    primitive_umath("Umathoverbarrule", set_math_param_cmd,
                     math_param_overbar_rule, 0);
    primitive_umath("Umathoverbarvgap", set_math_param_cmd,
                     math_param_overbar_vgap, 0);
    primitive_umath("Umathunderbarkern", set_math_param_cmd,
                     math_param_underbar_kern, 0);
    primitive_umath("Umathunderbarrule", set_math_param_cmd,
                     math_param_underbar_rule, 0);
    primitive_umath("Umathunderbarvgap", set_math_param_cmd,
                     math_param_underbar_vgap, 0);
    primitive_umath("Umathradicalkern", set_math_param_cmd,
                     math_param_radical_kern, 0);
    primitive_umath("Umathradicalrule", set_math_param_cmd,
                     math_param_radical_rule, 0);
    primitive_umath("Umathradicalvgap", set_math_param_cmd,
                     math_param_radical_vgap, 0);
    primitive_umath("Umathradicaldegreebefore", set_math_param_cmd,
                     math_param_radical_degree_before, 0);
    primitive_umath("Umathradicaldegreeafter", set_math_param_cmd,
                     math_param_radical_degree_after, 0);
    primitive_umath("Umathradicaldegreeraise", set_math_param_cmd,
                     math_param_radical_degree_raise, 0);
    primitive_umath("Umathstackvgap", set_math_param_cmd,
                     math_param_stack_vgap, 0);
    primitive_umath("Umathstacknumup", set_math_param_cmd,
                     math_param_stack_num_up, 0);
    primitive_umath("Umathstackdenomdown", set_math_param_cmd,
                     math_param_stack_denom_down, 0);
    primitive_umath("Umathfractionrule", set_math_param_cmd,
                     math_param_fraction_rule, 0);
    primitive_umath("Umathfractionnumvgap", set_math_param_cmd,
                     math_param_fraction_num_vgap, 0);
    primitive_umath("Umathfractionnumup", set_math_param_cmd,
                     math_param_fraction_num_up, 0);
    primitive_umath("Umathfractiondenomvgap", set_math_param_cmd,
                     math_param_fraction_denom_vgap, 0);
    primitive_umath("Umathfractiondenomdown", set_math_param_cmd,
                     math_param_fraction_denom_down, 0);
    primitive_umath("Umathfractiondelsize", set_math_param_cmd,
                     math_param_fraction_del_size, 0);
    primitive_umath("Umathlimitabovevgap", set_math_param_cmd,
                     math_param_limit_above_vgap, 0);
    primitive_umath("Umathlimitabovebgap", set_math_param_cmd,
                     math_param_limit_above_bgap, 0);
    primitive_umath("Umathlimitabovekern", set_math_param_cmd,
                     math_param_limit_above_kern, 0);
    primitive_umath("Umathlimitbelowvgap", set_math_param_cmd,
                     math_param_limit_below_vgap, 0);
    primitive_umath("Umathlimitbelowbgap", set_math_param_cmd,
                     math_param_limit_below_bgap, 0);
    primitive_umath("Umathlimitbelowkern", set_math_param_cmd,
                     math_param_limit_below_kern, 0);
    primitive_umath("Umathunderdelimitervgap", set_math_param_cmd,
                     math_param_under_delimiter_vgap, 0);
    primitive_umath("Umathunderdelimiterbgap", set_math_param_cmd,
                     math_param_under_delimiter_bgap, 0);
    primitive_umath("Umathoverdelimitervgap", set_math_param_cmd,
                     math_param_over_delimiter_vgap, 0);
    primitive_umath("Umathoverdelimiterbgap", set_math_param_cmd,
                     math_param_over_delimiter_bgap, 0);
    primitive_umath("Umathsubshiftdrop", set_math_param_cmd,
                     math_param_sub_shift_drop, 0);
    primitive_umath("Umathsupshiftdrop", set_math_param_cmd,
                     math_param_sup_shift_drop, 0);
    primitive_umath("Umathsubshiftdown", set_math_param_cmd,
                     math_param_sub_shift_down, 0);
    primitive_umath("Umathsubsupshiftdown", set_math_param_cmd,
                     math_param_sub_sup_shift_down, 0);
    primitive_umath("Umathsubtopmax", set_math_param_cmd,
                     math_param_sub_top_max, 0);
    primitive_umath("Umathsupshiftup", set_math_param_cmd,
                     math_param_sup_shift_up, 0);
    primitive_umath("Umathsupbottommin", set_math_param_cmd,
                     math_param_sup_bottom_min, 0);
    primitive_umath("Umathsupsubbottommax", set_math_param_cmd,
                     math_param_sup_sub_bottom_max, 0);
    primitive_umath("Umathsubsupvgap", set_math_param_cmd,
                     math_param_subsup_vgap, 0);
    primitive_umath("Umathspaceafterscript", set_math_param_cmd,
                     math_param_space_after_script, 0);
    primitive_umath("Umathconnectoroverlapmin", set_math_param_cmd,
                     math_param_connector_overlap_min, 0);
    primitive_umath("Umathordordspacing", set_math_param_cmd,
                     math_param_ord_ord_spacing, 0);
    primitive_umath("Umathordopspacing", set_math_param_cmd,
                     math_param_ord_op_spacing, 0);
    primitive_umath("Umathordbinspacing", set_math_param_cmd,
                     math_param_ord_bin_spacing, 0);
    primitive_umath("Umathordrelspacing", set_math_param_cmd,
                     math_param_ord_rel_spacing, 0);
    primitive_umath("Umathordopenspacing", set_math_param_cmd,
                     math_param_ord_open_spacing, 0);
    primitive_umath("Umathordclosespacing", set_math_param_cmd,
                     math_param_ord_close_spacing, 0);
    primitive_umath("Umathordpunctspacing", set_math_param_cmd,
                     math_param_ord_punct_spacing, 0);
    primitive_umath("Umathordinnerspacing", set_math_param_cmd,
                     math_param_ord_inner_spacing, 0);
    primitive_umath("Umathopordspacing", set_math_param_cmd,
                     math_param_op_ord_spacing, 0);
    primitive_umath("Umathopopspacing", set_math_param_cmd,
                     math_param_op_op_spacing, 0);
    primitive_umath("Umathopbinspacing", set_math_param_cmd,
                     math_param_op_bin_spacing, 0);
    primitive_umath("Umathoprelspacing", set_math_param_cmd,
                     math_param_op_rel_spacing, 0);
    primitive_umath("Umathopopenspacing", set_math_param_cmd,
                     math_param_op_open_spacing, 0);
    primitive_umath("Umathopclosespacing", set_math_param_cmd,
                     math_param_op_close_spacing, 0);
    primitive_umath("Umathoppunctspacing", set_math_param_cmd,
                     math_param_op_punct_spacing, 0);
    primitive_umath("Umathopinnerspacing", set_math_param_cmd,
                     math_param_op_inner_spacing, 0);
    primitive_umath("Umathbinordspacing", set_math_param_cmd,
                     math_param_bin_ord_spacing, 0);
    primitive_umath("Umathbinopspacing", set_math_param_cmd,
                     math_param_bin_op_spacing, 0);
    primitive_umath("Umathbinbinspacing", set_math_param_cmd,
                     math_param_bin_bin_spacing, 0);
    primitive_umath("Umathbinrelspacing", set_math_param_cmd,
                     math_param_bin_rel_spacing, 0);
    primitive_umath("Umathbinopenspacing", set_math_param_cmd,
                     math_param_bin_open_spacing, 0);
    primitive_umath("Umathbinclosespacing", set_math_param_cmd,
                     math_param_bin_close_spacing, 0);
    primitive_umath("Umathbinpunctspacing", set_math_param_cmd,
                     math_param_bin_punct_spacing, 0);
    primitive_umath("Umathbininnerspacing", set_math_param_cmd,
                     math_param_bin_inner_spacing, 0);
    primitive_umath("Umathrelordspacing", set_math_param_cmd,
                     math_param_rel_ord_spacing, 0);
    primitive_umath("Umathrelopspacing", set_math_param_cmd,
                     math_param_rel_op_spacing, 0);
    primitive_umath("Umathrelbinspacing", set_math_param_cmd,
                     math_param_rel_bin_spacing, 0);
    primitive_umath("Umathrelrelspacing", set_math_param_cmd,
                     math_param_rel_rel_spacing, 0);
    primitive_umath("Umathrelopenspacing", set_math_param_cmd,
                     math_param_rel_open_spacing, 0);
    primitive_umath("Umathrelclosespacing", set_math_param_cmd,
                     math_param_rel_close_spacing, 0);
    primitive_umath("Umathrelpunctspacing", set_math_param_cmd,
                     math_param_rel_punct_spacing, 0);
    primitive_umath("Umathrelinnerspacing", set_math_param_cmd,
                     math_param_rel_inner_spacing, 0);
    primitive_umath("Umathopenordspacing", set_math_param_cmd,
                     math_param_open_ord_spacing, 0);
    primitive_umath("Umathopenopspacing", set_math_param_cmd,
                     math_param_open_op_spacing, 0);
    primitive_umath("Umathopenbinspacing", set_math_param_cmd,
                     math_param_open_bin_spacing, 0);
    primitive_umath("Umathopenrelspacing", set_math_param_cmd,
                     math_param_open_rel_spacing, 0);
    primitive_umath("Umathopenopenspacing", set_math_param_cmd,
                     math_param_open_open_spacing, 0);
    primitive_umath("Umathopenclosespacing", set_math_param_cmd,
                     math_param_open_close_spacing, 0);
    primitive_umath("Umathopenpunctspacing", set_math_param_cmd,
                     math_param_open_punct_spacing, 0);
    primitive_umath("Umathopeninnerspacing", set_math_param_cmd,
                     math_param_open_inner_spacing, 0);
    primitive_umath("Umathcloseordspacing", set_math_param_cmd,
                     math_param_close_ord_spacing, 0);
    primitive_umath("Umathcloseopspacing", set_math_param_cmd,
                     math_param_close_op_spacing, 0);
    primitive_umath("Umathclosebinspacing", set_math_param_cmd,
                     math_param_close_bin_spacing, 0);
    primitive_umath("Umathcloserelspacing", set_math_param_cmd,
                     math_param_close_rel_spacing, 0);
    primitive_umath("Umathcloseopenspacing", set_math_param_cmd,
                     math_param_close_open_spacing, 0);
    primitive_umath("Umathcloseclosespacing", set_math_param_cmd,
                     math_param_close_close_spacing, 0);
    primitive_umath("Umathclosepunctspacing", set_math_param_cmd,
                     math_param_close_punct_spacing, 0);
    primitive_umath("Umathcloseinnerspacing", set_math_param_cmd,
                     math_param_close_inner_spacing, 0);
    primitive_umath("Umathpunctordspacing", set_math_param_cmd,
                     math_param_punct_ord_spacing, 0);
    primitive_umath("Umathpunctopspacing", set_math_param_cmd,
                     math_param_punct_op_spacing, 0);
    primitive_umath("Umathpunctbinspacing", set_math_param_cmd,
                     math_param_punct_bin_spacing, 0);
    primitive_umath("Umathpunctrelspacing", set_math_param_cmd,
                     math_param_punct_rel_spacing, 0);
    primitive_umath("Umathpunctopenspacing", set_math_param_cmd,
                     math_param_punct_open_spacing, 0);
    primitive_umath("Umathpunctclosespacing", set_math_param_cmd,
                     math_param_punct_close_spacing, 0);
    primitive_umath("Umathpunctpunctspacing", set_math_param_cmd,
                     math_param_punct_punct_spacing, 0);
    primitive_umath("Umathpunctinnerspacing", set_math_param_cmd,
                     math_param_punct_inner_spacing, 0);
    primitive_umath("Umathinnerordspacing", set_math_param_cmd,
                     math_param_inner_ord_spacing, 0);
    primitive_umath("Umathinneropspacing", set_math_param_cmd,
                     math_param_inner_op_spacing, 0);
    primitive_umath("Umathinnerbinspacing", set_math_param_cmd,
                     math_param_inner_bin_spacing, 0);
    primitive_umath("Umathinnerrelspacing", set_math_param_cmd,
                     math_param_inner_rel_spacing, 0);
    primitive_umath("Umathinneropenspacing", set_math_param_cmd,
                     math_param_inner_open_spacing, 0);
    primitive_umath("Umathinnerclosespacing", set_math_param_cmd,
                     math_param_inner_close_spacing, 0);
    primitive_umath("Umathinnerpunctspacing", set_math_param_cmd,
                     math_param_inner_punct_spacing, 0);
    primitive_umath("Umathinnerinnerspacing", set_math_param_cmd,
                     math_param_inner_inner_spacing, 0);

@ These are in a separate module due to a CWEAVE limitation.

@<Create another bunch of primitives@>=
    primitive_umath("Umathcode", extdef_math_code_cmd, math_code_base,
                     math_code_base);
    primitive_umath("Udelcode", extdef_del_code_cmd, del_code_base,
                     del_code_base);
    primitive_umath("Umathcodenum", extdef_math_code_cmd, math_code_base + 1,
                     math_code_base);
    primitive_umath("Udelcodenum", extdef_del_code_cmd, del_code_base + 1,
                     del_code_base);
    primitive_tex("hyphenation", hyph_data_cmd, 0, 0);
    primitive_tex("patterns", hyph_data_cmd, 1, 0);
    primitive_luatex("prehyphenchar", hyph_data_cmd, 2, 0);
    primitive_luatex("posthyphenchar", hyph_data_cmd, 3, 0);
    primitive_luatex("preexhyphenchar", hyph_data_cmd, 4, 0);
    primitive_luatex("postexhyphenchar", hyph_data_cmd, 5, 0);
    primitive_luatex("hyphenationmin", hyph_data_cmd, 6, 0);
    primitive_tex("hyphenchar", assign_font_int_cmd, 0, 0);
    primitive_tex("skewchar", assign_font_int_cmd, 1, 0);
    primitive_pdftex("lpcode", assign_font_int_cmd, lp_code_base, 0);
    primitive_pdftex("rpcode", assign_font_int_cmd, rp_code_base, 0);
    primitive_pdftex("efcode", assign_font_int_cmd, ef_code_base, 0);
    primitive_pdftex("tagcode", assign_font_int_cmd, tag_code, 0);
    primitive_pdftex("pdfnoligatures", assign_font_int_cmd, no_lig_code, 0);
    primitive_tex("batchmode", set_interaction_cmd, batch_mode, 0);
    primitive_tex("nonstopmode", set_interaction_cmd, nonstop_mode, 0);
    primitive_tex("scrollmode", set_interaction_cmd, scroll_mode, 0);
    primitive_tex("errorstopmode", set_interaction_cmd, error_stop_mode, 0);
    primitive_tex("openin", in_stream_cmd, 1, 0);
    primitive_tex("closein", in_stream_cmd, 0, 0);
    primitive_tex("message", message_cmd, 0, 0);
    primitive_tex("errmessage", message_cmd, 1, 0);
    primitive_tex("lowercase", case_shift_cmd, lc_code_base, lc_code_base);
    primitive_tex("uppercase", case_shift_cmd, uc_code_base, lc_code_base);
    primitive_tex("show", xray_cmd, show_code, 0);
    primitive_tex("showbox", xray_cmd, show_box_code, 0);
    primitive_tex("showthe", xray_cmd, show_the_code, 0);
    primitive_tex("showlists", xray_cmd, show_lists, 0);

    primitive_tex("openout", extension_cmd, open_node, 0);
    primitive_tex("write", extension_cmd, write_node, 0);
    write_loc = cur_val;
    primitive_tex("closeout", extension_cmd, close_node, 0);
    primitive_tex("special", extension_cmd, special_node, 0);
    cs_text(frozen_special) = maketexstring("special");
    eqtb[frozen_special] = eqtb[cur_val];
    primitive_tex("immediate", extension_cmd, immediate_code, 0);
    primitive_omega("localinterlinepenalty", assign_int_cmd,
                    int_base + local_inter_line_penalty_code, int_base);
    primitive_omega("localbrokenpenalty", assign_int_cmd,
                    int_base + local_broken_penalty_code, int_base);
    primitive_omega("pagedir", assign_dir_cmd, int_base + page_direction_code,
                    dir_base);
    primitive_omega("bodydir", assign_dir_cmd, int_base + body_direction_code,
                    dir_base);
    primitive_omega("pardir", assign_dir_cmd, int_base + par_direction_code,
                    dir_base);
    primitive_omega("textdir", assign_dir_cmd, int_base + text_direction_code,
                    dir_base);
    primitive_omega("mathdir", assign_dir_cmd, int_base + math_direction_code,
                    dir_base);
    primitive_luatex("pageleftoffset", assign_dimen_cmd,
                     dimen_base + page_left_offset_code, dimen_base);
    primitive_luatex("pagetopoffset", assign_dimen_cmd,
                     dimen_base + page_top_offset_code, dimen_base);
    primitive_aleph("pagerightoffset", assign_dimen_cmd,
                    dimen_base + page_right_offset_code, dimen_base);
    primitive_aleph("pagebottomoffset", assign_dimen_cmd,
                    dimen_base + page_bottom_offset_code, dimen_base);
    primitive_pdftex("pdfliteral", extension_cmd, pdf_literal_node, 0);
    primitive_pdftex("pdfcolorstack", extension_cmd, pdf_colorstack_node, 0);
    primitive_pdftex("pdfsetmatrix", extension_cmd, pdf_setmatrix_node, 0);
    primitive_pdftex("pdfsave", extension_cmd, pdf_save_node, 0);
    primitive_pdftex("pdfrestore", extension_cmd, pdf_restore_node, 0);
    primitive_pdftex("pdfobj", extension_cmd, pdf_obj_code, 0);
    primitive_pdftex("pdfrefobj", extension_cmd, pdf_refobj_node, 0);
    primitive_pdftex("pdfxform", extension_cmd, pdf_xform_code, 0);
    primitive_pdftex("pdfrefxform", extension_cmd, pdf_refxform_node, 0);
    primitive_pdftex("pdfximage", extension_cmd, pdf_ximage_code, 0);
    primitive_pdftex("pdfrefximage", extension_cmd, pdf_refximage_node, 0);
    primitive_pdftex("pdfannot", extension_cmd, pdf_annot_node, 0);
    primitive_pdftex("pdfstartlink", extension_cmd, pdf_start_link_node, 0);
    primitive_pdftex("pdfendlink", extension_cmd, pdf_end_link_node, 0);
    primitive_pdftex("pdfoutline", extension_cmd, pdf_outline_code, 0);
    primitive_pdftex("pdfdest", extension_cmd, pdf_dest_node, 0);
    primitive_pdftex("pdfthread", extension_cmd, pdf_thread_node, 0);
    primitive_pdftex("pdfstartthread", extension_cmd, pdf_start_thread_node, 0);
    primitive_pdftex("pdfendthread", extension_cmd, pdf_end_thread_node, 0);
    primitive_pdftex("pdfsavepos", extension_cmd, pdf_save_pos_node, 0);
    primitive_pdftex("pdfinfo", extension_cmd, pdf_info_code, 0);
    primitive_pdftex("pdfcatalog", extension_cmd, pdf_catalog_code, 0);
    primitive_pdftex("pdfnames", extension_cmd, pdf_names_code, 0);
    primitive_pdftex("pdfincludechars", extension_cmd, pdf_include_chars_code,
                     0);
    primitive_pdftex("pdffontattr", extension_cmd, pdf_font_attr_code, 0);
    primitive_pdftex("pdfmapfile", extension_cmd, pdf_map_file_code, 0);
    primitive_pdftex("pdfmapline", extension_cmd, pdf_map_line_code, 0);
    primitive_pdftex("pdftrailer", extension_cmd, pdf_trailer_code, 0);
    primitive_luatex("savecatcodetable", extension_cmd,
                     save_cat_code_table_code, 0);
    primitive_luatex("initcatcodetable", extension_cmd,
                     init_cat_code_table_code, 0);
    primitive_pdftex("pdfsetrandomseed", extension_cmd, set_random_seed_code,
                     0);
    primitive_luatex("latelua", extension_cmd, late_lua_node, 0);
    primitive_pdftex("pdffontexpand", extension_cmd, pdf_font_expand_code, 0);
    primitive_pdftex("pdfglyphtounicode", extension_cmd,
                     pdf_glyph_to_unicode_code, 0);

    /* some of the internal integer parameters are not associated with actual
       primitives at all. */

    primitive_no("nolocalwhatsits", assign_int_cmd,
                 int_base + no_local_whatsits_code, int_base);
    primitive_no("nolocaldirs", assign_int_cmd,
                 int_base + no_local_dirs_code, int_base);



@ @c
void initialize_etex_commands(void)
{
    primitive_etex("lastnodetype", last_item_cmd, last_node_type_code, 0);
    primitive_etex("eTeXversion", last_item_cmd, eTeX_version_code, 0);
    primitive_etex("eTeXminorversion", last_item_cmd, eTeX_minor_version_code,
                   0);
    primitive_etex("eTeXrevision", convert_cmd, eTeX_revision_code, 0);

    /*  First we implement the additional \eTeX\ parameters in the table of equivalents. */

    primitive_etex("everyeof", assign_toks_cmd, every_eof_loc, local_base);
    primitive_etex("tracingassigns", assign_int_cmd,
                   int_base + tracing_assigns_code, int_base);
    primitive_etex("tracinggroups", assign_int_cmd,
                   int_base + tracing_groups_code, int_base);
    primitive_etex("tracingifs", assign_int_cmd, int_base + tracing_ifs_code,
                   int_base);
    primitive_etex("tracingscantokens", assign_int_cmd,
                   int_base + tracing_scan_tokens_code, int_base);
    primitive_etex("tracingnesting", assign_int_cmd,
                   int_base + tracing_nesting_code, int_base);
    primitive_etex("predisplaydirection", assign_int_cmd,
                   int_base + pre_display_direction_code, int_base);
    primitive_etex("lastlinefit", assign_int_cmd, int_base + last_line_fit_code,
                   int_base);
    primitive_etex("savingvdiscards", assign_int_cmd,
                   int_base + saving_vdiscards_code, int_base);
    primitive_etex("savinghyphcodes", assign_int_cmd,
                   int_base + saving_hyph_codes_code, int_base);
    primitive_luatex("suppressfontnotfounderror", assign_int_cmd,
                     int_base + suppress_fontnotfound_error_code, int_base);
    primitive_luatex("suppresslongerror", assign_int_cmd,
                     int_base + suppress_long_error_code, int_base);
    primitive_luatex("suppressmathparerror", assign_int_cmd,
                     int_base + suppress_mathpar_error_code, int_base);
    primitive_luatex("suppressifcsnameerror", assign_int_cmd,
                     int_base + suppress_ifcsname_error_code, int_base);
    primitive_luatex("suppressoutererror", assign_int_cmd,
                     int_base + suppress_outer_error_code, int_base);
    primitive_luatex("matheqnogapstep", assign_int_cmd,
                     int_base + math_eqno_gap_step_code, int_base);
    primitive_luatex("synctex", assign_int_cmd, int_base + synctex_code,
                     int_base);

    primitive_etex("currentgrouplevel", last_item_cmd, current_group_level_code,
                   0);
    primitive_etex("currentgrouptype", last_item_cmd, current_group_type_code,
                   0);

    primitive_etex("currentiflevel", last_item_cmd, current_if_level_code, 0);
    primitive_etex("currentiftype", last_item_cmd, current_if_type_code, 0);
    primitive_etex("currentifbranch", last_item_cmd, current_if_branch_code, 0);

    primitive_omega("charwd", last_item_cmd, font_char_wd_code, 0);
    primitive_etex("fontcharwd", last_item_cmd, font_char_wd_code, 0);
    primitive_omega("charht", last_item_cmd, font_char_ht_code, 0);
    primitive_etex("fontcharht", last_item_cmd, font_char_ht_code, 0);
    primitive_omega("chardp", last_item_cmd, font_char_dp_code, 0);
    primitive_etex("fontchardp", last_item_cmd, font_char_dp_code, 0);
    primitive_omega("charit", last_item_cmd, font_char_ic_code, 0);
    primitive_etex("fontcharic", last_item_cmd, font_char_ic_code, 0);

    primitive_etex("parshapelength", last_item_cmd, par_shape_length_code, 0);
    primitive_etex("parshapeindent", last_item_cmd, par_shape_indent_code, 0);
    primitive_etex("parshapedimen", last_item_cmd, par_shape_dimen_code, 0);

    primitive_etex("showgroups", xray_cmd, show_groups, 0);
    /* The \.{\\showtokens} command displays a token list. */
    primitive_etex("showtokens", xray_cmd, show_tokens, 0);
    /*
       The \.{\\unexpanded} primitive prevents expansion of tokens much as
       the result from \.{\\the} applied to a token variable.  The
       \.{\\detokenize} primitive converts a token list into a list of
       character tokens much as if the token list were written to a file.  We
       use the fact that the command modifiers for \.{\\unexpanded} and
       \.{\\detokenize} are odd whereas those for \.{\\the} and \.{\\showthe}
       are even.
     */
    primitive_etex("unexpanded", the_cmd, 1, 0);
    primitive_etex("detokenize", the_cmd, show_tokens, 0);

    /* The \.{\\showifs} command displays all currently active conditionals. */

    primitive_etex("showifs", xray_cmd, show_ifs, 0);

    /* The \.{\\interactionmode} primitive allows to query and set the interaction mode. */

    primitive_etex("interactionmode", set_page_int_cmd, 2, 0);

    /* The |scan_tokens| feature of \eTeX\ defines the \.{\\scantokens} primitive. */

    primitive_etex("scantokens", input_cmd, 2, 0);
    primitive_luatex("scantextokens", input_cmd, 3, 0);

    primitive_etex("readline", read_to_cs_cmd, 1, 0);

    primitive_etex("unless", expand_after_cmd, 1, 0);
    primitive_etex("ifdefined", if_test_cmd, if_def_code, 0);
    primitive_etex("ifcsname", if_test_cmd, if_cs_code, 0);
    primitive_etex("iffontchar", if_test_cmd, if_font_char_code, 0);
    primitive_pdftex("ifincsname", if_test_cmd, if_in_csname_code, 0);
    primitive_luatex("ifabsnum", if_test_cmd, if_abs_num_code, 0);
    primitive_luatex("ifabsdim", if_test_cmd, if_abs_dim_code, 0);
    primitive_pdftex("ifpdfabsnum", if_test_cmd, if_abs_num_code, 0);
    primitive_pdftex("ifpdfabsdim", if_test_cmd, if_abs_dim_code, 0);

    /* The |protected| feature of \eTeX\ defines the \.{\\protected} prefix
       command for macro definitions.  Such macros are protected against
       expansions when lists of expanded tokens are built, e.g., for \.{\\edef}
       or during \.{\\write}. */

    primitive_etex("protected", prefix_cmd, 8, 0);

    /* Here are the additional \eTeX\ primitives for expressions. */

    primitive_etex("numexpr", last_item_cmd,
                   eTeX_expr - int_val_level + int_val_level, 0);
    primitive_etex("dimexpr", last_item_cmd,
                   eTeX_expr - int_val_level + dimen_val_level, 0);
    primitive_etex("glueexpr", last_item_cmd,
                   eTeX_expr - int_val_level + glue_val_level, 0);
    primitive_etex("muexpr", last_item_cmd,
                   eTeX_expr - int_val_level + mu_val_level, 0);

    primitive_etex("gluestretchorder", last_item_cmd, glue_stretch_order_code,
                   0);
    primitive_etex("glueshrinkorder", last_item_cmd, glue_shrink_order_code, 0);
    primitive_etex("gluestretch", last_item_cmd, glue_stretch_code, 0);
    primitive_etex("glueshrink", last_item_cmd, glue_shrink_code, 0);

    primitive_etex("mutoglue", last_item_cmd, mu_to_glue_code, 0);
    primitive_etex("gluetomu", last_item_cmd, glue_to_mu_code, 0);

    /* The \.{\\pagediscards} and \.{\\splitdiscards} commands share the
       command code |un_vbox| with \.{\\unvbox} and \.{\\unvcopy}, they are
       distinguished by their |chr_code| values |last_box_code| and
       |vsplit_code|.  These |chr_code| values are larger than |box_code| and
       |copy_code|. */

    primitive_etex("pagediscards", un_vbox_cmd, last_box_code, 0);
    primitive_etex("splitdiscards", un_vbox_cmd, vsplit_code, 0);

    /* The \.{\\interlinepenalties}, \.{\\clubpenalties}, \.{\\widowpenalties},
       and \.{\\displaywidowpenalties} commands allow to define arrays of
       penalty values to be used instead of the corresponding single values. */
    primitive_etex("interlinepenalties", set_etex_shape_cmd,
                   inter_line_penalties_loc, etex_pen_base);
    primitive_etex("clubpenalties", set_etex_shape_cmd, club_penalties_loc,
                   etex_pen_base);
    primitive_etex("widowpenalties", set_etex_shape_cmd, widow_penalties_loc,
                   etex_pen_base);
    primitive_etex("displaywidowpenalties", set_etex_shape_cmd,
                   display_widow_penalties_loc, etex_pen_base);

}
