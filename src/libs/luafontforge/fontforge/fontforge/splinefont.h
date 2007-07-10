/* Copyright (C) 2000-2007 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _SPLINEFONT_H
#define _SPLINEFONT_H

#include "basics.h"
#include "configure-pfaedit.h"
#ifdef HAVE_ICONV_H
# include <iconv.h>
/* libiconv.h defines iconv as taking a const pointer for inbuf. iconv doesn't*/
# ifdef _LIBICONV_VERSION
#  define ICONV_CONST	const
# else
#  define ICONV_CONST
# endif
#else
# include <gwwiconv.h>
#  define ICONV_CONST
#endif

#if defined( FONTFORGE_CONFIG_USE_LONGDOUBLE )
# define real		long double
# define bigreal	long double
#elif defined( FONTFORGE_CONFIG_USE_DOUBLE )
# define real		double
# define bigreal	double
#else
# define real		float
# define bigreal	double
#endif

#if defined( HAVE_LONG_DOUBLE ) && defined( This_does_not_seem_to_make_a_difference )
# define extended	long double
# define CheckExtremaForSingleBitErrors(sp,t)	(t)
# define EXTENDED_IS_LONG_DOUBLE	1
#else
# define extended	double
#endif
	/* Solaris wants to define extended to be unsigned [3] unless we do this*/
#define _EXTENDED

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

#define MmMax		16	/* PS says at most this many instances for type1/2 mm fonts */
#define AppleMmMax	26	/* Apple sort of has a limit of 4095, but we only support this many */

typedef struct val {
    enum val_type { v_int, v_real, v_str, v_unicode, v_lval, v_arr, v_arrfree,
	    v_int32pt, v_int16pt, v_int8pt, v_void } type;
    union {
	int ival;
	real fval;
	char *sval;
	struct val *lval;
	struct array *aval;
	uint32 *u32ptval;
	uint16 *u16ptval;
	uint8  *u8ptval;
    } u;
} Val;		/* Used by scripting */

struct psdict {
    int cnt, next;
    char **keys;
    char **values;
};

struct pschars {
    int cnt, next;
    char **keys;
    uint8 **values;
    int *lens;
    int bias;		/* for type2 strings */
};

enum linejoin {
    lj_miter,		/* Extend lines until they meet */
    lj_round,		/* circle centered at the join of expand radius */
    lj_bevel,		/* Straight line between the ends of next and prev */
    lj_inherited
};
enum linecap {
    lc_butt,		/* equiv to lj_bevel, straight line extends from one side to other */
    lc_round,		/* semi-circle */
    lc_square,		/* Extend lines by radius, then join them */
    lc_inherited
};
#define COLOR_INHERITED	0xfffffffe
struct brush {
    uint32 col;
    /*void *pattern;*/		/* Don't know how to deal with these yet */
    /*void *gradient;*/
    float opacity;		/* number between [0,1], only for svg */
};
#define WIDTH_INHERITED	(-1)
#define DASH_INHERITED	255	/* if the dashes[0]==0 && dashes[1]==DASH_INHERITED */
#define DASH_MAX	8
typedef unsigned char DashType;
struct pen {
    struct brush brush;
    uint8 linejoin;
    uint8 linecap;
    float width;
    real trans[4];
    DashType dashes[DASH_MAX];
};

struct spline;
enum si_type { si_std, si_caligraphic, si_elipse, si_centerline };
/* If you change this structure you may need to update MakeStrokeDlg */
/*  and cvpalettes.c both contain statically initialized StrokeInfos */
typedef struct strokeinfo {
    real radius;			/* or major axis of pen */
    enum linejoin join;
    enum linecap cap;
    enum si_type stroke_type;
    unsigned int toobigwarn: 1;
    unsigned int removeinternal: 1;
    unsigned int removeexternal: 1;
    unsigned int removeoverlapifneeded: 1;
    unsigned int gottoobig: 1;
    unsigned int gottoobiglocal: 1;
    real penangle;
    real ratio;				/* ratio of minor pen axis to major */
/* For eplipse */
    real minorradius;
/* For freehand tool */
    real radius2;
    int pressure1, pressure2;
/* End freehand tool */
    bigreal c,s;
    real xoff[8], yoff[8];
    void *data;
    bigreal (*factor)(void *data,struct spline *spline,real t);
} StrokeInfo;

enum overlap_type { over_remove, over_rmselected, over_intersect, over_intersel,
	over_exclude, over_findinter, over_fisel };

enum simpify_flags { sf_cleanup=-1, sf_normal=0, sf_ignoreslopes=1,
	sf_ignoreextremum=2, sf_smoothcurves=4, sf_choosehv=8,
	sf_forcelines=0x10, sf_nearlyhvlines=0x20,
	sf_mergelines=0x40, sf_setstart2extremum=0x80,
	sf_rmsingletonpoints=0x100 };
struct simplifyinfo {
    int flags;
    bigreal err;
    bigreal tan_bounds;
    bigreal linefixup;
    bigreal linelenmax;		/* Don't simplify any straight lines longer than this */
    int set_as_default;
    int check_selected_contours;
};

typedef struct ipoint {
    int x;
    int y;
} IPoint;

typedef struct basepoint {
    real x;
    real y;
} BasePoint;

typedef struct tpoint {
    real x;
    real y;
    real t;
} TPoint;

typedef struct dbounds {
    real minx, maxx;
    real miny, maxy;
} DBounds;

typedef struct ibounds {
    int minx, maxx;
    int miny, maxy;
} IBounds;

typedef struct bluedata {
    real xheight, xheighttop;		/* height of "x" and "o" (u,v,w,x,y,z) */
    real caph, caphtop;			/* height of "I" and "O" */
    real base, basebelow;		/* bottom of "I" and "O" */
    real ascent;			/* height of "l" */
    real descent;			/* depth of "p" */
    real numh, numhtop;			/* height of "7" and "8" */ /* numbers with ascenders */
    int bluecnt;			/* If the private dica contains bluevalues... */
    real blues[12][2];			/* 7 pairs from bluevalues, 5 from otherblues */
} BlueData;

typedef struct bdffloat {
    int16 xmin,xmax,ymin,ymax;
    int16 bytes_per_line;
    unsigned int byte_data:1;
    uint8 depth;
    uint8 *bitmap;
} BDFFloat;

/* OpenType does not document 'dflt' as a language, but we'll use it anyway. */
/* (Adobe uses it too) we'll turn it into a default entry when we output it. */
#define DEFAULT_LANG		CHR('d','f','l','t')
/* The OpenType spec says in one place that the default script is 'dflt' and */
/*  in another that it is 'DFLT'. 'DFLT' is correct */
#define DEFAULT_SCRIPT		CHR('D','F','L','T')
#define REQUIRED_FEATURE	CHR(' ','R','Q','D')

enum otlookup_type {
    ot_undef = 0,			/* Not a lookup type */
    gsub_start	       = 0x000,		/* Not a lookup type */
    gsub_single        = 0x001,
    gsub_multiple      = 0x002,
    gsub_alternate     = 0x003,
    gsub_ligature      = 0x004,
    gsub_context       = 0x005,
    gsub_contextchain  = 0x006,
     /* GSUB extension 7 */
    gsub_reversecchain = 0x008,
    /* mac state machines */
    morx_indic	       = 0x0fd,
    morx_context       = 0x0fe,
    morx_insert        = 0x0ff,
    /* ********************* */
    gpos_start         = 0x100,		/* Not a lookup type */

    gpos_single        = 0x101,
    gpos_pair          = 0x102,
    gpos_cursive       = 0x103,
    gpos_mark2base     = 0x104,
    gpos_mark2ligature = 0x105,
    gpos_mark2mark     = 0x106,
    gpos_context       = 0x107,
    gpos_contextchain  = 0x108,
    /* GPOS extension 9 */
    kern_statemachine  = 0x1ff

    /* otlookup&0xff == lookup type for the appropriate table */
    /* otlookup>>8:     0=>GSUB, 1=>GPOS */
};

enum otlookup_typemasks {
    gsub_single_mask        = 0x00001,
    gsub_multiple_mask      = 0x00002,
    gsub_alternate_mask     = 0x00004,
    gsub_ligature_mask      = 0x00008,
    gsub_context_mask       = 0x00010,
    gsub_contextchain_mask  = 0x00020,
    gsub_reversecchain_mask = 0x00040,
    morx_indic_mask         = 0x00080,
    morx_context_mask       = 0x00100,
    morx_insert_mask        = 0x00200,
    /* ********************* */
    gpos_single_mask        = 0x00400,
    gpos_pair_mask          = 0x00800,
    gpos_cursive_mask       = 0x01000,
    gpos_mark2base_mask     = 0x02000,
    gpos_mark2ligature_mask = 0x04000,
    gpos_mark2mark_mask     = 0x08000,
    gpos_context_mask       = 0x10000,
    gpos_contextchain_mask  = 0x20000,
    kern_statemachine_mask  = 0x40000
};

#define MAX_LANG 		4	/* If more than this we allocate more_langs in chunks of MAX_LANG */
struct scriptlanglist {
    uint32 script;
    uint32 langs[MAX_LANG];
    uint32 *morelangs;
    int lang_cnt;
    struct scriptlanglist *next;
};

extern struct opentype_feature_friendlynames {
    uint32 tag;
    char *tagstr;
    char *friendlyname;
    int masks;
} friendlies[];

typedef struct featurescriptlanglist {
    uint32 featuretag;
    struct scriptlanglist *scripts;
    struct featurescriptlanglist *next;
    unsigned int ismac: 1;	/* treat the featuretag as a mac feature/setting */
} FeatureScriptLangList;

enum pst_flags { pst_r2l=1, pst_ignorebaseglyphs=2, pst_ignoreligatures=4,
	pst_ignorecombiningmarks=8 };

typedef struct otlookup {
    struct otlookup *next;
    enum otlookup_type lookup_type;
    uint16 lookup_flags;
    char *lookup_name;
    FeatureScriptLangList *features;
    struct lookup_subtable {
	char *subtable_name;
	char *suffix;			/* for gsub_single, used to find a default replacement */
	struct otlookup *lookup;
	unsigned int unused: 1;
	unsigned int per_glyph_pst_or_kern: 1;
	unsigned int anchor_classes: 1;
	unsigned int vertical_kerning: 1;
	unsigned int ticked: 1;
	struct kernclass *kc;
	struct generic_fpst *fpst;
	struct generic_asm  *sm;
	/* Each time an item is added to a lookup we must place it into a */
	/*  subtable. If it's a kerning class, fpst or state machine it has */
	/*  a subtable all to itself. If it's an anchor class it can share */
	/*  a subtable with other anchor classes (merge with). If it's a glyph */
	/*  PST it may share a subtable with other PSTs */
	/* Note items may only be placed in lookups in which they fit. Can't */
	/*  put kerning data in a gpos_single lookup, etc. */
	struct lookup_subtable *next;
	int32 subtable_offset;
	int32 *extra_subtables;
	/* If a kerning subtable has too much stuff in it, we are prepared to */
	/*  break it up into several smaller subtables, each of which has */
	/*  an offset in this list (extra-subtables[0]==subtable_offset) */
	/*  the list is terminated by an entry of -1 */
    } *subtables;
    unsigned int unused: 1;	/* No subtable is used (call SFFindUnusedLookups before examining) */
    unsigned int empty: 1;	/* No subtable is used, and no anchor classes are used */
    unsigned int store_in_afm: 1;	/* Used for ligatures, some get stored */
    					/*  'liga' generally does, but 'frac' doesn't */
    unsigned int needs_extension: 1;	/* Used during opentype generation */
    unsigned int temporary_kern: 1;	/* Used when decomposing kerning classes into kern pairs for older formats */
    unsigned int def_lang_checked: 1;
    unsigned int def_lang_found: 1;
    unsigned int ticked: 1;
    int16 subcnt;		/* Actual number of subtables we will output */
				/* Some of our subtables may contain no data */
			        /* Some may be too big and need to be broken up.*/
			        /* So this field may be different than just counting the subtables */
    int lookup_index;		/* used during opentype generation */
    uint32 lookup_offset;
    uint32 lookup_length;
    char *tempname;
} OTLookup;

#ifdef FONTFORGE_CONFIG_DEVICETABLES
typedef struct devicetab {
    uint16 first_pixel_size, last_pixel_size;		/* A range of point sizes to which this table applies */
    int8 *corrections;					/* a set of pixel corrections, one for each point size */
} DeviceTable;

typedef struct valdev {		/* Value records can have four associated device tables */
    DeviceTable xadjust;
    DeviceTable yadjust;
    DeviceTable xadv;
    DeviceTable yadv;
} ValDevTab;
#endif

enum anchorclass_type { act_mark, act_mkmk, act_curs, act_mklg };
typedef struct anchorclass {
    char *name;			/* in utf8 */
    struct lookup_subtable *subtable;
    uint8 type;		/* anchorclass_type */
    uint8 has_base;
    uint8 processed, has_mark, matches, ac_num;
    struct anchorclass *next;
} AnchorClass;

enum anchor_type { at_mark, at_basechar, at_baselig, at_basemark, at_centry, at_cexit, at_max };
typedef struct anchorpoint {
    AnchorClass *anchor;
    BasePoint me;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    DeviceTable xadjust, yadjust;
#endif
    unsigned int type: 4;
    unsigned int selected: 1;
    unsigned int ticked: 1;
    unsigned int has_ttf_pt: 1;
    uint16 ttf_pt_index;
    int16  lig_index;
    struct anchorpoint *next;
} AnchorPoint;

typedef struct kernpair {
    struct lookup_subtable *subtable;
    struct splinechar *sc;
    int16 off;
    uint16 kcid;			/* temporary value */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    DeviceTable *adjust;		/* Only adjustment in one dimen, if more needed use pst */
#endif
    struct kernpair *next;
} KernPair;

typedef struct kernclass {
    int first_cnt, second_cnt;		/* Count of classes for first and second chars */
    char **firsts;			/* list of a space seperated list of char names */
    char **seconds;			/*  one entry for each class. Entry 0 is null */
    					/*  and means everything not specified elsewhere */
    struct lookup_subtable *subtable;
    uint16 kcid;			/* Temporary value, used for many things briefly */
    int16 *offsets;			/* array of first_cnt*second_cnt entries */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    DeviceTable *adjusts;		/* array of first_cnt*second_cnt entries */
#endif
    struct kernclass *next;
} KernClass;

enum possub_type { pst_null, pst_position, pst_pair,
	pst_substitution, pst_alternate,
	pst_multiple, pst_ligature,
	pst_lcaret /* must be pst_max-1, see charinfo.c*/,
	pst_max,
	/* These are not psts but are related so it's handly to have values for them */
	pst_kerning = pst_max, pst_vkerning, pst_anchors,
	/* And these are fpsts */
	pst_contextpos, pst_contextsub, pst_chainpos, pst_chainsub,
	pst_reversesub, fpst_max,
	/* And these are used to specify a kerning pair where the current */
	/*  char is the final glyph rather than the initial one */
	/* A kludge used when cutting and pasting features */
	pst_kernback, pst_vkernback
	};

struct vr {
    int16 xoff, yoff, h_adv_off, v_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    ValDevTab *adjust;
#endif
};

typedef struct generic_pst {
    unsigned int ticked: 1;
    unsigned int temporary: 1;		/* Used in afm ligature closure */
    /* enum possub_type*/ uint8 type;
    struct lookup_subtable *subtable;
    struct generic_pst *next;
    union {
	struct vr pos;
	struct { char *paired; struct vr *vr; } pair;
	struct { char *variant; } subs;
	struct { char *components; } mult, alt;
	struct { char *components; struct splinechar *lig; } lig;
	struct { int16 *carets; int cnt; } lcaret;	/* Ligature caret positions */
    } u;
} PST;

typedef struct liglist {
    PST *lig;
    struct splinechar *first;		/* First component */
    struct splinecharlist *components;	/* Other than the first */
    struct liglist *next;
    int ccnt;				/* Component count. (includes first component) */
} LigList;

enum fpossub_format { pst_glyphs, pst_class, pst_coverage,
		    pst_reversecoverage, pst_formatmax };

typedef struct generic_fpst {
    uint16 /*enum sfpossub_type*/ type;
    uint16 /*enum fpossub_format*/ format;
    struct lookup_subtable *subtable;
    struct generic_fpst *next;
    uint16 nccnt, bccnt, fccnt;
    uint16 rule_cnt;
    char **nclass, **bclass, **fclass;
    struct fpst_rule {
	union {
	    struct fpg { char *names, *back, *fore; } glyph;
	    struct fpc { int ncnt, bcnt, fcnt; uint16 *nclasses, *bclasses, *fclasses, *allclasses; } class;
	    struct fpv { int ncnt, bcnt, fcnt; char **ncovers, **bcovers, **fcovers; } coverage;
	    struct fpr { int always1, bcnt, fcnt; char **ncovers, **bcovers, **fcovers; char *replacements; } rcoverage;
	} u;
	int lookup_cnt;
	struct seqlookup {
	    int seq;
	    struct otlookup *lookup;
	} *lookups;
    } *rules;
    uint8 ticked;
    uint8 effectively_by_glyphs;
} FPST;

enum asm_type { asm_indic, asm_context, asm_lig, asm_simple=4, asm_insert,
	asm_kern=0x11 };
enum asm_flags { asm_vert=0x8000, asm_descending=0x4000, asm_always=0x2000 };

typedef struct generic_asm {		/* Apple State Machine */
    struct generic_asm *next;
    uint16 /*enum asm_type*/ type;
    struct lookup_subtable *subtable;	/* Lookup contains feature setting info */
    uint16 flags;	/* 0x8000=>vert, 0x4000=>r2l, 0x2000=>hor&vert */
    uint8 ticked;

    uint16 class_cnt, state_cnt;
    char **classes;
    struct asm_state {
	uint16 next_state;
	uint16 flags;
	union {
	    struct {
		struct otlookup *mark_lookup;	/* for contextual glyph subs (tag of a nested lookup) */
		struct otlookup *cur_lookup;	/* for contextual glyph subs */
	    } context;
	    struct {
		char *mark_ins;
		char *cur_ins;
	    } insert;
	    struct {
		int16 *kerns;
		int kcnt;
	    } kern;
	} u;
    } *state;
  uint32 opentype_tag;		/* If converted from opentype */
} ASM;
/* State Flags:
 Indic:
	0x8000	mark current glyph as first in rearrangement
	0x4000	don't advance to next glyph
	0x2000	mark current glyph as last
	0x000f	verb
		0 = no change		8 = AxCD => CDxA
		1 = Ax => xA		9 = AxCD => DCxA
		2 = xD => Dx		a = ABxD => DxAB
		3 = AxD => DxA		b = ABxD => DxBA
		4 = ABx => xAB		c = ABxCD => CDxAB
		5 = ABx => xBA		d = ABxCD => CDxBA
		6 = xCD => CDx		e = ABxCD => DCxAB
		7 = xCD => DCx		f = ABxCD => DCxBA
 Contextual:
	0x8000	mark current glyph
	0x4000	don't advance to next glyph
 Insert:
	0x8000	mark current glyph
	0x4000	don't advance to next glyph
	0x2000	current is Kashida like
	0x1000	mark is Kashida like
	0x0800	current insert before
	0x0400	mark insert before
	0x03e0	count of chars to be inserted at current (31 max)
	0x001f	count of chars to be inserted at mark (31 max)
 Kern:
	0x8000	add current glyph to kerning stack
	0x4000	don't advance to next glyph
	0x3fff	value offset
*/

struct opentype_str {
    struct splinechar *sc;
    struct vr vr;
    struct kernpair *kp;
    struct kernclass *kc;
    int kc_index;
    int16 lig_pos;		/* when skipping marks to form a ligature keep track of what ligature element a mark was attached to */
    int16 context_pos;		/* When doing a contextual match remember which glyphs are used, and where in the match they occur. Skipped glyphs have -1 */
    int16 orig_index;
};

struct macname {
    struct macname *next;
    uint16 enc;		/* Platform specific encoding. 0=>mac roman, 1=>sjis, 7=>russian */
    uint16 lang;	/* Mac languages 0=>english, 1=>french, 2=>german */
    char *name;		/* Not a unicode string, uninterpreted mac encoded string */
};

/* Wow, the GPOS 'size' feature stores a string in the name table just as mac */
/*  features do */
struct otfname {
    struct otfname *next;
    uint16 lang;	/* windows language code */
    char *name;		/* utf8 */
};

typedef struct macfeat {
    struct macfeat *next;
    uint16 feature;
    uint8 ismutex;
    uint8 default_setting;		/* Apple's docs say both that this is a byte and a short. It's a byte */
    uint16 strid;			/* Temporary value, used when reading in */
    struct macname *featname;
    struct macsetting {
	struct macsetting *next;
	uint16 setting;
	uint16 strid;
	struct macname *setname;
	unsigned int initially_enabled: 1;
    } *settings;
} MacFeat;

typedef struct undoes {
    struct undoes *next;
    enum undotype { ut_none=0, ut_state, ut_tstate, ut_statehint, ut_statename,
	    ut_statelookup,
	    ut_anchors,
	    ut_width, ut_vwidth, ut_lbearing, ut_rbearing, ut_possub,
	    ut_hints, ut_bitmap, ut_bitmapsel, ut_composit, ut_multiple, ut_layers,
	    ut_noop } undotype;
    unsigned int was_modified: 1;
    unsigned int was_order2: 1;
    union {
	struct {
	    int16 width, vwidth;
	    int16 lbearingchange;
	    int unicodeenc;			/* only for ut_statename */
	    char *charname;			/* only for ut_statename */
	    char *comment;			/* in utf8 */
	    PST *possub;			/* only for ut_statename */
	    struct splinepointlist *splines;
	    struct refchar *refs;
	    struct minimumdistance *md;
	    
	    struct imagelist *images;
	    void *hints;			/* ut_statehint, ut_statename */
	    uint8 *instrs;
	    int instrs_len;
	    AnchorPoint *anchor;
#ifdef FONTFORGE_CONFIG_TYPE3
	    struct brush fill_brush;
	    struct pen stroke_pen;
	    unsigned int dofill: 1;
	    unsigned int dostroke: 1;
	    unsigned int fillfirst: 1;
#endif
	} state;
	int width;	/* used by both ut_width and ut_vwidth */
	int lbearing;	/* used by ut_lbearing */
	int rbearing;	/* used by ut_rbearing */
	struct {
	    int16 width;	/* width should be controled by postscript, but people don't like that */
	    int16 xmin,xmax,ymin,ymax;
	    int16 bytes_per_line;
	    int16 pixelsize;
	    int16 depth;
	    uint8 *bitmap;
	    BDFFloat *selection;
	} bmpstate;
	struct {		/* copy contains an outline state and a set of bitmap states */
	    struct undoes *state;
	    struct undoes *bitmaps;
	} composit;
	struct {
	    struct undoes *mult; /* copy contains several sub copies (composits, or states or widths or...) */
		/* Also used for ut_layers, each sub copy is a state (first is ly_fore, next ly_fore+1...) */
	} multiple;
	struct {
	    enum possub_type pst;
	    char **data;		/* First 4 bytes is tag, then space then data */
	    struct undoes *more_pst;
	    short cnt,max;		/* Not always set */
	} possub;
	uint8 *bitmap;
    } u;
    struct splinefont *copied_from;
} Undoes;

typedef struct bdfchar {
    struct splinechar *sc;
    int16 xmin,xmax,ymin,ymax;
    int16 width;
    int16 bytes_per_line;
    uint8 *bitmap;
    int orig_pos;
    struct bitmapview *views;
    Undoes *undoes;
    Undoes *redoes;
    unsigned int changed: 1;
    unsigned int byte_data: 1;		/* for anti-aliased chars entries are grey-scale bytes not bw bits */
    unsigned int widthgroup: 1;		/* for ttf bitmap output */
    unsigned int isreference: 1;	/* for ttf bitmap input, */
    unsigned int ticked: 1;
    uint8 depth;			/* for ttf bitmap output */
    uint16 vwidth;
    BDFFloat *selection;
} BDFChar;

typedef struct enc {
    char *enc_name;
    int char_cnt;	/* Size of the next two arrays */
    int32 *unicode;	/* unicode value for each encoding point */
    char **psnames;	/* optional postscript name for each encoding point */
    struct enc *next;
    unsigned int builtin: 1;
    unsigned int hidden: 1;
    unsigned int only_1byte: 1;
    unsigned int has_1byte: 1;
    unsigned int has_2byte: 1;
    unsigned int is_unicodebmp: 1;
    unsigned int is_unicodefull: 1;
    unsigned int is_custom: 1;
    unsigned int is_original: 1;
    unsigned int is_compact: 1;
    unsigned int is_japanese: 1;
    unsigned int is_korean: 1;
    unsigned int is_tradchinese: 1;
    unsigned int is_simplechinese: 1;
    char iso_2022_escape[8];
    int iso_2022_escape_len;
    int low_page, high_page;
    char *iconv_name;	/* For compatibility to old versions we might use a different name from that used by iconv. */
    iconv_t *tounicode;
    iconv_t *fromunicode;
    int (*tounicode_func)(int);
    int (*fromunicode_func)(int);
    unsigned int is_temporary: 1;	/* freed when the map gets freed */
    int char_max;			/* Used by temporary encodings */
} Encoding;

typedef struct namelist {
    struct namelist *basedon;
    char *title;
    const char ***unicode[17];
    struct namelist *next;
    struct renames { char *from; char *to; } *renames;
    int uses_unicode;
    char *a_utf8_name;
} NameList;

enum uni_interp { ui_unset= -1, ui_none, ui_adobe, ui_greek, ui_japanese,
	ui_trad_chinese, ui_simp_chinese, ui_korean, ui_ams };

struct remap { uint32 firstenc, lastenc; int32 infont; };

typedef struct encmap {		/* A per-font map of encoding to glyph id */
    int32 *map;			/* Map from encoding to glyphid */
    int32 *backmap;		/* Map from glyphid to encoding */
    int enccount;		/* used size of the map array */
    				/*  strictly speaking this might include */
			        /*  glyphs that are not encoded, but which */
			        /*  are displayed after the proper encoding */
    int encmax;			/* allocated size of the map array */
    int backmax;		/* allocated size of the backmap array */
    struct remap *remap;
    Encoding *enc;
    unsigned int ticked: 1;
} EncMap;

enum property_type { prt_string, prt_atom, prt_int, prt_uint, prt_property=0x10 };

typedef struct bdfprops {
    char *name;		/* These include both properties (like SLANT) and non-properties (like FONT) */
    int type;
    union {
	char *str;
	char *atom;
	int val;
    } u;
} BDFProperties;

typedef struct bdffont {
    struct splinefont *sf;
    int glyphcnt, glyphmax;	/* used & allocated sizes of glyphs array */
    BDFChar **glyphs;		/* an array of charcnt entries */
    int16 pixelsize;
    int16 ascent, descent;
    unsigned int piecemeal: 1;
    unsigned int bbsized: 1;
    unsigned int ticked: 1;
    struct bdffont *next;
    struct clut *clut;
    char *foundry;
    int res;
    void *freetype_context;
    uint16 truesize;		/* for bbsized fonts */
    int16 prop_cnt;
    int16 prop_max;		/* only used within bdfinfo dlg */
    BDFProperties *props;
} BDFFont;

#define HntMax	96		/* PS says at most 96 hints */
typedef uint8 HintMask[HntMax/8];

enum pointtype { pt_curve, pt_corner, pt_tangent };
typedef struct splinepoint {
    BasePoint me;
    BasePoint nextcp;		/* control point */
    BasePoint prevcp;		/* control point */
    unsigned int nonextcp:1;
    unsigned int noprevcp:1;
    unsigned int nextcpdef:1;
    unsigned int prevcpdef:1;
    unsigned int selected:1;	/* for UI */
    unsigned int pointtype:2;
    unsigned int isintersection: 1;
    unsigned int flexy: 1;
    unsigned int flexx: 1;
    unsigned int roundx: 1;	/* For true type hinting */
    unsigned int roundy: 1;	/* For true type hinting */
    unsigned int dontinterpolate: 1;	/* in ttf, don't imply point by interpolating between cps */
    unsigned int ticked: 1;
    unsigned int watched: 1;
	/* 1 bits left... */
    uint16 ptindex;		/* Temporary value used by metafont routine */
    uint16 ttfindex;		/* Truetype point index */
	/* Special values 0xffff => point implied by averaging control points */
	/*		  0xfffe => point created with no real number yet */
	/* (or perhaps point in context where no number is possible as in a glyph with points & refs) */
    uint16 nextcpindex;		/* Truetype point index */
    struct spline *next;
    struct spline *prev;
    HintMask *hintmask;
} SplinePoint;

typedef struct linelist {
    IPoint here;
    struct linelist *next;
    /* The first two fields are constant for the linelist, the next ones */
    /*  refer to a particular screen. If some portion of the line from */
    /*  this point to the next one is on the screen then set cvli_onscreen */
    /*  if this point needs to be clipped then set cvli_clipped */
    /*  asend and asstart are the actual screen locations where this point */
    /*  intersects the clip edge. */
    enum { cvli_onscreen=0x1, cvli_clipped=0x2 } flags;
    IPoint asend, asstart;
} LineList;

typedef struct linearapprox {
    real scale;
    unsigned int oneline: 1;
    unsigned int onepoint: 1;
    unsigned int any: 1;		/* refers to a particular screen */
    struct linelist *lines;
    struct linearapprox *next;
} LinearApprox;

typedef struct spline1d {
    real a, b, c, d;
} Spline1D;

typedef struct spline {
    unsigned int islinear: 1;		/* No control points */
    unsigned int isquadratic: 1;	/* probably read in from ttf */
    unsigned int isticked: 1;
    unsigned int isneeded: 1;		/* Used in remove overlap */
    unsigned int isunneeded: 1;		/* Used in remove overlap */
    unsigned int exclude: 1;		/* Used in remove overlap varient: exclude */
    unsigned int ishorvert: 1;
    unsigned int knowncurved: 1;	/* We know that it curves */
    unsigned int knownlinear: 1;	/* it might have control points, but still traces out a line */
	/* If neither knownlinear nor curved then we haven't checked */
    unsigned int order2: 1;		/* It's a bezier curve with only one cp */
    unsigned int touched: 1;
    unsigned int leftedge: 1;
    unsigned int rightedge: 1;
    SplinePoint *from, *to;
    Spline1D splines[2];		/* splines[0] is the x spline, splines[1] is y */
    struct linearapprox *approx;
    /* Posible optimizations:
	Precalculate bounding box
	Precalculate min/max/ points of inflection
    */
} Spline;

typedef struct splinepointlist {
    SplinePoint *first, *last;
    struct splinepointlist *next;
} SplinePointList, SplineSet;

typedef struct imagelist {
    struct gimage *image;
    real xoff, yoff;		/* position in character space of upper left corner of image */
    real xscale, yscale;	/* scale to convert one pixel of image to one unit of character space */
    DBounds bb;
    struct imagelist *next;
    unsigned int selected: 1;
} ImageList;

typedef struct refchar {
    unsigned int checked: 1;
    unsigned int selected: 1;
    unsigned int point_match: 1;	/* match_pt* are point indexes */
					/*  and need to be converted to a */
			                /*  translation after truetype readin */
    unsigned int encoded: 1;		/* orig_pos is actually an encoded value, used for old sfd files */
    unsigned int justtranslated: 1;	/* The transformation matrix specifies a translation (or is identity) */
    unsigned int use_my_metrics: 1;	/* Retain the ttf "use_my_metrics" info. */
	/* important for glyphs with instructions which change the width used */
	/* inside composites */
    unsigned int round_translation_to_grid: 1;	/* Retain the ttf "round_to_grid" info. */
    unsigned int point_match_out_of_date: 1;	/* Someone has edited a base glyph */
    int16 adobe_enc;
    int orig_pos;
    int unicode_enc;		/* used by paste */
    real transform[6];		/* transformation matrix (first 2 rows of a 3x3 matrix, missing row is 0,0,1) */
#ifdef FONTFORGE_CONFIG_TYPE3
    struct reflayer {
	struct brush fill_brush;
	struct pen stroke_pen;
	unsigned int dofill: 1;
	unsigned int dostroke: 1;
	unsigned int fillfirst: 1;
	SplinePointList *splines;
	ImageList *images;
    } *layers;
#else
    struct reflayer {
	SplinePointList *splines;
    } layers[1];
#endif
    int layer_cnt;
    struct refchar *next;
    DBounds bb;
    struct splinechar *sc;
    BasePoint top;
    uint16 match_pt_base, match_pt_ref;
} RefChar;

/* Some stems may appear, disappear, reapear several times */
/* Serif stems on I which appear at 0, disappear, reappear at top */
/* Or the major vertical stems on H which disappear at the cross bar */
typedef struct hintinstance {
    real begin;			/* location in the non-major direction*/
    real end;				/* width/height in non-major direction*/
    unsigned int closed: 1;
    short int counternumber;
    struct hintinstance *next;
} HintInstance;

enum hinttypes { ht_unspecified=0, ht_h, ht_v, ht_d };
typedef real _MMArray[2][MmMax];

typedef struct steminfo {
    struct steminfo *next;
    unsigned int hinttype: 2;	/* Only used by undoes */
    unsigned int ghost: 1;	/* this is a ghost stem hint. As such truetype should ignore it, type2 output should negate it, and type1 should use as is */
		    /* stored width will be either 20 or 21 */
		    /* Type2 says: -20 is "width" of top edge, -21 is "width" of bottom edge, type1 accepts either */
    unsigned int haspointleft:1;
    unsigned int haspointright:1;
    unsigned int hasconflicts:1;/* Does this stem have conflicts within its cluster? */
    unsigned int used: 1;	/* Temporary for counter hints or hint substitution */
    unsigned int tobeused: 1;	/* Temporary for counter hints or hint substitution */
    unsigned int active: 1;	/* Currently active hint in Review Hints dlg */
				/*  displayed differently in char display */
    unsigned int enddone: 1;	/* Used by ttf instructing, indicates a prev */
				/*  hint had the same end as this one (so */
			        /*  the points on the end line have been */
			        /*  instructed already */
    unsigned int startdone: 1;	/* Used by ttf instructing */
    /*unsigned int backwards: 1;*/	/* If we think this hint is better done with a negative width */
    unsigned int reordered: 1;	/* In AutoHinting. Means we changed the start of the hint, need to test for out of order */
    unsigned int pendingpt: 1;	/* A pending stem creation, not a true stem */
    unsigned int linearedges: 1;/* If we have a nice rectangle then we aren't */
				/*  interested in the orientation which is */
			        /*  wider than long */
    unsigned int bigsteminfo: 1;/* See following structure */
    int16 hintnumber;		/* when dumping out hintmasks we need to know */
				/*  what bit to set for this hint */
    union {
	int mask;		/* Mask of all references that use this hint */
				/*  in type2 output */
	_MMArray *unblended /*[2][MmMax]*/;	/* Used when reading in type1 mm hints */
    } u;
    real start;			/* location at which the stem starts */
    real width;			/* or height */
    HintInstance *where;	/* location(s) in the other coord */
} StemInfo;

typedef struct pointlist { struct pointlist *next; SplinePoint *sp; } PointList;
typedef struct bigsteminfo {
    StemInfo s;
    PointList *left, *right;
} BigStemInfo;
    
typedef struct dsteminfo {
    struct dsteminfo *next;	/* First two fields match those in steminfo */
    unsigned int hinttype: 2;	/* Only used by undoes */
    unsigned int used: 1;	/* used only by tottf.c:gendinstrs, metafont.c to mark a hint that has been dealt with */
    unsigned int bigsteminfo: 1;/* See following structure */
    BasePoint leftedgetop, leftedgebottom, rightedgetop, rightedgebottom;	/* this order is important in tottf.c: DStemInteresect */
} DStemInfo;

typedef struct bigdsteminfo {
    DStemInfo s;
    PointList *left, *right;
} BigDStemInfo;

typedef struct minimumdistance {
    /* If either point is NULL it will be assumed to mean either the origin */
    /*  or the width point (depending on which is closer). This allows user */
    /*  to control metrics... */
    SplinePoint *sp1, *sp2;
    unsigned int x: 1;
    unsigned int done: 1;
    struct minimumdistance *next;
} MinimumDistance;

typedef struct layer /* : reflayer */{
#ifdef FONTFORGE_CONFIG_TYPE3
    struct brush fill_brush;
    struct pen stroke_pen;
    unsigned int dofill: 1;
    unsigned int dostroke: 1;
    unsigned int fillfirst: 1;
#endif
    SplinePointList *splines;
    ImageList *images;			/* Only in background or type3 layer(s) */
    RefChar *refs;			/* Only in foreground layer(s) */
    Undoes *undoes;
    Undoes *redoes;
} Layer;

enum layer_type { ly_grid= -1, ly_back=0, ly_fore=1 /* Possibly other foreground layers for multi-layered things */ };

typedef struct splinechar {
    char *name;
    int unicodeenc;
    int orig_pos;		/* Original position in the glyph list */
    int16 width, vwidth;
#ifdef LUA_FF_LIB
    int16 xmin, ymin, xmax, ymax;
#endif
    int16 lsidebearing;		/* only used when reading in a type1 font */
				/*  Or an otf font where it is the subr number of a refered character */
			        /*  or a ttf font with vert metrics where it is the ymax value */
			        /*  or when generating morx where it is the mask of tables in which the glyph occurs */
				/* Always a temporary value */
    int ttf_glyph;		/* only used when writing out a ttf or otf font */
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer *layers;		/* layer[0] is background, layer[1-n] foreground */
#else
    Layer layers[2];		/* layer[0] is background, layer[1] foreground */
#endif
    int layer_cnt;
    StemInfo *hstem;		/* hstem hints have a vertical offset but run horizontally */
    StemInfo *vstem;		/* vstem hints have a horizontal offset but run vertically */
    DStemInfo *dstem;		/* diagonal hints for ttf */
    MinimumDistance *md;
    struct charview *views;
    struct charinfo *charinfo;
    struct splinefont *parent;
    unsigned int changed: 1;
    unsigned int changedsincelasthinted: 1;
    unsigned int manualhints: 1;
    unsigned int ticked: 1;	/* For reference character processing */
				/* And fontview processing */
    unsigned int changed_since_autosave: 1;
    unsigned int widthset: 1;	/* needed so an emspace char doesn't disappear */
    unsigned int vconflicts: 1;	/* Any hint overlaps in the vstem list? */
    unsigned int hconflicts: 1;	/* Any hint overlaps in the hstem list? */
    unsigned int anyflexes: 1;
    unsigned int searcherdummy: 1;
    unsigned int changed_since_search: 1;
    unsigned int wasopen: 1;
    unsigned int namechanged: 1;
    unsigned int blended: 1;	/* An MM blended character */
    unsigned int unused_so_far: 1;
    unsigned int glyph_class: 3; /* 0=> fontforge determines class automagically, else one more than the class value in gdef */
    unsigned int numberpointsbackards: 1;
    unsigned int instructions_out_of_date: 1;
    unsigned int complained_about_ptnums: 1;
    /* 10 bits left (one more if we ignore compositionunit below) */
#if HANYANG
    unsigned int compositionunit: 1;
    int16 jamo, varient;
#endif
    struct splinecharlist { struct splinechar *sc; struct splinecharlist *next;} *dependents;
	    /* The dependents list is a list of all characters which refenence*/
	    /*  the current character directly */
    KernPair *kerns;
    KernPair *vkerns;
    PST *possub;		/* If we are a ligature then this tells us what */
				/*  It may also contain a bunch of other stuff now */
    LigList *ligofme;		/* If this is the first character of a ligature then this gives us the list of possible ones */
				/*  this field must be regenerated before the font is saved */
    char *comment;			/* in utf8 */
    uint32 /*Color*/ color;
    AnchorPoint *anchor;
    uint8 *ttf_instrs;
    int16 ttf_instrs_len;
    int16 countermask_cnt;
    HintMask *countermasks;
    int16 tex_height, tex_depth;
    int16 tex_sub_pos, tex_super_pos;	/* Only for math fonts */
    struct altuni { struct altuni *next; int unienc; } *altuni;
#ifndef _NO_PYTHON
    void *python_sc_object;
    void *python_data;
#endif
} SplineChar;

#define TEX_UNDEF 0x7fff

enum ttfnames { ttf_copyright=0, ttf_family, ttf_subfamily, ttf_uniqueid,
    ttf_fullname, ttf_version, ttf_postscriptname, ttf_trademark,
    ttf_manufacturer, ttf_designer, ttf_descriptor, ttf_venderurl,
    ttf_designerurl, ttf_license, ttf_licenseurl, ttf_idontknow/*reserved*/,
    ttf_preffamilyname, ttf_prefmodifiers, ttf_compatfull, ttf_sampletext,
    ttf_cidfindfontname, ttf_namemax };
struct ttflangname {
    int lang;
    char *names[ttf_namemax];			/* in utf8 */
    int frommac[(ttf_namemax+31)/32];		/* Used when parsing the 'name' table */
    struct ttflangname *next;
};

enum backedup_state { bs_dontknow=0, bs_not=1, bs_backedup=2 };
typedef struct splinefont {
    char *fontname, *fullname, *familyname, *weight;
    char *copyright;
    char *filename;				/* sfd name. NULL if we open a font, that's origname */
    char *defbasefilename;
    char *version;
    real italicangle, upos, uwidth;		/* In font info */
#ifdef LUA_FF_LIB
    int units_per_em;
#endif
    int ascent, descent;
    int vertical_origin;			/* height of vertical origin in character coordinate system */
    int uniqueid;				/* Not copied when reading in!!!! */
    int glyphcnt, glyphmax;			/* allocated size of glyphs array */
    SplineChar **glyphs;
    unsigned int changed: 1;
    unsigned int changed_since_autosave: 1;
    unsigned int changed_since_xuidchanged: 1;
    unsigned int display_antialias: 1;
    unsigned int display_bbsized: 1;
    unsigned int dotlesswarn: 1;		/* User warned that font doesn't have a dotless i character */
    unsigned int onlybitmaps: 1;		/* it's a bdf editor, not a postscript editor */
    unsigned int serifcheck: 1;			/* Have we checked to see if we have serifs? */
    unsigned int issans: 1;			/* We have no serifs */
    unsigned int isserif: 1;			/* We have serifs. If neither set then we don't know. */
    unsigned int hasvmetrics: 1;		/* We've got vertical metric data and should output vhea/vmtx/VORG tables */
    unsigned int loading_cid_map: 1;
    unsigned int dupnamewarn: 1;		/* Warn about duplicate names when loading bdf font */
    unsigned int encodingchanged: 1;		/* Font's encoding has changed since it was loaded */
    unsigned int order2: 1;			/* Font's data are order 2 bezier splines (truetype) rather than order 3 (postscript) */
    unsigned int multilayer: 1;			/* only applies if TYPE3 is set, means this font can contain strokes & fills */
						/*  I leave it in so as to avoid cluttering up code with #ifdefs */
    unsigned int strokedfont: 1;
    unsigned int new: 1;			/* A new and unsaved font */
    unsigned int compacted: 1;			/* only used when opening a font */
    unsigned int backedup: 2;			/* 0=>don't know, 1=>no, 2=>yes */
    unsigned int use_typo_metrics: 1;		/* The standard says to. But MS */
    						/* seems to feel that isn't good */
			                        /* enough and has created a bit */
			                        /* to mean "really use them" */
    unsigned int weight_width_slope_only: 1;	/* This bit seems stupid to me */
    unsigned int save_to_dir: 1;		/* Loaded from an sfdir collection rather than a simple sfd file */
    unsigned int head_optimized_for_cleartype: 1;/* Bit in the 'head' flags field, if unset "East Asian fonts in the Windows Presentation Framework (Avalon) will not be hinted" */
    unsigned int ticked: 1;
	/* 6 bits left */
    struct fontview *fv;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    struct metricsview *metrics;
#else
    void *metrics;
#endif
    enum uni_interp uni_interp;
    NameList *for_new_glyphs;
    EncMap *map;		/* only used when opening a font to provide original default encoding */
    Layer grid;
    BDFFont *bitmaps;
    char *origname;		/* filename of font file (ie. if not an sfd) */
    char *autosavename;
    int display_size;		/* a val <0 => Generate our own images from splines, a value >0 => find a bdf font of that size */
    struct psdict *private;	/* read in from type1 file or provided by user */
    char *xuid;
    struct pfminfo {		/* A misnomer now. OS/2 info would be more accurate, but that's stuff in here from all over ttf files */
	unsigned int pfmset: 1;
	unsigned int winascent_add: 1;
	unsigned int windescent_add: 1;
	unsigned int hheadascent_add: 1;
	unsigned int hheaddescent_add: 1;
	unsigned int typoascent_add: 1;
	unsigned int typodescent_add: 1;
	unsigned int subsuper_set: 1;
	unsigned int panose_set: 1;
	unsigned int hheadset: 1;
	unsigned int vheadset: 1;
	unsigned char pfmfamily;
	int16 weight;
	int16 width;
	char panose[10];
	int16 fstype;
	int16 linegap;		/* from hhea */
	int16 vlinegap;		/* from vhea */
	int16 hhead_ascent, hhead_descent;
	int16 os2_typoascent, os2_typodescent, os2_typolinegap;
	int16 os2_winascent, os2_windescent;
	int16 os2_subxsize, os2_subysize, os2_subxoff, os2_subyoff;
	int16 os2_supxsize, os2_supysize, os2_supxoff, os2_supyoff;
	int16 os2_strikeysize, os2_strikeypos;
	char os2_vendor[4];
	int16 os2_family_class;
#ifdef LUA_FF_LIB
        uint16 avgwidth;
        uint16 firstchar;
        uint16 lastchar;
        int16 os2_xheight;
        int16 os2_capheight;
        uint16 os2_defaultchar;
        uint16 os2_breakchar;
#endif
    } pfminfo;
    struct ttflangname *names;
    char *cidregistry, *ordering;
    int supplement;
    int subfontcnt;
    struct splinefont **subfonts;
    struct splinefont *cidmaster;		/* Top level cid font */
    float cidversion;
#if HANYANG
    struct compositionrules *rules;
#endif
    char *comments;
    int tempuniqueid;
    int top_enc;
    uint16 desired_row_cnt, desired_col_cnt;
    struct glyphnamehash *glyphnames;
    struct ttf_table {
	uint32 tag;
	int32 len, maxlen;
	uint8 *data;
	struct ttf_table *next;
	FILE *temp;	/* Temporary storage used during generation */
    } *ttf_tables, *ttf_tab_saved;
	/* We copy: fpgm, prep, cvt, maxp */
    struct instrdata *instr_dlgs;	/* Pointer to all table and character instruction dlgs in this font */
    struct shortview *cvt_dlg;
    struct kernclasslistdlg *kcld, *vkcld;
    struct kernclassdlg *kcd;
    struct texdata {
	enum { tex_unset, tex_text, tex_math, tex_mathext } type;
	int32 params[22];		/* param[6] has different meanings in normal and math fonts */
    } texdata;
    OTLookup *gsub_lookups, *gpos_lookups;
    /* Apple morx subtables become gsub, and kern subtables become gpos */
    AnchorClass *anchor;
    KernClass *kerns, *vkerns;
    FPST *possub;
    ASM *sm;				/* asm is a keyword */
    MacFeat *features;
    char *chosenname;			/* Set for files with multiple fonts in them */
    struct mmset *mm;			/* If part of a multiple master set */
    int16 macstyle;
    char *fondname;			/* For use in generating mac families */
    /* from the GPOS 'size' feature. design_size, etc. are measured in tenths of a point */
    /*  bottom is exclusive, top is inclusive */
    /*  if any field is 0, it is undefined. All may be undefined, All may be */
    /*  defined, or design_size may be defined without any of the others */
    /*  but we can't define the range without defining the other junk */
    /*  Name must contain an English language name, may contain others */
    uint16 design_size;
    uint16 fontstyle_id;
    struct otfname *fontstyle_name;
    uint16 design_range_bottom, design_range_top;
    real strokewidth;
/* For GDEF Mark Attachment Class -- used in lookup flags */
/* As usual, class 0 is unused */
    int mark_class_cnt;
    char **mark_classes;		/* glyph name list */
    char **mark_class_names;		/* used within ff, utf8 (the name we've given to this class of marks) */
#ifdef _HAS_LONGLONG
    long long creationtime;		/* seconds since 1970 */
    long long modificationtime;
#else
    long creationtime;
    long modificationtime;
#endif
    short os2_version;			/* 0 means default rather than the real version 0 */
    short compression;			/* If we opened a compressed sfd file, then save it out compressed too */
    short gasp_version;			/* 0/1 currently */
    short gasp_cnt;
    struct gasp {
	uint16 ppem;
	uint16 flags;
    } *gasp;
    uint8 sfd_version;			/* Used only when reading in an sfd file */
    struct gfi_data *fontinfo;
} SplineFont;

/* I am going to simplify my life and not encourage intermediate designs */
/*  this means I can easily calculate ConvertDesignVector, and don't have */
/*  to bother the user with specifying it. */
/* (NormalizeDesignVector is fairly basic and shouldn't need user help ever) */
/*  (As long as they want piecewise linear) */
/* I'm not going to support intermediate designs at all for apple var tables */
typedef struct mmset {
    int axis_count;
    char *axes[4];
    int instance_count;
    SplineFont **instances;
    SplineFont *normal;
    real *positions;	/* array[instance][axis] saying where each instance lies on each axis */
    real *defweights;	/* array[instance] saying how much of each instance makes the normal font */
			/* for adobe */
    struct axismap {
	int points;	/* size of the next two arrays */
	real *blends;	/* between [0,1] ordered so that blend[0]<blend[1]<... */
	real *designs;	/* between the design ranges for this axis, typically [1,999] or [6,72] */
	real min, def, max;		/* For mac */
	struct macname *axisnames;	/* For mac */
    } *axismaps;	/* array[axis] */
    char *cdv, *ndv;	/* for adobe */
    int named_instance_count;
    struct named_instance {	/* For mac */
	real *coords;	/* array[axis], these are in user units */
	struct macname *names;
    } *named_instances;
    unsigned int changed: 1;
    unsigned int apple: 1;
} MMSet;

/* mac styles. Useful idea we'll just steal it */
enum style_flags { sf_bold = 1, sf_italic = 2, sf_underline = 4, sf_outline = 8,
	sf_shadow = 0x10, sf_condense = 0x20, sf_extend = 0x40 };

struct sflist {
    SplineFont *sf;
    int32 *sizes;
    FILE *tempttf;		/* For ttf */
    int id;			/* For ttf */
    int* ids;			/* One for each size */
    BDFFont **bdfs;		/* Ditto */
    EncMap *map;
    struct sflist *next;
    char **former_names;
};

    /* Used for drawing text with mark to base anchors */
typedef struct anchorpos {
    SplineChar *sc;		/* This is the mark being positioned */
    int x,y;			/* Its origin should be shifted this much relative to that of the original base char */
    AnchorPoint *apm;		/* The anchor point in sc used to position it */
    AnchorPoint *apb;		/* The anchor point in the base character against which we are positioned */
    int base_index;		/* Index in this array to the base character (-1=> original base char) */
    unsigned int ticked: 1;	/* Used as a mark to mark */
} AnchorPos;

enum ttf_flags { ttf_flag_shortps = 1, ttf_flag_nohints = 2,
		    ttf_flag_applemode=4,
		    ttf_flag_pfed_comments=8, ttf_flag_pfed_colors=0x10,
		    ttf_flag_otmode=0x20,
		    ttf_flag_glyphmap=0x40,
		    ttf_flag_TeXtable=0x80,
		    ttf_flag_ofm=0x100,
		    ttf_flag_oldkern=0x200,	/* never set in conjunction with applemode */
		    ttf_flag_brokensize=0x400	/* Adobe originally issued fonts with a bug in the size feature. They now claim (Aug 2006) that this has been fixed. Legacy programs will do the wrong thing with the fixed feature though */
		};
enum openflags { of_fstypepermitted=1, of_askcmap=2 };
enum ps_flags { ps_flag_nohintsubs = 0x10000, ps_flag_noflex=0x20000,
		    ps_flag_nohints = 0x40000, ps_flag_restrict256=0x80000,
		    ps_flag_afm = 0x100000, ps_flag_pfm = 0x200000,
		    ps_flag_tfm = 0x400000,
		    ps_flag_round = 0x800000,
/* CFF fonts are wrapped up in some postscript sugar -- unless they are to */
/*  go into a pdf file or an otf font */
		    ps_flag_nocffsugar = 0x1000000,
/* in type42 cid fonts we sometimes want an identity map from gid to cid */
		    ps_flag_identitycidmap = 0x2000000,
		    ps_flag_afmwithmarks = 0x4000000,
		    ps_flag_noseac = 0x8000000,
		    ps_flag_mask = (ps_flag_nohintsubs|ps_flag_noflex|
			ps_flag_afm|ps_flag_pfm|ps_flag_tfm|ps_flag_round)
		};

struct compressors { char *ext, *decomp, *recomp; };

struct fontdict;
struct pschars;
struct findsel;
struct charprocs;
struct enc;

extern void *chunkalloc(int size);
extern void chunkfree(void *, int size);

extern char *strconcat(const char *str, const char *str2);
extern char *strconcat3(const char *str, const char *str2, const char *str3);

extern char *XUIDFromFD(int xuid[20]);
extern SplineFont *SplineFontFromPSFont(struct fontdict *fd);
extern int CheckAfmOfPostscript(SplineFont *sf,char *psname,EncMap *map);
extern int LoadKerningDataFromAmfm(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromAfm(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromTfm(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromOfm(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromPfm(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromMacFOND(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromMetricsFile(SplineFont *sf, char *filename, EncMap *map);
extern void FeatDumpFontLookups(FILE *out,SplineFont *sf);
extern void FeatDumpOneLookup(FILE *out,SplineFont *sf, OTLookup *otl);
extern void SFApplyFeatureFile(SplineFont *sf,FILE *file,char *filename);
extern void SFApplyFeatureFilename(SplineFont *sf,char *filename);
extern void SubsNew(SplineChar *to,enum possub_type type,int tag,char *components,
	    SplineChar *default_script);
extern void PosNew(SplineChar *to,int tag,int dx, int dy, int dh, int dv);
extern int SFOneWidth(SplineFont *sf);
extern int CIDOneWidth(SplineFont *sf);
extern int SFOneHeight(SplineFont *sf);
extern int SFIsCJK(SplineFont *sf,EncMap *map);
extern void CIDMasterAsDes(SplineFont *sf);
enum fontformat { ff_pfa, ff_pfb, ff_pfbmacbin, ff_multiple, ff_mma, ff_mmb,
	ff_ptype3, ff_ptype0, ff_cid, ff_cff, ff_cffcid,
	ff_type42, ff_type42cid,
	ff_ttf, ff_ttfsym, ff_ttfmacbin, ff_ttfdfont, ff_otf, ff_otfdfont,
	ff_otfcid, ff_otfciddfont, ff_svg, ff_ufo, ff_none };
extern struct pschars *SplineFont2ChrsSubrs(SplineFont *sf, int iscjk,
	struct pschars *subrs,int flags,enum fontformat format);
extern int CanonicalCombiner(int uni);
struct cidbytes;
struct fd2data;
struct ttfinfo;
struct alltabs;
struct growbuf;
struct glyphdata;
extern int UnitsParallel(BasePoint *u1,BasePoint *u2);
extern int CvtPsStem3(struct growbuf *gb, SplineChar *scs[MmMax], int instance_count,
	int ishstem, int round);
extern struct pschars *CID2ChrsSubrs(SplineFont *cidmaster,struct cidbytes *cidbytes,int flags);
extern struct pschars *SplineFont2ChrsSubrs2(SplineFont *sf, int nomwid,
	int defwid, const int *bygid, int cnt, int flags,
	struct pschars **_subrs);
extern struct pschars *CID2ChrsSubrs2(SplineFont *cidmaster,struct fd2data *fds,
	int flags, struct pschars **_glbls);
enum bitmapformat { bf_bdf, bf_ttf, bf_sfnt_dfont, bf_sfnt_ms, bf_otb,
	bf_nfntmacbin, /*bf_nfntdfont, */bf_fon, bf_fnt, bf_palm,
	bf_ptype3,
	bf_none };
extern const char *GetAuthor(void);
extern SplineChar *SFFindExistingCharMac(SplineFont *,EncMap *map, int unienc);
extern void SC_PSDump(void (*dumpchar)(int ch,void *data), void *data,
	SplineChar *sc, int refs_to_splines, int pdfopers );
extern int _WritePSFont(FILE *out,SplineFont *sf,enum fontformat format,int flags,EncMap *enc,SplineFont *fullsf);
extern int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format,int flags,EncMap *enc,SplineFont *fullsf);
extern int WriteMacPSFont(char *fontname,SplineFont *sf,enum fontformat format,
	int flags,EncMap *enc);
extern int _WriteTTFFont(FILE *ttf,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc);
extern int WriteTTFFont(char *fontname,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc);
extern int _WriteType42SFNTS(FILE *type42,SplineFont *sf,enum fontformat format,
	int flags,EncMap *enc);
extern int WriteMacTTFFont(char *fontname,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc);
extern int WriteMacBitmaps(char *filename,SplineFont *sf, int32 *sizes,
	int is_dfont,EncMap *enc);
extern int WritePalmBitmaps(char *filename,SplineFont *sf, int32 *sizes,EncMap *enc);
extern int WriteMacFamily(char *filename,struct sflist *sfs,enum fontformat format,
	enum bitmapformat bf,int flags,EncMap *enc);
extern long mactime(void);
extern int WriteSVGFont(char *fontname,SplineFont *sf,enum fontformat format,int flags,EncMap *enc);
extern int WriteUFOFont(char *fontname,SplineFont *sf,enum fontformat format,int flags,EncMap *enc);
extern void SfListFree(struct sflist *sfs);
extern void TTF_PSDupsDefault(SplineFont *sf);
extern void DefaultTTFEnglishNames(struct ttflangname *dummy, SplineFont *sf);
extern void TeXDefaultParams(SplineFont *sf);
extern void OS2FigureCodePages(SplineFont *sf, uint32 CodePage[2]);
extern void SFDefaultOS2Info(struct pfminfo *pfminfo,SplineFont *sf,char *fontname);
extern void SFDefaultOS2Simple(struct pfminfo *pfminfo,SplineFont *sf);
extern void SFDefaultOS2SubSuper(struct pfminfo *pfminfo,int emsize,double italicangle);
extern void VerifyLanguages(SplineFont *sf);
extern int ScriptIsRightToLeft(uint32 script);
extern uint32 ScriptFromUnicode(int u,SplineFont *sf);
extern uint32 SCScriptFromUnicode(SplineChar *sc);
extern int SCRightToLeft(SplineChar *sc);
extern int SLIContainsR2L(SplineFont *sf,int sli);
extern void SFFindNearTop(SplineFont *);
extern void SFRestoreNearTop(SplineFont *);
extern int SFForceEncoding(SplineFont *sf,EncMap *old,Encoding *new_map);
extern int CountOfEncoding(Encoding *encoding_name);
extern void SFMatchGlyphs(SplineFont *sf,SplineFont *target,int addempties);
extern void MMMatchGlyphs(MMSet *mm);
extern char *_GetModifiers(char *fontname, char *familyname,char *weight);
extern char *SFGetModifiers(SplineFont *sf);
extern void SFSetFontName(SplineFont *sf, char *family, char *mods, char *full);
extern void ttfdumpbitmap(SplineFont *sf,struct alltabs *at,int32 *sizes);
extern void ttfdumpbitmapscaling(SplineFont *sf,struct alltabs *at,int32 *sizes);

extern int RealNear(real a,real b);
extern int RealNearish(real a,real b);
extern int RealApprox(real a,real b);
extern int RealWithin(real a,real b,real fudge);
extern int RealRatio(real a,real b,real fudge);

extern void LineListFree(LineList *ll);
extern void LinearApproxFree(LinearApprox *la);
extern void SplineFree(Spline *spline);
extern SplinePoint *SplinePointCreate(real x, real y);
extern void SplinePointFree(SplinePoint *sp);
extern void SplinePointMDFree(SplineChar *sc,SplinePoint *sp);
extern void SplinePointsFree(SplinePointList *spl);
extern void SplinePointListFree(SplinePointList *spl);
extern void SplinePointListMDFree(SplineChar *sc,SplinePointList *spl);
extern void SplinePointListsMDFree(SplineChar *sc,SplinePointList *spl);
extern void SplinePointListsFree(SplinePointList *head);
extern void RefCharFree(RefChar *ref);
extern void RefCharsFree(RefChar *ref);
extern void RefCharsFreeRef(RefChar *ref);
extern void CopyBufferFree(void);
extern void CopyBufferClearCopiedFrom(SplineFont *dying);
extern void UndoesFree(Undoes *undo);
extern void StemInfosFree(StemInfo *h);
extern void StemInfoFree(StemInfo *h);
extern void DStemInfosFree(DStemInfo *h);
extern void DStemInfoFree(DStemInfo *h);
extern void KernPairsFree(KernPair *kp);
extern void SCOrderAP(SplineChar *sc);
extern void AnchorPointsFree(AnchorPoint *ap);
extern AnchorPoint *AnchorPointsCopy(AnchorPoint *alist);
extern void SFRemoveAnchorClass(SplineFont *sf,AnchorClass *an);
extern int AnchorClassesNextMerge(AnchorClass *ac);
extern int IsAnchorClassUsed(SplineChar *sc,AnchorClass *an);
extern AnchorPoint *APAnchorClassMerge(AnchorPoint *anchors,AnchorClass *into,AnchorClass *from);
extern void AnchorClassMerge(SplineFont *sf,AnchorClass *into,AnchorClass *from);
extern void AnchorClassesFree(AnchorClass *kp);
extern void TtfTablesFree(struct ttf_table *tab);
extern AnchorClass *AnchorClassMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorClass *restrict_, AnchorPoint **_ap1,AnchorPoint **_ap2 );
extern AnchorClass *AnchorClassMkMkMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorPoint **_ap1,AnchorPoint **_ap2 );
extern AnchorClass *AnchorClassCursMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorPoint **_ap1,AnchorPoint **_ap2 );
extern void SCInsertPST(SplineChar *sc,PST *new);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
extern void ValDevFree(ValDevTab *adjust);
extern ValDevTab *ValDevTabCopy(ValDevTab *orig);
extern void DeviceTableFree(DeviceTable *adjust);
extern DeviceTable *DeviceTableCopy(DeviceTable *orig);
extern void DeviceTableSet(DeviceTable *adjust, int size, int correction);
#endif
extern void PSTFree(PST *lig);
extern uint16 PSTDefaultFlags(enum possub_type type,SplineChar *sc );
extern int PSTContains(const char *components,const char *name);
extern StemInfo *StemInfoCopy(StemInfo *h);
extern DStemInfo *DStemInfoCopy(DStemInfo *h);
extern MinimumDistance *MinimumDistanceCopy(MinimumDistance *h);
struct sfmergecontext {
    SplineFont *sf_from, *sf_to;
    int lcnt;
    struct lookup_cvt { OTLookup *from, *to; int old;} *lks;
    int scnt;
    struct sub_cvt { struct lookup_subtable *from, *to; int old;} *subs;
    int acnt;
    struct ac_cvt { AnchorClass *from, *to; int old;} *acs;
    char *prefix;
};
extern PST *PSTCopy(PST *base,SplineChar *sc,struct sfmergecontext *mc);
extern struct lookup_subtable *MCConvertSubtable(struct sfmergecontext *mc,struct lookup_subtable *sub);
extern AnchorClass *MCConvertAnchorClass(struct sfmergecontext *mc,AnchorClass *ac);
extern void SFFinishMergeContext(struct sfmergecontext *mc);
extern SplineChar *SplineCharCopy(SplineChar *sc,SplineFont *into,struct sfmergecontext *);
extern BDFChar *BDFCharCopy(BDFChar *bc);
extern void BitmapsCopy(SplineFont *to, SplineFont *from, int to_index, int from_index );
extern struct gimage *ImageAlterClut(struct gimage *image);
extern void ImageListsFree(ImageList *imgs);
extern void TTFLangNamesFree(struct ttflangname *l);
extern void AltUniFree(struct altuni *altuni);
extern void AltUniFigure(SplineFont *sf,EncMap *map);
extern void AltUniRemove(SplineChar *sc,int uni);
extern void AltUniAdd(SplineChar *sc,int uni);
extern void MinimumDistancesFree(MinimumDistance *md);
extern void LayerDefault(Layer *);
extern SplineChar *SplineCharCreate(void);
extern RefChar *RefCharCreate(void);
extern void SCAddRef(SplineChar *sc,SplineChar *rsc,real xoff, real yoff);
extern void _SCAddRef(SplineChar *sc,SplineChar *rsc,real transform[6]);
extern KernClass *KernClassCopy(KernClass *kc);
extern void KernClassFreeContents(KernClass *kc);
extern void KernClassListFree(KernClass *kc);
extern int KernClassContains(KernClass *kc, char *name1, char *name2, int ordered );
extern void OTLookupFree(OTLookup *lookup);
extern void OTLookupListFree(OTLookup *lookup );
extern FPST *FPSTCopy(FPST *fpst);
extern void FPSTFree(FPST *fpst);
extern void ASMFree(ASM *sm);
extern struct macname *MacNameCopy(struct macname *mn);
extern void MacNameListFree(struct macname *mn);
extern void MacSettingListFree(struct macsetting *ms);
extern void MacFeatListFree(MacFeat *mf);
extern void SplineCharListsFree(struct splinecharlist *dlist);
extern void SplineCharFreeContents(SplineChar *sc);
extern void SplineCharFree(SplineChar *sc);
extern void EncMapFree(EncMap *map);
extern EncMap *EncMapFromEncoding(SplineFont *sf,Encoding *enc);
extern EncMap *CompactEncMap(EncMap *map, SplineFont *sf);
extern EncMap *EncMapNew(int encmax, int backmax, Encoding *enc);
extern EncMap *EncMap1to1(int enccount);
extern EncMap *EncMapCopy(EncMap *map);
extern void SFExpandGlyphCount(SplineFont *sf, int newcnt);
extern void ScriptLangListFree(struct scriptlanglist *sl);
extern void FeatureScriptLangListFree(FeatureScriptLangList *fl);
extern void SplineFontFree(SplineFont *sf);
extern void OtfNameListFree(struct otfname *on);
extern void MarkClassFree(int cnt,char **classes,char **names);
extern void MMSetFreeContents(MMSet *mm);
extern void MMSetFree(MMSet *mm);
extern void SFRemoveUndoes(SplineFont *sf,uint8 *selected,EncMap *map);
extern void SplineRefigure3(Spline *spline);
extern void SplineRefigure(Spline *spline);
extern Spline *SplineMake3(SplinePoint *from, SplinePoint *to);
extern LinearApprox *SplineApproximate(Spline *spline, real scale);
extern int SplinePointListIsClockwise(const SplineSet *spl);
extern void SplineSetFindBounds(const SplinePointList *spl, DBounds *bounds);
extern void SplineCharFindBounds(SplineChar *sc,DBounds *bounds);
extern void SplineFontFindBounds(SplineFont *sf,DBounds *bounds);
extern void CIDFindBounds(SplineFont *sf,DBounds *bounds);
extern void SplineSetQuickBounds(SplineSet *ss,DBounds *b);
extern void SplineCharQuickBounds(SplineChar *sc, DBounds *b);
extern void SplineSetQuickConservativeBounds(SplineSet *ss,DBounds *b);
extern void SplineCharQuickConservativeBounds(SplineChar *sc, DBounds *b);
extern void SplineFontQuickConservativeBounds(SplineFont *sf,DBounds *b);
extern void SplinePointCatagorize(SplinePoint *sp);
extern int SplinePointIsACorner(SplinePoint *sp);
extern void SPLCatagorizePoints(SplinePointList *spl);
extern void SCCatagorizePoints(SplineChar *sc);
extern SplinePointList *SplinePointListCopy1(const SplinePointList *spl);
extern SplinePointList *SplinePointListCopy(const SplinePointList *base);
extern SplinePointList *SplinePointListCopySelected(SplinePointList *base);
extern ImageList *ImageListCopy(ImageList *cimg);
extern ImageList *ImageListTransform(ImageList *cimg,real transform[6]);
extern void ApTransform(AnchorPoint *ap, real transform[6]);
extern SplinePointList *SplinePointListTransform(SplinePointList *base, real transform[6], int allpoints );
extern SplinePointList *SplinePointListShift(SplinePointList *base, real xoff, int allpoints );
extern HintMask *HintMaskFromTransformedRef(RefChar *ref,BasePoint *trans,
	SplineChar *basesc,HintMask *hm);
extern SplinePointList *SPLCopyTranslatedHintMasks(SplinePointList *base,
	SplineChar *basesc, SplineChar *subsc, BasePoint *trans);
extern SplinePointList *SPLCopyTransformedHintMasks(RefChar *r,
	SplineChar *basesc, BasePoint *trans);
extern SplinePointList *SplinePointListRemoveSelected(SplineChar *sc,SplinePointList *base);
extern void SplinePointListSet(SplinePointList *tobase, SplinePointList *frombase);
extern void SplinePointListSelect(SplinePointList *spl,int sel);
extern void SCRefToSplines(SplineChar *sc,RefChar *rf);
extern void RefCharFindBounds(RefChar *rf);
extern void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf);
extern void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc);
extern void SFReinstanciateRefs(SplineFont *sf);
extern void SFInstanciateRefs(SplineFont *sf);
extern SplineChar *MakeDupRef(SplineChar *base, int local_enc, int uni_enc);
extern void SCRemoveDependent(SplineChar *dependent,RefChar *rf);
extern void SCRemoveLayerDependents(SplineChar *dependent,int layer);
extern void SCRemoveDependents(SplineChar *dependent);
extern int SCDependsOnSC(SplineChar *parent, SplineChar *child);
extern void BCCompressBitmap(BDFChar *bdfc);
extern void BCRegularizeBitmap(BDFChar *bdfc);
extern void BCRegularizeGreymap(BDFChar *bdfc);
extern void BCPasteInto(BDFChar *bc,BDFChar *rbc,int ixoff,int iyoff, int invert, int cleartoo);
extern void BCRotateCharForVert(BDFChar *bc,BDFChar *from, BDFFont *frombdf);
extern BDFChar *SplineCharRasterize(SplineChar *sc, double pixelsize);
extern BDFFont *SplineFontToBDFHeader(SplineFont *_sf, int pixelsize, int indicate);
extern BDFFont *SplineFontRasterize(SplineFont *sf, int pixelsize, int indicate);
extern void BDFCAntiAlias(BDFChar *bc, int linear_scale);
extern BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize,int linear_scale);
extern BDFFont *SplineFontAntiAlias(SplineFont *sf, int pixelsize,int linear_scale);
extern struct clut *_BDFClut(int linear_scale);
extern void BDFClut(BDFFont *bdf, int linear_scale);
extern int BDFDepth(BDFFont *bdf);
extern BDFChar *BDFPieceMeal(BDFFont *bdf, int index);
enum piecemeal_flags { pf_antialias=1, pf_bbsized=2 };
extern BDFFont *SplineFontPieceMeal(SplineFont *sf,int pixelsize,int flags,void *freetype_context);
extern void BDFCharFindBounds(BDFChar *bc,IBounds *bb);
extern BDFFont *BitmapFontScaleTo(BDFFont *old, int to);
extern void BDFCharFree(BDFChar *bdfc);
extern void BDFPropsFree(BDFFont *bdf);
extern void BDFFontFree(BDFFont *bdf);
extern void SFDefaultAscent(SplineFont *sf);
extern int  PSBitmapDump(char *filename,BDFFont *font, EncMap *map);
extern int  BDFFontDump(char *filename,BDFFont *font, EncMap *map, int res);
extern int  FNTFontDump(char *filename,BDFFont *font, EncMap *map, int res);
extern int  FONFontDump(char *filename,SplineFont *sf, int32 *sizes,int res,
	EncMap *map);
extern void SFReplaceEncodingBDFProps(SplineFont *sf,EncMap *map);
extern void SFReplaceFontnameBDFProps(SplineFont *sf);
extern int  IsUnsignedBDFKey(char *key);
extern int  BdfPropHasInt(BDFFont *font,const char *key, int def );
extern void BDFDefaultProps(BDFFont *bdf, EncMap *map, int res);
struct xlfd_components {
    char foundry[80];
    char family[100];
    char weight[80];
    char slant[40];
    char setwidth[50];
    char add_style[50];
    int pixel_size;
    int point_size;
    int res_x;
    int res_y;
    char spacing[40];
    int avg_width;
    char cs_reg[80];		/* encoding */
    char cs_enc[80];		/* encoding version? */
    int char_cnt;
};
extern void XLFD_GetComponents(char *xlfd,struct xlfd_components *comp);
extern void XLFD_CreateComponents(BDFFont *bdf,EncMap *map,int res,struct xlfd_components *comp);
/* Two lines intersect in at most 1 point */
/* Two quadratics intersect in at most 4 points */
/* Two cubics intersect in at most 9 points */ /* Plus an extra space for a trailing -1 */
extern int SplinesIntersect(const Spline *s1, const Spline *s2, BasePoint pts[9],
	extended t1s[10], extended t2s[10]);
extern SplineSet *LayerAllSplines(Layer *layer);
extern SplineSet *LayerUnAllSplines(Layer *layer);
extern int SplineSetIntersect(SplineSet *spl, Spline **_spline, Spline **_spline2 );
extern int LineTangentToSplineThroughPt(Spline *s, BasePoint *pt, extended ts[4],
	extended tmin, extended tmax);
extern int CubicSolve(const Spline1D *sp,extended ts[3]);
extern extended IterateSplineSolve(const Spline1D *sp, extended tmin, extended tmax, extended sought_y, double err);
extern extended SplineSolve(const Spline1D *sp, real tmin, real tmax, extended sought_y, real err);
extern int SplineSolveFull(const Spline1D *sp,extended val, extended ts[3]);
extern void SplineFindExtrema(const Spline1D *sp, extended *_t1, extended *_t2 );

#define CURVATURE_ERROR	-1e9
extern double SplineCurvature(Spline *s, double t);

#ifndef EXTENDED_IS_LONG_DOUBLE
extern double CheckExtremaForSingleBitErrors(const Spline1D *sp, double t);
#define esqrt(d)	sqrt(d)
#else
extern extended esqrt(extended e);
#endif
extern int Spline2DFindExtrema(const Spline *sp, extended extrema[4] );
extern int Spline2DFindPointsOfInflection(const Spline *sp, extended poi[2] );
extern int SplineAtInflection(Spline1D *sp, double t );
extern int SplineAtMinMax(Spline1D *sp, double t );
extern void SplineRemoveExtremaTooClose(Spline1D *sp, extended *_t1, extended *_t2 );
extern int NearSpline(struct findsel *fs, Spline *spline);
extern real SplineNearPoint(Spline *spline, BasePoint *bp, real fudge);
extern void SCMakeDependent(SplineChar *dependent,SplineChar *base);
extern SplinePoint *SplineBisect(Spline *spline, extended t);
extern Spline *SplineSplit(Spline *spline, extended ts[3]);
extern Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt,int order2);
extern Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt,int order2);
extern double SplineLength(Spline *spline);
extern int SplineIsLinear(Spline *spline);
extern int SplineIsLinearMake(Spline *spline);
extern int SplineInSplineSet(Spline *spline, SplineSet *spl);
extern int SSPointWithin(SplineSet *spl,BasePoint *pt);
extern SplineSet *SSRemoveZeroLengthSplines(SplineSet *base);
extern void SSRemoveStupidControlPoints(SplineSet *base);
extern void SSOverlapClusterCpAngles(SplineSet *base,double within);
extern void SplinesRemoveBetween(SplineChar *sc, SplinePoint *from, SplinePoint *to,int type);
extern void SplineCharMerge(SplineChar *sc,SplineSet **head,int type);
extern void SPLNearlyHvCps(SplineChar *sc,SplineSet *ss,double err);
extern void SPLNearlyHvLines(SplineChar *sc,SplineSet *ss,double err);
extern int  SPLNearlyLines(SplineChar *sc,SplineSet *ss,double err);
extern int SPInterpolate(SplinePoint *sp);
extern void SplinePointListSimplify(SplineChar *sc,SplinePointList *spl,
	struct simplifyinfo *smpl);
extern SplineSet *SplineCharSimplify(SplineChar *sc,SplineSet *head,
	struct simplifyinfo *smpl);
extern void SPLStartToLeftmost(SplineChar *sc,SplinePointList *spl, int *changed);
extern void SPLsStartToLeftmost(SplineChar *sc);
extern void CanonicalContours(SplineChar *sc);
extern SplineSet *SplineSetJoin(SplineSet *start,int doall,real fudge,int *changed);
enum ae_type { ae_all, ae_between_selected, ae_only_good, ae_only_good_rm_later };
extern Spline *SplineAddExtrema(Spline *s,int always,real lenbound,
	real offsetbound,DBounds *b);
extern void SplineSetAddExtrema(SplineChar *sc,SplineSet *ss,enum ae_type between_selected, int emsize);
extern void SplineCharAddExtrema(SplineChar *sc,SplineSet *head,enum ae_type between_selected,int emsize);
extern SplineSet *SplineCharRemoveTiny(SplineChar *sc,SplineSet *head);
extern SplineFont *SplineFontNew(void);
extern char *GetNextUntitledName(void);
extern SplineFont *SplineFontEmpty(void);
extern SplineFont *SplineFontBlank(int charcnt);
extern void SFIncrementXUID(SplineFont *sf);
extern void SFRandomChangeXUID(SplineFont *sf);
extern SplineSet *SplineSetReverse(SplineSet *spl);
extern SplineSet *SplineSetsExtractOpen(SplineSet **tbase);
extern void SplineSetsInsertOpen(SplineSet **tbase,SplineSet *open);
extern SplineSet *SplineSetsCorrect(SplineSet *base,int *changed);
extern SplineSet *SplineSetsAntiCorrect(SplineSet *base);
extern SplineSet *SplineSetsDetectDir(SplineSet **_base, int *lastscan);
extern void SPAverageCps(SplinePoint *sp);
extern void SPLAverageCps(SplinePointList *spl);
extern void SPWeightedAverageCps(SplinePoint *sp);
extern void SplineCharDefaultPrevCP(SplinePoint *base);
extern void SplineCharDefaultNextCP(SplinePoint *base);
extern void SplineCharTangentNextCP(SplinePoint *sp);
extern void SplineCharTangentPrevCP(SplinePoint *sp);
extern void SPSmoothJoint(SplinePoint *sp);
extern int PointListIsSelected(SplinePointList *spl);
extern void SplineSetsUntick(SplineSet *spl);
extern void SFOrderBitmapList(SplineFont *sf);
extern int KernThreshold(SplineFont *sf, int cnt);
extern real SFGuessItalicAngle(SplineFont *sf);
extern void SFHasSerifs(SplineFont *sf);

extern SplinePoint *SplineTtfApprox(Spline *ps);
extern SplineSet *SSttfApprox(SplineSet *ss);
extern SplineSet *SplineSetsTTFApprox(SplineSet *ss);
extern SplineSet *SSPSApprox(SplineSet *ss);
extern SplineSet *SplineSetsPSApprox(SplineSet *ss);
extern SplineSet *SplineSetsConvertOrder(SplineSet *ss, int to_order2);
extern void SplineRefigure2(Spline *spline);
extern void SplineRefigureFixup(Spline *spline);
extern Spline *SplineMake2(SplinePoint *from, SplinePoint *to);
extern Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2);
extern Spline *SFSplineMake(SplineFont *sf,SplinePoint *from, SplinePoint *to);
extern void SCConvertToOrder2(SplineChar *sc);
extern void SFConvertToOrder2(SplineFont *sf);
extern void SCConvertToOrder3(SplineChar *sc);
extern void SFConvertToOrder3(SplineFont *sf);
extern void SCConvertOrder(SplineChar *sc, int to_order2);
extern void SplinePointPrevCPChanged2(SplinePoint *sp);
extern void SplinePointNextCPChanged2(SplinePoint *sp);
extern int IntersectLines(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2);
extern int IntersectLinesClip(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2);

#if 0
extern void SSBisectTurners(SplineSet *spl);
#endif
extern void SSRemoveBacktracks(SplineSet *ss);
extern SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc);
extern SplineSet *SSStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc);
extern SplineSet *SplineSetRemoveOverlap(SplineChar *sc,SplineSet *base,enum overlap_type);

extern double BlueScaleFigure(struct psdict *private,real bluevalues[], real otherblues[]);
extern void FindBlues( SplineFont *sf, real blues[14], real otherblues[10]);
extern void QuickBlues(SplineFont *sf, BlueData *bd);
extern void FindHStems( SplineFont *sf, real snaps[12], real cnt[12]);
extern void FindVStems( SplineFont *sf, real snaps[12], real cnt[12]);
extern int SplineCharIsFlexible(SplineChar *sc);
extern void SCGuessHHintInstancesAndAdd(SplineChar *sc, StemInfo *stem, real guess1, real guess2);
extern void SCGuessVHintInstancesAndAdd(SplineChar *sc, StemInfo *stem, real guess1, real guess2);
extern void SCGuessHHintInstancesList(SplineChar *sc);
extern void SCGuessVHintInstancesList(SplineChar *sc);
extern real HIlen( StemInfo *stems);
extern real HIoverlap( HintInstance *mhi, HintInstance *thi);
extern int StemInfoAnyOverlaps(StemInfo *stems);
extern int StemListAnyConflicts(StemInfo *stems);
extern HintInstance *HICopyTrans(HintInstance *hi, real mul, real offset);
extern void MDAdd(SplineChar *sc, int x, SplinePoint *sp1, SplinePoint *sp2);
extern int SFNeedsAutoHint( SplineFont *_sf);
extern void SCAutoInstr( SplineChar *sc,BlueData *bd );
extern void CVT_ImportPrivate(SplineFont *sf);
extern void SCModifyHintMasksAdd(SplineChar *sc,StemInfo *new);
extern void SCClearHints(SplineChar *sc);
extern void SCClearHintMasks(SplineChar *sc,int counterstoo);
extern void SCFigureVerticalCounterMasks(SplineChar *sc);
extern void SCFigureCounterMasks(SplineChar *sc);
extern void SCFigureHintMasks(SplineChar *sc);
extern void _SplineCharAutoHint( SplineChar *sc, BlueData *bd, struct glyphdata *gd2 );
extern void SplineCharAutoHint( SplineChar *sc,BlueData *bd);
extern void SFSCAutoHint( SplineChar *sc,BlueData *bd);
extern void SplineFontAutoHint( SplineFont *sf);
extern void SplineFontAutoHintRefs( SplineFont *sf);
extern StemInfo *HintCleanup(StemInfo *stem,int dosort,int instance_count);
extern int SplineFontIsFlexible(SplineFont *sf,int flags);
extern int SCDrawsSomething(SplineChar *sc);
extern int SCWorthOutputting(SplineChar *sc);
extern int SFFindNotdef(SplineFont *sf, int fixed);
extern int IsntBDFChar(BDFChar *bdfc);
extern int CIDWorthOutputting(SplineFont *cidmaster, int enc); /* Returns -1 on failure, font number on success */
extern int AmfmSplineFont(FILE *afm, MMSet *mm,int formattype,EncMap *map);
extern int AfmSplineFont(FILE *afm, SplineFont *sf,int formattype,EncMap *map, int docc, SplineFont *fullsf);
extern int PfmSplineFont(FILE *pfm, SplineFont *sf,int type0,EncMap *map);
extern int TfmSplineFont(FILE *afm, SplineFont *sf,int formattype,EncMap *map);
extern int OfmSplineFont(FILE *afm, SplineFont *sf,int formattype,EncMap *map);
extern char *EncodingName(Encoding *map);
extern char *SFEncodingName(SplineFont *sf,EncMap *map);
extern void SFLigaturePrepare(SplineFont *sf);
extern void SFLigatureCleanup(SplineFont *sf);
extern void SFKernClassTempDecompose(SplineFont *sf,int isv);
extern void SFKernCleanup(SplineFont *sf,int isv);
extern int SCSetMetaData(SplineChar *sc,char *name,int unienc,
	const char *comment);

extern enum uni_interp interp_from_encoding(Encoding *enc,enum uni_interp interp);
extern const char *EncName(Encoding *encname);
extern const char*FindUnicharName(void);
extern Encoding *_FindOrMakeEncoding(const char *name,int make_it);
extern Encoding *FindOrMakeEncoding(const char *name);
extern void SFDDumpMacFeat(FILE *sfd,MacFeat *mf);
extern MacFeat *SFDParseMacFeatures(FILE *sfd, char *tok);
extern int SFDWrite(char *filename,SplineFont *sf,EncMap *map,EncMap *normal, int todir);
extern int SFDWriteBak(SplineFont *sf,EncMap *map,EncMap *normal);
extern SplineFont *SFDRead(char *filename);
extern SplineFont *SFDirRead(char *filename);
extern SplineChar *SFDReadOneChar(SplineFont *sf,const char *name);
extern char *TTFGetFontName(FILE *ttf,int32 offset,int32 off2);
extern void TTFLoadBitmaps(FILE *ttf,struct ttfinfo *info, int onlyone);
enum ttfflags { ttf_onlystrikes=1, ttf_onlyonestrike=2, ttf_onlykerns=4, ttf_onlynames=8 };
extern SplineFont *_SFReadTTF(FILE *ttf,int flags,enum openflags openflags,
	char *filename,struct fontdict *fd);
extern SplineFont *SFReadTTF(char *filename,int flags,enum openflags openflags);
extern SplineFont *SFReadSVG(char *filename,int flags);
extern SplineFont *SFReadUFO(char *filename,int flags);
extern SplineFont *_CFFParse(FILE *temp,int len,char *fontsetname);
extern SplineFont *CFFParse(char *filename);
extern SplineFont *SFReadMacBinary(char *filename,int flags,enum openflags openflags);
extern SplineFont *SFReadWinFON(char *filename,int toback);
extern SplineFont *SFReadPalmPdb(char *filename,int toback);
extern SplineFont *LoadSplineFont(char *filename,enum openflags);
extern SplineFont *ReadSplineFont(char *filename,enum openflags);	/* Don't use this, use LoadSF instead */
extern SplineFont *SFFromBDF(char *filename,int ispk,int toback);
extern SplineFont *SFFromMF(char *filename);
extern void SFCheckPSBitmap(SplineFont *sf);
extern uint16 _MacStyleCode( char *styles, SplineFont *sf, uint16 *psstyle );
extern uint16 MacStyleCode( SplineFont *sf, uint16 *psstyle );
extern SplineFont *SFReadIkarus(char *fontname);
extern SplineFont *_SFReadPdfFont(FILE *ttf,char *filename,char *select_this_font, enum openflags openflags);
extern SplineFont *SFReadPdfFont(char *filename, enum openflags openflags);
extern char **GetFontNames(char *filename);
extern char **NamesReadSFD(char *filename);
extern char **NamesReadTTF(char *filename);
extern char **NamesReadCFF(char *filename);
extern char **NamesReadPostscript(char *filename);
extern char **_NamesReadPostscript(FILE *ps);
extern char **NamesReadSVG(char *filename);
extern char **NamesReadUFO(char *filename);
extern char **NamesReadMacBinary(char *filename);

extern void SFSetOrder(SplineFont *sf,int order2);
extern int SFFindOrder(SplineFont *sf);

extern const char *UnicodeRange(int unienc);
extern SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,EncMap *map,int i);
extern SplineChar *SFMakeChar(SplineFont *sf,EncMap *map,int i);
extern char *AdobeLigatureFormat(char *name);
extern uint32 LigTagFromUnicode(int uni);
extern void SCLigCaretCheck(SplineChar *sc,int clean);
extern BDFChar *BDFMakeGID(BDFFont *bdf,int gid);
extern BDFChar *BDFMakeChar(BDFFont *bdf,EncMap *map,int enc);

extern void SCUndoSetLBearingChange(SplineChar *sc,int lb);
extern Undoes *SCPreserveHints(SplineChar *sc);
extern Undoes *SCPreserveLayer(SplineChar *sc,int layer,int dohints);
extern Undoes *SCPreserveState(SplineChar *sc,int dohints);
extern Undoes *SCPreserveBackground(SplineChar *sc);
extern Undoes *SFPreserveGuide(SplineFont *sf);
extern Undoes *SCPreserveWidth(SplineChar *sc);
extern Undoes *SCPreserveVWidth(SplineChar *sc);
extern Undoes *BCPreserveState(BDFChar *bc);
extern void BCDoRedo(BDFChar *bc,struct fontview *fv);
extern void BCDoUndo(BDFChar *bc,struct fontview *fv);

extern int SFIsCompositBuildable(SplineFont *sf,int unicodeenc,SplineChar *sc);
extern int SFIsSomethingBuildable(SplineFont *sf,SplineChar *sc,int onlyaccents);
extern int SFIsRotatable(SplineFont *sf,SplineChar *sc);
extern int SCMakeDotless(SplineFont *sf, SplineChar *dotless, int copybmp, int doit);
extern void SCBuildComposit(SplineFont *sf, SplineChar *sc, int copybmp,
	struct fontview *fv);
extern int SCAppendAccent(SplineChar *sc,char *glyph_name,int uni,int pos);
#ifdef FONTFORGE_CONFIG_GTK
extern const char *SFGetAlternate(SplineFont *sf, int base,SplineChar *sc,int nocheck);
#else
extern const unichar_t *SFGetAlternate(SplineFont *sf, int base,SplineChar *sc,int nocheck);
#endif

extern int getAdobeEnc(char *name);

extern void SFSplinesFromLayers(SplineFont *sf,int tostroke);
extern void SFSetLayerWidthsStroked(SplineFont *sf, real strokewidth);
extern SplineSet *SplinePointListInterpretSVG(char *filename,char *memory, int memlen, int em_size, int ascent,int stroked);
extern SplineSet *SplinePointListInterpretGlif(char *filename,char *memory, int memlen, int em_size, int ascent,int stroked);
extern SplinePointList *SplinePointListInterpretPS(FILE *ps,int flags,int stroked);
extern void PSFontInterpretPS(FILE *ps,struct charprocs *cp,char **encoding);
extern struct enc *PSSlurpEncodings(FILE *file);
extern int EvaluatePS(char *str,real *stack,int size);
struct pscontext {
    int is_type2;
    int painttype;
    int instance_count;
    real blend_values[17];
    int blend_warn;
};
extern int UnblendedCompare(real u1[MmMax], real u2[MmMax], int cnt);
extern SplineChar *PSCharStringToSplines(uint8 *type1, int len, struct pscontext *context,
	struct pschars *subrs, struct pschars *gsubrs, const char *name);
extern void MatMultiply(real m1[6], real m2[6], real to[6]);

extern int NameToEncoding(SplineFont *sf,EncMap *map,const char *uname);
extern void GlyphHashFree(SplineFont *sf);
extern void SFHashGlyph(SplineFont *sf,SplineChar *sc);
extern int SFFindGID(SplineFont *sf, int unienc, const char *name );
extern int SFFindSlot(SplineFont *sf, EncMap *map, int unienc, const char *name );
extern int SFCIDFindCID(SplineFont *sf, int unienc, const char *name );
extern SplineChar *SFGetChar(SplineFont *sf, int unienc, const char *name );
extern int SFHasChar(SplineFont *sf, int unienc, const char *name );
extern SplineChar *SFGetOrMakeChar(SplineFont *sf, int unienc, const char *name );
extern int SFFindExistingSlot(SplineFont *sf, int unienc, const char *name );
extern int SFCIDFindExistingChar(SplineFont *sf, int unienc, const char *name );
extern int SFHasCID(SplineFont *sf, int cid);

extern char *getPfaEditDir(char *buffer);
extern void DoAutoSaves(void);
extern void CleanAutoRecovery(void);
extern int DoAutoRecovery(void);
extern SplineFont *SFRecoverFile(char *autosavename);
extern void SFAutoSave(SplineFont *sf,EncMap *map);
extern void SFClearAutoSave(SplineFont *sf);

extern void PSCharsFree(struct pschars *chrs);
extern void PSDictFree(struct psdict *chrs);
extern struct psdict *PSDictCopy(struct psdict *dict);
extern int PSDictFindEntry(struct psdict *dict, char *key);
extern char *PSDictHasEntry(struct psdict *dict, char *key);
extern int PSDictRemoveEntry(struct psdict *dict, char *key);
extern int PSDictChangeEntry(struct psdict *dict, char *key, char *newval);

extern void SplineSetsRound2Int(SplineSet *spl,real factor);
extern void SCRound2Int(SplineChar *sc,real factor);
extern int SCRoundToCluster(SplineChar *sc,int layer,int sel,double within,double max);
extern int SplineSetsRemoveAnnoyingExtrema(SplineSet *ss,double err);
extern int hascomposing(SplineFont *sf,int u,SplineChar *sc);
#if 0
extern void SFFigureGrid(SplineFont *sf);
#endif
extern int FVWinInfo(struct fontview *,int *cc,int *rc);

struct cidmap;			/* private structure to encoding.c */
extern int CIDFromName(char *name,SplineFont *cidmaster);
extern int CID2Uni(struct cidmap *map,int cid);
extern int CID2NameUni(struct cidmap *map,int cid, char *buffer, int len);
extern int NameUni2CID(struct cidmap *map,int uni, const char *name);
extern int MaxCID(struct cidmap *map);
extern struct cidmap *LoadMapFromFile(char *file,char *registry,char *ordering,
	int supplement);
extern struct cidmap *FindCidMap(char *registry,char *ordering,int supplement,
	SplineFont *sf);
extern void SFEncodeToMap(SplineFont *sf,struct cidmap *map);
extern struct cidmap *AskUserForCIDMap(SplineFont *sf);
extern SplineFont *CIDFlatten(SplineFont *cidmaster,SplineChar **chars,int charcnt);
extern void SFFlatten(SplineFont *cidmaster);
extern int  SFFlattenByCMap(SplineFont *sf,char *cmapname);
extern SplineFont *MakeCIDMaster(SplineFont *sf,EncMap *oldmap,int bycmap,char *cmapfilename,struct cidmap *cidmap);

int getushort(FILE *ttf);
int32 getlong(FILE *ttf);
int get3byte(FILE *ttf);
real getfixed(FILE *ttf);
real get2dot14(FILE *ttf);
void putshort(FILE *file,int sval);
void putlong(FILE *file,int val);
void putfixed(FILE *file,real dval);
int ttfcopyfile(FILE *ttf, FILE *other, int pos, char *table_name);

extern void SCCopyFgToBg(SplineChar *sc,int show);
extern void SCCopyBgToFg(SplineChar *sc,int show);

extern int hasFreeType(void);
extern int hasFreeTypeDebugger(void);
extern int hasFreeTypeByteCode(void);
extern int FreeTypeAtLeast(int major, int minor, int patch);
extern void doneFreeType(void);
extern void *_FreeTypeFontContext(SplineFont *sf,SplineChar *sc,struct fontview *fv,
	enum fontformat ff,int flags,void *shared_ftc);
extern void *FreeTypeFontContext(SplineFont *sf,SplineChar *sc,struct fontview *fv);
extern BDFFont *SplineFontFreeTypeRasterize(void *freetypecontext,int pixelsize,int depth);
extern BDFChar *SplineCharFreeTypeRasterize(void *freetypecontext,int gid,
	int pixelsize,int depth);
extern void FreeTypeFreeContext(void *freetypecontext);
extern SplineSet *FreeType_GridFitChar(void *single_glyph_context,
	int enc, real ptsize, int dpi, int16 *width, SplineChar *sc);
extern struct freetype_raster *FreeType_GetRaster(void *single_glyph_context,
	int enc, real ptsize, int dpi,int depth);
extern BDFChar *SplineCharFreeTypeRasterizeNoHints(SplineChar *sc,
	int pixelsize,int depth);
extern BDFFont *SplineFontFreeTypeRasterizeNoHints(SplineFont *sf,int pixelsize,int depth);
extern void FreeType_FreeRaster(struct freetype_raster *raster);
struct TT_ExecContextRec_;
extern struct freetype_raster *DebuggerCurrentRaster(struct  TT_ExecContextRec_ *exc,int depth);

extern int UniFromName(const char *name,enum uni_interp interp, Encoding *encname);
extern const char *StdGlyphName(char *buffer, int uni, enum uni_interp interp, NameList *for_this_font);
extern char **AllGlyphNames(int uni, NameList *for_this_font,SplineChar *sc/* May be NULL*/);
extern char **AllNamelistNames(void);
extern NameList *DefaultNameListForNewFonts(void);
extern NameList *NameListByName(char *name);
extern NameList *LoadNamelist(char *filename);
extern void LoadNamelistDir(char *dir);
extern const char *RenameGlyphToNamelist(char *buffer, SplineChar *sc,NameList *old,NameList *new);
extern void SFRenameGlyphsToNamelist(SplineFont *sf,NameList *new);
extern char **SFTemporaryRenameGlyphsToNamelist(SplineFont *sf,NameList *new);
extern void SFTemporaryRestoreGlyphNames(SplineFont *sf,char **former);

extern void doversion(void);

#ifdef FONTFORGE_CONFIG_GTK
extern AnchorPos *AnchorPositioning(SplineChar *sc,char *ustr,SplineChar **sstr );
#else
extern AnchorPos *AnchorPositioning(SplineChar *sc,unichar_t *ustr,SplineChar **sstr );
#endif
extern void AnchorPosFree(AnchorPos *apos);

extern int SFCloseAllInstrs(SplineFont *sf);
extern void SCMarkInstrDlgAsChanged(SplineChar *sc);
extern int SSTtfNumberPoints(SplineSet *ss);
extern int  SCNumberPoints(SplineChar *sc);
extern int  SCPointsNumberedProperly(SplineChar *sc);
extern int  ttfFindPointInSC(SplineChar *sc,int pnum,BasePoint *pos,
	RefChar *bound);

int SFFigureDefWidth(SplineFont *sf, int *_nomwid);

extern int SFRenameTheseFeatureTags(SplineFont *sf, uint32 tag, int sli, int flags,
	uint32 totag, int tosli, int toflags, int ismac);
extern int SFRemoveUnusedNestedFeatures(SplineFont *sf);
extern int SFHasNestedLookupWithTag(SplineFont *sf,uint32 tag,int ispos);
extern int ClassesMatch(int cnt1,char **classes1,int cnt2,char **classes2);
extern FPST *FPSTGlyphToClass(FPST *fpst);

extern ASM *ASMFromOpenTypeForms(SplineFont *sf,uint32 script);
extern ASM *ASMFromFPST(SplineFont *sf,FPST *fpst,int ordered);

extern char *utf8_verify_copy(const char *str);

extern char *MacStrToUtf8(const char *str,int macenc,int maclang);
extern char *Utf8ToMacStr(const char *ustr,int macenc,int maclang);
extern uint8 MacEncFromMacLang(int maclang);
extern uint16 WinLangFromMac(int maclang);
extern uint16 WinLangToMac(int winlang);
extern int CanEncodingWinLangAsMac(int winlang);
extern const int32 *MacEncToUnicode(int script,int lang);
extern int MacLangFromLocale(void);
extern char *MacLanguageFromCode(int code);
extern char *FindEnglishNameInMacName(struct macname *mn);
extern char *PickNameFromMacName(struct macname *mn);
extern MacFeat *FindMacFeature(SplineFont *sf, int feat,MacFeat **secondary);
extern struct macsetting *FindMacSetting(SplineFont *sf, int feat, int set,struct macsetting **secondary);
extern struct macname *FindMacSettingName(SplineFont *sf, int feat, int set);

extern int32 UniFromEnc(int enc, Encoding *encname);
extern int32 EncFromUni(int32 uni, Encoding *encname);
extern int32 EncFromName(const char *name,enum uni_interp interp,Encoding *encname);

extern void MatInverse(real into[6], real orig[6]);

extern int BpColinear(BasePoint *first, BasePoint *mid, BasePoint *last);

enum psstrokeflags { sf_toobigwarn=1, sf_removeoverlap=2, sf_handle_eraser=4,
	sf_correctdir=8, sf_clearbeforeinput=16 };
extern enum psstrokeflags PsStrokeFlagsDlg(void);

extern char *MMAxisAbrev(char *axis_name);
extern char *MMMakeMasterFontname(MMSet *mm,int ipos,char **fullname);
extern char *MMGuessWeight(MMSet *mm,int ipos,char *def);
extern char *MMExtractNth(char *pt,int ipos);
extern char *MMExtractArrayNth(char *pt,int ipos);
extern int MMValid(MMSet *mm,int complain);
extern void MMKern(SplineFont *sf,SplineChar *first,SplineChar *second,int diff,
	struct lookup_subtable *sub,KernPair *oldkp);
extern char *MMBlendChar(MMSet *mm, int gid);
extern int   MMReblend(struct fontview *fv, MMSet *mm);
struct fontview *MMCreateBlendedFont(MMSet *mm,struct fontview *fv,real blends[MmMax],int tonew );

extern char *EnforcePostScriptName(char *old);

extern const char *TTFNameIds(int id);
extern const char *MSLangString(int language);
extern void FontInfoInit(void);
extern void LookupUIInit(void);
extern char *ToAbsolute(char *filename);

enum Compare_Ret {	SS_DiffContourCount	= 1,
			SS_MismatchOpenClosed	= 2,
			SS_DisorderedContours	= 4,
			SS_DisorderedStart	= 8,
			SS_DisorderedDirection	= 16,
			SS_PointsMatch		= 32,
			SS_ContourMatch		= 64,
			SS_NoMatch		= 128,
			SS_RefMismatch		= 256,
			SS_WidthMismatch	= 512,
			SS_VWidthMismatch	= 1024,
			SS_HintMismatch		= 2048,
			SS_HintMaskMismatch	= 4096,
			SS_LayerCntMismatch	= 8192,
			SS_ContourMismatch	= 16384,
			SS_UnlinkRefMatch	= 32768,

			BC_DepthMismatch	= 1<<16,
			BC_BoundingBoxMismatch	= 2<<16,
			BC_BitmapMismatch	= 4<<16,
			BC_NoMatch		= 8<<16,
			BC_Match		= 16<<16,

			SS_RefPtMismatch	= 32<<16
		};

extern enum Compare_Ret BitmapCompare(BDFChar *bc1, BDFChar *bc2, int err, int bb_err);
extern enum Compare_Ret SSsCompare(const SplineSet *ss1, const SplineSet *ss2,
	real pt_err, real spline_err, SplinePoint **hmfail);
enum font_compare_flags { fcf_outlines=1, fcf_exact=2, fcf_warn_not_exact=4,
	fcf_hinting=8, fcf_hintmasks=0x10, fcf_hmonlywithconflicts=0x20,
	fcf_warn_not_ref_exact=0x40,
	fcf_bitmaps=0x80, fcf_names = 0x100, fcf_gpos=0x200, fcf_gsub=0x400,
	fcf_adddiff2sf1=0x800, fcf_addmissing=0x1000 };
extern int CompareFonts(SplineFont *sf1, EncMap *map1, SplineFont *sf2,
	FILE *diffs, int flags);


# if HANYANG
extern void SFDDumpCompositionRules(FILE *sfd,struct compositionrules *rules);
extern struct compositionrules *SFDReadCompositionRules(FILE *sfd);
extern void SFModifyComposition(SplineFont *sf);
extern void SFBuildSyllables(SplineFont *sf);
# endif

extern void DefaultOtherSubrs(void);
extern int ReadOtherSubrsFile(char *filename);

extern char *utf8toutf7_copy(const char *_str);
extern char *utf7toutf8_copy(const char *_str);

extern void SFSetModTime(SplineFont *sf);
extern void SFTimesFromFile(SplineFont *sf,FILE *);

extern int SFHasInstructions(SplineFont *sf);

extern SplineChar *SCHasSubs(SplineChar *sc,uint32 tag);

extern char *TagFullName(SplineFont *sf,uint32 tag, int ismac, int onlyifknown);

extern uint32 *SFScriptsInLookups(SplineFont *sf,int gpos);
extern uint32 *SFLangsInScript(SplineFont *sf,int gpos,uint32 script);
extern uint32 *SFFeaturesInScriptLang(SplineFont *sf,int gpos,uint32 script,uint32 lang);
extern OTLookup **SFLookupsInScriptLangFeature(SplineFont *sf,int gpos,uint32 script,uint32 lang, uint32 feature);
extern SplineChar **SFGlyphsWithPSTinSubtable(SplineFont *sf,struct lookup_subtable *subtable);
extern SplineChar **SFGlyphsWithLigatureinLookup(SplineFont *sf,struct lookup_subtable *subtable);
extern void SFFindUnusedLookups(SplineFont *sf);
extern void SFFindClearUnusedLookupBits(SplineFont *sf);
extern void SFRemoveUnusedLookupSubTables(SplineFont *sf,
	int remove_incomplete_anchorclasses,
	int remove_unused_lookups);
extern void SFRemoveLookupSubTable(SplineFont *sf,struct lookup_subtable *sub);
extern void SFRemoveLookup(SplineFont *sf,OTLookup *otl);
extern struct lookup_subtable *SFFindLookupSubtable(SplineFont *sf,char *name);
extern struct lookup_subtable *SFFindLookupSubtableAndFreeName(SplineFont *sf,char *name);
extern OTLookup *SFFindLookup(SplineFont *sf,char *name);
extern void NameOTLookup(OTLookup *otl,SplineFont *sf);
extern void FListAppendScriptLang(FeatureScriptLangList *fl,uint32 script_tag,uint32 lang_tag);
extern void FListsAppendScriptLang(FeatureScriptLangList *fl,uint32 script_tag,uint32 lang_tag);
struct scriptlanglist *SLCopy(struct scriptlanglist *sl);
extern FeatureScriptLangList *FeatureListCopy(FeatureScriptLangList *fl);
extern void SLMerge(FeatureScriptLangList *into, struct scriptlanglist *fsl);
extern void FLMerge(OTLookup *into, OTLookup *from);
extern FeatureScriptLangList *FLOrder(FeatureScriptLangList *fl);
extern int FeatureScriptTagInFeatureScriptList(uint32 tag, uint32 script, FeatureScriptLangList *fl);
extern int FeatureTagInFeatureScriptList(uint32 tag, FeatureScriptLangList *fl);
extern int DefaultLangTagInOneScriptList(struct scriptlanglist *sl);
extern struct scriptlanglist *DefaultLangTagInScriptList(struct scriptlanglist *sl, int DFLT_ok);
extern int ScriptInFeatureScriptList(uint32 script, FeatureScriptLangList *fl);
extern int FeatureOrderId( int isgpos,FeatureScriptLangList *fl );
extern void SFSubTablesMerge(SplineFont *_sf,struct lookup_subtable *subfirst,
	struct lookup_subtable *subsecond);
extern struct lookup_subtable *SFSubTableFindOrMake(SplineFont *sf,uint32 tag,uint32 script,
	int lookup_type );
extern struct lookup_subtable *SFSubTableMake(SplineFont *sf,uint32 tag,uint32 script,
	int lookup_type );
extern OTLookup *OTLookupCopyInto(SplineFont *into_sf,SplineFont *from_sf, OTLookup *from_otl);
extern struct opentype_str *ApplyTickedFeatures(SplineFont *sf,uint32 *flist, uint32 script, uint32 lang,
	SplineChar **glyphs);
extern int VerticalKernFeature(SplineFont *sf, OTLookup *otl, int ask);

extern int KCFindIndex(KernClass *kc,char *name1, char *name2);
extern KernClass *SFFindKernClass(SplineFont *sf,SplineChar *first,SplineChar *last,
	int *index,int allow_zero);
extern KernClass *SFFindVKernClass(SplineFont *sf,SplineChar *first,SplineChar *last,
	int *index,int allow_zero);

#ifdef LUA_FF_LIB
extern SplineFont *SFReadTTFInfo(char *filename,int flags,enum openflags openflags);
extern SplineFont *CFFParseInfo(char *filename);
extern SplineFont *SFReadMacBinaryInfo(char *filename,int flags,enum openflags openflags);
extern SplineFont *ReadSplineFontInfo(char *filename,enum openflags);
#endif

#if !defined(_NO_PYTHON)
extern void FontForge_PythonInit(void);
extern void PyFF_ErrorString(const char *msg,const char *str);
extern void PyFF_ErrorF3(const char *frmt, const char *str, int size, int depth);
extern void PyFF_Stdin(void);
extern void PyFF_Main(int argc,char **argv,int start);
extern void PyFF_ScriptFile(struct fontview *fv,char *filename);
extern void PyFF_ScriptString(struct fontview *fv,char *str);
extern void PyFF_FreeFV(struct fontview *fv);
extern void PyFF_FreeSC(SplineChar *sc);
struct gtextinfo;
extern void scriptingSaveEnglishNames(struct gtextinfo *ids,struct gtextinfo *langs);
#endif
#endif
