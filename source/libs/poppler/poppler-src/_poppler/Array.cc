//========================================================================
//
// Array.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013, 2017 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <stddef.h>
#include "goo/gmem.h"
#include "Object.h"
#include "Array.h"

#ifdef MULTITHREADED
#  define arrayLocker()   MutexLocker locker(&mutex)
#else
#  define arrayLocker()
#endif
//------------------------------------------------------------------------
// Array
//------------------------------------------------------------------------

Array::Array(XRef *xrefA) {
  xref = xrefA;
  elems = NULL;
  size = length = 0;
  ref = 1;
#ifdef MULTITHREADED
  gInitMutex(&mutex);
#endif
}

Array::~Array() {
  int i;

  for (i = 0; i < length; ++i)
    elems[i].free();
  gfree(elems);
#ifdef MULTITHREADED
  gDestroyMutex(&mutex);
#endif
}

Object Array::copy(XRef *xrefA) const {
  arrayLocker();
  Array *a = new Array(xrefA);
  for (int i = 0; i < length; ++i) {
    a->add(elems[i].copy());
  }
  return Object(a);
}

int Array::incRef() {
  arrayLocker();
  ++ref;
  return ref;
}

int Array::decRef() {
  arrayLocker();
  --ref;
  return ref;
}

void Array::add(Object &&elem) {
  arrayLocker();
  if (length == size) {
    if (length == 0) {
      size = 8;
    } else {
      size *= 2;
    }
    elems = (Object *)greallocn(elems, size, sizeof(Object));
  }
  elems[length].initNullAfterMalloc();
  elems[length] = std::move(elem);
  ++length;
}

void Array::remove(int i) {
  arrayLocker();
  if (i < 0 || i >= length) {
#ifdef DEBUG_MEM
    abort();
#else
    return;
#endif
  }
  --length;
  memmove( elems + i, elems + i + 1, sizeof(elems[0]) * (length - i) );
}

Object Array::get(int i, int recursion) const {
  if (i < 0 || i >= length) {
#ifdef DEBUG_MEM
    abort();
#else
    return Object(objNull);
#endif
  }
  return elems[i].fetch(xref, recursion);
}

Object Array::getNF(int i) const {
  if (i < 0 || i >= length) {
#ifdef DEBUG_MEM
    abort();
#else
    return Object(objNull);
#endif
  }
  return elems[i].copy();
}

GBool Array::getString(int i, GooString *string) const
{
  Object obj = getNF(i);
  if (obj.isString()) {
    string->clear();
    string->append(obj.getString());
    return gTrue;
  } else {
    return gFalse;
  }
}
