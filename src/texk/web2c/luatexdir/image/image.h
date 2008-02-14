/*
Copyright (c) 1996-2002 Han The Thanh, <thanh@pdftex.org>

This file is part of pdfTeX.
pdfTeX is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

pdfTeX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pdfTeX; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id$
*/

#ifndef IMAGE_H
#  define IMAGE_H

#  include <png.h>
#  include <lua.h>

#  define JPG_UINT16      unsigned int
#  define JPG_UINT32      unsigned long
#  define JPG_UINT8       unsigned char

#  define IMAGE_COLOR_B   1
#  define IMAGE_COLOR_C   2
#  define IMAGE_COLOR_I   4

#  define bp2int(p)       round(p * (one_hundred_bp / 100.0))

#  define TYPE_IMG        "image"
#  define TYPE_IMG_DICT   "image-dict"

#  define scaled          integer

/**********************************************************************/

typedef struct {
    float width;
    float height;
    float orig_x;
    float orig_y;
    integer page_box;
    char *page_name;
    void *doc;
} pdf_img_struct;

typedef struct {
    png_structp png_ptr;
    png_infop info_ptr;
} png_img_struct;

typedef struct {
    int color_space;            /* used color space. See JPG_ constants */
    JPG_UINT32 length;          /* length of file/data */
} jpg_img_struct;

typedef struct {                /* currently unused */
} jb2_img_struct;

typedef enum { DICT_NEW,        /* fresh dictionary */
    DICT_FILESCANNED,           /* image file scanned */
    DICT_REFERED,               /* pdf_refximage_node in node list --> read-only dict */
    DICT_OUTIMG,                /* /Im* appears in pagestream */
    DICT_SCHEDULED,             /* image dict scheduled for writing */
    DICT_WRITTEN                /* image dict written to file */
} dict_state;

typedef enum { IMAGE_TYPE_NONE, IMAGE_TYPE_PDF, IMAGE_TYPE_PNG, IMAGE_TYPE_JPG,
    IMAGE_TYPE_JBIG2, IMAGE_TYPE_SENTINEL
} imgtype_e;

typedef enum { PDF_BOX_SPEC_NONE, PDF_BOX_SPEC_MEDIA, PDF_BOX_SPEC_CROP,
    PDF_BOX_SPEC_BLEED, PDF_BOX_SPEC_TRIM, PDF_BOX_SPEC_ART,
    PDF_BOX_SPEC_SENTINEL
} pdfboxspec_e;

/**********************************************************************/

typedef struct {
    integer objnum;
    integer index;              /* /Im1, /Im2, ... */
    integer width;              /* dimensions in pixel counts as in JPG/PNG/JBIG2 file */
    integer height;
    integer x_res;              /* pixel resolution as in JPG/PNG/JBIG2 file */
    integer y_res;
    integer colorspace_obj;
    integer page_num;           /* requested page (by number) */
    integer total_pages;
    char *pagename;             /* requested raw file name */
    char *filename;             /* requested raw file name */
    char *filepath;             /* full file path after kpathsea */
    char *attrib;               /* image attributes */
    FILE *file;
    imgtype_e image_type;
    int color_space;            /* used color space. See JPG_ constants */
    int color_depth;            /* color depth */
    pdfboxspec_e page_box_spec; /* PDF page box spec.: media/crop/bleed/trim/art */
    dict_state state;
    union {
        pdf_img_struct *pdf;
        png_img_struct *png;
        jpg_img_struct *jpg;
        jb2_img_struct *jb2;
    } img_struct;
} image_dict;

#  define img_objnum(N)         ((N)->objnum)
#  define img_index(N)          ((N)->index)
#  define img_width(N)          ((N)->width)
#  define img_height(N)         ((N)->height)
#  define img_xres(N)           ((N)->x_res)
#  define img_yres(N)           ((N)->y_res)
#  define img_colorspace_obj(N) ((N)->colorspace_obj)
#  define img_pagenum(N)        ((N)->page_num)
#  define img_totalpages(N)     ((N)->total_pages)
#  define img_pagename(N)       ((N)->pagename)
#  define img_filename(N)       ((N)->filename)
#  define img_filepath(N)       ((N)->filepath)
#  define img_attrib(N)         ((N)->attrib)
#  define img_file(N)           ((N)->file)
#  define img_type(N)           ((N)->image_type)
#  define img_color(N)          ((N)->color_space)
#  define img_colordepth(N)     ((N)->color_depth)
#  define img_pageboxspec(N)    ((N)->page_box_spec)
#  define img_state(N)          ((N)->state)

#  define img_pdf_ptr(N)        ((N)->img_struct.pdf)
#  define img_pdf_width(N)      ((N)->img_struct.pdf->width)
#  define img_pdf_height(N)     ((N)->img_struct.pdf->height)
#  define img_pdf_orig_x(N)     ((N)->img_struct.pdf->orig_x)
#  define img_pdf_orig_y(N)     ((N)->img_struct.pdf->orig_y)
#  define img_pdf_page_box(N)   ((N)->img_struct.pdf->page_box)
#  define img_pdf_page_name(N)  ((N)->img_struct.pdf->page_name)
#  define img_pdf_doc(N)        ((N)->img_struct.pdf->doc)

#  define img_png_ptr(N)        ((N)->img_struct.png)
#  define img_png_png_ptr(N)    ((N)->img_struct.png->png_ptr)
#  define img_png_info_ptr(N)   ((N)->img_struct.png->info_ptr)

#  define img_jpg_ptr(N)        ((N)->img_struct.jpg)
#  define img_jpg_color(N)      ((N)->img_struct.jpg->color_space)

#  define img_jb2_ptr(N)        ((N)->img_struct.jb2)

/**********************************************************************/

typedef struct {
    integer wd;                 /* requested/actual TeX dimensions */
    integer ht;
    integer dp;
    integer flags;
    image_dict *dict;
    int dict_ref;               /* luaL_ref() reference */
} image;

#  define img_wd(N)             ((N)->wd)
#  define img_ht(N)             ((N)->ht)
#  define img_dp(N)             ((N)->dp)
#  define img_flags(N)          ((N)->flags)
#  define img_dict(N)           ((N)->dict)
#  define img_dictref(N)        ((N)->dict_ref)

#  define F_FLAG_SCALED         0x01
#  define F_FLAG_REFERED        0x02

#  define img_flags(N)          ((N)->flags)
#  define img_set_scaled(N)     (img_flags(N) |= F_FLAG_SCALED)
#  define img_set_refered(N)    (img_flags(N) |= F_FLAG_REFERED)
#  define img_unset_scaled(N)   (img_flags(N) &= ~F_FLAG_SCALED)
#  define img_unset_refered(N)  (img_flags(N) &= ~F_FLAG_REFERED)
#  define img_is_scaled(N)      ((img_flags(N) & F_FLAG_SCALED) != 0)
#  define img_is_refered(N)     ((img_flags(N) & F_FLAG_REFERED) != 0)

#  define set_wd_running(N)     (img_wd(N) = null_flag)
#  define set_ht_running(N)     (img_ht(N) = null_flag)
#  define set_dp_running(N)     (img_dp(N) = null_flag)
#  define is_wd_running(N)      (img_wd(N) == null_flag)
#  define is_ht_running(N)      (img_ht(N) == null_flag)
#  define is_dp_running(N)      (img_dp(N) == null_flag)

/**********************************************************************/

/* writeimg.c */

image *new_image();
image_dict *new_image_dict();
void init_image(image *);
void init_image_size(image *);
void init_image_dict(image_dict *);
void scale_img(image *);
integer img_to_array(image *);
void delete_image(image *);
void free_image_dict(image_dict * p);
void read_img(image_dict *, integer, char *, integer, integer);

/* writepng.c */

#  ifndef boolean               /* TODO: from where to get the original definition? */
#    define boolean int
#  endif

void read_png_info(image_dict *, boolean);
void read_jpg_info(image_dict *, boolean);
void read_jbig2_info(image_dict *);
void read_pdf_info(image_dict *, integer, char *, integer, integer);
void write_png(image_dict *);
void write_jpg(image_dict *);
void write_jbig2(image_dict *);
void write_epdf(image_dict *);

#endif
