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
#include "pfaeditui.h"
#include "groups.h"
#include "plugins.h"
#include <charset.h>
#include <gfile.h>
#include <gresource.h>
#include <ustring.h>
#include <gkeysym.h>

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include "ttf.h"

#if HAVE_LANGINFO_H
#ifndef LUA_FF_LIB
# include <langinfo.h>
#endif
#endif

#ifndef N_ 
#define N_(a) a
#endif

int splash = 1;
int adjustwidth = true;
int adjustlbearing = true;
Encoding *default_encoding = NULL;
int autohint_before_rasterize = 1;
int autohint_before_generate = 1;
int use_freetype_to_rasterize_fv = 1;
int OpenCharsInNewWindow = 1;
int ItalicConstrained=true;
int accent_offset = 6;
int GraveAcuteCenterBottom = 1;
int PreferSpacingAccents = true;
int CharCenterHighest = 1;
int ask_user_for_resolution = true;
int stop_at_join = false;
int cv_auto_goto = true;
int recognizePUA = true;
float arrowAmount=1;
float snapdistance=3.5;
float joinsnap=0;
char *BDFFoundry=NULL;
char *TTFFoundry=NULL;
char *xuid=NULL;
char *SaveTablesPref=NULL;
char *RecentFiles[RECENT_MAX] = { NULL };
/*struct cvshows CVShows = { 1, 1, 1, 1, 1, 0, 1 };*/ /* in charview */
/* int default_fv_font_size = 24; */	/* in fontview */
/* int default_fv_antialias = false */	/* in fontview */
/* int default_fv_bbsized = false */	/* in fontview */
int default_fv_row_count = 4;
int default_fv_col_count = 16;
extern int default_fv_showhmetrics;	/* in fontview */
extern int default_fv_showvmetrics;	/* in fontview */
extern int default_fv_glyphlabel;	/* in fontview */
int save_to_dir = 0;			/* in fontview, use sfdir rather than sfd */
extern int palettes_docked;		/* in cvpalettes */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
extern int cvvisible[2], bvvisible[3];	/* in cvpalettes.c */
#endif
extern int maxundoes;			/* in cvundoes */
extern int prefer_cjk_encodings;	/* in parsettf */
extern int onlycopydisplayed, copymetadata, copyttfinstr;
extern struct cvshows CVShows;
extern int oldformatstate;		/* in savefontdlg.c */
extern int oldbitmapstate;		/* in savefontdlg.c */
extern int old_ttf_flags;		/* in savefontdlg.c */
extern int old_ps_flags;		/* in savefontdlg.c */
extern int old_otf_flags;		/* in savefontdlg.c */
extern int oldsystem;			/* in bitmapdlg.c */
#ifndef LUA_FF_LIB
extern int preferpotrace;		/* in autotrace.c */
extern int autotrace_ask;		/* in autotrace.c */
extern int mf_ask;			/* in autotrace.c */
extern int mf_clearbackgrounds;		/* in autotrace.c */
extern int mf_showerrors;		/* in autotrace.c */
extern char *mf_args;			/* in autotrace.c */
#endif
static int glyph_2_name_map=0;		/* was in tottf.c, now a flag in savefont options dlg */
extern int coverageformatsallowed;	/* in tottfgpos.c */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
extern int debug_wins;			/* in cvdebug.c */
extern int gridfit_dpi, gridfit_depth;	/* in cvgridfit.c */
extern float gridfit_pointsize;		/* in cvgridfit.c */
#endif
extern int hint_diagonal_ends;		/* in stemdb.c */
extern int hint_diagonal_intersections;	/* in stemdb.c */
extern int hint_bounding_boxes;		/* in stemdb.c */
unichar_t *script_menu_names[SCRIPT_MENU_MAX];
char *script_filenames[SCRIPT_MENU_MAX];
static char *xdefs_filename;
int new_em_size = 1000;
int new_fonts_are_order2 = false;
int loaded_fonts_same_as_new = false;
int use_second_indic_scripts = false;
char *helpdir;
char *othersubrsfile;
extern MacFeat *default_mac_feature_map,	/* from macenc.c */
		*user_mac_feature_map;
int updateflex = false;
int allow_utf8_glyphnames = false;
int clear_tt_instructions_when_needed = true;
int ask_user_for_cmap = false;

extern int rectelipse, polystar, regular_star;	/* from cvpalettes.c */
extern int center_out[2];			/* from cvpalettes.c */
extern float rr_radius;				/* from cvpalettes.c */
extern int ps_pointcnt;				/* from cvpalettes.c */
extern float star_percent;			/* from cvpalettes.c */

NameList *force_names_when_opening=NULL;
NameList *force_names_when_saving=NULL;
extern NameList *namelist_for_new_fonts;

int default_font_filter_index=0;
struct openfilefilters *user_font_filters = NULL;
static int alwaysgenapple=false, alwaysgenopentype=false;

static int pointless;

    /* These first three must match the values in macenc.c */
#define CID_Features	101
#define CID_FeatureDel	103
#define CID_FeatureEdit	105

#define CID_Mapping	102
#define CID_MappingDel	104
#define CID_MappingEdit	106

#define CID_ScriptMNameBase	200
#define CID_ScriptMFileBase	(200+SCRIPT_MENU_MAX)
#define CID_ScriptMBrowseBase	(200+2*SCRIPT_MENU_MAX)

#define CID_PrefsBase	1000
#define CID_PrefsOffset	100
#define CID_PrefsBrowseOffset	(CID_PrefsOffset/2)

/* ************************************************************************** */
/* *****************************    mac data    ***************************** */
/* ************************************************************************** */

struct macsettingname macfeat_otftag[] = {
    { 1, 0, CHR('r','l','i','g') },	/* Required ligatures */
    { 1, 2, CHR('l','i','g','a') },	/* Common ligatures */
    { 1, 4, CHR('d','l','i','g') },	/* rare ligatures => discretionary */
#if 0
    { 1, 4, CHR('h','l','i','g') },	/* rare ligatures => historic */
    { 1, 4, CHR('a','l','i','g') },	/* rare ligatures => ?ancient? */
#endif
    /* 2, 1, partially connected cursive */
    { 2, 2, CHR('i','s','o','l') },	/* Arabic forms */
    { 2, 2, CHR('c','a','l','t') },	/* ??? */
    /* 3, 1, all caps */
    /* 3, 2, all lower */
    { 3, 3, CHR('s','m','c','p') },	/* small caps */
    /* 3, 4, initial caps */
    /* 3, 5, initial caps, small caps */
    { 4, 0, CHR('v','r','t','2') },	/* vertical forms => vertical rotation */
#if 0
    { 4, 0, CHR('v','k','n','a') },	/* vertical forms => vertical kana */
#endif
    { 6, 0, CHR('t','n','u','m') },	/* monospace numbers => Tabular numbers */
    { 10, 1, CHR('s','u','p','s') },	/* superior vertical position => superscript */
    { 10, 2, CHR('s','u','b','s') },	/* inferior vertical position => subscript */
#if 0
    { 10, 3, CHR('s','u','p','s') },	/* ordinal vertical position => superscript */
#endif
    { 11, 1, CHR('a','f','r','c') },	/* vertical fraction => fraction ligature */
    { 11, 2, CHR('f','r','a','c') },	/* diagonal fraction => fraction ligature */
    { 16, 1, CHR('o','r','n','m') },	/* vertical fraction => fraction ligature */
    { 20, 0, CHR('t','r','a','d') },	/* traditional characters => traditional forms */
#if 0
    { 20, 0, CHR('t','n','a','m') },	/* traditional characters => traditional names */
#endif
    { 20, 1, CHR('s','m','p','l') },	/* simplified characters */
    { 20, 2, CHR('j','p','7','8') },	/* jis 1978 */
    { 20, 3, CHR('j','p','8','3') },	/* jis 1983 */
    { 20, 4, CHR('j','p','9','0') },	/* jis 1990 */
    { 21, 0, CHR('o','n','u','m') },	/* lower case number => old style numbers */
    { 22, 0, CHR('p','w','i','d') },	/* proportional text => proportional widths */
    { 22, 2, CHR('h','w','i','d') },	/* half width text => half widths */
    { 22, 3, CHR('f','w','i','d') },	/* full width text => full widths */
    { 25, 0, CHR('f','w','i','d') },	/* full width kana => full widths */
    { 25, 1, CHR('p','w','i','d') },	/* proportional kana => proportional widths */
    { 26, 0, CHR('f','w','i','d') },	/* full width ideograph => full widths */
    { 26, 1, CHR('p','w','i','d') },	/* proportional ideograph => proportional widths */
    { 103, 0, CHR('h','w','i','d') },	/* half width cjk roman => half widths */
    { 103, 1, CHR('p','w','i','d') },	/* proportional cjk roman => proportional widths */
    { 103, 3, CHR('f','w','i','d') },	/* full width cjk roman => full widths */
    { 0, 0, 0 }
}, *user_macfeat_otftag;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void UserSettingsFree(void) {

    free( user_macfeat_otftag );
    user_macfeat_otftag = NULL;
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int UserSettingsDiffer(void) {
    int i,j;

    if ( user_macfeat_otftag==NULL )
return( false );

    for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i );
    for ( j=0; macfeat_otftag[j].otf_tag!=0; ++j );
    if ( i!=j )
return( true );
    for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i ) {
	for ( j=0; macfeat_otftag[j].otf_tag!=0; ++j ) {
	    if ( macfeat_otftag[j].mac_feature_type ==
		    user_macfeat_otftag[i].mac_feature_type &&
		    macfeat_otftag[j].mac_feature_setting ==
		    user_macfeat_otftag[i].mac_feature_setting &&
		    macfeat_otftag[j].otf_tag ==
		    user_macfeat_otftag[i].otf_tag )
	break;
	}
	if ( macfeat_otftag[j].otf_tag==0 )
return( true );
    }
return( false );
}

/**************************************************************************** */


/* don't use mnemonics 'C' or 'O' (Cancel & OK) */
enum pref_types { pr_int, pr_real, pr_bool, pr_enum, pr_encoding, pr_string,
	pr_file, pr_namelist };
struct enums { char *name; int value; };

struct enums fvsize_enums[] = { {NULL} };

static struct prefs_list {
    char *name;
    	/* In the prefs file the untranslated name will always be used, but */
	/* in the UI that name may be translated. */
    enum pref_types type;
    void *val;
    void *(*get)(void);
    void (*set)(void *);
    char mn;
    struct enums *enums;
    unsigned int dontdisplay: 1;
    char *popup;
} general_list[] = {
	{ N_("ResourceFile"), pr_file, &xdefs_filename, NULL, NULL, 'R', NULL, 0, N_("When FontForge starts up, it loads display related resources from a\nproperty on the screen. Sometimes it is useful to be able to store\nthese resources in a file. These resources are only read at start\nup, so changing this has no effect until the next time you start\nFontForge.") },
	{ N_("HelpDir"), pr_file, &helpdir, NULL, NULL, 'H', NULL, 0, N_("The directory on your local system in which FontForge will search for help\nfiles.  If a file is not found there, then FontForge will look for it on the net.") },
	{ N_("OtherSubrsFile"), pr_file, &othersubrsfile, NULL, NULL, 'O', NULL, 0, N_("If you wish to replace Adobe's OtherSubrs array (for Type1 fonts)\nwith an array of your own, set this to point to a file containing\na list of up to 14 PostScript subroutines. Each subroutine must\nbe preceded by a line starting with '%%%%' (any text before the\nfirst '%%%%' line will be treated as an initial copyright notice).\nThe first three subroutines are for flex hints, the next for hint\nsubstitution (this MUST be present), the 14th (or 13 as the\nnumbering actually starts with 0) is for counter hints.\nThe subroutines should not be enclosed in a [ ] pair.") },
	{ N_("FreeTypeInFontView"), pr_bool, &use_freetype_to_rasterize_fv, NULL, NULL, 'O', NULL, 0, N_("Use the FreeType rasterizer (when available)\nto rasterize glyphs in the font view.\nThis generally results in better quality.") },
	{ N_("AutoHint"), pr_bool, &autohint_before_rasterize, NULL, NULL, 'A', NULL, 0, N_("AutoHint before rasterizing") },
	{ N_("SplashScreen"), pr_bool, &splash, NULL, NULL, 'S', NULL, 0, N_("Show splash screen on start-up") },
	{ NULL }
},
  new_list[] = {
	{ N_("NewCharset"), pr_encoding, &default_encoding, NULL, NULL, 'N', NULL, 0, N_("Default encoding for\nnew fonts") },
	{ N_("NewEmSize"), pr_int, &new_em_size, NULL, NULL, 'S', NULL, 0, N_("The default size of the Em-Square in a newly created font.") },
	{ N_("NewFontsQuadratic"), pr_bool, &new_fonts_are_order2, NULL, NULL, 'Q', NULL, 0, N_("Whether new fonts should contain splines of quadratic (truetype)\nor cubic (postscript & opentype).") },
	{ N_("LoadedFontsAsNew"), pr_bool, &loaded_fonts_same_as_new, NULL, NULL, 'L', NULL, 0, N_("Whether fonts loaded from the disk should retain their splines\nwith the original order (quadratic or cubic), or whether the\nsplines should be converted to the default order for new fonts\n(see NewFontsQuadratic).") },
	{ NULL }
},
  open_list[] = {
	{ N_("PreferCJKEncodings"), pr_bool, &prefer_cjk_encodings, NULL, NULL, 'C', NULL, 0, N_("When loading a truetype or opentype font which has both a unicode\nand a CJK encoding table, use this flag to specify which\nshould be loaded for the font.") },
	{ N_("AskUserForCMap"), pr_bool, &ask_user_for_cmap, NULL, NULL, 'O', NULL, 0, N_("When loading a font in sfnt format (TrueType, OpenType, etc.),\nask the user to specify which cmap to use initially.") },
	{ N_("PreserveTables"), pr_string, &SaveTablesPref, NULL, NULL, 'P', NULL, 0, N_("Enter a list of 4 letter table tags, separated by commas.\nFontForge will make a binary copy of these tables when it\nloads a True/OpenType font, and will output them (unchanged)\nwhen it generates the font. Do not include table tags which\nFontForge thinks it understands.") },
	{ NULL }
},
  navigation_list[] = {
	{ N_("GlyphAutoGoto"), pr_bool, &cv_auto_goto, NULL, NULL, '\0', NULL, 0, N_("Typing a normal character in the glyph view window changes the window to look at that character") },
	{ N_("OpenCharsInNewWindow"), pr_bool, &OpenCharsInNewWindow, NULL, NULL, '\0', NULL, 0, N_("When double clicking on a character in the font view\nopen that character in a new window, otherwise\nreuse an existing one.") },
	{ NULL }
},
  editing_list[] = {
	{ N_("ItalicConstrained"), pr_bool, &ItalicConstrained, NULL, NULL, '\0', NULL, 0, N_("In the Outline View, the Shift key constrains motion to be parallel to the ItalicAngle rather than constraining it to be vertical.") },
	{ N_("ArrowMoveSize"), pr_real, &arrowAmount, NULL, NULL, '\0', NULL, 0, N_("The number of em-units by which an arrow key will move a selected point") },
	{ N_("SnapDistance"), pr_real, &snapdistance, NULL, NULL, '\0', NULL, 0, N_("When the mouse pointer is within this many pixels\nof one of the various interesting features (baseline,\nwidth, grid splines, etc.) the pointer will snap\nto that feature.") },
	{ N_("JoinSnap"), pr_real, &joinsnap, NULL, NULL, '\0', NULL, 0, N_("The Edit->Join command will join points which are this close together\nA value of 0 means they must be coincident") },
	{ N_("StopAtJoin"), pr_bool, &stop_at_join, NULL, NULL, '\0', NULL, 0, N_("When dragging points in the outline view a join may occur\n(two open contours may connect at their endpoints). When\nthis is On a join will cause FontForge to stop moving the\nselection (as if the user had released the mouse button).\nThis is handy if your fingers are inclined to wiggle a bit.") },
	{ N_("CopyMetaData"), pr_bool, &copymetadata, NULL, NULL, '\0', NULL, 0, N_("When copying glyphs from the font view, also copy the\nglyphs' metadata (name, encoding, comment, etc).") },
	{ N_("UndoDepth"), pr_int, &maxundoes, NULL, NULL, '\0', NULL, 0, N_("The maximum number of Undoes/Redoes stored in a glyph") },
	{ N_("UpdateFlex"), pr_bool, &updateflex, NULL, NULL, '\0', NULL, 0, N_("Figure out flex hints after every change") },
	{ NULL }
},
  sync_list[] = {
	{ N_("AutoWidthSync"), pr_bool, &adjustwidth, NULL, NULL, '\0', NULL, 0, N_("Changing the width of a glyph\nchanges the widths of all accented\nglyphs based on it.") },
	{ N_("AutoLBearingSync"), pr_bool, &adjustlbearing, NULL, NULL, '\0', NULL, 0, N_("Changing the left side bearing\nof a glyph adjusts the lbearing\nof other references in all accented\nglyphs based on it.") },
	{ NULL }
},
 tt_list[] = {
	{ N_("ClearInstrsBigChanges"), pr_bool, &clear_tt_instructions_when_needed, NULL, NULL, 'C', NULL, 0, N_("Instructions in a TrueType font refer to\npoints by number, so if you edit a glyph\nin such a way that some points have different\nnumbers (add points, remove them, etc.) then\nthe instructions will be applied to the wrong\npoints with disasterous results.\n  Normally FontForge will remove the instructions\nif it detects that the points have been renumbered\nin order to avoid the above problem. You may turn\nthis behavior off -- but be careful!") },
	{ N_("CopyTTFInstrs"), pr_bool, &copyttfinstr, NULL, NULL, '\0', NULL, 0, N_("When copying glyphs from the font view, also copy the\nglyphs' metadata (name, encoding, comment, etc).") },
	{ NULL }
},
  accent_list[] = {
	{ N_("AccentOffsetPercent"), pr_int, &accent_offset, NULL, NULL, '\0', NULL, 0, N_("The percentage of an em by which an accent is offset from its base glyph in Build Accent") },
	{ N_("AccentCenterLowest"), pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 0, N_("When placing grave and acute accents above letters, should\nFontForge center them based on their full width, or\nshould it just center based on the lowest point\nof the accent.") },
	{ N_("CharCenterHighest"), pr_bool, &CharCenterHighest, NULL, NULL, '\0', NULL, 0, N_("When centering an accent over a glyph, should the accent\nbe centered on the highest point(s) of the glyph,\nor the middle of the glyph?") },
	{ N_("PreferSpacingAccents"), pr_bool, &PreferSpacingAccents, NULL, NULL, '\0', NULL, 0, N_("Use spacing accents (Unicode: 02C0-02FF) rather than\ncombining accents (Unicode: 0300-036F) when\nbuilding accented glyphs.") },
	{ NULL }
},
 args_list[] = {
#ifndef LUA_FF_LIB
	{ N_("PreferPotrace"), pr_bool, &preferpotrace, NULL, NULL, '\0', NULL, 0, N_("FontForge supports two different helper applications to do autotracing\n autotrace and potrace\nIf your system only has one it will use that one, if you have both\nuse this option to tell FontForge which to pick.") },
	{ N_("AutotraceArgs"), pr_string, NULL, GetAutoTraceArgs, SetAutoTraceArgs, '\0', NULL, 0, N_("Extra arguments for configuring the autotrace program\n(either autotrace or potrace)") },
	{ N_("AutotraceAsk"), pr_bool, &autotrace_ask, NULL, NULL, '\0', NULL, 0, N_("Ask the user for autotrace arguments each time autotrace is invoked") },
	{ N_("MfArgs"), pr_string, &mf_args, NULL, NULL, '\0', NULL, 0, N_("Commands to pass to mf (metafont) program, the filename will follow these") },
	{ N_("MfAsk"), pr_bool, &mf_ask, NULL, NULL, '\0', NULL, 0, N_("Ask the user for mf commands each time mf is invoked") },
	{ N_("MfClearBg"), pr_bool, &mf_clearbackgrounds, NULL, NULL, '\0', NULL, 0, N_("FontForge loads large images into the background of each glyph\nprior to autotracing them. You may retain those\nimages to look at after mf processing is complete, or\nremove them to save space") },
	{ N_("MfShowErr"), pr_bool, &mf_showerrors, NULL, NULL, '\0', NULL, 0, N_("MetaFont (mf) generates lots of verbiage to stdout.\nMost of the time I find it an annoyance but it is\nimportant to see if something goes wrong.") },
#endif
	{ NULL }
},
 fontinfo_list[] = {
	{ N_("FoundryName"), pr_string, &BDFFoundry, NULL, NULL, 'F', NULL, 0, N_("Name used for foundry field in bdf\nfont generation") },
	{ N_("TTFFoundry"), pr_string, &TTFFoundry, NULL, NULL, 'T', NULL, 0, N_("Name used for Vendor ID field in\nttf (OS/2 table) font generation.\nMust be no more than 4 characters") },
	{ N_("NewFontNameList"), pr_namelist, &namelist_for_new_fonts, NULL, NULL, '\0', NULL, 0, N_("FontForge will use this namelist when assigning\nglyph names to code points in a new font.") },
	{ N_("RecognizePUANames"), pr_bool, &recognizePUA, NULL, NULL, 'U', NULL, 0, N_("Once upon a time, Adobe assigned PUA (public use area) encodings\nfor many stylistic variants of characters (small caps, old style\nnumerals, etc.). Adobe no longer believes this to be a good idea,\nand recommends that these encodings be ignored.\n\n The assignments were originally made because most applications\ncould not handle OpenType features for accessing variants. Adobe\nnow believes that all apps that matter can now do so. Applications\nlike Word and OpenOffice still can't handle these features, so\n fontforge's default behavior is to ignore Adobe's current\nrecommendations.\n\nNote: This does not affect figuring out unicode from the font's encoding,\nit just controls determining unicode from a name.") },
	{ N_("UnicodeGlyphNames"), pr_bool, &allow_utf8_glyphnames, NULL, NULL, 'O', NULL, 0, N_("Allow the full unicode character set in glyph names.\nThis does not conform to adobe's glyph name standard.\nSuch names should be for internal use only and\nshould NOT end up in production fonts." ) },
	{ N_("XUID-Base"), pr_string, &xuid, NULL, NULL, 'X', NULL, 0, N_("If specified this should be a space separated list of integers each\nless than 16777216 which uniquely identify your organization\nFontForge will generate a random number for the final component.") },
	{ NULL }
},
 generate_list[] = {
	{ N_("AskBDFResolution"), pr_bool, &ask_user_for_resolution, NULL, NULL, 'B', NULL, 0, N_("When generating a set of BDF fonts ask the user\nto specify the screen resolution of the fonts\notherwise FontForge will guess depending on the pixel size.") },
	{ N_("HintForGen"), pr_bool, &autohint_before_generate, NULL, NULL, 'H', NULL, 0, N_("AutoHint changed glyphs before generating a font") },
	{ NULL }
},
 hints_list[] = {
	{ N_("HintBoundingBoxes"), pr_bool, &hint_bounding_boxes, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints to describe the bounding boxes of suitable glyphs.") },
	{ N_("HintDiagonalEnds"), pr_bool, &hint_diagonal_ends, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints at the ends of diagonal stems.") },
	{ N_("HintDiagonalInter"), pr_bool, &hint_diagonal_intersections, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints at the intersections of diagonal stems.") },
	{ NULL }
},
 opentype_list[] = {
	{ N_("UseNewIndicScripts"), pr_bool, &use_second_indic_scripts, NULL, NULL, 'C', NULL, 0, N_("MS has changed (in August 2006) the inner workings of their Indic shaping\nengine, and to disambiguate this change has created a parallel set of script\ntags (generally ending in '2') for Indic writing systems. If you are working\nwith the new system set this flag, if you are working with the old unset it.\n(if you aren't doing Indic work, this flag is irrelevant).") },
	{ NULL }
},
/* These are hidden, so will never appear in ui, hence, no "N_(" */
 hidden_list[] = {
	{ "AntiAlias", pr_bool, &default_fv_antialias, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowHmetrics", pr_int, &default_fv_showhmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowVmetrics", pr_int, &default_fv_showvmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVSize", pr_int, &default_fv_font_size, NULL, NULL, 'S', NULL, 1 },
	{ "DefaultFVRowCount", pr_int, &default_fv_row_count, NULL, NULL, 'S', NULL, 1 },
	{ "DefaultFVColCount", pr_int, &default_fv_col_count, NULL, NULL, 'S', NULL, 1 },
	{ "DefaultFVGlyphLabel", pr_int, &default_fv_glyphlabel, NULL, NULL, 'S', NULL, 1 },
	{ "SaveToDir", pr_int, &save_to_dir, NULL, NULL, 'S', NULL, 1 },
	{ "OnlyCopyDisplayed", pr_bool, &onlycopydisplayed, NULL, NULL, '\0', NULL, 1 },
#ifndef LUA_FF_LIB
	{ "PalettesDocked", pr_bool, &palettes_docked, NULL, NULL, '\0', NULL, 1 },
#endif
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	{ "CVVisible0", pr_bool, &cvvisible[0], NULL, NULL, '\0', NULL, 1 },
	{ "CVVisible1", pr_bool, &cvvisible[1], NULL, NULL, '\0', NULL, 1 },
	{ "BVVisible0", pr_bool, &bvvisible[0], NULL, NULL, '\0', NULL, 1 },
	{ "BVVisible1", pr_bool, &bvvisible[1], NULL, NULL, '\0', NULL, 1 },
	{ "BVVisible2", pr_bool, &bvvisible[2], NULL, NULL, '\0', NULL, 1 },
	{ "MarkExtrema", pr_int, &CVShows.markextrema, NULL, NULL, '\0', NULL, 1 },
	{ "MarkPointsOfInflect", pr_int, &CVShows.markpoi, NULL, NULL, '\0', NULL, 1 },
	{ "ShowRulers", pr_bool, &CVShows.showrulers, NULL, NULL, '\0', NULL, 1, N_("Display rulers in the Outline Glyph View") },
	{ "ShowCPInfo", pr_int, &CVShows.showcpinfo, NULL, NULL, '\0', NULL, 1 },
	{ "ShowSideBearings", pr_int, &CVShows.showsidebearings, NULL, NULL, '\0', NULL, 1 },
	{ "ShowPoints", pr_bool, &CVShows.showpoints, NULL, NULL, '\0', NULL, 1 },
	{ "ShowFilled", pr_int, &CVShows.showfilled, NULL, NULL, '\0', NULL, 1 },
	{ "ShowTabs", pr_int, &CVShows.showtabs, NULL, NULL, '\0', NULL, 1 },
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
#ifndef LUA_FF_LIB
	{ "DefaultScreenDpiSystem", pr_int, &oldsystem, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultOutputFormat", pr_int, &oldformatstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultBitmapFormat", pr_int, &oldbitmapstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultTTFflags", pr_int, &old_ttf_flags, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultPSflags", pr_int, &old_ps_flags, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultOTFflags", pr_int, &old_otf_flags, NULL, NULL, '\0', NULL, 1 },
	{ "PageWidth", pr_int, &pagewidth, NULL, NULL, '\0', NULL, 1 },
	{ "PageHeight", pr_int, &pageheight, NULL, NULL, '\0', NULL, 1 },
	{ "PrintType", pr_int, &printtype, NULL, NULL, '\0', NULL, 1 },
	{ "PrintCommand", pr_string, &printcommand, NULL, NULL, '\0', NULL, 1 },
	{ "PageLazyPrinter", pr_string, &printlazyprinter, NULL, NULL, '\0', NULL, 1 },
	{ "RegularStar", pr_bool, &regular_star, NULL, NULL, '\0', NULL, 1 },
	{ "PolyStar", pr_bool, &polystar, NULL, NULL, '\0', NULL, 1 },
	{ "RectEllipse", pr_bool, &rectelipse, NULL, NULL, '\0', NULL, 1 },
	{ "RectCenterOut", pr_bool, &center_out[0], NULL, NULL, '\0', NULL, 1 },
	{ "EllipseCenterOut", pr_bool, &center_out[1], NULL, NULL, '\0', NULL, 1 },
	{ "PolyStartPointCnt", pr_int, &ps_pointcnt, NULL, NULL, '\0', NULL, 1 },
	{ "RoundRectRadius", pr_real, &rr_radius, NULL, NULL, '\0', NULL, 1 },
	{ "StarPercent", pr_real, &star_percent, NULL, NULL, '\0', NULL, 1 },
	{ "CoverageFormatsAllowed", pr_int, &coverageformatsallowed, NULL, NULL, '\0', NULL, 1 },
#endif
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	{ "DebugWins", pr_int, &debug_wins, NULL, NULL, '\0', NULL, 1 },
	{ "GridFitDpi", pr_int, &gridfit_dpi, NULL, NULL, '\0', NULL, 1 },
	{ "GridFitDepth", pr_int, &gridfit_depth, NULL, NULL, '\0', NULL, 1 },
	{ "GridFitPointSize", pr_real, &gridfit_pointsize, NULL, NULL, '\0', NULL, 1 },
#endif
	{ "ForceNamesWhenOpening", pr_namelist, &force_names_when_opening, NULL, NULL, '\0', NULL, 1 },
	{ "ForceNamesWhenSaving", pr_namelist, &force_names_when_saving, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFontFilterIndex", pr_int, &default_font_filter_index, NULL, NULL, '\0', NULL, 1 },
	{ NULL }
},
 oldnames[] = {
	{ "DumpGlyphMap", pr_bool, &glyph_2_name_map, NULL, NULL, '\0', NULL, 0, N_("When generating a truetype or opentype font it is occasionally\nuseful to know the mapping between truetype glyph ids and\nglyph names. Setting this option will cause FontForge to\nproduce a file (with extension .g2n) containing those data.") },
	{ "DefaultTTFApple", pr_int, &pointless, NULL, NULL, '\0', NULL, 1 },
	{ "AcuteCenterBottom", pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 1, N_("When placing grave and acute accents above letters, should\nFontForge center them based on their full width, or\nshould it just center based on the lowest point\nof the accent.") },
	{ "AlwaysGenApple", pr_bool, &alwaysgenapple, NULL, NULL, 'A', NULL, 0, N_("Apple and MS/Adobe differ about the format of truetype and opentype files.\nThis controls the default setting of the Apple checkbox in the\nFile->Generate Font dialog.\nThe main differences are:\n Bitmap data are stored in different tables\n Scaled composite glyphs are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop\nIf both this and OpenType are set, both formats are generated") },
	{ "AlwaysGenOpenType", pr_bool, &alwaysgenopentype, NULL, NULL, 'O', NULL, 0, N_("Apple and MS/Adobe differ about the format of truetype and opentype files.\nThis controls the default setting of the OpenType checkbox in the\nFile->Generate Font dialog.\nThe main differences are:\n Bitmap data are stored in different tables\n Scaled composite glyphs are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop\nIf both this and Apple are set, both formats are generated") },
	{ NULL }
},
 *prefs_list[] = { general_list, new_list, open_list, navigation_list, sync_list, editing_list, accent_list, args_list, fontinfo_list, generate_list, tt_list, opentype_list, hints_list, hidden_list, NULL },
 *load_prefs_list[] = { general_list, new_list, open_list, navigation_list, sync_list, editing_list, accent_list, args_list, fontinfo_list, generate_list, tt_list, opentype_list, hints_list, hidden_list, oldnames, NULL };

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
struct visible_prefs_list { char *tab_name; int nest; struct prefs_list *pl; } visible_prefs_list[] = {
    { N_("Generic"), 0, general_list},
    { N_("New Font"), 0, new_list},
    { N_("Open Font"), 0, open_list},
    { N_("Navigation"), 0, navigation_list},
    { N_("Editing"), 0, editing_list},
    { N_("Synchronize"), 1, sync_list},
    { N_("TT"), 1, tt_list},
    { N_("Accents"), 1, accent_list},
    { N_("Apps"), 1, args_list},
    { N_("Font Info"), 0, fontinfo_list},
    { N_("Generate"), 0, generate_list},
    { N_("PS Hints"), 1, hints_list},
    { N_("OpenType"), 1, opentype_list},
    { 0 }
 };

#define TOPICS	(sizeof(visible_prefs_list)/sizeof(visible_prefs_list[0])-1)
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int GetPrefs(char *name,Val *val) {
    int i,j;

    /* Support for obsolete preferences */
#ifndef LUA_FF_LIB
    alwaysgenapple=(old_ttf_flags&ttf_flag_applemode)?1:0;
    alwaysgenopentype=(old_ttf_flags&ttf_flag_otmode)?1:0;
#else
    alwaysgenapple=0;
    alwaysgenopentype=0;
#endif
    
    for ( i=0; prefs_list[i]!=NULL; ++i ) for ( j=0; prefs_list[i][j].name!=NULL; ++j ) {
	if ( strcmp(prefs_list[i][j].name,name)==0 ) {
	    struct prefs_list *pf = &prefs_list[i][j];
	    if ( pf->type == pr_bool || pf->type == pr_int ) {
		val->type = v_int;
		val->u.ival = *((int *) (pf->val));
	    } else if ( pf->type == pr_string || pf->type == pr_file ) {
		val->type = v_str;
		val->u.sval = copy( *((char **) (pf->val)));
	    } else if ( pf->type == pr_encoding ) {
		val->type = v_str;
		if ( *((NameList **) (pf->val))==NULL )
		    val->u.sval = copy( "NULL" );
		else
		    val->u.sval = copy( (*((Encoding **) (pf->val)))->enc_name );
	    } else if ( pf->type == pr_namelist ) {
		val->type = v_str;
		val->u.sval = copy( (*((NameList **) (pf->val)))->title );
	    } else if ( pf->type == pr_real ) {
		val->type = v_real;
		val->u.fval = *((float *) (pf->val));
	    } else
return( false );

return( true );
	}
    }
return( false );
}

static void CheckObsoletePrefs(void) {
#ifndef LUA_FF_LIB
    if ( alwaysgenapple==false ) {
	old_ttf_flags &= ~ttf_flag_applemode;
	old_otf_flags &= ~ttf_flag_applemode;
    } else if ( alwaysgenapple==true ) {
	old_ttf_flags |= ttf_flag_applemode;
	old_otf_flags |= ttf_flag_applemode;
    }
    if ( alwaysgenopentype==false ) {
	old_ttf_flags &= ~ttf_flag_otmode;
	old_otf_flags &= ~ttf_flag_otmode;
    } else if ( alwaysgenopentype==true ) {
	old_ttf_flags |= ttf_flag_otmode;
	old_otf_flags |= ttf_flag_otmode;
    }
#endif
}

int SetPrefs(char *name,Val *val1, Val *val2) {
    int i,j;

    /* Support for obsolete preferences */
    alwaysgenapple=-1; alwaysgenopentype=-1;

    for ( i=0; prefs_list[i]!=NULL; ++i ) for ( j=0; prefs_list[i][j].name!=NULL; ++j ) {
	if ( strcmp(prefs_list[i][j].name,name)==0 ) {
	    struct prefs_list *pf = &prefs_list[i][j];
	    if ( pf->type == pr_bool || pf->type == pr_int ) {
		if ( (val1->type!=v_int && val1->type!=v_unicode) || val2!=NULL )
return( -1 );
		*((int *) (pf->val)) = val1->u.ival;
	    } else if ( pf->type == pr_real ) {
		if ( val1->type==v_real && val2==NULL )
		    *((float *) (pf->val)) = val1->u.fval;
		else if ( val1->type!=v_int || (val2!=NULL && val2->type!=v_int ))
return( -1 );
		else
		    *((float *) (pf->val)) = (val2==NULL ? val1->u.ival : val1->u.ival / (double) val2->u.ival);
	    } else if ( pf->type == pr_string || pf->type == pr_file ) {
		if ( val1->type!=v_str || val2!=NULL )
return( -1 );
		if ( pf->set ) {
		    pf->set( val1->u.sval );
		} else {
		    free( *((char **) (pf->val)));
		    *((char **) (pf->val)) = copy( val1->u.sval );
		}
	    } else if ( pf->type == pr_encoding ) {
		if ( val2!=NULL )
return( -1 );
		else if ( val1->type==v_str && pf->val == &default_encoding) {
		    Encoding *enc = FindOrMakeEncoding(val1->u.sval);
		    if ( enc==NULL )
return( -1 );
		    *((Encoding **) (pf->val)) = enc;
		} else
return( -1 );
	    } else if ( pf->type == pr_namelist ) {
		if ( val2!=NULL )
return( -1 );
		else if ( val1->type==v_str ) {
#ifndef LUA_FF_LIB
		    NameList *nl = NameListByName(val1->u.sval);
		    if ( strcmp(val1->u.sval,"NULL")==0 && pf->val != &namelist_for_new_fonts )
			  nl = NULL;
		    else if ( nl==NULL )
			  return( -1 );
		    *((NameList **) (pf->val)) = nl;
#endif
		} else
		  return( -1 );
	    } else
return( false );

	    CheckObsoletePrefs();
	    SavePrefs();
return( true );
	}
    }
return( false );
}

static char *getPfaEditPrefs(void) {
    static char *prefs=NULL;
    char buffer[1025];

    if ( prefs!=NULL )
return( prefs );
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/prefs", getPfaEditDir(buffer));
    prefs = copy(buffer);
return( prefs );
}

char *getPfaEditShareDir(void) {
    static char *sharedir=NULL;
    static int set=false;
    char *pt = NULL;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int len;
#endif

    if ( set )
return( sharedir );

    set = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    pt = strstr(GResourceProgramDir,"/bin");
#endif
    if ( pt==NULL )
return( NULL );
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    len = (pt-GResourceProgramDir)+strlen("/share/fontforge")+1;
    sharedir = galloc(len);
    strncpy(sharedir,GResourceProgramDir,pt-GResourceProgramDir);
    strcpy(sharedir+(pt-GResourceProgramDir),"/share/fontforge");
return( sharedir );
#endif
return( NULL );
}

#if !defined(FONTFORGE_CONFIG_GTK)
#ifndef LUA_FF_LIB
#  include <charset.h>		/* we still need the charsets & encoding to set local_encoding */
static int encmatch(const char *enc,int subok) {
    static struct { char *name; int enc; } encs[] = {
	{ "US-ASCII", e_usascii },
	{ "ASCII", e_usascii },
	{ "ISO646-NO", e_iso646_no },
	{ "ISO646-SE", e_iso646_se },
	{ "LATIN1", e_iso8859_1 },
	{ "ISO-8859-1", e_iso8859_1 },
	{ "ISO-8859-2", e_iso8859_2 },
	{ "ISO-8859-3", e_iso8859_3 },
	{ "ISO-8859-4", e_iso8859_4 },
	{ "ISO-8859-5", e_iso8859_4 },
	{ "ISO-8859-6", e_iso8859_4 },
	{ "ISO-8859-7", e_iso8859_4 },
	{ "ISO-8859-8", e_iso8859_4 },
	{ "ISO-8859-9", e_iso8859_4 },
	{ "ISO-8859-10", e_iso8859_10 },
	{ "ISO-8859-11", e_iso8859_11 },
	{ "ISO-8859-13", e_iso8859_13 },
	{ "ISO-8859-14", e_iso8859_14 },
	{ "ISO-8859-15", e_iso8859_15 },
	{ "ISO_8859-1", e_iso8859_1 },
	{ "ISO_8859-2", e_iso8859_2 },
	{ "ISO_8859-3", e_iso8859_3 },
	{ "ISO_8859-4", e_iso8859_4 },
	{ "ISO_8859-5", e_iso8859_4 },
	{ "ISO_8859-6", e_iso8859_4 },
	{ "ISO_8859-7", e_iso8859_4 },
	{ "ISO_8859-8", e_iso8859_4 },
	{ "ISO_8859-9", e_iso8859_4 },
	{ "ISO_8859-10", e_iso8859_10 },
	{ "ISO_8859-11", e_iso8859_11 },
	{ "ISO_8859-13", e_iso8859_13 },
	{ "ISO_8859-14", e_iso8859_14 },
	{ "ISO_8859-15", e_iso8859_15 },
	{ "ISO8859-1", e_iso8859_1 },
	{ "ISO8859-2", e_iso8859_2 },
	{ "ISO8859-3", e_iso8859_3 },
	{ "ISO8859-4", e_iso8859_4 },
	{ "ISO8859-5", e_iso8859_4 },
	{ "ISO8859-6", e_iso8859_4 },
	{ "ISO8859-7", e_iso8859_4 },
	{ "ISO8859-8", e_iso8859_4 },
	{ "ISO8859-9", e_iso8859_4 },
	{ "ISO8859-10", e_iso8859_10 },
	{ "ISO8859-11", e_iso8859_11 },
	{ "ISO8859-13", e_iso8859_13 },
	{ "ISO8859-14", e_iso8859_14 },
	{ "ISO8859-15", e_iso8859_15 },
	{ "ISO88591", e_iso8859_1 },
	{ "ISO88592", e_iso8859_2 },
	{ "ISO88593", e_iso8859_3 },
	{ "ISO88594", e_iso8859_4 },
	{ "ISO88595", e_iso8859_4 },
	{ "ISO88596", e_iso8859_4 },
	{ "ISO88597", e_iso8859_4 },
	{ "ISO88598", e_iso8859_4 },
	{ "ISO88599", e_iso8859_4 },
	{ "ISO885910", e_iso8859_10 },
	{ "ISO885911", e_iso8859_11 },
	{ "ISO885913", e_iso8859_13 },
	{ "ISO885914", e_iso8859_14 },
	{ "ISO885915", e_iso8859_15 },
	{ "8859_1", e_iso8859_1 },
	{ "8859_2", e_iso8859_2 },
	{ "8859_3", e_iso8859_3 },
	{ "8859_4", e_iso8859_4 },
	{ "8859_5", e_iso8859_4 },
	{ "8859_6", e_iso8859_4 },
	{ "8859_7", e_iso8859_4 },
	{ "8859_8", e_iso8859_4 },
	{ "8859_9", e_iso8859_4 },
	{ "8859_10", e_iso8859_10 },
	{ "8859_11", e_iso8859_11 },
	{ "8859_13", e_iso8859_13 },
	{ "8859_14", e_iso8859_14 },
	{ "8859_15", e_iso8859_15 },
	{ "KOI8-R", e_koi8_r },
	{ "KOI8R", e_koi8_r },
	{ "WINDOWS-1252", e_win },
	{ "CP1252", e_win },
	{ "Big5", e_big5 },
	{ "Big-5", e_big5 },
	{ "BigFive", e_big5 },
	{ "Big-Five", e_big5 },
	{ "Big5HKSCS", e_big5hkscs },
	{ "Big5-HKSCS", e_big5hkscs },
	{ "UTF-8", e_utf8 },
	{ "ISO-10646/UTF-8", e_utf8 },
	{ "ISO_10646/UTF-8", e_utf8 },
	{ "UCS2", e_unicode },
	{ "UCS-2", e_unicode },
	{ "UCS-2-INTERNAL", e_unicode },
	{ "ISO-10646", e_unicode },
	{ "ISO_10646", e_unicode },
#if 0
	{ "eucJP", e_euc },
	{ "EUC-JP", e_euc },
	{ "ujis", ??? },
	{ "EUC-KR", e_euckorean },
#endif
	{ NULL }};
    int i;
#if HAVE_ICONV_H
    static char *last_complaint;
#endif
    char buffer[80];

#if HAVE_ICONV_H
    iconv_t test;
    free(iconv_local_encoding_name);
    iconv_local_encoding_name= NULL;
#endif

    if ( strchr(enc,'@')!=NULL && strlen(enc)<sizeof(buffer)-1 ) {
	strcpy(buffer,enc);
	*strchr(buffer,'@') = '\0';
	enc = buffer;
    }

    for ( i=0; encs[i].name!=NULL; ++i )
	if ( strmatch(enc,encs[i].name)==0 )
return( encs[i].enc );

    if ( subok ) {
	for ( i=0; encs[i].name!=NULL; ++i )
	    if ( strstrmatch(enc,encs[i].name)!=NULL )
return( encs[i].enc );

#if HAVE_ICONV_H
	/* I only try to use iconv if the encoding doesn't match one I support*/
	/*  loading iconv unicode data takes a while */
	test = iconv_open(enc,FindUnicharName());
	if ( test==(iconv_t) (-1) || test==NULL ) {
	    if ( last_complaint==NULL || strcmp(last_complaint,enc)!=0 ) {
		fprintf( stderr, "Neither FontForge nor iconv() supports your encoding (%s) we will pretend\n you asked for latin1 instead.\n", enc );
		free( last_complaint );
		last_complaint = copy(enc);
	    }
	} else {
	    if ( last_complaint==NULL || strcmp(last_complaint,enc)!=0 ) {
		fprintf( stderr, "FontForge does not support your encoding (%s), it will try to use iconv()\n or it will pretend the local encoding is latin1\n", enc );
		free( last_complaint );
		last_complaint = copy(enc);
	    }
	    iconv_local_encoding_name= copy(enc);
	    iconv_close(test);
	}
#else
	fprintf( stderr, "FontForge does not support your encoding (%s), it will pretend the local encoding is latin1\n", enc );
#endif

return( e_iso8859_1 );
    }
return( e_unknown );
}

static int DefaultEncoding(void) {
    const char *loc;
    int enc;

#if HAVE_LANGINFO_H
    loc = nl_langinfo(CODESET);
    enc = encmatch(loc,false);
    if ( enc!=e_unknown )
return( enc );
#endif
    loc = getenv("LC_ALL");
    if ( loc==NULL ) loc = getenv("LC_CTYPE");
    /*if ( loc==NULL ) loc = getenv("LC_MESSAGES");*/
    if ( loc==NULL ) loc = getenv("LANG");

    if ( loc==NULL )
return( e_iso8859_1 );

    enc = encmatch(loc,false);
    if ( enc==e_unknown ) {
	loc = strrchr(loc,'.');
	if ( loc==NULL )
return( e_iso8859_1 );
	enc = encmatch(loc+1,true);
    }
    if ( enc==e_unknown )
return( e_iso8859_1 );

return( enc );
}
#endif
#endif

static void ParseMacMapping(char *pt,struct macsettingname *ms) {
    char *end;

    ms->mac_feature_type = strtol(pt,&end,10);
    if ( *end==',' ) ++end;
    ms->mac_feature_setting = strtol(end,&end,10);
    if ( *end==' ' ) ++end;
    ms->otf_tag =
	((end[0]&0xff)<<24) |
	((end[1]&0xff)<<16) |
	((end[2]&0xff)<<8) |
	(end[3]&0xff);
}

#ifndef LUA_FF_LIB
static void ParseNewMacFeature(FILE *p,char *line) {
    fseek(p,-(strlen(line)-strlen("MacFeat:")),SEEK_CUR);
    line[strlen("MacFeat:")] ='\0';
    default_mac_feature_map = SFDParseMacFeatures(p,line);
    fseek(p,-strlen(line),SEEK_CUR);
    if ( user_mac_feature_map!=NULL )
	MacFeatListFree(user_mac_feature_map);
    user_mac_feature_map = default_mac_feature_map;
}
#endif

static void DefaultXUID(void) {
    /* Adobe has assigned PfaEdit a base XUID of 1021. Each new user is going */
    /*  to get a couple of random numbers appended to that, hoping that will */
    /*  make for a fairly safe system. */
    /* FontForge will use the same scheme */
    int r1 = 0, r2 = 0;
    char buffer[50];
#ifndef LUA_FF_LIB /* TH odd, the crosscompiler gets ld errors from gettimeofday() */
    struct timeval tv;

    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
    do {
	r1 = rand()&0x3ff;
    } while ( r1==0 );		/* I reserve "0" for me! */
    gettimeofday(&tv,NULL);
    srandom(tv.tv_usec+1);
    r2 = random();
#endif
    sprintf( buffer, "1021 %d %d", r1, r2 );
    free(xuid);
    xuid = copy(buffer);
}

static void DefaultHelp(void) {
    if ( helpdir==NULL ) {
#ifdef DOCDIR
	helpdir = copy(DOCDIR "/");
#elif defined(SHAREDIR)
	helpdir = copy(SHAREDIR "/../doc/fontforge/");
#else
	helpdir = copy("/usr/local/share/doc/fontforge/");
#endif
    }
}

void SetDefaults(void) {

    DefaultXUID();
    DefaultHelp();
}

void LoadPrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100];
    int i=0, j=0, ri=0, mn=0, ms=0, fn=0, ff=0, filt_max=0;
    int msp=0, msc=0;
    char *pt;
    struct prefs_list *pl;

#if !defined(NOPLUGIN)
    LoadPluginDir(NULL);
#endif
    LoadPfaEditEncodings();
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    LoadGroupList();
#endif

    if ( prefs!=NULL && (p=fopen(prefs,"r"))!=NULL ) {
	while ( fgets(line,sizeof(line),p)!=NULL ) {
	    if ( *line=='#' )
	continue;
	    pt = strchr(line,':');
	    if ( pt==NULL )
	continue;
	    for ( j=0; load_prefs_list[j]!=NULL; ++j ) {
		for ( i=0; load_prefs_list[j][i].name!=NULL; ++i )
		    if ( strncmp(line,load_prefs_list[j][i].name,pt-line)==0 )
		break;
		if ( load_prefs_list[j][i].name!=NULL )
	    break;
	    }
	    pl = NULL;
	    if ( load_prefs_list[j]!=NULL )
		pl = &load_prefs_list[j][i];
	    for ( ++pt; *pt=='\t'; ++pt );
	    if ( line[strlen(line)-1]=='\n' )
		line[strlen(line)-1] = '\0';
	    if ( line[strlen(line)-1]=='\r' )
		line[strlen(line)-1] = '\0';
	    if ( pl==NULL ) {
		if ( strncmp(line,"Recent:",strlen("Recent:"))==0 && ri<RECENT_MAX )
		    RecentFiles[ri++] = copy(pt);
		else if ( strncmp(line,"MenuScript:",strlen("MenuScript:"))==0 && ms<SCRIPT_MENU_MAX )
		    script_filenames[ms++] = copy(pt);
		else if ( strncmp(line,"MenuName:",strlen("MenuName:"))==0 && mn<SCRIPT_MENU_MAX )
		    script_menu_names[mn++] = utf82u_copy(pt);
		else if ( strncmp(line,"FontFilterName:",strlen("FontFilterName:"))==0 ) {
		    if ( fn>=filt_max )
			user_font_filters = grealloc(user_font_filters,((filt_max+=10)+1)*sizeof( struct openfilefilters));
		    user_font_filters[fn].filter = NULL;
		    user_font_filters[fn++].name = copy(pt);
		    user_font_filters[fn].name = NULL;
		} else if ( strncmp(line,"FontFilter:",strlen("FontFilter:"))==0 ) {
		    if ( ff<filt_max )
			user_font_filters[ff++].filter = copy(pt);
		} else if ( strncmp(line,"MacMapCnt:",strlen("MacSetCnt:"))==0 ) {
		    sscanf( pt, "%d", &msc );
		    msp = 0;
		    user_macfeat_otftag = gcalloc(msc+1,sizeof(struct macsettingname));
		} else if ( strncmp(line,"MacMapping:",strlen("MacMapping:"))==0 && msp<msc ) {
		    ParseMacMapping(pt,&user_macfeat_otftag[msp++]);
#ifndef LUA_FF_LIB
		} else if ( strncmp(line,"MacFeat:",strlen("MacFeat:"))==0 ) {
		    ParseNewMacFeature(p,line);
#endif
		}
	continue;
	    }
	    switch ( pl->type ) {
	      case pr_encoding:
		{ Encoding *enc = FindOrMakeEncoding(pt);
		    if ( enc==NULL )
			enc = FindOrMakeEncoding("ISO8859-1");
		    if ( enc==NULL )
			enc = &custom;
		    *((Encoding **) (pl->val)) = enc;
		}
	      break;
	      case pr_namelist:
#ifndef LUA_FF_LIB
		{ NameList *nl = NameListByName(pt);
		    if ( strcmp(pt,"NULL")==0 && pl->val != &namelist_for_new_fonts )
			*((NameList **) (pl->val)) = NULL;
		    else if ( nl!=NULL )
			*((NameList **) (pl->val)) = nl;
		}
#endif
	      break;
	      case pr_bool: case pr_int:
		sscanf( pt, "%d", (int *) pl->val );
	      break;
	      case pr_real:
		sscanf( pt, "%f", (float *) pl->val );
	      break;
	      case pr_string: case pr_file:
		if ( *pt=='\0' ) pt=NULL;
		if ( pl->val!=NULL )
		    *((char **) (pl->val)) = copy(pt);
		else
		    (pl->set)(copy(pt));
	      break;
		case pr_enum:
		  break;
	    }
	}
	fclose(p);
    }
#if defined(FONTFORGE_CONFIG_GTK)
    /* Nothing */;
#else
#ifndef LUA_FF_LIB
    local_encoding = DefaultEncoding();
#endif
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( xdefs_filename!=NULL )
	GResourceAddResourceFile(xdefs_filename,GResourceProgramName);
#endif
#ifndef LUA_FF_LIB
    if ( othersubrsfile!=NULL && ReadOtherSubrsFile(othersubrsfile)<=0 )
	fprintf( stderr, "Failed to read OtherSubrs from %s\n", othersubrsfile );
	
#endif
    if ( glyph_2_name_map ) {
#ifndef LUA_FF_LIB
	old_ttf_flags |= ttf_flag_glyphmap;
	old_otf_flags |= ttf_flag_glyphmap;
#endif
    }
#ifndef LUA_FF_LIB
    LoadNamelistDir(NULL);
#endif
}

void PrefDefaultEncoding(void) {
#ifndef LUA_FF_LIB
    local_encoding = DefaultEncoding();
#endif
}

void _SavePrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    int i, j;
    char *temp;
    struct prefs_list *pl;

    if ( prefs==NULL )
return;
    if ( (p=fopen(prefs,"w"))==NULL )
return;

    for ( j=0; prefs_list[j]!=NULL; ++j ) for ( i=0; prefs_list[j][i].name!=NULL; ++i ) {
	pl = &prefs_list[j][i];
	switch ( pl->type ) {
	  case pr_encoding:
	    fprintf( p, "%s:\t%s\n", pl->name, (*((Encoding **) (pl->val)))->enc_name );
	  break;
	  case pr_namelist:
	    fprintf( p, "%s:\t%s\n", pl->name, *((NameList **) (pl->val))==NULL ? "NULL" :
		    (*((NameList **) (pl->val)))->title );
	  break;
	  case pr_bool: case pr_int:
	    fprintf( p, "%s:\t%d\n", pl->name, *(int *) (pl->val) );
	  break;
	  case pr_real:
	    fprintf( p, "%s:\t%g\n", pl->name, (double) *(float *) (pl->val) );
	  break;
	  case pr_string: case pr_file:
	    if ( (pl->val)!=NULL )
		temp = *(char **) (pl->val);
	    else
		temp = (char *) (pl->get());
	    if ( temp!=NULL )
		fprintf( p, "%s:\t%s\n", pl->name, temp );
	    if ( (pl->val)==NULL )
		free(temp);
	  break;
		case pr_enum:
		  break;
	}
    }

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	fprintf( p, "Recent:\t%s\n", RecentFiles[i]);
    for ( i=0; i<SCRIPT_MENU_MAX && script_filenames[i]!=NULL; ++i ) {
	fprintf( p, "MenuScript:\t%s\n", script_filenames[i]);
	fprintf( p, "MenuName:\t%s\n", temp = u2utf8_copy(script_menu_names[i]));
	free(temp);
    }
    if ( user_font_filters!=NULL ) {
	for ( i=0; user_font_filters[i].name!=NULL; ++i ) {
	    fprintf( p, "FontFilterName:\t%s\n", user_font_filters[i].name);
	    fprintf( p, "FontFilter:\t%s\n", user_font_filters[i].filter);
	}
    }
    if ( user_macfeat_otftag!=NULL && UserSettingsDiffer()) {
	for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i );
	fprintf( p, "MacMapCnt: %d\n", i );
	for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i ) {
	    fprintf( p, "MacMapping: %d,%d %c%c%c%c\n",
		    user_macfeat_otftag[i].mac_feature_type,
		    user_macfeat_otftag[i].mac_feature_setting,
			(int) (user_macfeat_otftag[i].otf_tag>>24),
			(int) ((user_macfeat_otftag[i].otf_tag>>16)&0xff),
			(int) ((user_macfeat_otftag[i].otf_tag>>8)&0xff),
			(int) (user_macfeat_otftag[i].otf_tag&0xff) );
	}
    }

#ifndef LUA_FF_LIB
    if ( UserFeaturesDiffer())
	SFDDumpMacFeat(p,default_mac_feature_map);
#endif
    fclose(p);
}

void SavePrefs(void) {
    extern int running_script;
    if ( running_script )
return;
    _SavePrefs();
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
struct pref_data {
    int done;
};

static int Prefs_ScriptBrowse(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *tf = GWidgetGetControl(gw,GGadgetGetCid(g)-SCRIPT_MENU_MAX);
	char *cur = GGadgetGetTitle8(tf); char *ret;

	if ( *cur=='\0' ) cur=NULL;
	ret = gwwv_open_filename(_("Call Script"), cur, "*.pe", NULL);
	free(cur);
	if ( ret==NULL )
return(true);
	GGadgetSetTitle8(tf,ret);
	free(ret);
    }
return( true );
}

static int Prefs_BrowseFile(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *tf = GWidgetGetControl(gw,GGadgetGetCid(g)-CID_PrefsBrowseOffset);
	char *cur = GGadgetGetTitle8(tf); char *ret;
	struct prefs_list *pl = GGadgetGetUserData(tf);

	ret = gwwv_open_filename(pl->name, *cur=='\0'? NULL : cur, NULL, NULL);
	free(cur);
	if ( ret==NULL )
return(true);
	GGadgetSetTitle8(tf,ret);
	free(ret);
    }
return( true );
}

static GTextInfo *Pref_MappingList(int use_user) {
    struct macsettingname *msn = use_user && user_macfeat_otftag!=NULL ?
	    user_macfeat_otftag :
	    macfeat_otftag;
    GTextInfo *ti;
    int i;
    char buf[60];

    for ( i=0; msn[i].otf_tag!=0; ++i );
    ti = gcalloc(i+1,sizeof( GTextInfo ));

    for ( i=0; msn[i].otf_tag!=0; ++i ) {
	sprintf(buf,"%3d,%2d %c%c%c%c",
	    msn[i].mac_feature_type, msn[i].mac_feature_setting,
	    (int) (msn[i].otf_tag>>24), (int) ((msn[i].otf_tag>>16)&0xff), (int) ((msn[i].otf_tag>>8)&0xff), (int) (msn[i].otf_tag&0xff) );
	ti[i].text = uc_copy(buf);
    }
return( ti );
}

void GListAddStr(GGadget *list,unichar_t *str, void *ud) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    GTextInfo **replace = galloc((len+2)*sizeof(GTextInfo *));

    replace[len+1] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<len; ++i ) {
	replace[i] = galloc(sizeof(GTextInfo));
	*replace[i] = *ti[i];
	replace[i]->text = u_copy(ti[i]->text);
    }
    replace[i] = gcalloc(1,sizeof(GTextInfo));
    replace[i]->fg = replace[i]->bg = COLOR_DEFAULT;
    replace[i]->text = str;
    replace[i]->userdata = ud;
    GGadgetSetList(list,replace,false);
}

void GListReplaceStr(GGadget *list,int index, unichar_t *str, void *ud) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    GTextInfo **replace = galloc((len+2)*sizeof(GTextInfo *));

    for ( i=0; i<len; ++i ) {
	replace[i] = galloc(sizeof(GTextInfo));
	*replace[i] = *ti[i];
	if ( i!=index )
	    replace[i]->text = u_copy(ti[i]->text);
    }
    replace[i] = gcalloc(1,sizeof(GTextInfo));
    replace[index]->text = str;
    replace[index]->userdata = ud;
    GGadgetSetList(list,replace,false);
}

struct setdata {
    GWindow gw;
    GGadget *list;
    GGadget *flist;
    GGadget *feature;
    GGadget *set_code;
    GGadget *otf;
    GGadget *ok;
    GGadget *cancel;
    int index;
    int done;
    unichar_t *ret;
};

static int set_e_h(GWindow gw, GEvent *event) {
    struct setdata *sd = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti;
    const unichar_t *ret1; unichar_t *end;
    int on, feat, val1, val2;
    unichar_t ubuf[4];
    char buf[40];

    if ( event->type==et_close ) {
	sd->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html#Features");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	if ( event->u.control.g == sd->cancel ) {
	    sd->done = true;
	} else if ( event->u.control.g == sd->ok ) {
	    ret1 = _GGadgetGetTitle(sd->set_code);
	    on = u_strtol(ret1,&end,10);
	    if ( *end!='\0' ) {
		gwwv_post_error(_("Bad Number"),_("Bad Number"));
return( true );
	    }
	    ret1 = _GGadgetGetTitle(sd->feature);
	    feat = u_strtol(ret1,&end,10);
	    if ( *end!='\0' && *end!=' ' ) {
		gwwv_post_error(_("Bad Number"),_("Bad Number"));
return( true );
	    }
	    ti = GGadgetGetList(sd->list,&len);
	    for ( i=0; i<len; ++i ) if ( i!=sd->index ) {
		val1 = u_strtol(ti[i]->text,&end,10);
		val2 = u_strtol(end+1,NULL,10);
		if ( val1==feat && val2==on ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		    static char *buts[3];
		    buts[0] = _("_Yes");
		    buts[1] = _("_No");
		    buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
		    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_NO, NULL };
#endif
		    if ( gwwv_ask(_("This feature, setting combination is already used"),(const char **) buts,0,1,
			    _("This feature, setting combination is already used\nDo you really wish to reuse it?"))==1 )
return( true );
		}
	    }

	    ret1 = _GGadgetGetTitle(sd->otf);
	    if ( (ubuf[0] = ret1[0])==0 )
		ubuf[0] = ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[1] = ret1[1])==0 )
		ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[2] = ret1[2])==0 )
		ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[3] = ret1[3])==0 )
		ubuf[3] = ' ';
	    len = u_strlen(ret1);
	    if ( len<2 || len>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		gwwv_post_error(_("Tag too long"),_("Feature tags must be exactly 4 ASCII characters"));
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Tag too long"),_("Feature tags must be exactly 4 ASCII characters"));
#endif
return( true );
	    }
	    sprintf(buf,"%3d,%2d %c%c%c%c",
		    feat, on,
		    ubuf[0], ubuf[1], ubuf[2], ubuf[3]);
	    sd->done = true;
	    sd->ret = uc_copy(buf);
	}
    }
return( true );
}
    
static unichar_t *AskSetting(struct macsettingname *temp,GGadget *list, int index,GGadget *flist) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17];
    GTextInfo label[17];
    struct setdata sd;
    char buf[20];
    unichar_t ubuf3[6];
    int32 len, i;
    GTextInfo **ti;

    memset(&sd,0,sizeof(sd));
    sd.list = list;
    sd.flist = flist;
    sd.index = index;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Mapping");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,240));
    pos.height = GDrawPointsToPixels(NULL,120);
    gw = GDrawCreateTopWindow(NULL,&pos,set_e_h,&sd,&wattrs);
    sd.gw = gw;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _("_Feature:");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+4;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 50; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 170;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].creator = GListButtonCreate;

    label[2].text = (unichar_t *) _("Setting");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[0].gd.pos.y+26;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( buf, "%d", temp->mac_feature_setting );
    label[3].text = (unichar_t *) buf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y-4; gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) _("_Tag:");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+26; 
    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].creator = GLabelCreate;

    ubuf3[0] = temp->otf_tag>>24; ubuf3[1] = (temp->otf_tag>>16)&0xff; ubuf3[2] = (temp->otf_tag>>8)&0xff; ubuf3[3] = temp->otf_tag&0xff; ubuf3[4] = 0;
    label[5].text = ubuf3;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = gcd[3].gd.pos.x; gcd[5].gd.pos.y = gcd[4].gd.pos.y-4; gcd[5].gd.pos.width = 50;
    gcd[5].gd.flags = gg_enabled|gg_visible;
    /*gcd[5].gd.u.list = tags;*/
    gcd[5].creator = GTextFieldCreate;

    gcd[6].gd.pos.x = 13-3; gcd[6].gd.pos.y = gcd[5].gd.pos.y+30;
    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[6].text = (unichar_t *) _("_OK");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    /*gcd[6].gd.handle_controlevent = Prefs_Ok;*/
    gcd[6].creator = GButtonCreate;

    gcd[7].gd.pos.x = -13; gcd[7].gd.pos.y = gcd[7-1].gd.pos.y+3;
    gcd[7].gd.pos.width = -1; gcd[7].gd.pos.height = 0;
    gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[7].text = (unichar_t *) _("_Cancel");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    sd.feature = gcd[1].ret;
    sd.set_code = gcd[3].ret;
    sd.otf = gcd[5].ret;
    sd.ok = gcd[6].ret;
    sd.cancel = gcd[7].ret;

    ti = GGadgetGetList(flist,&len);
    GGadgetSetList(sd.feature,ti,true);
    for ( i=0; i<len; ++i ) {
	int val = u_strtol(ti[i]->text,NULL,10);
	if ( val==temp->mac_feature_type ) {
	    GGadgetSetTitle(sd.feature,ti[i]->text);
    break;
	}
    }

    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
    while ( !sd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

return( sd.ret );
}

static void ChangeSetting(GGadget *list,int index,GGadget *flist) {
    struct macsettingname temp;
    int32 len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    char *str;
    unichar_t *ustr;

    str = cu_copy(ti[index]->text);
    ParseMacMapping(str,&temp);
    free(str);
    if ( (ustr=AskSetting(&temp,list,index,flist))==NULL )
return;
    GListReplaceStr(list,index,ustr,NULL);
}

static int Pref_NewMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_Mapping);
	GGadget *flist = GWidgetGetControl(GDrawGetParentWindow(gw),CID_Features);
	struct macsettingname temp;
	unichar_t *str;

	memset(&temp,0,sizeof(temp));
	temp.mac_feature_type = -1;
	if ( (str=AskSetting(&temp,list,-1,flist))==NULL )
return( true );
	GListAddStr(list,str,NULL);
	/*free(str);*/
    }
return( true );
}

static int Pref_DelMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GListDelSelected(GWidgetGetControl(gw,CID_Mapping));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingEdit),false);
    }
return( true );
}

static int Pref_EditMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(gw,CID_Mapping);
	GGadget *flist = GWidgetGetControl(gw,CID_Features);
	ChangeSetting(list,GGadgetGetFirstListSelectedItem(list),flist);
    }
return( true );
}

static int Pref_MappingSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingDel),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	GGadget *flist = GWidgetGetControl( GDrawGetParentWindow(GGadgetGetWindow(g)),CID_Features);
	ChangeSetting(g,e->u.control.u.list.changed_index!=-1?e->u.control.u.list.changed_index:
		GGadgetGetFirstListSelectedItem(g),flist);
    }
return( true );
}

static int Pref_DefaultMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_Mapping);
	GTextInfo *ti, **arr;
	uint16 cnt;

	ti = Pref_MappingList(false);
	arr = GTextInfoArrayFromList(ti,&cnt);
	GGadgetSetList(list,arr,false);
	GTextInfoListFree(ti);
    }
return( true );
}

static int Prefs_Ok(GGadget *g, GEvent *e) {
    int i, j, mi;
    int err=0, enc;
    struct pref_data *p;
    GWindow gw;
    const unichar_t *ret;
    const unichar_t *names[SCRIPT_MENU_MAX], *scripts[SCRIPT_MENU_MAX];
    struct prefs_list *pl;
    GTextInfo **list;
    int32 len;
    int maxl, t;
    char *str;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	p = GDrawGetUserData(gw);
	for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	    names[i] = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ScriptMNameBase+i));
	    scripts[i] = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ScriptMFileBase+i));
	    if ( *names[i]=='\0' ) names[i] = NULL;
	    if ( *scripts[i]=='\0' ) scripts[i] = NULL;
	    if ( scripts[i]==NULL && names[i]!=NULL ) {
		gwwv_post_error(_("Menu name with no associated script"),_("Menu name with no associated script"));
return( true );
	    } else if ( scripts[i]!=NULL && names[i]==NULL ) {
		gwwv_post_error(_("Script with no associated menu name"),_("Script with no associated menu name"));
return( true );
	    }
	}
	for ( i=mi=0; i<SCRIPT_MENU_MAX; ++i ) {
	    if ( names[i]!=NULL ) {
		names[mi] = names[i];
		scripts[mi] = scripts[i];
		++mi;
	    }
	}
	for ( j=0; visible_prefs_list[j].tab_name!=0; ++j ) for ( i=0; visible_prefs_list[j].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[j].pl[i];
	    /* before assigning values, check for any errors */
	    /* if any errors, then NO values should be assigned, in case they cancel */
	    if ( pl->dontdisplay )
	continue;
	    if ( pl->type==pr_int ) {
		GetInt8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    } else if ( pl->type==pr_int ) {
		GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    }
	}
	if ( err )
return( true );

	for ( j=0; visible_prefs_list[j].tab_name!=0; ++j ) for ( i=0; visible_prefs_list[j].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[j].pl[i];
	    if ( pl->dontdisplay )
	continue;
	    switch( pl->type ) {
	      case pr_int:
	        *((int *) (pl->val)) = GetInt8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	      break;
	      case pr_bool:
	        *((int *) (pl->val)) = GGadgetIsChecked(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
	      break;
	      case pr_real:
	        *((float *) (pl->val)) = GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	      break;
	      case pr_encoding:
		{ Encoding *e;
		    e = ParseEncodingNameFromList(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
		    if ( e!=NULL )
			*((Encoding **) (pl->val)) = e;
		    enc = 1;	/* So gcc doesn't complain about unused. It is unused, but why add the ifdef and make the code even messier? Sigh. icc complains anyway */
		}
	      break;
	      case pr_namelist:
		{ NameList *nl;
		  GTextInfo *ti = GGadgetGetListItemSelected(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
		  if ( ti!=NULL ) {
			char *name = u2utf8_copy(ti->text);
			nl = NameListByName(name);
			free(name);
			if ( nl!=NULL && nl->uses_unicode && !allow_utf8_glyphnames)
			    gwwv_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
			else if ( nl!=NULL )
			    *((NameList **) (pl->val)) = nl;
		    }
		}
	      break;
	      case pr_string: case pr_file:
	        ret = _GGadgetGetTitle(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
		if ( pl->val!=NULL ) {
		    free( *((char **) (pl->val)) );
		    *((char **) (pl->val)) = NULL;
		    if ( ret!=NULL && *ret!='\0' )
			*((char **) (pl->val)) = /* u2def_*/ cu_copy(ret);
		} else {
		    char *cret = cu_copy(ret);
		    (pl->set)(cret);
		    free(cret);
		}
	      break;
	    }
	}
	for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	    free(script_menu_names[i]); script_menu_names[i] = NULL;
	    free(script_filenames[i]); script_filenames[i] = NULL;
	}
	for ( i=0; i<mi; ++i ) {
	    script_menu_names[i] = u_copy(names[i]);
	    script_filenames[i] = u2def_copy(scripts[i]);
	}

	list = GGadgetGetList(GWidgetGetControl(gw,CID_Mapping),&len);
	UserSettingsFree();
	user_macfeat_otftag = galloc((len+1)*sizeof(struct macsettingname));
	user_macfeat_otftag[len].otf_tag = 0;
	maxl = 0;
	for ( i=0; i<len; ++i ) {
	    t = u_strlen(list[i]->text);
	    if ( t>maxl ) maxl = t;
	}
	str = galloc(maxl+3);
	for ( i=0; i<len; ++i ) {
	    u2encoding_strncpy(str,list[i]->text,maxl+1,e_mac);
	    ParseMacMapping(str,&user_macfeat_otftag[i]);
	}
	free(str);

	Prefs_ReplaceMacFeatures(GWidgetGetControl(gw,CID_Features));

	if ( xuid!=NULL ) {
	    char *pt;
	    for ( pt=xuid; *pt==' ' ; ++pt );
	    if ( *pt=='[' ) {	/* People who know PS well, might want to put brackets arround the xuid base array, but I don't want them */
		pt = copy(pt+1);
		free( xuid );
		xuid = pt;
	    }
	    for ( pt=xuid+strlen(xuid)-1; pt>xuid && *pt==' '; --pt );
	    if ( pt >= xuid && *pt==']' ) *pt = '\0';
	}

	p->done = true;
	SavePrefs();
	if ( maxundoes==0 ) { FontView *fv;
	    for ( fv=fv_list ; fv!=NULL; fv=fv->next )
		SFRemoveUndoes(fv->sf,NULL,NULL);
	}
	if ( othersubrsfile!=NULL && ReadOtherSubrsFile(othersubrsfile)<=0 )
	    fprintf( stderr, "Failed to read OtherSubrs from %s\n", othersubrsfile );
    }
return( true );
}

static int Prefs_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct pref_data *p = GDrawGetUserData(GGadgetGetWindow(g));
	MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(
		GGadgetGetWindow(g),CID_Features))));
	p->done = true;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct pref_data *p = GDrawGetUserData(gw);
	p->done = true;
	MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(gw,CID_Features))));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html");
return( true );
	}
return( false );
    }
return( true );
}

static void PrefsInit(void) {
    static int done = false;
    int i;

    if ( done )
return;
    done = true;
    for ( i=0; visible_prefs_list[i].tab_name!=NULL; ++i )
	visible_prefs_list[i].tab_name = _(visible_prefs_list[i].tab_name);
}

void DoPrefs(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData *pgcd, gcd[5], sgcd[45], mgcd[3], mfgcd[9], msgcd[9];
    GGadgetCreateData mfboxes[3], *mfarray[14];
    GGadgetCreateData mpboxes[3], *mparray[14];
    GGadgetCreateData sboxes[2], *sarray[50];
    GGadgetCreateData mboxes[3], *varray[5], *harray[8];
    GTextInfo *plabel, **list, label[5], slabel[45], *plabels[TOPICS+5], mflabels[9], mslabels[9];
    GTabInfo aspects[TOPICS+5], subaspects[3];
    GGadgetCreateData **hvarray, boxes[2*TOPICS];
    struct pref_data p;
    int i, gc, sgc, j, k, line, line_max, y, y2, ii, si;
    int32 llen;
    char buf[20];
    int gcnt[20];
    static unichar_t nullstr[] = { 0 };
    struct prefs_list *pl;
    char *tempstr;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    FontRequest rq;
    GFont *font;

    PrefsInit();

    MfArgsInit();
    for ( k=line_max=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	for ( i=line=gcnt[k]=0; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	    if ( visible_prefs_list[k].pl[i].dontdisplay )
	continue;
	    gcnt[k] += 2;
	    if ( visible_prefs_list[k].pl[i].type==pr_bool ) ++gcnt[k];
	    else if ( visible_prefs_list[k].pl[i].type==pr_file ) ++gcnt[k];
	    ++line;
	}
	if ( visible_prefs_list[k].pl == args_list ) {
	    gcnt[k] += 6;
	    line += 6;
	}
	if ( line>line_max ) line_max = line;
    }

    memset(&p,'\0',sizeof(p));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Preferences...");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,290));
    pos.height = GDrawPointsToPixels(NULL,line_max*26+69);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    memset(sgcd,0,sizeof(sgcd));
    memset(slabel,0,sizeof(slabel));
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&msgcd,0,sizeof(msgcd));
    memset(&mflabels,0,sizeof(mflabels));
    memset(&mslabels,0,sizeof(mslabels));
    memset(&mfboxes,0,sizeof(mfboxes));
    memset(&mpboxes,0,sizeof(mpboxes));
    memset(&sboxes,0,sizeof(sboxes));
    memset(&boxes,0,sizeof(boxes));

    GCDFillMacFeat(mfgcd,mflabels,250,default_mac_feature_map, true, mfboxes, mfarray);

    sgc = 0;

    msgcd[sgc].gd.pos.x = 6; msgcd[sgc].gd.pos.y = 6;
    msgcd[sgc].gd.pos.width = 250; msgcd[sgc].gd.pos.height = 16*12+10;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
    msgcd[sgc].gd.cid = CID_Mapping;
    msgcd[sgc].gd.u.list = Pref_MappingList(true);
    msgcd[sgc].gd.handle_controlevent = Pref_MappingSel;
    msgcd[sgc++].creator = GListCreate;
    mparray[0] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = 6; msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y+msgcd[sgc-1].gd.pos.height+10;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled;
    mslabels[sgc].text = (unichar_t *) S_("MacMap|_New...");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.handle_controlevent = Pref_NewMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[4] = GCD_Glue; mparray[5] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible ;
    mslabels[sgc].text = (unichar_t *) _("_Delete");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.cid = CID_MappingDel;
    msgcd[sgc].gd.handle_controlevent = Pref_DelMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[5] = GCD_Glue; mparray[6] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible ;
    mslabels[sgc].text = (unichar_t *) _("_Edit...");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.cid = CID_MappingEdit;
    msgcd[sgc].gd.handle_controlevent = Pref_EditMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[7] = GCD_Glue; mparray[8] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled;
    mslabels[sgc].text = (unichar_t *) S_("MacMapping|Default");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.handle_controlevent = Pref_DefaultMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[9] = GCD_Glue; mparray[10] = &msgcd[sgc-1];
    mparray[11] = GCD_Glue; mparray[12] = NULL;

    mpboxes[2].gd.flags = gg_enabled|gg_visible;
    mpboxes[2].gd.u.boxelements = mparray+4;
    mpboxes[2].creator = GHBoxCreate;
    mparray[1] = GCD_Glue;
    mparray[2] = &mpboxes[2];
    mparray[3] = NULL;

    mpboxes[0].gd.flags = gg_enabled|gg_visible;
    mpboxes[0].gd.u.boxelements = mparray;
    mpboxes[0].creator = GVBoxCreate;

    sgc = 0;
    y2=5;
    si = 0;

    slabel[sgc].text = (unichar_t *) _("Menu Name");
    slabel[sgc].text_is_1byte = true;
    sgcd[sgc].gd.label = &slabel[sgc];
    sgcd[sgc].gd.popup_msg = (unichar_t *) _("You may create a script menu containing up to 10 frequently used scripts.\nEach entry in the menu needs both a name to display in the menu and\na script file to execute. The menu name may contain any unicode characters.\nThe button labeled \"...\" will allow you to browse for a script file.");
    sgcd[sgc].gd.pos.x = 8;
    sgcd[sgc].gd.pos.y = y2;
    sgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    sgcd[sgc++].creator = GLabelCreate;
    sarray[si++] = &sgcd[sgc-1];

    slabel[sgc].text = (unichar_t *) _("Script File");
    slabel[sgc].text_is_1byte = true;
    sgcd[sgc].gd.label = &slabel[sgc];
    sgcd[sgc].gd.popup_msg = (unichar_t *) _("You may create a script menu containing up to 10 frequently used scripts\nEach entry in the menu needs both a name to display in the menu and\na script file to execute. The menu name may contain any unicode characters.\nThe button labeled \"...\" will allow you to browse for a script file.");
    sgcd[sgc].gd.pos.x = 110;
    sgcd[sgc].gd.pos.y = y2;
    sgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    sgcd[sgc++].creator = GLabelCreate;
    sarray[si++] = &sgcd[sgc-1];
    sarray[si++] = GCD_Glue;
    sarray[si++] = NULL;

    y2 += 14;

    for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	sgcd[sgc].gd.pos.x = 8; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_text_xim;
	slabel[sgc].text = script_menu_names[i]==NULL?nullstr:script_menu_names[i];
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+CID_ScriptMNameBase;
	sgcd[sgc++].creator = GTextFieldCreate;
	sarray[si++] = &sgcd[sgc-1];

	sgcd[sgc].gd.pos.x = 110; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled;
	slabel[sgc].text = (unichar_t *) (script_filenames[i]==NULL?"":script_filenames[i]);
	slabel[sgc].text_is_1byte = true;
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+CID_ScriptMFileBase;
	sgcd[sgc++].creator = GTextFieldCreate;
	sarray[si++] = &sgcd[sgc-1];

	sgcd[sgc].gd.pos.x = 210; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled;
	slabel[sgc].text = (unichar_t *) _("...");
	slabel[sgc].text_is_1byte = true;
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+CID_ScriptMBrowseBase;
	sgcd[sgc].gd.handle_controlevent = Prefs_ScriptBrowse;
	sgcd[sgc++].creator = GButtonCreate;
	sarray[si++] = &sgcd[sgc-1];
	sarray[si++] = NULL;

	y2 += 26;
    }
    sarray[si++] = GCD_Glue; sarray[si++] = GCD_Glue; sarray[si++] = GCD_Glue;
    sarray[si++] = NULL;
    sarray[si++] = NULL;

    sboxes[0].gd.flags = gg_enabled|gg_visible;
    sboxes[0].gd.u.boxelements = sarray;
    sboxes[0].creator = GHVBoxCreate;

    memset(&mgcd,0,sizeof(mgcd));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&subaspects,'\0',sizeof(subaspects));
    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&aspects,'\0',sizeof(aspects));
    aspects[0].selected = true;

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	pgcd = gcalloc(gcnt[k]+4,sizeof(GGadgetCreateData));
	plabel = gcalloc(gcnt[k]+4,sizeof(GTextInfo));
	hvarray = gcalloc((gcnt[k]+6)*5+2,sizeof(GGadgetCreateData *));

	aspects[k].text = (unichar_t *) visible_prefs_list[k].tab_name;
	aspects[k].text_is_1byte = true;
	aspects[k].gcd = &boxes[2*k];
	aspects[k].nesting = visible_prefs_list[k].nest;
	plabels[k] = plabel;

	gc = si = 0;
	for ( i=line=0, y=5; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[k].pl[i];
	    if ( pl->dontdisplay )
	continue;
	    plabel[gc].text = (unichar_t *) _(pl->name);
	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = '\0';
	    pgcd[gc].gd.popup_msg = (unichar_t *) _(pl->popup);
	    pgcd[gc].gd.pos.x = 8;
	    pgcd[gc].gd.pos.y = y + 6;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    pgcd[gc++].creator = GLabelCreate;
	    hvarray[si++] = &pgcd[gc-1];

	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = '\0';
	    pgcd[gc].gd.popup_msg = (unichar_t *) _(pl->popup);
	    pgcd[gc].gd.pos.x = 110;
	    pgcd[gc].gd.pos.y = y;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    pgcd[gc].data = pl;
	    pgcd[gc].gd.cid = k*CID_PrefsOffset+CID_PrefsBase+i;
	    switch ( pl->type ) {
	      case pr_bool:
		plabel[gc].text = (unichar_t *) _("On");
		pgcd[gc].gd.pos.y += 3;
		pgcd[gc++].creator = GRadioCreate;
		hvarray[si++] = &pgcd[gc-1];
		pgcd[gc] = pgcd[gc-1];
		pgcd[gc].gd.pos.x += 50;
		pgcd[gc].gd.cid = 0;
		pgcd[gc].gd.label = &plabel[gc];
		plabel[gc].text = (unichar_t *) _("Off");
		plabel[gc].text_is_1byte = true;
		hvarray[si++] = &pgcd[gc];
		hvarray[si++] = GCD_Glue;
		if ( *((int *) pl->val))
		    pgcd[gc-1].gd.flags |= gg_cb_on;
		else
		    pgcd[gc].gd.flags |= gg_cb_on;
		++gc;
		y += 22;
	      break;
	      case pr_int:
		sprintf(buf,"%d", *((int *) pl->val));
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_real:
		sprintf(buf,"%g", *((float *) pl->val));
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_encoding:
		pgcd[gc].gd.u.list = GetEncodingTypes();
		pgcd[gc].gd.label = EncodingTypesFindEnc(pgcd[gc].gd.u.list,
			*(Encoding **) pl->val);
		for ( ii=0; pgcd[gc].gd.u.list[ii].text!=NULL ||pgcd[gc].gd.u.list[ii].line; ++ii )
		    if ( pgcd[gc].gd.u.list[ii].userdata!=NULL &&
			    (strcmp(pgcd[gc].gd.u.list[ii].userdata,"Compacted")==0 ||
			     strcmp(pgcd[gc].gd.u.list[ii].userdata,"Original")==0 ))
			pgcd[gc].gd.u.list[ii].disabled = true;
		pgcd[gc].creator = GListFieldCreate;
		pgcd[gc].gd.pos.width = 160;
		if ( pgcd[gc].gd.label==NULL ) pgcd[gc].gd.label = &encodingtypes[0];
		++gc;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		y += 28;
	      break;
	      case pr_namelist:
	        { char **nlnames = AllNamelistNames();
		int cnt;
		GTextInfo *namelistnames;
		for ( cnt=0; nlnames[cnt]!=NULL; ++cnt);
		namelistnames = gcalloc(cnt+1,sizeof(GTextInfo));
		for ( cnt=0; nlnames[cnt]!=NULL; ++cnt) {
		    namelistnames[cnt].text = (unichar_t *) nlnames[cnt];
		    namelistnames[cnt].text_is_1byte = true;
		    if ( strcmp(_((*(NameList **) (pl->val))->title),nlnames[cnt])==0 ) {
			namelistnames[cnt].selected = true;
			pgcd[gc].gd.label = &namelistnames[cnt];
		    }
		}
		pgcd[gc].gd.u.list = namelistnames;
		pgcd[gc].creator = GListButtonCreate;
		pgcd[gc].gd.pos.width = 160;
		++gc;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		y += 28;
	      } break;
	      case pr_string: case pr_file:
		if ( pl->set==SetAutoTraceArgs || ((char **) pl->val)==&mf_args )
		    pgcd[gc].gd.pos.width = 160;
		if ( pl->val!=NULL )
		    tempstr = *((char **) (pl->val));
		else
		    tempstr = (char *) ((pl->get)());
		if ( tempstr!=NULL )
		    plabel[gc].text = /* def2u_*/ uc_copy( tempstr );
		else if ( ((char **) pl->val)==&BDFFoundry )
		    plabel[gc].text = /* def2u_*/ uc_copy( "FontForge" );
		else
		    plabel[gc].text = /* def2u_*/ uc_copy( "" );
		plabel[gc].text_is_1byte = false;
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		if ( pl->type==pr_file ) {
		    pgcd[gc] = pgcd[gc-1];
		    pgcd[gc-1].gd.pos.width = 140;
		    hvarray[si++] = GCD_ColSpan;
		    pgcd[gc].gd.pos.x += 145;
		    pgcd[gc].gd.cid += CID_PrefsBrowseOffset;
		    pgcd[gc].gd.label = &plabel[gc];
		    plabel[gc].text = (unichar_t *) "...";
		    plabel[gc].text_is_1byte = true;
		    pgcd[gc].gd.handle_controlevent = Prefs_BrowseFile;
		    pgcd[gc++].creator = GButtonCreate;
		    hvarray[si++] = &pgcd[gc-1];
		} else if ( pl->set==SetAutoTraceArgs || ((char **) pl->val)==&mf_args ) {
		    hvarray[si++] = GCD_ColSpan;
		    hvarray[si++] = GCD_Glue;
		} else {
		    hvarray[si++] = GCD_Glue;
		    hvarray[si++] = GCD_Glue;
		}
		y += 26;
		if ( pl->val==NULL )
		    free(tempstr);
	      break;
	    }
	    ++line;
	    hvarray[si++] = NULL;
	}
	if ( visible_prefs_list[k].pl == args_list ) {
	    static char *text[] = {
/* GT: See the long comment at "Property|New" */
/* GT: This and the next few strings show a limitation of my widget set which */
/* GT: cannot handle multi-line text labels. These strings should be concatenated */
/* GT: (after striping off "Prefs|App|") together, translated, and then broken up */
/* GT: to fit the dialog. There is an extra blank line, not used in English, */
/* GT: into which your text may extend if needed. */
		N_("Prefs|App|Normally FontForge will find applications by searching for"),
		N_("Prefs|App|them in your PATH environment variable, if you want"),
		N_("Prefs|App|to alter that behavior you may set an environment"),
		N_("Prefs|App|variable giving the full path spec of the application."),
		N_("Prefs|App|FontForge recognizes BROWSER, MF and AUTOTRACE."),
		N_("Prefs|App| "), /* A blank line */
		NULL };
	    y += 8;
	    for ( i=0; text[i]!=0; ++i ) {
		plabel[gc].text = (unichar_t *) S_(text[i]);
		plabel[gc].text_is_1byte = true;
		pgcd[gc].gd.label = &plabel[gc];
		pgcd[gc].gd.pos.x = 8;
		pgcd[gc].gd.pos.y = y;
		pgcd[gc].gd.flags = gg_visible | gg_enabled;
		pgcd[gc++].creator = GLabelCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		hvarray[si++] = NULL;
		y += 12;
	    }
	}
	if ( y>y2 ) y2 = y;
	hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
	hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
	hvarray[si++] = NULL;
	hvarray[si++] = NULL;
	boxes[2*k].gd.flags = gg_enabled|gg_visible;
	boxes[2*k].gd.u.boxelements = hvarray;
	boxes[2*k].creator = GHVBoxCreate;
    }

    aspects[k].text = (unichar_t *) _("Script Menu");
    aspects[k].text_is_1byte = true;
    aspects[k++].gcd = sboxes;

    subaspects[0].text = (unichar_t *) _("Features");
    subaspects[0].text_is_1byte = true;
    subaspects[0].gcd = mfboxes;

    subaspects[1].text = (unichar_t *) _("Mapping");
    subaspects[1].text_is_1byte = true;
    subaspects[1].gcd = mpboxes;

    mgcd[0].gd.pos.x = 4; gcd[0].gd.pos.y = 6;
    mgcd[0].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-20;
    mgcd[0].gd.pos.height = y2;
    mgcd[0].gd.u.tabs = subaspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].creator = GTabSetCreate;

    aspects[k].text = (unichar_t *) _("Mac");
    aspects[k].text_is_1byte = true;
    aspects[k++].gcd = mgcd;

    gc = 0;

    gcd[gc].gd.pos.x = gcd[gc].gd.pos.y = 2;
    gcd[gc].gd.pos.width = pos.width-4; gcd[gc].gd.pos.height = pos.height-2;
    gcd[gc].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[gc++].creator = GGroupCreate;

    gcd[gc].gd.pos.x = 4; gcd[gc].gd.pos.y = 6;
    gcd[gc].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-8;
    gcd[gc].gd.pos.height = y2+20+18+4;
    gcd[gc].gd.u.tabs = aspects;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    gcd[gc++].creator = GTabSetCreate;
    varray[0] = &gcd[gc-1]; varray[1] = NULL;

    y = gcd[gc-1].gd.pos.y+gcd[gc-1].gd.pos.height;

    gcd[gc].gd.pos.x = 30-3; gcd[gc].gd.pos.y = y+5-3;
    gcd[gc].gd.pos.width = -1; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gc].text = (unichar_t *) _("_OK");
    label[gc].text_is_1byte = true;
    label[gc].text_in_resource = true;
    gcd[gc].gd.mnemonic = 'O';
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.handle_controlevent = Prefs_Ok;
    gcd[gc++].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[gc-1]; harray[2] = GCD_Glue; harray[3] = GCD_Glue;

    gcd[gc].gd.pos.x = -30; gcd[gc].gd.pos.y = gcd[gc-1].gd.pos.y+3;
    gcd[gc].gd.pos.width = -1; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[gc].text = (unichar_t *) _("_Cancel");
    label[gc].text_is_1byte = true;
    label[gc].text_in_resource = true;
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.mnemonic = 'C';
    gcd[gc].gd.handle_controlevent = Prefs_Cancel;
    gcd[gc++].creator = GButtonCreate;
    harray[4] = GCD_Glue; harray[5] = &gcd[gc-1]; harray[6] = GCD_Glue; harray[7] = NULL;

    memset(mboxes,0,sizeof(mboxes));
    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;
    varray[2] = &mboxes[2];
    varray[3] = NULL;
    varray[4] = NULL;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = varray;
    mboxes[0].creator = GHVGroupCreate;

    y = GDrawPointsToPixels(NULL,y+37);
    gcd[0].gd.pos.height = y-4;

    GGadgetsCreate(gw,mboxes);
    GTextInfoListFree(mfgcd[0].gd.u.list);
    GTextInfoListFree(msgcd[0].gd.u.list);

    GHVBoxSetExpandableRow(mboxes[0].ret,0);
    GHVBoxSetExpandableCol(mboxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(mfboxes[0].ret,0);
    GHVBoxSetExpandableCol(mfboxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(mpboxes[0].ret,0);
    GHVBoxSetExpandableCol(mpboxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(sboxes[0].ret,gb_expandglue);
    for ( k=0; k<TOPICS; ++k )
	GHVBoxSetExpandableRow(boxes[2*k].ret,gb_expandglue);
    
    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = 12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GGadgetSetFont(mfgcd[0].ret,font);
    GGadgetSetFont(msgcd[0].ret,font);
    GHVBoxFitWindow(mboxes[0].ret);

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) for ( gc=0,i=0; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	GGadgetCreateData *gcd = aspects[k].gcd[0].gd.u.boxelements[0];
	pl = &visible_prefs_list[k].pl[i];
	if ( pl->dontdisplay )
    continue;
	switch ( pl->type ) {
	  case pr_bool:
	    ++gc;
	  break;
	  case pr_encoding: {
	    GGadget *g = gcd[gc+1].ret;
	    list = GGadgetGetList(g,&llen);
	    for ( j=0; j<llen ; ++j ) {
		if ( list[j]->text!=NULL &&
			(void *) (intpt) ( *((int *) pl->val)) == list[j]->userdata )
		    list[j]->selected = true;
		else
		    list[j]->selected = false;
	    }
	    if ( gcd[gc+1].gd.u.list!=encodingtypes )
		GTextInfoListFree(gcd[gc+1].gd.u.list);
	  } break;
	  case pr_namelist:
	    free(gcd[gc+1].gd.u.list);
	  break;
	  case pr_string: case pr_file: case pr_int: case pr_real:
	    free(plabels[k][gc+1].text);
	    if ( pl->type==pr_file )
		++gc;
	  break;
	}
	gc += 2;
    }

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	free(aspects[k].gcd->gd.u.boxelements[0]);
	free(aspects[k].gcd->gd.u.boxelements);
	free(plabels[k]);
    }

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

#ifndef LUA_FF_LIB
void RecentFilesRemember(char *filename) {
    int i;

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	if ( strcmp(RecentFiles[i],filename)==0 )
    break;

    if ( i<RECENT_MAX && RecentFiles[i]!=NULL ) {
	if ( i!=0 ) {
	    filename = RecentFiles[i];
	    RecentFiles[i] = RecentFiles[0];
	    RecentFiles[0] = filename;
	}
    } else {
	if ( RecentFiles[RECENT_MAX-1]!=NULL )
	    free( RecentFiles[RECENT_MAX-1]);
	for ( i=RECENT_MAX-1; i>0; --i )
	    RecentFiles[i] = RecentFiles[i-1];
	RecentFiles[0] = copy(filename);
    }
    SavePrefs();
}
#endif
