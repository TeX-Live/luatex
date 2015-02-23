% writeimg.w
%
% Copyright 1996-2006 Han The Thanh <thanh@@pdftex.org>
% Copyright 2006-2012 Taco Hoekwater <taco@@luatex.org>
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

@* Image inclusion.

@ @c


#include "ptexlib.h"
#include <assert.h>
#include <kpathsea/c-auto.h>
#include <kpathsea/c-memstr.h>

@ @c
#include "image/image.h"
#include "image/writejpg.h"
#include "image/writejp2.h"
#include "image/writepng.h"
#include "image/writejbig2.h"

#include "lua.h"          /* for |LUA_NOREF| */
#include "lauxlib.h"

@ @c
#define pdf_image_resolution int_par(pdf_image_resolution_code)
#define pdf_pagebox int_par(pdf_pagebox_code)

@* Patch ImageTypeDetection 2003/02/08 by Heiko Oberdiek.

  Function |readimage| performs some basic initializations.
  Then it looks at the file extension to determine the
  image type and calls specific code/functions.

    The main disadvantage is that standard file extensions
  have to be used, otherwise pdfTeX is not able to detect
  the correct image type.

  The patch now looks at the file header first regardless of
  the file extension. This is implemented in function
  |check_type_by_header|. If this check fails, the traditional
  test of standard file extension is tried, done in function
  |check_type_by_extension|.

  Magic headers:

  * "PNG (Portable Network Graphics) Specification", Version 1.2
    (http://www.libpng.org/pub/png):

     3.1. PNG file signature

        The first eight bytes of a PNG file always contain the following
        (decimal) values:

           137 80 78 71 13 10 26 10

  Translation to C: |"\x89PNG\r\n\x1A\n"|

  * "JPEG File Interchange Format", Version 1.02:

   o you can identify a JFIF file by looking for the following
     sequence: X'FF', SOI X'FF', APP0, <2 bytes to be skipped>,
     "JFIF", X'00'.

  Function |check_type_by_header| only looks at the first two bytes:
    |"\xFF\xD8"|

  * ISO/IEC JTC 1/SC 29/WG 1
    (ITU-T SG8)
    Coding of Still Pictures
    Title: 14492 FCD
    Source: JBIG Committee
    Project: JTC 1.29.10
    Status: Final Committee Draft

   D.4.1, ID string

   This is an 8-byte sequence containing 0x97 0x4A 0x42 0x32 0x0D 0x0A
   0x1A 0x0A.

  * "PDF Reference", third edition:

    * The first line should contain "\%PDF-1.0" until "\%PDF-1.4"
      (section 3.4.1 "File Header").
    * The "implementation notes" say:

     3.4.1,  File Header
       12. Acrobat viewers require only that the header appear
           somewhere within the first 1024 bytes of the file.
       13. Acrobat viewers will also accept a header of the form
               \%!PS-Adobe-N.n PDF-M.m

    The check in function |check_type_by_header| only implements
    the first issue. The implementation notes are not considered.
    Therefore files with garbage at start of file must have the
    standard extension.

    Functions |check_type_by_header| and |check_type_by_extension|:
    |img_type(img)| is set to |IMG_TYPE_NONE| by |new_image_dict()|.
    Both functions try to detect a type and set |img_type(img)|.
    Thus a value other than |IMG_TYPE_NONE| indicates that a
    type has been found.

@c
#define HEADER_JPG "\xFF\xD8"
#define HEADER_PNG "\x89PNG\r\n\x1A\n"
#define HEADER_JBIG2 "\x97\x4A\x42\x32\x0D\x0A\x1A\x0A"
#define HEADER_JP2 "\x6A\x50\x20\x20"
#define HEADER_PDF "%PDF-1."
#define MAX_HEADER (sizeof(HEADER_PNG)-1)

static void check_type_by_header(image_dict * idict)
{
    int i;
    FILE *file = NULL;
    char header[MAX_HEADER];

    assert(idict != NULL);
    if (img_type(idict) != IMG_TYPE_NONE)       /* nothing to do */
        return;
    /* read the header */
    file = xfopen(img_filepath(idict), FOPEN_RBIN_MODE);
    for (i = 0; (unsigned) i < MAX_HEADER; i++) {
        header[i] = (char) xgetc(file);
        if (feof(file))
            luatex_fail("reading image file failed");
    }
    xfclose(file, img_filepath(idict));
    /* tests */
    if (strncmp(header, HEADER_JPG, sizeof(HEADER_JPG) - 1) == 0)
        img_type(idict) = IMG_TYPE_JPG;
    else if (strncmp(header + 4, HEADER_JP2, sizeof(HEADER_JP2) - 1) == 0)
        img_type(idict) = IMG_TYPE_JP2;
    else if (strncmp(header, HEADER_PNG, sizeof(HEADER_PNG) - 1) == 0)
        img_type(idict) = IMG_TYPE_PNG;
    else if (strncmp(header, HEADER_JBIG2, sizeof(HEADER_JBIG2) - 1) == 0)
        img_type(idict) = IMG_TYPE_JBIG2;
    else if (strncmp(header, HEADER_PDF, sizeof(HEADER_PDF) - 1) == 0)
        img_type(idict) = IMG_TYPE_PDF;
}

@ @c
static void check_type_by_extension(image_dict * idict)
{
    char *image_suffix;

    assert(idict != NULL);
    if (img_type(idict) != IMG_TYPE_NONE)       /* nothing to do */
        return;
    /* tests */
    if ((image_suffix = strrchr(img_filename(idict), '.')) == 0)
        img_type(idict) = IMG_TYPE_NONE;
    else if (strcasecmp(image_suffix, ".png") == 0)
        img_type(idict) = IMG_TYPE_PNG;
    else if (strcasecmp(image_suffix, ".jpg") == 0 ||
             strcasecmp(image_suffix, ".jpeg") == 0)
        img_type(idict) = IMG_TYPE_JPG;
    else if (strcasecmp(image_suffix, ".jp2") == 0)
        img_type(idict) = IMG_TYPE_JP2;
    else if (strcasecmp(image_suffix, ".jbig2") == 0 ||
             strcasecmp(image_suffix, ".jb2") == 0)
        img_type(idict) = IMG_TYPE_JBIG2;
    else if (strcasecmp(image_suffix, ".pdf") == 0)
        img_type(idict) = IMG_TYPE_PDF;
}

@ @c
void new_img_pdfstream_struct(image_dict * p)
{
    assert(p != NULL);
    assert(img_pdfstream_ptr(p) == NULL);
    img_pdfstream_ptr(p) = xtalloc(1, pdf_stream_struct);
    img_pdfstream_stream(p) = NULL;
}

@ @c
static void init_image(image * p)
{
    assert(p != NULL);
    set_wd_running(p);
    set_ht_running(p);
    set_dp_running(p);
    img_transform(p) = 0;
    img_dict(p) = NULL;
    img_dictref(p) = LUA_NOREF;
}

@ @c
image *new_image(void)
{
    image *p = xtalloc(1, image);
    init_image(p);
    return p;
}

@ @c
static void init_image_dict(image_dict * p)
{
    assert(p != NULL);
    memset(p, 0, sizeof(image_dict));
    set_wd_running(p);
    set_ht_running(p);
    set_dp_running(p);
    img_transform(p) = 0;
    img_pagenum(p) = 1;
    img_type(p) = IMG_TYPE_NONE;
    img_pagebox(p) = PDF_BOX_SPEC_MEDIA;
    img_unset_bbox(p);
    img_unset_group(p);
    img_state(p) = DICT_NEW;
    img_index(p) = -1;          /* -1 = unused, used count from 0 */
}

@ @c
image_dict *new_image_dict(void)
{
    image_dict *p = xtalloc(1, image_dict);
    init_image_dict(p);
    return p;
}

@ @c
static void free_dict_strings(image_dict * p)
{
    xfree(img_filename(p));
    xfree(img_filepath(p));
    xfree(img_attr(p));
    xfree(img_pagename(p));
}

@ @c
void free_image_dict(image_dict * p)
{
    if (ini_version)
        return;                 /* The image may be \.{\\dump}ed to a format */
    /* called from limglib.c */
    assert(img_state(p) < DICT_REFERED);
    switch (img_type(p)) {
    case IMG_TYPE_PDF:
        unrefPdfDocument(img_filepath(p));
        break;
    case IMG_TYPE_PNG:         /* assuming |IMG_CLOSEINBETWEEN| */
        assert(img_png_ptr(p) == NULL);
        break;
    case IMG_TYPE_JPG:         /* assuming |IMG_CLOSEINBETWEEN| */
        assert(img_jpg_ptr(p) == NULL);
        break;
    case IMG_TYPE_JP2:         /* */
        assert(img_jp2_ptr(p) == NULL);
        break;
    case IMG_TYPE_JBIG2:       /* todo: writejbig2.w cleanup */
        break;
    case IMG_TYPE_PDFSTREAM:
        if (img_pdfstream_ptr(p) != NULL) {
            xfree(img_pdfstream_stream(p));
            xfree(img_pdfstream_ptr(p));
        }
        break;
    case IMG_TYPE_NONE:
        break;
    default:
        assert(0);
    }
    free_dict_strings(p);
    assert(img_file(p) == NULL);
    xfree(p);
}

@ @c
void read_img(PDF pdf,
              image_dict * idict, int minor_version, int inclusion_errorlevel)
{
    char *filepath = NULL;
    int callback_id;
    assert(idict != NULL);
    if (img_filename(idict) == NULL)
        luatex_fail("image file name missing");
    callback_id = callback_defined(find_image_file_callback);
    if (img_filepath(idict) == NULL) {
        if (callback_id > 0
            && run_callback(callback_id, "S->S", img_filename(idict),
                            &filepath)) {
            if (filepath && (strlen(filepath) > 0))
                img_filepath(idict) = strdup(filepath);
        } else
            img_filepath(idict) =
                kpse_find_file(img_filename(idict), kpse_tex_format, true);
        if (img_filepath(idict) == NULL)
            luatex_fail("cannot find image file '%s'", img_filename(idict));
    }
    recorder_record_input(img_filename(idict));
    /* type checks */
    check_type_by_header(idict);
    check_type_by_extension(idict);
    /* read image */
    switch (img_type(idict)) {
    case IMG_TYPE_PDF:
        assert(pdf != NULL);    /* TODO! */
        read_pdf_info(idict, minor_version, inclusion_errorlevel,
                      IMG_CLOSEINBETWEEN);
        break;
    case IMG_TYPE_PNG:
        read_png_info(idict, IMG_CLOSEINBETWEEN);
        break;
    case IMG_TYPE_JPG:
        read_jpg_info(pdf, idict, IMG_CLOSEINBETWEEN);
        break;
    case IMG_TYPE_JP2:
        read_jp2_info(idict, IMG_CLOSEINBETWEEN);
        break;
    case IMG_TYPE_JBIG2:
        if (minor_version < 4) {
            luatex_fail
                ("JBIG2 images only possible with at least PDF 1.4; you are generating PDF 1.%i",
                 (int) minor_version);
        }
        read_jbig2_info(idict);
        break;
    default:
        luatex_fail("internal error: unknown image type (2)");
    }
    cur_file_name = NULL;
    if (img_state(idict) < DICT_FILESCANNED)
        img_state(idict) = DICT_FILESCANNED;
}

@ @c
static image_dict *read_image(PDF pdf, char *file_name, int page_num,
                              char *page_name, int colorspace,
                              int page_box, int minor_version,
                              int inclusion_errorlevel)
{
    image *a = new_image();
    image_dict *idict = img_dict(a) = new_image_dict();
    pdf->ximage_count++;
    img_objnum(idict) = pdf_create_obj(pdf, obj_type_ximage, pdf->ximage_count);
    img_index(idict) = pdf->ximage_count;
    set_obj_data_ptr(pdf, img_objnum(idict), img_index(idict));
    idict_to_array(idict);
    img_colorspace(idict) = colorspace;
    img_pagenum(idict) = page_num;
    img_pagename(idict) = page_name;
    assert(file_name != NULL);
    cur_file_name = file_name;
    img_filename(idict) = file_name;
    img_pagebox(idict) = page_box;
    read_img(pdf, idict, minor_version, inclusion_errorlevel);
    return idict;
}

@ scans PDF pagebox specification
@c
static pdfboxspec_e scan_pdf_box_spec(void)
{
    if (scan_keyword("mediabox"))
        return PDF_BOX_SPEC_MEDIA;
    else if (scan_keyword("cropbox"))
        return PDF_BOX_SPEC_CROP;
    else if (scan_keyword("bleedbox"))
        return PDF_BOX_SPEC_BLEED;
    else if (scan_keyword("trimbox"))
        return PDF_BOX_SPEC_TRIM;
    else if (scan_keyword("artbox"))
        return PDF_BOX_SPEC_ART;
    return PDF_BOX_SPEC_NONE;
}

@ @c
void scan_pdfximage(PDF pdf)
{
    scaled_whd alt_rule;
    image_dict *idict;
    int transform = 0, page = 1, pagebox, colorspace = 0;
    char *named = NULL, *attr = NULL, *file_name = NULL;
    alt_rule = scan_alt_rule(); /* scans |<rule spec>| to |alt_rule| */
    if (scan_keyword("attr")) {
        scan_pdf_ext_toks();
        attr = tokenlist_to_cstring(def_ref, true, NULL);
        delete_token_ref(def_ref);
    }
    if (scan_keyword("named")) {
        scan_pdf_ext_toks();
        named = tokenlist_to_cstring(def_ref, true, NULL);
        delete_token_ref(def_ref);
        page = 0;
    } else if (scan_keyword("page")) {
        scan_int();
        page = cur_val;
    }
    if (scan_keyword("colorspace")) {
        scan_int();
        colorspace = cur_val;
    }
    pagebox = scan_pdf_box_spec();
    if (pagebox == PDF_BOX_SPEC_NONE) {
        pagebox = pdf_pagebox;
        if (pagebox == PDF_BOX_SPEC_NONE)
            pagebox = PDF_BOX_SPEC_CROP;
    }
    scan_pdf_ext_toks();
    file_name = tokenlist_to_cstring(def_ref, true, NULL);
    assert(file_name != NULL);
    delete_token_ref(def_ref);
    idict =
        read_image(pdf, file_name, page, named, colorspace, pagebox,
                   pdf_minor_version, pdf_inclusion_errorlevel);
    img_attr(idict) = attr;
    img_dimen(idict) = alt_rule;
    img_transform(idict) = transform;
    pdf_last_ximage = img_objnum(idict);
    pdf_last_ximage_pages = img_totalpages(idict);
    pdf_last_ximage_colordepth = img_colordepth(idict);
}

@ @c
#define tail          cur_list.tail_field

void scan_pdfrefximage(PDF pdf)
{
    int transform = 0;          /* one could scan transform as well */
    image_dict *idict;
    scaled_whd alt_rule, dim;
    alt_rule = scan_alt_rule(); /* scans |<rule spec>| to |alt_rule| */
    scan_int();
    check_obj_type(pdf, obj_type_ximage, cur_val);
    new_whatsit(pdf_refximage_node);
    idict = idict_array[obj_data_ptr(pdf, cur_val)];
    if (alt_rule.wd != null_flag || alt_rule.ht != null_flag
        || alt_rule.dp != null_flag)
        dim = scale_img(idict, alt_rule, transform);
    else
        dim = scale_img(idict, img_dimen(idict), img_transform(idict));
    width(tail) = dim.wd;
    height(tail) = dim.ht;
    depth(tail) = dim.dp;
    pdf_ximage_transform(tail) = transform;
    pdf_ximage_index(tail) = img_index(idict);
}

@ |tex_scale()| sequence of decisions:

{\obeylines\obeyspaces\tt
wd ht dp : res = tex;
wd ht --
wd -- dp
wd -- --
-- ht dp
-- ht --
-- -- dp
-- -- -- : res = nat;
}

@c
scaled_whd tex_scale(scaled_whd nat, scaled_whd tex)
{
    scaled_whd res;
    if (!is_running(tex.wd) && !is_running(tex.ht) && !is_running(tex.dp)) {
        /* width, height, and depth specified */
        res = tex;
    } else /* max. 2 dimensions are specified */ if (!is_running(tex.wd)) {
        res.wd = tex.wd;
        if (!is_running(tex.ht)) {
            res.ht = tex.ht;
            /* width and height specified */
            res.dp = ext_xn_over_d(tex.ht, nat.dp, nat.ht);
        } else if (!is_running(tex.dp)) {
            res.dp = tex.dp;
            /* width and depth specified */
            res.ht = ext_xn_over_d(tex.wd, nat.ht + nat.dp, nat.wd) - tex.dp;
        } else {
            /* only width specified */
            res.ht = ext_xn_over_d(tex.wd, nat.ht, nat.wd);
            res.dp = ext_xn_over_d(tex.wd, nat.dp, nat.wd);
        }
    } else if (!is_running(tex.ht)) {
        res.ht = tex.ht;
        if (!is_running(tex.dp)) {
            res.dp = tex.dp;
            /* height and depth specified */
            res.wd = ext_xn_over_d(tex.ht + tex.dp, nat.wd, nat.ht + nat.dp);
        } else {
            /* only height specified */
            res.wd = ext_xn_over_d(tex.ht, nat.wd, nat.ht);
            res.dp = ext_xn_over_d(tex.ht, nat.dp, nat.ht);
        }
    } else if (!is_running(tex.dp)) {
        res.dp = tex.dp;
        /* only depth specified */
        res.ht = nat.ht - (tex.dp - nat.dp);
        res.wd = nat.wd;
    } else {
        /* nothing specified */
        res = nat;
    }
    return res;
}

@ Within |scale_img()| only image width and height matter;
the offsets and positioning are not interesting here.
But one needs rotation info to swap width and height.
|img_rotation()| comes from the optional /Rotate key in the PDF file.

@c
scaled_whd scale_img(image_dict * idict, scaled_whd alt_rule, int transform)
{
    int x, y, xr, yr, tmp;      /* size and resolution of image */
    scaled_whd nat;             /* natural size corresponding to image resolution */
    int default_res;
    assert(idict != NULL);
    if ((img_type(idict) == IMG_TYPE_PDF
         || img_type(idict) == IMG_TYPE_PDFSTREAM) && img_is_bbox(idict)) {
        x = img_xsize(idict) = img_bbox(idict)[2] - img_bbox(idict)[0]; /* dimensions from image.bbox */
        y = img_ysize(idict) = img_bbox(idict)[3] - img_bbox(idict)[1];
        img_xorig(idict) = img_bbox(idict)[0];
        img_yorig(idict) = img_bbox(idict)[1];
    } else {
        x = img_xsize(idict);   /* dimensions, resolutions from image file */
        y = img_ysize(idict);
    }
    xr = img_xres(idict);
    yr = img_yres(idict);
    if (x <= 0 || y <= 0 || xr < 0 || yr < 0)
        luatex_fail("ext1: invalid image dimensions");
    if (xr > 65535 || yr > 65535) {
        xr = 0;
        yr = 0;
        luatex_warn("ext1: too large image resolution ignored");
    }
    if (((transform - img_rotation(idict)) & 1) == 1) {
        tmp = x;
        x = y;
        y = tmp;
        tmp = xr;
        xr = yr;
        yr = tmp;
    }
    nat.dp = 0;                 /* always for images */
    if (img_type(idict) == IMG_TYPE_PDF
        || img_type(idict) == IMG_TYPE_PDFSTREAM) {
        nat.wd = x;
        nat.ht = y;
    } else {
        default_res = fix_int(pdf_image_resolution, 0, 65535);
        if (default_res > 0 && (xr == 0 || yr == 0)) {
            xr = default_res;
            yr = default_res;
        }
        if (xr > 0 && yr > 0) {
            nat.wd = ext_xn_over_d(one_hundred_inch, x, 100 * xr);
            nat.ht = ext_xn_over_d(one_hundred_inch, y, 100 * yr);
        } else {
            nat.wd = ext_xn_over_d(one_hundred_inch, x, 7200);
            nat.ht = ext_xn_over_d(one_hundred_inch, y, 7200);
        }
    }
    return tex_scale(nat, alt_rule);
}

@ @c
void write_img(PDF pdf, image_dict * idict)
{
    assert(idict != NULL);
    if (img_state(idict) < DICT_WRITTEN) {
        report_start_file(filetype_image, img_filepath(idict));
        switch (img_type(idict)) {
        case IMG_TYPE_PNG:
            write_png(pdf, idict);
            break;
        case IMG_TYPE_JPG:
            write_jpg(pdf, idict);
            break;
        case IMG_TYPE_JP2:
            write_jp2(pdf, idict);
            break;
        case IMG_TYPE_JBIG2:
            write_jbig2(pdf, idict);
            break;
        case IMG_TYPE_PDF:
            write_epdf(pdf, idict);
            break;
        case IMG_TYPE_PDFSTREAM:
            write_pdfstream(pdf, idict);
            break;
        default:
            luatex_fail("internal error: unknown image type (1)");
        }
        report_stop_file(filetype_image);
        if (img_type(idict) == IMG_TYPE_PNG) {
            write_additional_png_objects(pdf);
        }
    }
    if (img_state(idict) < DICT_WRITTEN)
        img_state(idict) = DICT_WRITTEN;
}

@ write an image
@c
void pdf_write_image(PDF pdf, int n)
{
    if (pdf->draftmode == 0)
        write_img(pdf, idict_array[obj_data_ptr(pdf, n)]);
}

@ @c
void check_pdfstream_dict(image_dict * idict)
{
    if (!img_is_bbox(idict))
        luatex_fail("image.stream: no bbox given");
    if (img_state(idict) < DICT_FILESCANNED)
        img_state(idict) = DICT_FILESCANNED;
}

@ @c
void write_pdfstream(PDF pdf, image_dict * idict)
{
    assert(img_pdfstream_ptr(idict) != NULL);
    assert(img_is_bbox(idict));
    pdf_begin_obj(pdf, img_objnum(idict), OBJSTM_NEVER);
    pdf_begin_dict(pdf);
    pdf_dict_add_name(pdf, "Type", "XObject");
    pdf_dict_add_name(pdf, "Subtype", "Form");
    if (img_attr(idict) != NULL && strlen(img_attr(idict)) > 0)
        pdf_printf(pdf, "\n%s\n", img_attr(idict));
    pdf_dict_add_int(pdf, "FormType", 1);
    pdf_add_name(pdf, "BBox");
    pdf_begin_array(pdf);
    copyReal(pdf, sp2bp(img_bbox(idict)[0]));
    copyReal(pdf, sp2bp(img_bbox(idict)[1]));
    copyReal(pdf, sp2bp(img_bbox(idict)[2]));
    copyReal(pdf, sp2bp(img_bbox(idict)[3]));
    pdf_end_array(pdf);
    pdf_dict_add_streaminfo(pdf);
    pdf_end_dict(pdf);
    pdf_begin_stream(pdf);
    if (img_pdfstream_stream(idict) != NULL)
        pdf_puts(pdf, img_pdfstream_stream(idict));
    pdf_end_stream(pdf);
    pdf_end_obj(pdf);
}

@ @c
/* define |idict_ptr|, |idict_array|, and |idict_limit| */
idict_entry *idict_ptr, *idict_array = NULL;
size_t idict_limit;

void idict_to_array(image_dict * idict)
{
    assert(idict != NULL);
    if (idict_ptr - idict_array == 0) { /* align to count from 1 */
        alloc_array(idict, 1, SMALL_BUF_SIZE);  /* /Im0 unused */
        idict_ptr++;
    }
    alloc_array(idict, 1, SMALL_BUF_SIZE);
    *idict_ptr = idict;
    assert(img_index(idict) == idict_ptr - idict_array);
    idict_ptr++;
}

void pdf_dict_add_img_filename(PDF pdf, image_dict * idict)
{
    char s[21], *p;
    assert(idict != NULL);
    /* for now PTEX.FileName only for PDF, but prepared for JPG, PNG, ... */
    if (img_type(idict) != IMG_TYPE_PDF)
        return;
    if (img_visiblefilename(idict) != NULL) {
        if (strlen(img_visiblefilename(idict)) == 0)
            return;             /* empty string blocks PTEX.FileName output */
        else
            p = img_visiblefilename(idict);
    } else
        p = img_filepath(idict);
    // write additional information
    snprintf(s, 20, "%s.FileName", pdfkeyprefix);
    pdf_add_name(pdf, s);
    pdf_printf(pdf, " (%s)", convertStringToPDFString(p, strlen(p)));
}

@ To allow the use of \.{\\pdfrefximage} inside saved boxes in -ini mode,
the information in the array has to be (un)dumped with the format.
The next two routines take care of that.

Most of the work involved in setting up the images is simply
executed again. This solves the many possible errors resulting from
the split in two separate runs.

There was only one problem remaining: The pdfversion and
pdfinclusionerrorlevel can have changed inbetween the call to
|readimage()| and dump time. That is why they are passed as arguments
to undumpimagemeta once more.

some of the dumped values are really type int, not integer,
but since the macro falls back to |generic_dump| anyway, that
does not matter.

@c
#define dumpinteger generic_dump
#define undumpinteger generic_undump

@ (un)dumping a string means dumping the allocation size, followed
 by the bytes. The trailing \.{\\0} is dumped as well, because that
 makes the code simpler.

@c
#define dumpcharptr(a)                          \
  do {                                          \
    int x;                                      \
    if (a!=NULL) {                              \
	x = (int)strlen(a)+1;			\
      dumpinteger(x);  dump_things(*a, x);      \
    } else {                                    \
      x = 0; dumpinteger(x);                    \
    }                                           \
  } while (0)

#define undumpcharptr(s)                        \
  do {                                          \
    int x;                                      \
    char *a;                                    \
    undumpinteger (x);                          \
    if (x>0) {                                  \
      a = xmalloc((unsigned)x);  		\
      undump_things(*a,x);                      \
      s = a ;                                   \
    } else { s = NULL; }                        \
  } while (0)

@ @c
void dumpimagemeta(void)
{
    int cur_index, i;
    image_dict *idict;

    i = (int) idict_limit;
    dumpinteger(i);
    cur_index = (int) (idict_ptr - idict_array);
    dumpinteger(cur_index);

    for (i = 1; i < cur_index; i++) {
        idict = idict_array[i];
        assert(idict != NULL);
        dumpcharptr(img_filename(idict));
        dumpinteger(img_type(idict));
        dumpinteger(img_procset(idict));
        dumpinteger(img_xsize(idict));
        dumpinteger(img_ysize(idict));
        dumpinteger(img_xres(idict));
        dumpinteger(img_yres(idict));
        dumpinteger(img_totalpages(idict));
        dumpinteger(img_colorspace(idict));

        /* the |image_struct| is not dumped at all, except for a few
           variables that are needed to restore the contents */

        if (img_type(idict) == IMG_TYPE_PDF) {
            dumpinteger(img_pagebox(idict));
            dumpinteger(img_pagenum(idict));
        } else if (img_type(idict) == IMG_TYPE_JBIG2) {
            dumpinteger(img_pagenum(idict));
        }

    }
}

@ @c
void undumpimagemeta(PDF pdf, int pdfversion, int pdfinclusionerrorlevel)
{
    int cur_index, i;
    image_dict *idict;

    assert(pdf != NULL);
    undumpinteger(i);
    idict_limit = (size_t) i;

    idict_array = xtalloc(idict_limit, idict_entry);
    undumpinteger(cur_index);
    idict_ptr = idict_array + cur_index;

    for (i = 1; i < cur_index; i++) {
        idict = new_image_dict();
        assert(idict != NULL);
        assert(img_index(idict) == -1);
        idict_to_array(idict);
        undumpcharptr(img_filename(idict));
        undumpinteger(img_type(idict));
        undumpinteger(img_procset(idict));
        undumpinteger(img_xsize(idict));
        undumpinteger(img_ysize(idict));
        undumpinteger(img_xres(idict));
        undumpinteger(img_yres(idict));
        undumpinteger(img_totalpages(idict));
        undumpinteger(img_colorspace(idict));

        switch (img_type(idict)) {
        case IMG_TYPE_PDF:
            undumpinteger(img_pagebox(idict));
            undumpinteger(img_pagenum(idict));
            break;
        case IMG_TYPE_PNG:
        case IMG_TYPE_JPG:
        case IMG_TYPE_JP2:
            break;
        case IMG_TYPE_JBIG2:
            if (pdfversion < 4) {
                luatex_fail
                    ("JBIG2 images only possible with at least PDF 1.4; you are generating PDF 1.%i",
                     (int) pdfversion);
            }
            undumpinteger(img_pagenum(idict));
            break;
        default:
            luatex_fail("unknown type of image");
        }
        read_img(pdf, idict, pdfversion, pdfinclusionerrorlevel);
    }
}

@ scan rule spec to |alt_rule|
@c
scaled_whd scan_alt_rule(void)
{
    boolean loop;
    scaled_whd alt_rule;
    alt_rule.wd = null_flag;
    alt_rule.ht = null_flag;
    alt_rule.dp = null_flag;
    do {
        loop = false;
        if (scan_keyword("width")) {
            scan_normal_dimen();
            alt_rule.wd = cur_val;
            loop = true;
        } else if (scan_keyword("height")) {
            scan_normal_dimen();
            alt_rule.ht = cur_val;
            loop = true;
        } else if (scan_keyword("depth")) {
            scan_normal_dimen();
            alt_rule.dp = cur_val;
            loop = true;
        }
    } while (loop);
    return alt_rule;
}

@ copy file of arbitrary size to PDF buffer and flush as needed
@c
size_t read_file_to_buf(PDF pdf, FILE * f, size_t len)
{
    size_t i, j, k = 0;
    while (len > 0) {
        i = (size_t) (len > pdf->buf->size) ? (size_t) pdf->buf->size : len;
        pdf_room(pdf, (int) i);
        j = fread(pdf->buf->p, 1, i, f);
        pdf->buf->p += j;
        k += j;
        len -= j;
        if (i != j)
            break;
    }
    return k;
}
