/* equivalents.h

   Copyright 2009 Taco Hoekwater <taco@luatex.org>

   This file is part of LuaTeX.

   LuaTeX is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   LuaTeX is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with LuaTeX; if not, see <http://www.gnu.org/licenses/>. */


#ifndef EQUIVALENTS_H
#  define EQUIVALENTS_H

/*
Like the preceding parameters, the following quantities can be changed
at compile time to extend or reduce \TeX's capacity. But if they are changed,
it is necessary to rerun the initialization program \.{INITEX}
@.INITEX@>
to generate new tables for the production \TeX\ program.
One can't simply make helter-skelter changes to the following constants,
since certain rather complex initialization
numbers are computed from them. They are defined here using
\.{WEB} macros, instead of being put into \PASCAL's |const| list, in order to
emphasize this distinction.
*/

#  define font_base 0           /* smallest internal font number; must not be less than |min_quarterword| */
#  define biggest_reg 65535     /* the largest allowed register number; must be |< max_quarterword| */
#  define number_regs 65536     /* |biggest_reg+1| */
#  define number_attrs 65536    /* total numbeer of attributes */
#  define biggest_char 1114111  /* the largest allowed character number; must be |< max_halfword| */
#  define too_big_char 1114112  /* |biggest_char+1| */
#  define special_char 1114113  /* |biggest_char+2| */
#  define number_chars 1114112  /* |biggest_char+1| */
#  define number_fonts (5535-font_base+1)
#  define biggest_lang 32767
#  define too_big_lang 32768
#  define text_size 0           /* size code for the largest size in a family */
#  define script_size 1         /* size code for the medium size in a family */
#  define script_script_size 2  /* size code for the smallest size in a family */


/*
Each entry in |eqtb| is a |memory_word|. Most of these words are of type
|two_halves|, and subdivided into three fields:

\yskip\hangg 1) The |eq_level| (a quarterword) is the level of grouping at
which this equivalent was defined. If the level is |level_zero|, the
equivalent has never been defined; |level_one| refers to the outer level
(outside of all groups), and this level is also used for global
definitions that never go away. Higher levels are for equivalents that
will disappear at the end of their group.  @^global definitions@>

\yskip\hangg 2) The |eq_type| (another quarterword) specifies what kind of
entry this is. There are many types, since each \TeX\ primitive like
\.{\\hbox}, \.{\\def}, etc., has its own special code. The list of
command codes above includes all possible settings of the |eq_type| field.

\yskip\hangg 3) The |equiv| (a halfword) is the current equivalent value.
This may be a font number, a pointer into |mem|, or a variety of other
things.
*/


/*
Many locations in |eqtb| have symbolic names. The purpose of the next
paragraphs is to define these names, and to set up the initial values of the
equivalents.

In the first region we have a single entry for the `null csname' of
length zero. In luatex, the active characters and and single-letter
control sequence names are part of the next region.

Then comes region~2, which corresponds to the hash table that we will
define later.  The maximum address in this region is used for a dummy
control sequence that is perpetually undefined. There also are several
locations for control sequences that are perpetually defined
(since they are used in error recovery).
*/



/*
@ Region 3 of |eqtb| contains the |number_regs| \.{\\skip} registers, as well as the
glue parameters defined here. It is important that the ``muskip''
parameters have larger numbers than the others.

@ Region 4 of |eqtb| contains the local quantities defined here. The
bulk of this region is taken up by five tables that are indexed by eight-bit
characters; these tables are important to both the syntactic and semantic
portions of \TeX. There are also a bunch of special things like font and
token parameters, as well as the tables of \.{\\toks} and \.{\\box}
registers.

@ Region 5 of |eqtb| contains the integer parameters and registers defined
here, as well as the |del_code| table. The latter table differs from the
|cat_code..math_code| tables that precede it, since delimiter codes are
fullword integers while the other kinds of codes occupy at most a
halfword. This is what makes region~5 different from region~4. We will
store the |eq_level| information in an auxiliary array of quarterwords
that will be defined later.

@ The integer parameters should really be initialized by a macro package;
the following initialization does the minimum to keep \TeX\ from
complete failure.
@^null delimiter@>

@ The final region of |eqtb| contains the dimension parameters defined
here, and the |number_regs| \.{\\dimen} registers.
*/

#  define null_cs 1                                                     /* equivalent of \.{\\csname\\endcsname} */
#  define hash_base (null_cs+1)                                         /* beginning of region 2, for the hash table */
#  define frozen_control_sequence (hash_base+hash_size)                 /* for error recovery */
#  define frozen_protection (frozen_control_sequence)                   /* inaccessible but definable */
#  define frozen_cr (frozen_control_sequence+1)                         /* permanent `\.{\\cr}' */
#  define frozen_end_group (frozen_control_sequence+2)                  /* permanent `\.{\\endgroup}' */
#  define frozen_right (frozen_control_sequence+3)                      /* permanent `\.{\\right}' */
#  define frozen_fi (frozen_control_sequence+4)                         /* permanent `\.{\\fi}' */
#  define frozen_end_template (frozen_control_sequence+5)               /* permanent `\.{\\endtemplate}' */
#  define frozen_endv (frozen_control_sequence+6)                       /* second permanent `\.{\\endtemplate}' */
#  define frozen_relax (frozen_control_sequence+7)                      /* permanent `\.{\\relax}' */
#  define end_write (frozen_control_sequence+8)                         /* permanent `\.{\\endwrite}' */
#  define frozen_dont_expand (frozen_control_sequence+9 )               /* permanent `\.{\\notexpanded:}' */
#  define frozen_primitive (frozen_control_sequence+11 )                /* permanent `\.{\\primitive}' */
#  define frozen_special (frozen_control_sequence+12 )                  /* permanent `\.{\\special}' */
#  define frozen_null_font (frozen_control_sequence+13 )                /* permanent `\.{\\nullfont}' */
#  define font_id_base (frozen_null_font-font_base )                    /* begins table of |number_fonts| permanent font identifiers */
#  define undefined_control_sequence (frozen_null_font+number_fonts)
#  define glue_base (undefined_control_sequence+1)                      /* beginning of region 3 */

#  define line_skip_code 0                                              /* interline glue if |baseline_skip| is infeasible */
#  define baseline_skip_code 1                                          /* desired glue between baselines */
#  define par_skip_code 2                                               /* extra glue just above a paragraph */
#  define above_display_skip_code 3                                     /* extra glue just above displayed math */
#  define below_display_skip_code 4                                     /* extra glue just below displayed math */
#  define above_display_short_skip_code 5                               /* glue above displayed math following short lines */
#  define below_display_short_skip_code 6                               /* glue below displayed math following short lines */
#  define left_skip_code 7                                              /* glue at left of justified lines */
#  define right_skip_code 8                                             /* glue at right of justified lines */
#  define top_skip_code 9                                               /* glue at top of main pages */
#  define split_top_skip_code 10                                        /* glue at top of split pages */
#  define tab_skip_code 11                                              /* glue between aligned entries */
#  define space_skip_code 12                                            /* glue between words (if not |zero_glue|) */
#  define xspace_skip_code 13                                           /* glue after sentences (if not |zero_glue|) */
#  define par_fill_skip_code 14                                         /* glue on last line of paragraph */
#  define thin_mu_skip_code 15                                          /* thin space in math formula */
#  define med_mu_skip_code 16                                           /* medium space in math formula */
#  define thick_mu_skip_code 17                                         /* thick space in math formula */
#  define math_skip_code 18
#  define glue_pars 19                                                  /* total number of glue parameters */

#  define skip_base (glue_base+glue_pars)                               /* table of |number_regs| ``skip'' registers */
#  define mu_skip_base (skip_base+number_regs)                          /* table of |number_regs| ``muskip'' registers */
#  define local_base (mu_skip_base+number_regs)                         /* beginning of region 4 */

#  define par_shape_loc (local_base)                                    /* specifies paragraph shape */
#  define output_routine_loc (local_base+1)                             /* points to token list for \.{\\output} */
#  define every_par_loc (local_base+2)                                  /* points to token list for \.{\\everypar} */
#  define every_math_loc (local_base+3)                                 /* points to token list for \.{\\everymath} */
#  define every_display_loc (local_base+4)                              /* points to token list for \.{\\everydisplay} */
#  define every_hbox_loc (local_base+5)                                 /* points to token list for \.{\\everyhbox} */
#  define every_vbox_loc (local_base+6)                                 /* points to token list for \.{\\everyvbox} */
#  define every_job_loc (local_base+7)                                  /* points to token list for \.{\\everyjob} */
#  define every_cr_loc (local_base+8)                                   /* points to token list for \.{\\everycr} */
#  define err_help_loc (local_base+9)                                   /* points to token list for \.{\\errhelp} */
#  define every_eof_loc (local_base+10)                                 /* points to token list for \.{\\everyeof} */

/* define toks_base (local_base+11) */                                  /* table of |number_regs| token list registers */

#  define backend_toks_base (local_base+11)
#  define backend_toks_last (backend_toks_base+10)

#  define toks_base (backend_toks_last+1)                               /* table of |number_regs| token list registers */

#  define etex_pen_base (toks_base+number_regs)                         /* start of table of \eTeX's penalties */
#  define inter_line_penalties_loc (etex_pen_base)                      /* additional penalties between lines */
#  define club_penalties_loc (etex_pen_base+1)                          /* penalties for creating club lines */
#  define widow_penalties_loc (etex_pen_base+2)                         /* penalties for creating widow lines */
#  define display_widow_penalties_loc (etex_pen_base+3)                 /* ditto, just before a display */
#  define etex_pens (etex_pen_base+4)                                   /* end of table of \eTeX's penalties */
#  define local_left_box_base (etex_pens)
#  define local_right_box_base (local_left_box_base+1)
#  define box_base (local_right_box_base+1)                             /* table of |number_regs| box registers */
#  define cur_font_loc (box_base+number_regs)                           /* internal font number outside math mode */
#  define internal_math_param_base (cur_font_loc+1 )                    /* current math parameter data index  */
#  define cat_code_base (internal_math_param_base+1)                    /* current category code data index  */
#  define lc_code_base (cat_code_base+1)                                /* table of |number_chars| lowercase mappings */
#  define uc_code_base (lc_code_base+1)                                 /* table of |number_chars| uppercase mappings */
#  define sf_code_base (uc_code_base+1)                                 /* table of |number_chars| spacefactor mappings */
#  define math_code_base (sf_code_base+1)                               /* table of |number_chars| math mode mappings */
#  define int_base (math_code_base+1)                                   /* beginning of region 5 */

#  define pretolerance_code 0                                           /* badness tolerance before hyphenation */
#  define tolerance_code 1                                              /* badness tolerance after hyphenation */
#  define line_penalty_code 2                                           /* added to the badness of every line */
#  define hyphen_penalty_code 3                                         /* penalty for break after discretionary hyphen */
#  define ex_hyphen_penalty_code 4                                      /* penalty for break after explicit hyphen */
#  define club_penalty_code 5                                           /* penalty for creating a club line */
#  define widow_penalty_code 6                                          /* penalty for creating a widow line */
#  define display_widow_penalty_code 7                                  /* ditto, just before a display */
#  define broken_penalty_code 8                                         /* penalty for breaking a page at a broken line */
#  define bin_op_penalty_code 9                                         /* penalty for breaking after a binary operation */
#  define rel_penalty_code 10                                           /* penalty for breaking after a relation */
#  define pre_display_penalty_code 11                                   /* penalty for breaking just before a displayed formula */
#  define post_display_penalty_code 12                                  /* penalty for breaking just after a displayed formula */
#  define inter_line_penalty_code 13                                    /* additional penalty between lines */
#  define double_hyphen_demerits_code 14                                /* demerits for double hyphen break */
#  define final_hyphen_demerits_code 15                                 /* demerits for final hyphen break */
#  define adj_demerits_code 16                                          /* demerits for adjacent incompatible lines */
#  define mag_code 17                                                   /* magnification ratio */
#  define delimiter_factor_code 18                                      /* ratio for variable-size delimiters */
#  define looseness_code 19                                             /* change in number of lines for a paragraph */
#  define time_code 20                                                  /* current time of day */
#  define day_code 21                                                   /* current day of the month */
#  define month_code 22                                                 /* current month of the year */
#  define year_code 23                                                  /* current year of our Lord */
#  define show_box_breadth_code 24                                      /* nodes per level in |show_box| */
#  define show_box_depth_code 25                                        /* maximum level in |show_box| */
#  define hbadness_code 26                                              /* hboxes exceeding this badness will be shown by |hpack| */
#  define vbadness_code 27                                              /* vboxes exceeding this badness will be shown by |vpack| */
#  define pausing_code 28                                               /* pause after each line is read from a file */
#  define tracing_online_code 29                                        /* show diagnostic output on terminal */
#  define tracing_macros_code 30                                        /* show macros as they are being expanded */
#  define tracing_stats_code 31                                         /* show memory usage if \TeX\ knows it */
#  define tracing_paragraphs_code 32                                    /* show line-break calculations */
#  define tracing_pages_code 33                                         /* show page-break calculations */
#  define tracing_output_code 34                                        /* show boxes when they are shipped out */
#  define tracing_lost_chars_code 35                                    /* show characters that aren't in the font */
#  define tracing_commands_code 36                                      /* show command codes at |big_switch| */
#  define tracing_restores_code 37                                      /* show equivalents when they are restored */
#  define uc_hyph_code 38                                               /* hyphenate words beginning with a capital letter */
#  define output_penalty_code 39                                        /* penalty found at current page break */
#  define max_dead_cycles_code 40                                       /* bound on consecutive dead cycles of output */
#  define hang_after_code 41                                            /* hanging indentation changes after this many lines */
#  define floating_penalty_code 42                                      /* penalty for insertions heldover after a split */
#  define global_defs_code 43                                           /* override \.{\\global} specifications */
#  define cur_fam_code 44                                               /* current family */
#  define escape_char_code 45                                           /* escape character for token output */
#  define default_hyphen_char_code 46                                   /* value of \.{\\hyphenchar} when a font is loaded */
#  define default_skew_char_code 47                                     /* value of \.{\\skewchar} when a font is loaded */
#  define end_line_char_code 48                                         /* character placed at the right end of the buffer */
#  define new_line_char_code 49                                         /* character that prints as |print_ln| */
#  define language_code 50                                              /* current hyphenation table */
#  define left_hyphen_min_code 51                                       /* minimum left hyphenation fragment size */
#  define right_hyphen_min_code 52                                      /* minimum right hyphenation fragment size */
#  define holding_inserts_code 53                                       /* do not remove insertion nodes from \.{\\box255} */
#  define error_context_lines_code 54                                   /* maximum intermediate line pairs shown */
#  define local_inter_line_penalty_code 55                              /* local \.{\\interlinepenalty} */
#  define local_broken_penalty_code 56                                  /* local \.{\\brokenpenalty} */
#  define no_local_whatsits_code 57                                     /* counts local whatsits */
#  define no_local_dirs_code 58

#  define luastartup_id_code 60
#  define disable_lig_code 61
#  define disable_kern_code 62
#  define cat_code_table_code 63
#  define output_box_code 64
#  define cur_lang_code 65                                              /* current language id */
#  define ex_hyphen_char_code 66
#  define hyphenation_min_code 67                                       /* minimum word length */
#  define adjust_spacing_code 68                                        /* level of spacing adjusting */
#  define protrude_chars_code 69                                        /* protrude chars at left/right edge of paragraphs */
#  define output_mode_code 70                                           /* switch on PDF output if positive */
#  define draft_mode_code 71
#  define tracing_fonts_code 72

#  define math_no_italic_compensation_code 73    /* just for tracing, can change */
#  define math_no_char_italic_code 74            /* just for tracing, can change */
#  define math_use_old_fraction_scaling_code 75  /* just for tracing, can change */
#  define math_old_code 76                       /* this one is stable */

#  define math_option_code 77

#  define etex_first_integer_code (math_option_code+1)                  /* base for \eTeX's integer parameters */

#  define tracing_assigns_code (etex_first_integer_code)                /* show assignments */
#  define tracing_groups_code (etex_first_integer_code+1)               /* show save/restore groups */
#  define tracing_ifs_code (etex_first_integer_code+2)                  /* show conditionals */
#  define tracing_scan_tokens_code (etex_first_integer_code+3)          /* show pseudo file open and close */
#  define tracing_nesting_code (etex_first_integer_code+4)              /* show incomplete groups and ifs within files */
#  define pre_display_direction_code (etex_first_integer_code+5)        /* text direction preceding a display */
#  define last_line_fit_code (etex_first_integer_code+6)                /* adjustment for last line of paragraph */
#  define saving_vdiscards_code (etex_first_integer_code+7)             /* save items discarded from vlists */
#  define saving_hyph_codes_code (etex_first_integer_code+8)            /* save hyphenation codes for languages */
#  define suppress_fontnotfound_error_code (etex_first_integer_code+9)  /* suppress errors for missing fonts */
#  define suppress_long_error_code (etex_first_integer_code+10)         /* suppress errors for missing fonts */
#  define suppress_ifcsname_error_code (etex_first_integer_code+11)     /* suppress errors for failed \.{\\ifcsname} */
#  define suppress_outer_error_code (etex_first_integer_code+12)        /* suppress errors for \.{\\outer} */
#  define suppress_mathpar_error_code (etex_first_integer_code+13)      /* suppress errors for \.{\\par}} in math */
#  define math_eqno_gap_step_code (etex_first_integer_code+14)          /* factor/1000 used for distance between eq and eqno */
#  define synctex_code (etex_first_integer_code+15)                     /* is synctex file generation enabled ?  */

/* define tex_int_pars (synctex_code+1) */                              /* total number of integer parameters */

#  define backend_int_base (synctex_code+1)
#  define backend_int_last (backend_int_base+20)

#  define tex_int_pars (backend_int_last+1)                             /* total number of integer parameters */

#  define page_direction_code (tex_int_pars)
#  define body_direction_code (tex_int_pars+1)
#  define par_direction_code (tex_int_pars+2)
#  define text_direction_code (tex_int_pars+3)
#  define math_direction_code (tex_int_pars+4)

#  define int_pars (tex_int_pars+5)                                     /* total number of integer parameters */

#  define dir_base (int_base+tex_int_pars)
#  define count_base (int_base+int_pars)                                /* |number_regs| user \.{\\count} registers */
#  define attribute_base (count_base+number_regs)                       /* |number_attrs| user \.{\\attribute} registers */
#  define del_code_base (attribute_base+number_attrs)                   /* |number_chars| delimiter code mappings */
#  define dimen_base (del_code_base+1)                                  /* beginning of region 6 */

#  define par_indent_code 0                                             /* indentation of paragraphs */
#  define math_surround_code 1                                          /* space around math in text */
#  define line_skip_limit_code 2                                        /* threshold for |line_skip| instead of |baseline_skip| */
#  define hsize_code 3                                                  /* line width in horizontal mode */
#  define vsize_code 4                                                  /* page height in vertical mode */
#  define max_depth_code 5                                              /* maximum depth of boxes on main pages */
#  define split_max_depth_code 6                                        /* maximum depth of boxes on split pages */
#  define box_max_depth_code 7                                          /* maximum depth of explicit vboxes */
#  define hfuzz_code 8                                                  /* tolerance for overfull hbox messages */
#  define vfuzz_code 9                                                  /* tolerance for overfull vbox messages */
#  define delimiter_shortfall_code 10                                   /* maximum amount uncovered by variable delimiters */
#  define null_delimiter_space_code 11                                  /* blank space in null delimiters */
#  define script_space_code 12                                          /* extra space after subscript or superscript */
#  define pre_display_size_code 13                                      /* length of text preceding a display */
#  define display_width_code 14                                         /* length of line for displayed equation */
#  define display_indent_code 15                                        /* indentation of line for displayed equation */
#  define overfull_rule_code 16                                         /* width of rule that identifies overfull hboxes */
#  define hang_indent_code 17                                           /* amount of hanging indentation */
#  define h_offset_code 18                                              /* amount of horizontal offset when shipping pages out */
#  define v_offset_code 19                                              /* amount of vertical offset when shipping pages out */
#  define emergency_stretch_code 20                                     /* reduces badnesses on final pass of line-breaking */
#  define page_left_offset_code 21
#  define page_top_offset_code 22
#  define page_right_offset_code 23
#  define page_bottom_offset_code 24
#  define px_dimen_code 25
#  define page_width_code 26                                            /* page width of the output */
#  define page_height_code 27                                           /* page height of the output */

/* define dimen_pars (page_height_code + 1) */                          /* total number of dimension parameters */

#  define backend_dimen_base (page_height_code+1)
#  define backend_dimen_last (backend_dimen_base+10)

#  define dimen_pars (backend_dimen_last + 1)                           /* total number of dimension parameters */

#  define scaled_base (dimen_base+dimen_pars)                           /* table of |number_regs| user-defined \.{\\dimen} registers */
#  define eqtb_size (scaled_base+biggest_reg)                           /* largest subscript of |eqtb| */

extern memory_word *eqtb;

extern halfword eqtb_top;                                               /* maximum of the |eqtb| */

extern quarterword xeq_level[(eqtb_size + 1)];
extern void initialize_equivalents(void);

#  define eq_level_field(A) (A).hh.b1
#  define eq_type_field(A) (A).hh.b0
#  define equiv_field(A) (A).hh.rh

#  define eq_level(A) eq_level_field(eqtb[(A)])                         /* level of definition */
#  define eq_type(A) eq_type_field(eqtb[(A)])                           /* command code for equivalent */
#  define equiv(A) equiv_field(eqtb[(A)])                               /* equivalent value */

#  define set_eq_level(A,B) eq_level((A)) = (B)
#  define set_eq_type(A,B) eq_type((A)) = (B)
#  define set_equiv(A,B) equiv((A)) = (B)

typedef struct save_record_ {
    quarterword type_;
    quarterword level_;
    memory_word word_;
} save_record;

extern save_record *save_stack;
extern int save_ptr;                                                    /* first unused entry on |save_stack| */
extern int max_save_stack;                                              /* maximum usage of save stack */
extern quarterword cur_level;                                           /* current nesting level for groups */
extern group_code cur_group;                                            /* current group type */
extern int cur_boundary;                                                /* where the current level begins */

#  define save_type(A) save_stack[(A)].type_                            /* classifies a |save_stack| entry */
#  define save_level(A) save_stack[(A)].level_                          /* saved level for regions 5 and 6, or group code */
#  define save_value(A) save_stack[(A)].word_.cint                      /* |eqtb| location or token or |save_stack| location */
#  define save_word(A) save_stack[(A)].word_                            /* |eqtb| entry */

/*
We use the notation |saved(k)| to stand for an item that
appears in location |save_ptr+k| of the save stack.
*/

#  define saved_type(A) save_stack[save_ptr+(A)].type_
#  define saved_level(A) save_stack[save_ptr+(A)].level_
#  define saved_value(A) save_stack[save_ptr+(A)].word_.cint

#  define set_saved_record(A,B,C,D) do {	\
        saved_type(A) = (quarterword)(B);   \
        saved_level(A) = (quarterword)(C);  \
        saved_value(A) = (D);               \
    } while (0)

#  define restore_old_value 0                                           /* |save_type| when a value should be restored later */
#  define restore_zero 1                                                /* |save_type| when an undefined entry should be restored */
#  define insert_token 2                                                /* |save_type| when a token is being saved for later use */
#  define level_boundary 3                                              /* |save_type| corresponding to beginning of group */
#  define saved_line 4
#  define saved_adjust 5
#  define saved_insert 6
#  define saved_disc 7
#  define saved_boxtype 8
#  define saved_textdir 9
#  define saved_eqno 10
#  define saved_choices 11
#  define saved_math 12
#  define saved_boxcontext 13
#  define saved_boxspec 14
#  define saved_boxdir 15
#  define saved_boxattr 16
#  define saved_eqtb 17

extern void print_save_stack(void);

#  define assign_trace(A,B) if (int_par(tracing_assigns_code)>0) restore_trace((A),(B))

#  define int_par(A)   eqtb[int_base+(A)].cint
#  define dimen_par(A) eqtb[dimen_base+(A)].cint
#  define loc_par(A)   equiv(local_base+(A))
#  define glue_par(A)  equiv(glue_base+(A))

extern int mag_set;             /* if nonzero, this magnification should be used henceforth */
extern void prepare_mag(void);

/*
@ Here are the group codes that are used to discriminate between different
kinds of groups. They allow \TeX\ to decide what special actions, if any,
should be performed when a group ends.
\def\grp{\.{\char'173...\char'175}}

Some groups are not supposed to be ended by right braces. For example,
the `\.\$' that begins a math formula causes a |math_shift_group| to
be started, and this should be terminated by a matching `\.\$'. Similarly,
a group that starts with \.{\\left} should end with \.{\\right}, and
one that starts with \.{\\begingroup} should end with \.{\\endgroup}.

*/

typedef enum {
    bottom_level = 0,           /* group code for the outside world */
    simple_group,               /* group code for local structure only */
    hbox_group,                 /* code for `\.{\\hbox}\grp' */
    adjusted_hbox_group,        /* code for `\.{\\hbox}\grp' in vertical mode */
    vbox_group,                 /* code for `\.{\\vbox}\grp' */
    vtop_group,                 /* code for `\.{\\vtop}\grp' */
    align_group,                /* code for `\.{\\halign}\grp', `\.{\\valign}\grp' */
    no_align_group,             /* code for `\.{\\noalign}\grp' */
    output_group,               /* code for output routine */
    math_group,                 /* code for, e.g., `\.{\char'136}\grp' */
    disc_group,                 /* code for `\.{\\discretionary}\grp\grp\grp' */
    insert_group,               /* code for `\.{\\insert}\grp', `\.{\\vadjust}\grp' */
    vcenter_group,              /* code for `\.{\\vcenter}\grp' */
    math_choice_group,          /* code for `\.{\\mathchoice}\grp\grp\grp\grp' */
    semi_simple_group,          /* code for `\.{\\begingroup...\\endgroup}' */
    math_shift_group,           /* code for `\.{\$...\$}' */
    math_left_group,            /* code for `\.{\\left...\\right}' */
    local_box_group,            /* code for `\.{\\localleftbox...\\localrightbox}' */
    split_off_group,            /* box code for the top part of a \.{\\vsplit} */
    split_keep_group,           /* box code for the bottom part of a \.{\\vsplit} */
    preamble_group,             /* box code for the preamble processing  in an alignment */
    align_set_group,            /* box code for the final item pass in an alignment */
    fin_row_group               /* box code for a provisory line in an alignment */
} tex_group_codes;

#  define max_group_code local_box_group                        /* which is wrong, but is what the web says */

extern int cur_cmd;                                             /* current command set by |get_next| */
extern halfword cur_chr;                                        /* operand of current command */
extern halfword cur_cs;                                         /* control sequence found here, zero if none found */
extern halfword cur_tok;                                        /* packed representative of |cur_cmd| and |cur_chr| */

extern void show_cur_cmd_chr(void);

extern void new_save_level(group_code c);                       /* begin a new level of grouping */
extern void eq_destroy(memory_word w);                          /* gets ready to forget |w| */
extern void eq_save(halfword p, quarterword l);                 /* saves |eqtb[p]| */
extern void eq_define(halfword p, quarterword t, halfword e);   /* new data for |eqtb| */
extern void eq_word_define(halfword p, int w);
extern void geq_define(halfword p, quarterword t, halfword e);  /* global |eq_define| */
extern void geq_word_define(halfword p, int w);                 /* global |eq_word_define| */
extern void save_for_after(halfword t);
extern void unsave(void);                                       /* pops the top level off the save stack */
extern void restore_trace(halfword p, const char *s);           /* |eqtb[p]| has just been restored or retained */
extern void show_save_groups(void);

#  define level_zero 0                                          /* level for undefined quantities */
#  define level_one 1                                           /* outermost level for defined quantities */

extern void show_eqtb(halfword n);

/* more will move here */

#define space_skip glue_par(space_skip_code)
#define xspace_skip glue_par(xspace_skip_code)
#define math_skip glue_par(math_skip_code)

#endif
