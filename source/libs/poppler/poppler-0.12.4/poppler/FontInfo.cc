//========================================================================
//
// FontInfo.cc
//
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005-2008 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2005 Brad Hards <bradh@frogmouth.net>
// Copyright (C) 2006 Kouhei Sutou <kou@cozmixng.org>
// Copyright (C) 2009 Pino Toscano <pino@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

//========================================================================
//
// Based on code from pdffonts.cc
//
// Copyright 2001-2007 Glyph & Cog, LLC
//
//========================================================================

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "GlobalParams.h"
#include "Error.h"
#include "Object.h"
#include "Dict.h"
#include "GfxFont.h"
#include "Annot.h"
#include "PDFDoc.h"
#include "FontInfo.h"

FontInfoScanner::FontInfoScanner(PDFDoc *docA, int firstPage) {
  doc = docA;
  currentPage = firstPage + 1;
  fonts = NULL;
  fontsLen = fontsSize = 0;
  visitedXObjects = NULL;
  visitedXObjectsLen = visitedXObjectsSize = 0;
}

FontInfoScanner::~FontInfoScanner() {
  gfree(fonts);
  gfree(visitedXObjects);
}

GooList *FontInfoScanner::scan(int nPages) {
  GooList *result;
  Page *page;
  Dict *resDict;
  Annots *annots;
  Object obj1, obj2;
  int lastPage;

  if (currentPage > doc->getNumPages()) {
    return NULL;
  }
 
  result = new GooList();

  lastPage = currentPage + nPages;
  if (lastPage > doc->getNumPages() + 1) {
    lastPage = doc->getNumPages() + 1;
  }

  for (int pg = currentPage; pg < lastPage; ++pg) {
    page = doc->getCatalog()->getPage(pg);
    if ((resDict = page->getResourceDict())) {
      scanFonts(resDict, result);
    }
    annots = new Annots(doc->getXRef(), doc->getCatalog(), page->getAnnots(&obj1));
    obj1.free();
    for (int i = 0; i < annots->getNumAnnots(); ++i) {
      if (annots->getAnnot(i)->getAppearance(&obj1)->isStream()) {
	obj1.streamGetDict()->lookup("Resources", &obj2);
	if (obj2.isDict()) {
	  scanFonts(obj2.getDict(), result);
	}
	obj2.free();
      }
      obj1.free();
    }
    delete annots;
  }

  currentPage = lastPage;

  return result;
}

void FontInfoScanner::scanFonts(Dict *resDict, GooList *fontsList) {
  Object obj1, obj2, xObjDict, xObj, xObj2, resObj;
  Ref r;
  GfxFontDict *gfxFontDict;
  GfxFont *font;
  int i;

  // scan the fonts in this resource dictionary
  gfxFontDict = NULL;
  resDict->lookupNF("Font", &obj1);
  if (obj1.isRef()) {
    obj1.fetch(doc->getXRef(), &obj2);
    if (obj2.isDict()) {
      r = obj1.getRef();
      gfxFontDict = new GfxFontDict(doc->getXRef(), &r, obj2.getDict());
    }
    obj2.free();
  } else if (obj1.isDict()) {
    gfxFontDict = new GfxFontDict(doc->getXRef(), NULL, obj1.getDict());
  }
  if (gfxFontDict) {
    for (i = 0; i < gfxFontDict->getNumFonts(); ++i) {
      int k;
      if ((font = gfxFontDict->getFont(i))) {
        Ref fontRef = *font->getID();
	GBool alreadySeen = gFalse;

        // check for an already-seen font
        for (k = 0; k < fontsLen; ++k) {
          if (fontRef.num == fonts[k].num && fontRef.gen == fonts[k].gen) {
            alreadySeen = gTrue;
          }
        }

	// add this font to the list
        if (!alreadySeen) {
          fontsList->append(new FontInfo(font, doc));
          if (fontsLen == fontsSize) {
            fontsSize += 32;
            fonts = (Ref *)grealloc(fonts, fontsSize * sizeof(Ref));
          }
          fonts[fontsLen++] = *font->getID();
        }
      }
    }
    delete gfxFontDict;
  }
  obj1.free();

  // recursively scan any resource dictionaries in objects in this
  // resource dictionary
  resDict->lookup("XObject", &xObjDict);
  if (xObjDict.isDict()) {
    for (i = 0; i < xObjDict.dictGetLength(); ++i) {
      xObjDict.dictGetValNF(i, &xObj);
      if (xObj.isRef()) {
        GBool alreadySeen = gFalse;
        // check for an already-seen XObject
        for (int k = 0; k < visitedXObjectsLen; ++k) {
          if (xObj.getRef().num == visitedXObjects[k].num &&
              xObj.getRef().gen == visitedXObjects[k].gen) {
            alreadySeen = gTrue;
          }
        }

        if (alreadySeen) {
          xObj.free();
          continue;
        }

        if (visitedXObjectsLen == visitedXObjectsSize) {
          visitedXObjectsSize += 32;
          visitedXObjects = (Ref *)grealloc(visitedXObjects, visitedXObjectsSize * sizeof(Ref));
        }
        visitedXObjects[visitedXObjectsLen++] = xObj.getRef();
      }

      xObj.fetch(doc->getXRef(), &xObj2);

      if (xObj2.isStream()) {
        xObj2.streamGetDict()->lookup("Resources", &resObj);
        if (resObj.isDict() && resObj.getDict() != resDict) {
          scanFonts(resObj.getDict(), fontsList);
        }
        resObj.free();
      }
      xObj.free();
      xObj2.free();
    }
  }
  xObjDict.free();
}

FontInfo::FontInfo(GfxFont *font, PDFDoc *doc) {
  GooString *origName;
  Object fontObj, toUnicodeObj;
  int i;

  fontRef = *font->getID();

  // font name
  origName = font->getOrigName();
  if (origName != NULL) {
    name = font->getOrigName()->copy();
  } else {
    name = NULL;
  }

  // font type
  type = (FontInfo::Type)font->getType();

  // check for an embedded font
  if (font->getType() == fontType3) {
    emb = gTrue;
  } else {
    emb = font->getEmbeddedFontID(&embRef);
  }

  if (!emb)
  {
    DisplayFontParam *dfp = globalParams->getDisplayFont(font);
    if (dfp)
    {
      if (dfp->kind == displayFontT1) file = dfp->t1.fileName->copy();
      else file = dfp->tt.fileName->copy();
    }
    else file = NULL;
  }
  else file = NULL;

  // look for a ToUnicode map
  hasToUnicode = gFalse;
  if (doc->getXRef()->fetch(fontRef.num, fontRef.gen, &fontObj)->isDict()) {
    hasToUnicode = fontObj.dictLookup("ToUnicode", &toUnicodeObj)->isStream();
    toUnicodeObj.free();
  }
  fontObj.free();

  // check for a font subset name: capital letters followed by a '+'
  // sign
  subset = gFalse;
  if (name) {
    for (i = 0; i < name->getLength(); ++i) {
      if (name->getChar(i) < 'A' || name->getChar(i) > 'Z') {
	break;
      }
    }
    subset = i > 0 && i < name->getLength() && name->getChar(i) == '+';
  }
}

FontInfo::FontInfo(FontInfo& f) {
  name = f.name ? f.name->copy() : NULL;
  file = f.file ? f.file->copy() : NULL;
  type = f.type;
  emb = f.emb;
  subset = f.subset;
  hasToUnicode = f.hasToUnicode;
  fontRef = f.fontRef;
  embRef = f.embRef;
}

FontInfo::~FontInfo() {
  delete name;
  delete file;
}
