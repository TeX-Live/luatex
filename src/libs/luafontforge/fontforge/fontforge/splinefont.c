/* Copyright (C) 2000-2008 by George Williams */
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

#include "fontforgevw.h"
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gfile.h>
#include <time.h>
#include "unicoderange.h"
#include "psfont.h"

void SFUntickAll(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;
}

void SFOrderBitmapList(SplineFont *sf) {
    BDFFont *bdf, *prev, *next;
    BDFFont *bdf2, *prev2;

    for ( prev = NULL, bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	for ( prev2=NULL, bdf2=bdf->next; bdf2!=NULL; bdf2 = bdf2->next ) {
	    if ( bdf->pixelsize>bdf2->pixelsize ||
		    (bdf->pixelsize==bdf2->pixelsize && BDFDepth(bdf)>BDFDepth(bdf2)) ) {
		if ( prev==NULL )
		    sf->bitmaps = bdf2;
		else
		    prev->next = bdf2;
		if ( prev2==NULL ) {
		    bdf->next = bdf2->next;
		    bdf2->next = bdf;
		} else {
		    next = bdf->next;
		    bdf->next = bdf2->next;
		    bdf2->next = next;
		    prev2->next = bdf;
		}
		next = bdf;
		bdf = bdf2;
		bdf2 = next;
	    }
	    prev2 = bdf2;
	}
	prev = bdf;
    }
}

SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,EncMap *map,int i) {
    static char namebuf[100];
    static Layer layers[2];

    memset(dummy,'\0',sizeof(*dummy));
    dummy->color = COLOR_DEFAULT;
    dummy->layer_cnt = 2;
    dummy->layers = layers;
    if ( sf->cidmaster!=NULL ) {
	/* CID fonts don't have encodings, instead we must look up the cid */
	if ( sf->cidmaster->loading_cid_map )
	    dummy->unicodeenc = -1;
	else
	    dummy->unicodeenc = CID2NameUni(FindCidMap(sf->cidmaster->cidregistry,sf->cidmaster->ordering,sf->cidmaster->supplement,sf->cidmaster),
		    i,namebuf,sizeof(namebuf));
    } else
	dummy->unicodeenc = UniFromEnc(i,map->enc);

    if ( sf->cidmaster!=NULL )
	dummy->name = namebuf;
    else if ( map->enc->psnames!=NULL && i<map->enc->char_cnt &&
	    map->enc->psnames[i]!=NULL )
	dummy->name = map->enc->psnames[i];
    else if ( dummy->unicodeenc==-1 )
	dummy->name = NULL;
    else
	dummy->name = (char *) StdGlyphName(namebuf,dummy->unicodeenc,sf->uni_interp,sf->for_new_glyphs);
    if ( dummy->name==NULL ) {
	/*if ( dummy->unicodeenc!=-1 || i<256 )
	    dummy->name = ".notdef";
	else*/ {
	    int j;
	    sprintf( namebuf, "NameMe.%d", i);
	    j=0;
	    while ( SFFindExistingSlot(sf,-1,namebuf)!=-1 )
		sprintf( namebuf, "NameMe.%d.%d", i, ++j);
	    dummy->name = namebuf;
	}
    }
    dummy->width = dummy->vwidth = sf->ascent+sf->descent;
    if ( dummy->unicodeenc>0 && dummy->unicodeenc<0x10000 &&
	    iscombining(dummy->unicodeenc)) {
	/* Mark characters should be 0 width */
	dummy->width = 0;
	/* Except in monospaced fonts on windows, where they should be the */
	/*  same width as everything else */
    }
    /* Actually, in a monospace font, all glyphs should be the same width */
    /*  whether mark or not */
    if ( sf->pfminfo.panose_set && sf->pfminfo.panose[3]==9 &&
	    sf->glyphcnt>0 ) {
	for ( i=sf->glyphcnt-1; i>=0; --i )
	    if ( SCWorthOutputting(sf->glyphs[i])) {
		dummy->width = sf->glyphs[i]->width;
	break;
	    }
    }
    dummy->parent = sf;
    dummy->orig_pos = 0xffff;
return( dummy );
}

static SplineChar *_SFMakeChar(SplineFont *sf,EncMap *map,int enc) {
    SplineChar dummy, *sc;
    SplineFont *ssf;
    int j, real_uni, gid;
    extern const int cns14pua[], amspua[];

    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->subfontcnt!=0 && gid!=-1 ) {
	ssf = NULL;
	for ( j=0; j<sf->subfontcnt; ++j )
	    if ( gid<sf->subfonts[j]->glyphcnt ) {
		ssf = sf->subfonts[j];
		if ( ssf->glyphs[gid]!=NULL ) {
return( ssf->glyphs[gid] );
		}
	    }
	sf = ssf;
    }

    if ( gid==-1 || (sc = sf->glyphs[gid])==NULL ) {
	if (( map->enc->is_unicodebmp || map->enc->is_unicodefull ) &&
		( enc>=0xe000 && enc<=0xf8ff ) &&
		( sf->uni_interp==ui_ams || sf->uni_interp==ui_trad_chinese ) &&
		( real_uni = (sf->uni_interp==ui_ams ? amspua : cns14pua)[enc-0xe000])!=0 ) {
	    if ( real_uni<map->enccount ) {
		SplineChar *sc;
		/* if necessary, create the real unicode code point */
		/*  and then make us be a duplicate of it */
		sc = _SFMakeChar(sf,map,real_uni);
		map->map[enc] = gid = sc->orig_pos;
		SCCharChangedUpdate(sc,ly_all);
return( sc );
	    }
	}

	SCBuildDummy(&dummy,sf,map,enc);
	/* Let's say a user has a postscript encoding where the glyph ".notdef" */
	/*  is assigned to many slots. Once the user creates a .notdef glyph */
	/*  all those slots should fill in. If they don't they damn well better*/
	/*  when the user clicks on one to edit it */
	/* Used to do that with all encodings. It just confused people */
	if ( map->enc->psnames!=NULL &&
		(sc = SFGetChar(sf,dummy.unicodeenc,dummy.name))!=NULL ) {
	    map->map[enc] = sc->orig_pos;
	    AltUniAdd(sc,dummy.unicodeenc);
return( sc );
	}
	sc = SFSplineCharCreate(sf);
	sc->unicodeenc = dummy.unicodeenc;
	sc->name = copy(dummy.name);
	sc->width = dummy.width;
	sc->orig_pos = 0xffff;
	/*SCLigDefault(sc);*/
	SFAddGlyphAndEncode(sf,sc,map,enc);
    }
return( sc );
}

SplineChar *SFMakeChar(SplineFont *sf,EncMap *map, int enc) {
    int gid;

    if ( enc==-1 )
return( NULL );
    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->mm!=NULL && (gid==-1 || sf->glyphs[gid]==NULL) ) {
	int j;
	_SFMakeChar(sf->mm->normal,map,enc);
	for ( j=0; j<sf->mm->instance_count; ++j )
	    _SFMakeChar(sf->mm->instances[j],map,enc);
    }
return( _SFMakeChar(sf,map,enc));
}

struct unicoderange specialnames[] = {
    { NULL }
};

int NameToEncoding(SplineFont *sf,EncMap *map,const char *name) {
    int enc, uni, i, ch;
    char *end, *freeme=NULL;
    const char *upt = name;

    ch = utf8_ildb(&upt);
    if ( *upt=='\0' ) {
	enc = SFFindSlot(sf,map,ch,NULL);
	if ( enc!=-1 )
return( enc );
    }

    enc = uni = -1;
	
    enc = SFFindSlot(sf,map,-1,name);
    if ( enc!=-1 ) {
	free(freeme);
return( enc );
    }
    if ( (*name=='U' || *name=='u') && name[1]=='+' ) {
	uni = strtol(name+2,&end,16);
	if ( *end!='\0' )
	    uni = -1;
    } else if ( name[0]=='u' && name[1]=='n' && name[2]=='i' ) {
	uni = strtol(name+3,&end,16);
	if ( *end!='\0' )
	    uni = -1;
    } else if ( name[0]=='g' && name[1]=='l' && name[2]=='y' && name[3]=='p' && name[4]=='h' ) {
	int orig = strtol(name+5,&end,10);
	if ( *end!='\0' )
	    orig = -1;
	if ( orig!=-1 )
	    enc = map->backmap[orig];
    } else if ( isdigit(*name)) {
	enc = strtoul(name,&end,0);
	if ( *end!='\0' )
	    enc = -1;
	if ( map->remap!=NULL && enc!=-1 ) {
	    struct remap *remap = map->remap;
	    while ( remap->infont!=-1 ) {
		if ( enc>=remap->firstenc && enc<=remap->lastenc ) {
		    enc += remap->infont-remap->firstenc;
	    break;
		}
		++remap;
	    }
	}
    } else {
	if ( enc==-1 ) {
	    uni = UniFromName(name,sf->uni_interp,map->enc);
	    if ( uni<0 ) {
		for ( i=0; specialnames[i].name!=NULL; ++i )
		    if ( strcmp(name,specialnames[i].name)==0 ) {
			uni = specialnames[i].first;
		break;
		    }
	    }
	    if ( uni<0 && name[1]=='\0' )
		uni = name[0];
	}
    }
    if ( enc>=map->enccount || enc<0 )
	enc = -1;
    if ( enc==-1 && uni!=-1 )
	enc = SFFindSlot(sf,map,uni,NULL);
    /* Used to have code to remove dotted variant names and apply extensions */
    /*  like ".initial" to get the unicode for arabic init/medial/final variants */
    /*  But that doesn't sound like a good idea. And it would also map "a.sc" */
    /*  to "a" -- which was confusing */
return( enc );
}

void SFRemoveUndoes(SplineFont *sf,uint8 *selected, EncMap *map) {
    SplineFont *main = sf->cidmaster? sf->cidmaster : sf, *ssf;
    int i,k, max, layer, gid;
    SplineChar *sc;
    BDFFont *bdf;

    if ( selected!=NULL || main->subfontcnt==0 )
	max = sf->glyphcnt;
    else {
	max = 0;
	for ( k=0; k<main->subfontcnt; ++k )
	    if ( main->subfonts[k]->glyphcnt>max ) max = main->subfonts[k]->glyphcnt;
    }
    for ( i=0; ; ++i ) {
	if ( selected==NULL || main->subfontcnt!=0 ) {
	    if ( i>=max )
    break;
	    gid = i;
	} else {
	    if ( i>=map->enccount )
    break;
	    if ( !selected[i])
    continue;
	    gid = map->map[i];
	    if ( gid==-1 )
    continue;
	}
	for ( bdf=main->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->glyphs[gid]!=NULL ) {
		UndoesFree(bdf->glyphs[gid]->undoes); bdf->glyphs[gid]->undoes = NULL;
		UndoesFree(bdf->glyphs[gid]->redoes); bdf->glyphs[gid]->redoes = NULL;
	    }
	}
	k = 0;
	do {
	    ssf = main->subfontcnt==0? main: main->subfonts[k];
	    if ( gid<ssf->glyphcnt && ssf->glyphs[gid]!=NULL ) {
		sc = ssf->glyphs[gid];
		for ( layer = 0; layer<sc->layer_cnt; ++layer ) {
		    UndoesFree(sc->layers[layer].undoes); sc->layers[layer].undoes = NULL;
		    UndoesFree(sc->layers[layer].redoes); sc->layers[layer].redoes = NULL;
		}
	    }
	    ++k;
	} while ( k<main->subfontcnt );
    }
}

static void _SplineFontSetUnChanged(SplineFont *sf) {
    int i;
    int was = sf->changed;
    BDFFont *bdf;

    sf->changed = false;
    SFClearAutoSave(sf);
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	if ( sf->glyphs[i]->changed ) {
	    sf->glyphs[i]->changed = false;
	    SCRefreshTitles(sf->glyphs[i]);
	}
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL )
	    bdf->glyphs[i]->changed = false;
    if ( was )
	FVRefreshAll(sf);
    if ( was )
	FVSetTitles(sf);
    for ( i=0; i<sf->subfontcnt; ++i )
	_SplineFontSetUnChanged(sf->subfonts[i]);
}

void SplineFontSetUnChanged(SplineFont *sf) {
    int i;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    if ( sf->mm!=NULL ) sf = sf->mm->normal;
    _SplineFontSetUnChanged(sf);
    if ( sf->mm!=NULL )
	for ( i=0; i<sf->mm->instance_count; ++i )
	    _SplineFontSetUnChanged(sf->mm->instances[i]);
}

static char *scaleString(char *string, double scale) {
    char *result;
    char *pt;
    char *end;
    double val;

    if ( string==NULL )
return( NULL );

    while ( *string==' ' ) ++string;
    result = galloc(10*strlen(string)+1);
    if ( *string!='[' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free( result );
return( NULL );
	}
	sprintf( result, "%g", val*scale);
return( result );
    }

    pt = result;
    *pt++ = '[';
    ++string;
    while ( *string!='\0' && *string!=']' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free(result);
return( NULL );
	}
	sprintf( pt, "%g ", val*scale);
	pt += strlen(pt);
	string = end;
    }
    if ( pt[-1] == ' ' ) pt[-1] = ']';
    else *pt++ = ']';
    *pt = '\0';
return( result );
}

static char *iscaleString(char *string, double scale) {
    char *result;
    char *pt;
    char *end;
    double val;

    if ( string==NULL )
return( NULL );

    while ( *string==' ' ) ++string;
    result = galloc(10*strlen(string)+1);
    if ( *string!='[' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free( result );
return( NULL );
	}
	sprintf( result, "%g", rint(val*scale));
return( result );
    }

    pt = result;
    *pt++ = '[';
    ++string;
    while ( *string!='\0' && *string!=']' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free(result);
return( NULL );
	}
	sprintf( pt, "%g ", rint(val*scale));
	pt += strlen(pt);
	string = end;
	while ( *string==' ' ) ++string;
    }
    if ( pt[-1] == ' ' ) pt[-1] = ']';
    else *pt++ = ']';
    *pt = '\0';
return( result );
}

static void SFScalePrivate(SplineFont *sf,double scale) {
    static char *scalethese[] = {
	NULL
    };
    static char *integerscalethese[] = {
	"BlueValues", "OtherBlues",
	"FamilyBlues", "FamilyOtherBlues",
	"BlueShift", "BlueFuzz",
	"StdHW", "StdVW", "StemSnapH", "StemSnapV",
	NULL
    };
    int i;

    for ( i=0; integerscalethese[i]!=NULL; ++i ) {
	char *str = PSDictHasEntry(sf->private,integerscalethese[i]);
	char *new = iscaleString(str,scale);
	if ( new!=NULL )
	    PSDictChangeEntry(sf->private,integerscalethese[i],new);
	free(new);
    }
    for ( i=0; scalethese[i]!=NULL; ++i ) {
	char *str = PSDictHasEntry(sf->private,scalethese[i]);
	char *new = scaleString(str,scale);
	if ( new!=NULL )
	    PSDictChangeEntry(sf->private,scalethese[i],new);
	free(new);
    }
}

static void ScaleBase(struct Base *base, double scale) {
    struct basescript *bs;
    struct baselangextent *bl, *feat;
    int i;

    for ( bs=base->scripts; bs!=NULL; bs=bs->next ) {
	for ( i=0 ; i<base->baseline_cnt; ++i )
	    bs->baseline_pos[i] = (int) rint(bs->baseline_pos[i]*scale);
	for ( bl = bs->langs; bl!=NULL; bl=bl->next ) {
	    bl->ascent  = (int) rint( scale*bl->ascent );
	    bl->descent = (int) rint( scale*bl->descent );
	    for ( feat = bl->features; feat!=NULL; feat = feat->next ) {
		feat->ascent  = (int) rint( scale*feat->ascent );
		feat->descent = (int) rint( scale*feat->descent );
	    }
	}
    }
}

int SFScaleToEm(SplineFont *sf, int as, int des) {
    double scale;
    real transform[6];
    BVTFunc bvts;
    uint8 *oldselected = sf->fv->selected;

    scale = (as+des)/(double) (sf->ascent+sf->descent);
    sf->pfminfo.hhead_ascent = rint( sf->pfminfo.hhead_ascent * scale);
    sf->pfminfo.hhead_descent = rint( sf->pfminfo.hhead_descent * scale);
    sf->pfminfo.linegap = rint( sf->pfminfo.linegap * scale);
    sf->pfminfo.vlinegap = rint( sf->pfminfo.vlinegap * scale);
    sf->pfminfo.os2_winascent = rint( sf->pfminfo.os2_winascent * scale);
    sf->pfminfo.os2_windescent = rint( sf->pfminfo.os2_windescent * scale);
    sf->pfminfo.os2_typoascent = rint( sf->pfminfo.os2_typoascent * scale);
    sf->pfminfo.os2_typodescent = rint( sf->pfminfo.os2_typodescent * scale);
    sf->pfminfo.os2_typolinegap = rint( sf->pfminfo.os2_typolinegap * scale);

    sf->pfminfo.os2_subxsize = rint( sf->pfminfo.os2_subxsize * scale);
    sf->pfminfo.os2_subysize = rint( sf->pfminfo.os2_subysize * scale);
    sf->pfminfo.os2_subxoff = rint( sf->pfminfo.os2_subxoff * scale);
    sf->pfminfo.os2_subyoff = rint( sf->pfminfo.os2_subyoff * scale);
    sf->pfminfo.os2_supxsize = rint( sf->pfminfo.os2_supxsize * scale);
    sf->pfminfo.os2_supysize = rint(sf->pfminfo.os2_supysize *  scale);
    sf->pfminfo.os2_supxoff = rint( sf->pfminfo.os2_supxoff * scale);
    sf->pfminfo.os2_supyoff = rint( sf->pfminfo.os2_supyoff * scale);
    sf->pfminfo.os2_strikeysize = rint( sf->pfminfo.os2_strikeysize * scale);
    sf->pfminfo.os2_strikeypos = rint( sf->pfminfo.os2_strikeypos * scale);
    sf->upos *= scale;
    sf->uwidth *= scale;

    if ( sf->private!=NULL )
	SFScalePrivate(sf,scale);
    if ( sf->horiz_base!=NULL )
	ScaleBase(sf->horiz_base, scale);
    if ( sf->vert_base!=NULL )
	ScaleBase(sf->vert_base, scale);

    if ( as+des == sf->ascent+sf->descent ) {
	if ( as!=sf->ascent && des!=sf->descent ) {
	    sf->ascent = as; sf->descent = des;
	    sf->changed = true;
	}
return( false );
    }

    transform[0] = transform[3] = scale;
    transform[1] = transform[2] = transform[4] = transform[5] = 0;
    bvts.func = bvt_none;
    sf->fv->selected = galloc(sf->fv->map->enccount);
    memset(sf->fv->selected,1,sf->fv->map->enccount);

    sf->ascent = as; sf->descent = des;

    FVTransFunc(sf->fv,transform,0,&bvts,
	    fvt_dobackground|fvt_round_to_int|fvt_dontsetwidth|fvt_scalekernclasses|fvt_scalepstpos|fvt_dogrid);
    free(sf->fv->selected);
    sf->fv->selected = oldselected;

    if ( !sf->changed ) {
	sf->changed = true;
	FVSetTitles(sf);
    }
	
return( true );
}

void SFSetModTime(SplineFont *sf) {
    time_t now;
    time(&now);
    sf->modificationtime = now;
}

static SplineFont *_SFReadPostscript(FILE *file,char *filename) {
    FontDict *fd=NULL;
    SplineFont *sf=NULL;

    ff_progress_change_stages(2);
    fd = _ReadPSFont(file);
    ff_progress_next_stage();
    ff_progress_change_line2(_("Interpreting Glyphs"));
    if ( fd!=NULL ) {
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
	if ( sf!=NULL )
	    CheckAfmOfPostscript(sf,filename,sf->map);
    }
return( sf );
}

static SplineFont *SFReadPostscript(char *filename) {
    FontDict *fd=NULL;
    SplineFont *sf=NULL;

    ff_progress_change_stages(2);
    fd = ReadPSFont(filename);
    ff_progress_next_stage();
    ff_progress_change_line2(_("Interpreting Glyphs"));
    if ( fd!=NULL ) {
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
	if ( sf!=NULL )
	    CheckAfmOfPostscript(sf,filename,sf->map);
    }
return( sf );
}

struct archivers archivers[] = {
    { ".tar", "tar", "tar", "tf", "xf", "rf", ars_tar },
    { ".tgz", "tar", "tar", "tfz", "xfz", "rfz", ars_tar },
    { ".tar.gz", "tar", "tar", "tfz", "xfz", "rfz", ars_tar },
    { ".tar.bz2", "tar", "tar", "tfj", "xfj", "rfj", ars_tar },
    { ".tbz2", "tar", "tar", "tfj", "xfj", "rfj", ars_tar },
    { ".tbz", "tar", "tar", "tfj", "xfj", "rfj", ars_tar },
    { ".zip", "unzip", "zip", "-l", "", "", ars_zip },
    NULL
};

void ArchiveCleanup(char *archivedir) {
    /* Free this directory and all files within it */
    char *cmd;

    cmd = galloc(strlen(archivedir) + 20);
    sprintf( cmd, "rm -rf %s", archivedir );
    system( cmd );
    free( cmd ); free(archivedir);
}

static char *ArchiveParseTOC(char *listfile, enum archive_list_style ars, int *doall) {
    FILE *file;
    int nlcnt, ch, linelen, linelenmax, fcnt, choice, i, def, def_prio, prio;
    char **files, *linebuffer, *pt, *name;

    *doall = false;
    file = fopen(listfile,"r");
    if ( file==NULL )
return( NULL );

    nlcnt=linelenmax=linelen=0;
    while ( (ch=getc(file))!=EOF ) {
	if ( ch=='\n' ) {
	    ++nlcnt;
	    if ( linelen>linelenmax ) linelenmax = linelen;
	    linelen = 0;
	} else
	    ++linelen;
    }
    rewind(file);

    /* tar outputs its table of contents as a simple list of names */
    /* zip includes a bunch of other info, headers (and lines for directories)*/

    linebuffer = galloc(linelenmax+3);
    fcnt = 0;
    files = galloc((nlcnt+1)*sizeof(char *));

    if ( ars == ars_tar ) {
	pt = linebuffer;
	while ( (ch=getc(file))!=EOF ) {
	    if ( ch=='\n' ) {
		*pt = '\0';
		/* Blessed if I know what encoded was used for filenames */
		/*  inside the tar file. I shall assume utf8, faut de mieux */
		files[fcnt++] = copy(linebuffer);
		pt = linebuffer;
	    } else
		*pt++ = ch;
	}
    } else {
	/* Skip the first three lines, header info */
	fgets(linebuffer,linelenmax+3,file);
	fgets(linebuffer,linelenmax+3,file);
	fgets(linebuffer,linelenmax+3,file);
	pt = linebuffer;
	while ( (ch=getc(file))!=EOF ) {
	    if ( ch=='\n' ) {
		*pt = '\0';
		if ( linebuffer[0]==' ' && linebuffer[1]=='-' && linebuffer[2]=='-' )
	break;		/* End of file list */
		/* Blessed if I know what encoded was used for filenames */
		/*  inside the zip file. I shall assume utf8, faut de mieux */
		if ( pt-linebuffer>=28 && pt[-1]!='/' )
		    files[fcnt++] = copy(linebuffer+28);
		pt = linebuffer;
	    } else
		*pt++ = ch;
	}
    }
    files[fcnt] = NULL;

    free(linebuffer);
    if ( fcnt==0 ) {
	free(files);
return( NULL );
    } else if ( fcnt==1 ) {
	char *onlyname = files[0];
	free(files);
return( onlyname );
    }

    /* Suppose they've got an archive of a directory format font? I mean a ufo*/
    /*  or a sfdir. It won't show up in the list of files (because either     */
    /*  tar or I have removed all directories from that list) */
    pt = strrchr(files[0],'/');
    if ( pt!=NULL ) {
	if (( pt-files[0]>4 && (strncasecmp(pt-4,".ufo",4)==0 || strncasecmp(pt-4,"_ufo",4)==0)) ||
		( pt-files[0]>6 && (strncasecmp(pt-6,".sfdir",6)==0 || strncasecmp(pt-6,"_sfdir",6)==0)) ) {
	    /* Ok, looks like a potential directory font. Now is EVERYTHING */
	    /*  in the archive inside this guy? */
	    for ( i=0; i<fcnt; ++i )
		if ( strncmp(files[i],files[0],pt-files[0]+1)!=0 )
	    break;
	    if ( i==fcnt ) {
		char *onlydirfont = copyn(files[0],pt-files[0]+1);
		for ( i=0; i<fcnt; ++i )
		    free(files[i]);
		free(files);
		*doall = true;
return( onlydirfont );
	    }
	}
    }

    def=0; def_prio = -1;
    for ( i=0; i<fcnt; ++i ) {
	pt = strrchr(files[i],'.');
	if ( pt==NULL )
    continue;
	if ( strcasecmp(pt,".svg")==0 )
	    prio = 10;
	else if ( strcasecmp(pt,".pfb")==0 || strcasecmp(pt,".pfa")==0 ||
		strcasecmp(pt,".cff")==0 || strcasecmp(pt,".cid")==0 )
	    prio = 20;
	else if ( strcasecmp(pt,".otf")==0 || strcasecmp(pt,".ttf")==0 || strcasecmp(pt,".ttc")==0 )
	    prio = 30;
	else if ( strcasecmp(pt,".sfd")==0 )
	    prio = 40;
	else
    continue;
	if ( prio>def_prio ) {
	    def = i;
	    def_prio = prio;
	}
    }

    choice = ff_choose(_("Which archived item should be opened?"),(const char **) files,fcnt,def,_("There are multiple files in this archive, pick one"));
    if ( choice==-1 )
	name = NULL;
    else
	name = copy(files[choice]);

    for ( i=0; i<fcnt; ++i )
	free(files[i]);
    free(files);
return( name );
}
    
#define TOC_NAME	"ff-archive-table-of-contents"

char *Unarchive(char *name, char **_archivedir) {
    char *dir = getenv("TMPDIR");
    char *pt, *archivedir, *listfile, *listcommand, *unarchivecmd, *desiredfile;
    char *finalfile;
    int i;
    int doall=false;
    static int cnt=0;

    *_archivedir = NULL;

    pt = strrchr(name,'.');
    if ( pt==NULL )
return( NULL );
    for ( i=0; archivers[i].ext!=NULL; ++i )
	if ( strcmp(archivers[i].ext,pt)==0 )
    break;
    if ( archivers[i].ext==NULL )
return( NULL );

    if ( dir==NULL ) dir = P_tmpdir;
    archivedir = galloc(strlen(dir)+100);
    sprintf( archivedir, "%s/ffarchive-%d-%d", dir, getpid(), ++cnt );
    if ( mkdir(archivedir,0700)!=0 ) {
	free(archivedir);
return( NULL );
    }

    listfile = galloc(strlen(archivedir)+strlen("/" TOC_NAME)+1);
    sprintf( listfile, "%s/" TOC_NAME, archivedir );

    listcommand = galloc( strlen(archivers[i].unarchive) + 1 +
			strlen( archivers[i].listargs) + 1 +
			strlen( name ) + 3 +
			strlen( listfile ) +4 );
    sprintf( listcommand, "%s %s %s > %s", archivers[i].unarchive,
	    archivers[i].listargs, name, listfile );
    if ( system(listcommand)!=0 ) {
	free(listcommand); free(listfile);
	ArchiveCleanup(archivedir);
return( NULL );
    }
    free(listcommand);

    desiredfile = ArchiveParseTOC(listfile, archivers[i].ars, &doall);
    free(listfile);
    if ( desiredfile==NULL ) {
	ArchiveCleanup(archivedir);
return( NULL );
    }

    /* I tried sending everything to stdout, but that doesn't work if the */
    /*  output is a directory file (ufo, sfdir) */
    unarchivecmd = galloc( strlen(archivers[i].unarchive) + 1 +
			strlen( archivers[i].listargs) + 1 +
			strlen( name ) + 1 +
			strlen( desiredfile ) + 3 +
			strlen( archivedir ) + 30 );
    sprintf( unarchivecmd, "( cd %s ; %s %s %s %s ) > /dev/null", archivedir,
	    archivers[i].unarchive,
	    archivers[i].extractargs, name, doall ? "" : desiredfile );
    if ( system(unarchivecmd)!=0 ) {
	free(unarchivecmd); free(desiredfile);
	ArchiveCleanup(archivedir);
return( NULL );
    }
    free(unarchivecmd);

    finalfile = galloc( strlen(archivedir) + 1 + strlen(desiredfile) + 1);
    sprintf( finalfile, "%s/%s", archivedir, desiredfile );
    free( desiredfile );

    *_archivedir = archivedir;
return( finalfile );
}

struct compressors compressors[] = {
    { ".gz", "gunzip", "gzip" },
    { ".bz2", "bunzip2", "bzip2" },
    { ".bz", "bunzip2", "bzip2" },
    { ".Z", "gunzip", "compress" },
/* file types which are both archived and compressed (.tgz, .zip) are handled */
/*  by the archiver above */
    NULL
};

char *Decompress(char *name, int compression) {
    char *dir = getenv("TMPDIR");
    char buf[1500];
    char *tmpfile;

    if ( dir==NULL ) dir = P_tmpdir;
    tmpfile = galloc(strlen(dir)+strlen(GFileNameTail(name))+2);
    strcpy(tmpfile,dir);
    strcat(tmpfile,"/");
    strcat(tmpfile,GFileNameTail(name));
    *strrchr(tmpfile,'.') = '\0';
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    sprintf( buf, "%s < %s > %s", compressors[compression].decomp, name, tmpfile );
#else
    snprintf( buf, sizeof(buf), "%s < %s > %s", compressors[compression].decomp, name, tmpfile );
#endif
    if ( system(buf)==0 )
return( tmpfile );
    free(tmpfile);
return( NULL );
}

static char *ForceFileToHaveName(FILE *file, char *exten) {
    char tmpfilename[L_tmpnam+100];
    static int try=0;
    FILE *newfile;

    forever {
	sprintf( tmpfilename, P_tmpdir "/fontforge%d-%d", getpid(), try++ );
	if ( exten!=NULL )
	    strcat(tmpfilename,exten);
	if ( access( tmpfilename, F_OK )==-1 &&
		(newfile = fopen(tmpfilename,"w"))!=NULL ) {
	    char buffer[1024];
	    int len;
	    while ( (len = fread(buffer,1,sizeof(buffer),file))>0 )
		fwrite(buffer,1,len,newfile);
	    fclose(newfile);
	}
return(copy(tmpfilename));			/* The filename does not exist */
    }
}

/* This does not check currently existing fontviews, and should only be used */
/*  by LoadSplineFont (which does) and by RevertFile (which knows what it's doing) */
SplineFont *_ReadSplineFont(FILE *file,char *filename,enum openflags openflags) {
    SplineFont *sf;
    char ubuf[250], *temp;
    int fromsfd = false;
    int i;
    char *pt, *strippedname, *oldstrippedname, *tmpfile=NULL, *paren=NULL, *fullname=filename, *rparen;
    char *archivedir=NULL;
    int len;
    int checked;
    int compression=0;
    int wasurl = false, nowlocal = true, wasarchived=false;

    if ( filename==NULL )
return( NULL );

    strippedname = filename;
    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    /* Someone gave me a font "Nafees Nastaleeq(Updated).ttf" and complained */
    /*  that ff wouldn't open it */
    /* Now someone will complain about "Nafees(Updated).ttc(fo(ob)ar)" */
    if ( (paren = strrchr(pt,'('))!=NULL &&
	    (rparen = strrchr(paren,')'))!=NULL &&
	    rparen[1]=='\0' ) {
	strippedname = copy(filename);
	strippedname[paren-filename] = '\0';
    }

    if ( strstr(strippedname,"://")!=NULL ) {
	if ( file==NULL )
	    file = URLToTempFile(strippedname,NULL);
	if ( file==NULL )
return( NULL );
	wasurl = true; nowlocal = false;
    }

    pt = strrchr(strippedname,'.');
    if ( pt!=NULL ) {
	for ( i=0; archivers[i].ext!=NULL; ++i ) {
	    if ( strcmp(archivers[i].ext,pt)==0 ) {
		if ( file!=NULL ) {
		    char *spuriousname = ForceFileToHaveName(file,archivers[i].ext);
		    strippedname = Unarchive(spuriousname,&archivedir);
		    fclose(file); file = NULL;
		    unlink(spuriousname); free(spuriousname);
		} else
		    strippedname = Unarchive(strippedname,&archivedir);
		if ( strippedname==NULL )
return( NULL );
		if ( strippedname!=filename && paren!=NULL ) {
		    fullname = galloc(strlen(strippedname)+strlen(paren)+1);
		    strcpy(fullname,strippedname);
		    strcat(fullname,paren);
		} else
		    fullname = strippedname;
		pt = strrchr(strippedname,'.');
		wasarchived = true;
	break;
	    }
	}
    }

    i = -1;
    if ( pt!=NULL ) for ( i=0; compressors[i].ext!=NULL; ++i )
	if ( strcmp(compressors[i].ext,pt)==0 )
    break;
    oldstrippedname = strippedname;
    if ( i==-1 || compressors[i].ext==NULL )
	i=-1;
    else {
	if ( file!=NULL ) {
	    char *spuriousname = ForceFileToHaveName(file,compressors[i].ext);
	    tmpfile = Decompress(spuriousname,i);
	    fclose(file); file = NULL;
	    unlink(spuriousname); free(spuriousname);
	} else
	    tmpfile = Decompress(strippedname,i);
	if ( tmpfile!=NULL ) {
	    strippedname = tmpfile;
	} else {
	    ff_post_error(_("Decompress Failed!"),_("Decompress Failed!"));
return( NULL );
	}
	compression = i+1;
	if ( strippedname!=filename && paren!=NULL ) {
	    fullname = galloc(strlen(strippedname)+strlen(paren)+1);
	    strcpy(fullname,strippedname);
	    strcat(fullname,paren);
	} else
	    fullname = strippedname;
    }

    /* If there are no pfaedit windows, give them something to look at */
    /*  immediately. Otherwise delay a bit */
    strcpy(ubuf,_("Loading font from "));
    len = strlen(ubuf);
    if ( !wasurl || i==-1 )	/* If it wasn't compressed, or it wasn't an url, then the fullname is reasonable, else use the original name */
	strncat(ubuf,temp = def2utf8_copy(GFileNameTail(fullname)),100);
    else
	strncat(ubuf,temp = def2utf8_copy(GFileNameTail(filename)),100);
    free(temp);
    ubuf[100+len] = '\0';
    ff_progress_start_indicator(FontViewFirst()==NULL?0:10,_("Loading..."),ubuf,_("Reading Glyphs"),0,1);
    ff_progress_enable_stop(0);
    if ( FontViewFirst()==NULL && !no_windowing_ui )
	ff_progress_allow_events();

    if ( file==NULL ) {
	file = fopen(strippedname,"rb");
	nowlocal = true;
    }

    sf = NULL;
    checked = false;
/* checked == false => not checked */
/* checked == 'u'   => UFO */
/* checked == 't'   => TTF/OTF */
/* checked == 'p'   => pfb/general postscript */
/* checked == 'P'   => pdf */
/* checked == 'c'   => cff */
/* checked == 'S'   => svg */
/* checked == 'f'   => sfd */
/* checked == 'F'   => sfdir */
/* checked == 'b'   => bdf */
/* checked == 'i'   => ikarus */
    if ( !wasurl && GFileIsDir(strippedname) ) {
	char *temp = galloc(strlen(strippedname)+strlen("/glyphs/contents.plist")+1);
	strcpy(temp,strippedname);
	strcat(temp,"/glyphs/contents.plist");
	if ( GFileExists(temp)) {
	    sf = SFReadUFO(strippedname,0);
	    checked = 'u';
	} else {
	    strcpy(temp,strippedname);
	    strcat(temp,"/font.props");
	    if ( GFileExists(temp)) {
		sf = SFDirRead(strippedname);
		checked = 'F';
	    }
	}
	free(temp);
	if ( file!=NULL )
	    fclose(file);
    } else if ( file!=NULL ) {
	/* Try to guess the file type from the first few characters... */
	int ch1 = getc(file);
	int ch2 = getc(file);
	int ch3 = getc(file);
	int ch4 = getc(file);
	int ch5 = getc(file);
	int ch6 = getc(file);
	int ch7 = getc(file);
	int ch9, ch10;
	fseek(file, 98, SEEK_SET);
	ch9 = getc(file);
	ch10 = getc(file);
	rewind(file);
	if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		(ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		(ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		(ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) {
	    sf = _SFReadTTF(file,0,openflags,fullname,NULL);
	    checked = 't';
	} else if (( ch1=='%' && ch2=='!' ) ||
		    ( ch1==0x80 && ch2=='\01' ) ) {	/* PFB header */
	    sf = _SFReadPostscript(file,fullname);
	    checked = 'p';
	} else if ( ch1=='%' && ch2=='P' && ch3=='D' && ch4=='F' ) {
	    sf = _SFReadPdfFont(file,fullname,NULL,openflags);
	    checked = 'P';
	} else if ( ch1==1 && ch2==0 && ch3==4 ) {
	    int len;
	    fseek(file,0,SEEK_END);
	    len = ftell(file);
	    fseek(file,0,SEEK_SET);
	    sf = _CFFParse(file,len,NULL);
	    checked = 'c';
	} else if (( ch1=='<' && ch2=='?' && (ch3=='x'||ch3=='X') && (ch4=='m'||ch4=='M') ) ||
		/* or UTF-8 SVG with initial byte ordering mark */
			 (( ch1==0xef && ch2==0xbb && ch3==0xbf &&
			    ch4=='<' && ch5=='?' && (ch6=='x'||ch6=='X') && (ch7=='m'||ch7=='M') )) ) {
	    if ( nowlocal )
		sf = SFReadSVG(fullname,0);
	    else {
		char *spuriousname = ForceFileToHaveName(file,NULL);
		sf = SFReadSVG(spuriousname,0);
		unlink(spuriousname); free(spuriousname);
	    }
	    checked = 'S';
#if 0		/* I'm not sure if this is a good test for mf files... */
	} else if ( ch1=='%' && ch2==' ' ) {
	    sf = SFFromMF(fullname);
#endif
	} else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) {
	    sf = _SFDRead(fullname,file); file = NULL;
	    checked = 'f';
	    fromsfd = true;
	} else if ( ch1=='S' && ch2=='T' && ch3=='A' && ch4=='R' ) {
	    sf = SFFromBDF(fullname,0,false);
	    checked = 'b';
	} else if ( ch1=='\1' && ch2=='f' && ch3=='c' && ch4=='p' ) {
	    sf = SFFromBDF(fullname,2,false);
	} else if ( ch9=='I' && ch10=='K' && ch3==0 && ch4==55 ) {
	    /* Ikarus font type appears at word 50 (byte offset 98) */
	    /* Ikarus name section length (at word 2, byte offset 2) was 55 in the 80s at URW */
	    checked = 'i';
	    sf = SFReadIkarus(fullname);
	} /* Too hard to figure out a valid mark for a mac resource file */
	if ( file!=NULL ) fclose(file);
    }

    if ( sf!=NULL )
	/* good */;
    else if (( strmatch(fullname+strlen(fullname)-4, ".sfd")==0 ||
	 strmatch(fullname+strlen(fullname)-5, ".sfd~")==0 ) && checked!='f' ) {
	sf = SFDRead(fullname);
	fromsfd = true;
    } else if (( strmatch(fullname+strlen(fullname)-4, ".ttf")==0 ||
		strmatch(fullname+strlen(strippedname)-4, ".ttc")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".gai")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".otf")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".otb")==0 ) && checked!='t') {
	sf = SFReadTTF(fullname,0,openflags);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".svg")==0 && checked!='S' ) {
	sf = SFReadSVG(fullname,0);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".ufo")==0 && checked!='u' ) {
	sf = SFReadUFO(fullname,0);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".bdf")==0 && checked!='b' ) {
	sf = SFFromBDF(fullname,0,false);
    } else if ( strmatch(fullname+strlen(fullname)-2, "pk")==0 ) {
	sf = SFFromBDF(fullname,1,true);
    } else if ( strmatch(fullname+strlen(fullname)-2, "gf")==0 ) {
	sf = SFFromBDF(fullname,3,true);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".pcf")==0 ||
		 strmatch(fullname+strlen(fullname)-4, ".pmf")==0 ) {
	/* Sun seems to use a variant of the pcf format which they call pmf */
	/*  the encoding actually starts at 0x2000 and the one I examined was */
	/*  for a pixel size of 200. Some sort of printer font? */
	sf = SFFromBDF(fullname,2,false);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".bin")==0 ||
		strmatch(fullname+strlen(strippedname)-4, ".hqx")==0 ||
		strmatch(fullname+strlen(strippedname)-6, ".dfont")==0 ) {
	sf = SFReadMacBinary(fullname,0,openflags);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".fon")==0 ||
		strmatch(fullname+strlen(strippedname)-4, ".fnt")==0 ) {
	sf = SFReadWinFON(fullname,0);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".pdb")==0 ) {
	sf = SFReadPalmPdb(fullname,0);
    } else if ( (strmatch(fullname+strlen(fullname)-4, ".pfa")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pfb")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pf3")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".cid")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".gsf")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pt3")==0 ||
		strmatch(fullname+strlen(fullname)-3, ".ps")==0 ) && checked!='p' ) {
	sf = SFReadPostscript(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".cff")==0 && checked!='c' ) {
	sf = CFFParse(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-3, ".mf")==0 ) {
	sf = SFFromMF(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".pdf")==0 && checked!='P' ) {
	sf = SFReadPdfFont(fullname,openflags);
    } else if ( strmatch(fullname+strlen(fullname)-3, ".ik")==0 && checked!='i' ) {
	sf = SFReadIkarus(fullname);
    } else {
	sf = SFReadMacBinary(fullname,0,openflags);
    }
    ff_progress_end_indicator();

    if ( sf!=NULL ) {
	SplineFont *norm = sf->mm!=NULL ? sf->mm->normal : sf;
	if ( compression!=0 ) {
	    free(sf->filename);
	    *strrchr(oldstrippedname,'.') = '\0';
	    sf->filename = copy( oldstrippedname );
	}
	if ( fromsfd )
	    sf->compression = compression;
	free( norm->origname );
	if ( wasarchived ) {
	    norm->origname = NULL;
	    free(norm->filename); norm->filename = NULL;
	    norm->new = true;
	} else if ( sf->chosenname!=NULL && strippedname==filename ) {
	    norm->origname = galloc(strlen(filename)+strlen(sf->chosenname)+8);
	    strcpy(norm->origname,filename);
	    strcat(norm->origname,"(");
	    strcat(norm->origname,sf->chosenname);
	    strcat(norm->origname,")");
	} else
	    norm->origname = copy(filename);
	free( norm->chosenname ); norm->chosenname = NULL;
	if ( sf->mm!=NULL ) {
	    int j;
	    for ( j=0; j<sf->mm->instance_count; ++j ) {
		free(sf->mm->instances[j]->origname);
		sf->mm->instances[j]->origname = copy(norm->origname);
	    }
	}
    } else if ( !GFileExists(filename) )
	ff_post_error(_("Couldn't open font"),_("The requested file, %.100s, does not exist"),GFileNameTail(filename));
    else if ( !GFileReadable(filename) )
	ff_post_error(_("Couldn't open font"),_("You do not have permission to read %.100s"),GFileNameTail(filename));
    else
	ff_post_error(_("Couldn't open font"),_("%.100s is not in a known format (or is so badly corrupted as to be unreadable)"),GFileNameTail(filename));

    if ( oldstrippedname!=filename )
	free(oldstrippedname);
    if ( fullname!=filename && fullname!=strippedname )
	free(fullname);
    if ( tmpfile!=NULL ) {
	unlink(tmpfile);
	free(tmpfile);
    }
    if ( wasarchived )
	ArchiveCleanup(archivedir);
    if ( (openflags&of_fstypepermitted) && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
	/* Ok, they have told us from a script they have access to the font */
    } else if ( !fromsfd && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
	char *buts[3];
	buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
	if ( ff_ask(_("Restricted Font"),(const char **) buts,1,1,_("This font is marked with an FSType of 2 (Restricted\nLicense). That means it is not editable without the\npermission of the legal owner.\n\nDo you have such permission?"))==1 ) {
	    SplineFontFree(sf);
return( NULL );
	}
    }
return( sf );
}

SplineFont *ReadSplineFont(char *filename,enum openflags openflags) {
return( _ReadSplineFont(NULL,filename,openflags));
}


#ifdef LUA_FF_LIB
SplineFont *ReadSplineFontInfo(char *filename,enum openflags openflags) {
  SplineFont *sf, *sf_ptr;
	char **fontlist;
    char *pt =NULL, *strippedname=filename, *paren=NULL, *fullname=filename;
    FILE *foo = NULL;
    int checked = 0;
	char s[512] = {0};

    if ( filename==NULL )
return( NULL );

    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    if ( (paren=strchr(pt,'('))!=NULL && strchr(paren,')')!=NULL ) {
	    strippedname = copy(filename);
        strippedname[paren-filename] = '\0';
    }

    sf = NULL;
    foo = fopen(strippedname,"rb");
    checked = false;
    if ( foo!=NULL ) {
	/* Try to guess the file type from the first few characters... */
	int ch1 = getc(foo);
	int ch2 = getc(foo);
	int ch3 = getc(foo);
	int ch4 = getc(foo);
	fclose(foo);
	if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		(ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		(ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ) {
	    sf = SFReadTTFInfo(fullname,0,openflags);
	    checked = 't';
	} else if ((ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f')) {
	  /* read all fonts in a collection */
	  fontlist = NamesReadTTF(fullname);
	  if (fontlist) {
		while (*fontlist != NULL) {
		  snprintf(s,511, "%s(%s)", fullname,*fontlist);
		  sf_ptr = SFReadTTFInfo(s,0,openflags);
		  if (sf != NULL)
			sf_ptr->next = sf;	  
		  sf = sf_ptr;
		  fontlist++;
		}
	  }
	} else {
      sf = ReadSplineFont (fullname, openflags);
    }
    }
    if ( strippedname!=filename )
      free(strippedname);
return( sf );
}
#endif


char *ToAbsolute(char *filename) {
    char buffer[1025];

    GFileGetAbsoluteName(filename,buffer,sizeof(buffer));
return( copy(buffer));
}

SplineFont *LoadSplineFont(char *filename,enum openflags openflags) {
    SplineFont *sf;
    char *pt, *ept, *tobefreed1=NULL, *tobefreed2=NULL;
    static char *extens[] = { ".sfd", ".pfa", ".pfb", ".ttf", ".otf", ".ps", ".cid", ".bin", ".dfont", ".PFA", ".PFB", ".TTF", ".OTF", ".PS", ".CID", ".BIN", ".DFONT", NULL };
    int i;

    if ( filename==NULL )
return( NULL );

    if (( pt = strrchr(filename,'/'))==NULL ) pt = filename;
    if ( strchr(pt,'.')==NULL ) {
	/* They didn't give an extension. If there's a file with no extension */
	/*  see if it's a valid font file (and if so use the extensionless */
	/*  filename), otherwise guess at an extension */
	/* For some reason Adobe distributes CID keyed fonts (both OTF and */
	/*  postscript) as extensionless files */
	int ok = false;
	FILE *test = fopen(filename,"rb");
	if ( test!=NULL ) {
#if 0
	    int ch1 = getc(test);
	    int ch2 = getc(test);
	    int ch3 = getc(test);
	    int ch4 = getc(test);
	    if ( ch1=='%' ) ok = true;
	    else if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		    (  ch1==0 && ch2==2 && ch3==0 && ch4==0 ) ||
		    /* Windows 3.1 Chinese version used this version for some arphic fonts */
		    /* See discussion on freetype list, july 2004 */
		    (ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		    (ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		    (ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) ok = true;
	    else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) ok = true;
#endif
	    ok = true;		/* Mac resource files are too hard to check for */
		    /* If file exists, assume good */
	    fclose(test);
	}
	if ( !ok ) {
	    tobefreed1 = galloc(strlen(filename)+8);
	    strcpy(tobefreed1,filename);
	    ept = tobefreed1+strlen(tobefreed1);
	    for ( i=0; extens[i]!=NULL; ++i ) {
		strcpy(ept,extens[i]);
		if ( GFileExists(tobefreed1))
	    break;
	    }
	    if ( extens[i]!=NULL )
		filename = tobefreed1;
	    else {
		free(tobefreed1);
		tobefreed1 = NULL;
	    }
	}
    } else
	tobefreed1 = NULL;

    sf = NULL;
    sf = FontWithThisFilename(filename);
    if ( sf==NULL && *filename!='/' && strstr(filename,"://")==NULL )
	filename = tobefreed2 = ToAbsolute(filename);

    if ( sf==NULL )
	sf = ReadSplineFont(filename,openflags);

    free(tobefreed1);
    free(tobefreed2);
return( sf );
}

/* Use URW 4 letter abbreviations */
char *knownweights[] = { "Demi", "Bold", "Regu", "Medi", "Book", "Thin",
	"Ligh", "Heav", "Blac", "Ultr", "Nord", "Norm", "Gras", "Stan", "Halb",
	"Fett", "Mage", "Mitt", "Buch", NULL };
char *realweights[] = { "Demi", "Bold", "Regular", "Medium", "Book", "Thin",
	"Light", "Heavy", "Black", "Ultra", "Nord", "Normal", "Gras", "Standard", "Halbfett",
	"Fett", "Mager", "Mittel", "Buchschrift", NULL};
static char *moreweights[] = { "ExtraLight", "VeryLight", NULL };
char **noticeweights[] = { moreweights, realweights, knownweights, NULL };

static char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive", "Slanted",
	"Expa", "Cond", NULL };
static char *modifierlistfull[] = { "Italic", "Oblique", "Kursive", "Cursive", "Slanted",
    "Expanded", "Condensed", NULL };
static char **mods[] = { knownweights, modifierlist, NULL };
static char **fullmods[] = { realweights, modifierlistfull, NULL };

char *_GetModifiers(char *fontname, char *familyname,char *weight) {
    char *pt, *fpt;
    int i, j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt == NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }
    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( strcmp(fpt,mods[i][j])==0 )
return( fullmods[i][j]);
	}
	if ( strcmp(fpt,"BoldItal")==0 )
return( "BoldItalic" );
	else if ( strcmp(fpt,"BoldObli")==0 )
return( "BoldOblique" );

return( fpt );
    }

return( weight==NULL || *weight=='\0' ? "Regular": weight );
}

char *SFGetModifiers(SplineFont *sf) {
return( _GetModifiers(sf->fontname,sf->familyname,sf->weight));
}

const unichar_t *_uGetModifiers(const unichar_t *fontname, const unichar_t *familyname,
	const unichar_t *weight) {
    const unichar_t *pt, *fpt;
    static unichar_t regular[] = { 'R','e','g','u','l','a','r', 0 };
    static unichar_t space[20];
    int i,j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=u_strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt==NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = uc_strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }

    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( uc_strcmp(fpt,mods[i][j])==0 ) {
		uc_strcpy(space,fullmods[i][j]);
return( space );
	    }
	}
	if ( uc_strcmp(fpt,"BoldItal")==0 ) {
	    uc_strcpy(space,"BoldItalic");
return( space );
	} else if ( uc_strcmp(fpt,"BoldObli")==0 ) {
	    uc_strcpy(space,"BoldOblique");
return( space );
	}
return( fpt );
    }

return( weight==NULL || *weight=='\0' ? regular: weight );
}

int SFIsDuplicatable(SplineFont *sf, SplineChar *sc) {
    extern const int cns14pua[], amspua[];
    const int *pua = sf->uni_interp==ui_trad_chinese ? cns14pua : sf->uni_interp==ui_ams ? amspua : NULL;
    int baseuni = 0;
    const unichar_t *pt;

    if ( pua!=NULL && sc->unicodeenc>=0xe000 && sc->unicodeenc<=0xf8ff )
	baseuni = pua[sc->unicodeenc-0xe000];
    if ( baseuni==0 && ( pt = SFGetAlternate(sf,sc->unicodeenc,sc,false))!=NULL &&
	    pt[0]!='\0' && pt[1]=='\0' )
	baseuni = pt[0];
    if ( baseuni!=0 && SFGetChar(sf,baseuni,NULL)!=NULL )
return( true );

return( false );
}

static void arraystring(char *buffer,real *array,int cnt) {
    int i, ei;

    for ( ei=cnt; ei>1 && array[ei-1]==0; --ei );
    *buffer++ = '[';
    for ( i=0; i<ei; ++i ) {
	sprintf(buffer, "%d ", (int) array[i]);
	buffer += strlen(buffer);
    }
    if ( buffer[-1] ==' ' ) --buffer;
    *buffer++ = ']'; *buffer='\0';
}

static void SnapSet(struct psdict *private,real stemsnap[12], real snapcnt[12],
	char *name1, char *name2, int which ) {
    int i, mi;
    char buffer[211];

    mi = -1;
    for ( i=0; stemsnap[i]!=0 && i<12; ++i )
	if ( mi==-1 ) mi = i;
	else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
    if ( mi==-1 )
return;
    if ( which<2 ) {
	sprintf( buffer, "[%d]", (int) stemsnap[mi]);
	PSDictChangeEntry(private,name1,buffer);
    }
    if ( which==0 || which==2 ) {
	arraystring(buffer,stemsnap,12);
	PSDictChangeEntry(private,name2,buffer);
    }
}

int SFPrivateGuess(SplineFont *sf,int layer, struct psdict *private,char *name, int onlyone) {
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnap[12];
    char buffer[211];

    if ( strcmp(name,"BlueValues")==0 || strcmp(name,"OtherBlues")==0 ) {
	FindBlues(sf,layer,bluevalues,otherblues);
	if ( !onlyone || strcmp(name,"BlueValues")==0 ) {
	    arraystring(buffer,bluevalues,14);
	    PSDictChangeEntry(private,"BlueValues",buffer);
	}
	if ( !onlyone || strcmp(name,"OtherBlues")==0 ) {
	    if ( otherblues[0]!=0 || otherblues[1]!=0 ) {
		arraystring(buffer,otherblues,10);
		PSDictChangeEntry(private,"OtherBlues",buffer);
	    } else
		PSDictRemoveEntry(private, "OtherBlues");
	}
    } else if ( strcmp(name,"StdHW")==0 || strcmp(name,"StemSnapH")==0 ) {
	FindHStems(sf,stemsnap,snapcnt);
	SnapSet(private,stemsnap,snapcnt,"StdHW","StemSnapH",
		!onlyone ? 0 : strcmp(name,"StdHW")==0 ? 1 : 0 );
    } else if ( strcmp(name,"StdVW")==0 || strcmp(name,"StemSnapV")==0 ) {
	FindVStems(sf,stemsnap,snapcnt);
	SnapSet(private,stemsnap,snapcnt,"StdVW","StemSnapV",
		!onlyone ? 0 : strcmp(name,"StdVW")==0 ? 1 : 0);
    } else if ( strcmp(name,"BlueScale")==0 ) {
	double val = -1;
	if ( PSDictFindEntry(private,"BlueValues")!=-1 ) {
	    /* Can guess BlueScale if we've got a BlueValues */
	    val = BlueScaleFigureForced(private,NULL,NULL);
	}
	if ( val==-1 ) val = .039625;
	sprintf(buffer,"%g", val );
	PSDictChangeEntry(private,"BlueScale",buffer);
    } else if ( strcmp(name,"BlueShift")==0 ) {
	PSDictChangeEntry(private,"BlueShift","7");
    } else if ( strcmp(name,"BlueFuzz")==0 ) {
	PSDictChangeEntry(private,"BlueFuzz","1");
    } else if ( strcmp(name,"ForceBold")==0 ) {
	int isbold = false;
	if ( sf->weight!=NULL &&
		(strstrmatch(sf->weight,"Bold")!=NULL ||
		 strstrmatch(sf->weight,"Heavy")!=NULL ||
		 strstrmatch(sf->weight,"Black")!=NULL ||
		 strstrmatch(sf->weight,"Grass")!=NULL ||
		 strstrmatch(sf->weight,"Fett")!=NULL))
	    isbold = true;
	if ( sf->pfminfo.pfmset && sf->pfminfo.weight>=700 )
	    isbold = true;
	PSDictChangeEntry(private,"ForceBold",isbold ? "true" : "false" );
    } else if ( strcmp(name,"LanguageGroup")==0 ) {
	PSDictChangeEntry(private,"LanguageGroup","0" );
    } else if ( strcmp(name,"ExpansionFactor")==0 ) {
	PSDictChangeEntry(private,"ExpansionFactor","0.06" );
    } else
return( 0 );

return( true );
}

void SFRemoveLayer(SplineFont *sf,int l) {
    int gid, i;
    SplineChar *sc;
    CharViewBase *cvs;
    FontViewBase *fvs;
    int layers, any_quads;

    if ( sf->subfontcnt!=0 || l<=ly_fore || sf->multilayer )
return;

    for ( layers=ly_fore, any_quads=0; layers<sf->layer_cnt; ++layers ) {
	if ( layers!=l && sf->layers[layers].order2 )
	    any_quads = true;
    }
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	LayerFreeContents(sc,l);
	for ( i=l+1; i<sc->layer_cnt; ++i )
	    sc->layers[i-1] = sc->layers[i];
	-- sc->layer_cnt;
	for ( cvs = sc->views; cvs!=NULL; cvs=cvs->next ) {
	    if ( cvs->layerheads[dm_back] - sc->layers >= sc->layer_cnt )
		cvs->layerheads[dm_back] = &sc->layers[ly_back];
	    if ( cvs->layerheads[dm_fore] - sc->layers >= sc->layer_cnt )
		cvs->layerheads[dm_fore] = &sc->layers[ly_fore];
	}
	if ( !any_quads ) {
	    free(sc->ttf_instrs); sc->ttf_instrs = NULL;
	    sc->ttf_instrs_len = 0;
	}
    }

    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->next ) {
	if ( fvs->active_layer>=l ) {
	    --fvs->active_layer;
	    if ( fvs->active_layer+1==l )
		FontViewLayerChanged(fvs);
	}
    }
    MVDestroyAll(sf);

    free(sf->layers[l].name);
    for ( i=l+1; i<sf->layer_cnt; ++i )
	sf->layers[i-1] = sf->layers[i];
    -- sf->layer_cnt;
}

void SFAddLayer(SplineFont *sf,char *name,int order2,int background) {
    int gid, l;
    SplineChar *sc;
    CharViewBase *cvs;

    if ( sf->layer_cnt>=BACK_LAYER_MAX-1 ) {
	ff_post_error(_("Too many layers"),_("Attempt to have a font with more than %d layers"),
		BACK_LAYER_MAX );
return;
    }
    if ( name==NULL || *name=='\0' )
	name = _("Back");
    
    l = sf->layer_cnt;
    ++sf->layer_cnt;
    sf->layers = grealloc(sf->layers,(l+1)*sizeof(LayerInfo));
    memset(&sf->layers[l],0,sizeof(LayerInfo));
    sf->layers[l].name = copy(name);
    sf->layers[l].order2 = order2;
    sf->layers[l].background = background;

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	Layer *old = sc->layers;
	sc->layers = grealloc(sc->layers,(l+1)*sizeof(Layer));
	memset(&sc->layers[l],0,sizeof(Layer));
	LayerDefault(&sc->layers[l]);
	sc->layers[l].order2 = order2;
	sc->layers[l].background = background;
	++ sc->layer_cnt;
	for ( cvs = sc->views; cvs!=NULL; cvs=cvs->next ) {
	    cvs->layerheads[dm_back] = sc->layers + (cvs->layerheads[dm_back]-old);
	    cvs->layerheads[dm_fore] = sc->layers + (cvs->layerheads[dm_fore]-old);
	}
    }
}

void SFLayerSetBackground(SplineFont *sf,int layer,int is_back) {
    int k,gid;
    SplineFont *_sf;
    SplineChar *sc;

    sf->layers[layer].background = is_back;
    k=0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
	    sc->layers[layer].background = is_back;
	    if ( !is_back && sc->layers[layer].images!=NULL ) {
		ImageListsFree(sc->layers[layer].images);
		sc->layers[layer].images = NULL;
		SCCharChangedUpdate(sc,layer);
	    }
	}
	++k;
    } while ( k<sf->subfontcnt );
}
