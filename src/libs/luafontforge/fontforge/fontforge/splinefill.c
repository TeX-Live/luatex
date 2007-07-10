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
#include "pfaedit.h"
#include <stdio.h>
#include <string.h>
#include <ustring.h>
#include <math.h>
#include "gdraw.h"
#include "splinefont.h"
#include "edgelist.h"

uint32 default_background = 0xffffff;		/* white */

static void HintsFree(Hints *h) {
    Hints *hnext;
    for ( ; h!=NULL; h = hnext ) {
	hnext = h->next;
	free(h);
    }
}

static void _FreeEdgeList(EdgeList *es) {
    int i;

    /* edges will be NULL if the user tries to make an enormous bitmap */
    /*  if the linear size is bigger than several thousand, we just */
    /*  ignore the request */
    if ( es->edges!=NULL ) {
	for ( i=0; i<es->cnt; ++i ) {
	    Edge *e, *next;
	    for ( e = es->edges[i]; e!=NULL; e = next ) {
		next = e->esnext;
		free(e);
	    }
	    es->edges[i] = NULL;
	}
    }
}

void FreeEdges(EdgeList *es) {
    _FreeEdgeList(es);
    free(es->edges);
    free(es->interesting);
    HintsFree(es->hhints);
    HintsFree(es->vhints);
}

extended IterateSplineSolve(const Spline1D *sp, extended tmin, extended tmax,
	extended sought,double err) {
    extended t, low, high, test;
    Spline1D temp;
    int cnt;

    /* Now the closed form CubicSolver can have rounding errors so if we know */
    /*  the spline to be monotonic, an iterative approach is more accurate */

    temp = *sp;
    temp.d -= sought;

    if ( temp.a==0 && temp.b==0 && temp.c!=0 ) {
	t = -temp.d/(extended) temp.c;
	if ( t<0 || t>1 )
return( -1 );
return( t );
    }

    low = ((temp.a*tmin+temp.b)*tmin+temp.c)*tmin+temp.d;
    high = ((temp.a*tmax+temp.b)*tmax+temp.c)*tmax+temp.d;
    if ( low<err && low>-err )
return(tmin);
    if ( high<err && high>-err )
return(tmax);
    if (( low<0 && high>0 ) ||
	    ( low>0 && high<0 )) {
	
	for ( cnt=0; cnt<1000; ++cnt ) {	/* Avoid impossible error limits */
	    t = (tmax+tmin)/2;
	    test = ((temp.a*t+temp.b)*t+temp.c)*t+temp.d;
	    if ( test>-err && test<err )
return( t );
	    if ( (low<0 && test<0) || (low>0 && test>0) )
		tmin=t;
	    else
		tmax = t;
	}
return( (tmax+tmin)/2 );	
    }
return( -1 );
}

double TOfNextMajor(Edge *e, EdgeList *es, double sought_m ) {
    /* We want to find t so that Mspline(t) = sought_m */
    /*  the curve is monotonic */
    Spline1D *msp = &e->spline->splines[es->major];
    double new_t;

    if ( es->is_overlap ) {

	/* if we've adjusted the height then we won't be able to find it restricting */
	/*  t between [0,1] as we do. So it's a special case. (this is to handle */
	/*  hstem hints) */
	if ( e->max_adjusted && sought_m==e->mmax ) {
	    e->m_cur = sought_m;
return( e->up?1.0:0.0 );
	}

	new_t = IterateSplineSolve(msp,e->t_mmin,e->t_mmax,(sought_m+es->mmin)/es->scale,.001);
	if ( new_t==-1 )
	    IError( "No Solution");
	e->m_cur = (((msp->a*new_t + msp->b)*new_t+msp->c)*new_t + msp->d)*es->scale - es->mmin;
return( new_t );
    } else {
	Spline *sp = e->spline;

	if ( sp->islinear ) {
	    new_t = e->t_cur + (sought_m-e->m_cur)/(es->scale * msp->c);
	    e->m_cur = (msp->c*new_t + msp->d)*es->scale - es->mmin;
return( new_t );
	}
	/* if we have a spline that is nearly horizontal at its max. endpoint */
	/*  then finding A value of t for which y has the right value isn't good */
	/*  enough (at least not when finding intersections) */
	if ( sought_m+1>e->mmax ) {
	    e->m_cur = e->mmax;
return( e->t_mmax );
	}

	/* if we've adjusted the height then we won't be able to find it restricting */
	/*  t between [0,1] as we do. So it's a special case. (this is to handle */
	/*  hstem hints) */
	if ( e->max_adjusted && sought_m==e->mmax ) {
	    e->m_cur = sought_m;
return( e->up?1.0:0.0 );
	}
	new_t = IterateSplineSolve(msp,e->t_mmin,e->t_mmax,(sought_m+es->mmin)/es->scale,.001);
	if ( new_t==-1 )
	    IError( "No Solution");
	e->m_cur = (((msp->a*new_t + msp->b)*new_t+msp->c)*new_t + msp->d)*es->scale - es->mmin;
return( new_t );
    }
}

static int SlopeLess(Edge *e, Edge *p, int other) {
    Spline1D *osp = &e->spline->splines[other];
    Spline1D *psp = &p->spline->splines[other];
    Spline1D *msp = &e->spline->splines[!other];
    Spline1D *qsp = &p->spline->splines[!other];
    real os = (3*osp->a*e->t_cur+2*osp->b)*e->t_cur+osp->c,
	   ps = (3*psp->a*p->t_cur+2*psp->b)*p->t_cur+psp->c;
    real ms = (3*msp->a*e->t_cur+2*msp->b)*e->t_cur+msp->c,
	   qs = (3*qsp->a*p->t_cur+2*qsp->b)*p->t_cur+qsp->c;
    if ( ms<.0001 && ms>-.0001 ) ms = 0;
    if ( qs<.0001 && qs>-.0001 ) qs = 0;
    if ( qs==0 ) {
	if ( p->t_cur==1 ) {
	   qs = (3*qsp->a*.9999+2*qsp->b)*.9999+qsp->c;
	   ps = (3*psp->a*.9999+2*psp->b)*.9999+psp->c;
       } else {
	   qs = (3*qsp->a*(p->t_cur+.0001)+2*qsp->b)*(p->t_cur+.0001)+qsp->c;
	   ps = (3*psp->a*(p->t_cur+.0001)+2*psp->b)*(p->t_cur+.0001)+psp->c;
       }
    }
    if ( ms==0 ) {
	if ( e->t_cur==1 ) {
	    ms = (3*msp->a*.9999+2*msp->b)*.9999+msp->c;
	    os = (3*osp->a*.9999+2*osp->b)*.9999+osp->c;
	} else {
	    ms = (3*msp->a*(e->t_cur+.0001)+2*msp->b)*(e->t_cur+.0001)+msp->c;
	    os = (3*osp->a*(e->t_cur+.0001)+2*osp->b)*(e->t_cur+.0001)+osp->c;
	}
    }
    if ( e->t_cur-e->tmin > e->tmax-e->t_cur ) { os = -os; ms = -ms; }
    if ( p->t_cur-p->tmin > p->tmax-p->t_cur ) { ps = -ps; qs = -qs; }
    if ( ms!=0 && qs!=0 ) { os /= ms; ps /= qs; }
    else if ( ms==0 && qs==0 ) /* Do Nothing */;
    else if ( (ms==0 && os>0) || (qs==0 && ps<0) )		/* Does this make sense? */
return( false );
    else if ( (ms==0 && os<0) || (qs==0 && ps>0) )		/* Does this make sense? */
return( true );

    if ( os==ps || ms==0 || qs==0 )
return( e->o_mmax<p->o_mmax );

return( os<ps );
}

static void AddEdge(EdgeList *es, Spline *sp, real tmin, real tmax ) {
    Edge *e, *pr;
    real m1, m2;
    int mpos;
    Hints *hint;
    Spline1D *msp = &sp->splines[es->major], *osp = &sp->splines[es->other];

    e = gcalloc(1,sizeof(Edge));
    e->spline = sp;

    m1 = ( ((msp->a*tmin+msp->b)*tmin+msp->c)*tmin + msp->d ) * es->scale;
    m2 = ( ((msp->a*tmax+msp->b)*tmax+msp->c)*tmax + msp->d ) * es->scale;
    if ( m1>m2 ) {
	e->mmin = m2;
	e->t_mmin = tmax;
	e->mmax = m1;
	e->t_mmax = tmin;
	e->up = false;
    } else {
	e->mmax = m2;
	e->t_mmax = tmax;
	e->mmin = m1;
	e->t_mmin = tmin;
	e->up = true;
    }
    if ( RealNear(e->mmin,es->mmin)) e->mmin = es->mmin;
    e->o_mmin = ( ((osp->a*e->t_mmin+osp->b)*e->t_mmin+osp->c)*e->t_mmin + osp->d ) * es->scale;
    e->o_mmax = ( ((osp->a*e->t_mmax+osp->b)*e->t_mmax+osp->c)*e->t_mmax + osp->d ) * es->scale;
    e->mmin -= es->mmin; e->mmax -= es->mmin;
    e->t_cur = e->t_mmin;
    e->o_cur = e->o_mmin;
    e->m_cur = e->mmin;
    e->last_opos = e->last_mpos = -2;
    e->tmin = tmin; e->tmax = tmax;

    if ( e->mmin<0 || e->mmin>=e->mmax ) {
	/*IError("Probably not serious, but we've got a zero length spline in AddEdge in %s",es->sc==NULL?<nameless>:es->sc->name);*/
	free(e);
return;
    }

    if ( es->sc!=NULL ) for ( hint=es->hhints; hint!=NULL; hint=hint->next ) {
	if ( hint->adjustb ) {
	    if ( e->m_cur>hint->b1 && e->m_cur<hint->b2 ) {
		e->m_cur = e->mmin = hint->ab;
		e->min_adjusted = true;
	    } else if ( e->mmax>hint->b1 && e->mmax<hint->b2 ) {
		e->mmax = hint->ab;
		e->max_adjusted = true;
	    }
	} else if ( hint->adjuste ) {
	    if ( e->m_cur>hint->e1 && e->m_cur<hint->e2 ) {
		e->m_cur = e->mmin = hint->ae;
		e->min_adjusted = true;
	    } else if ( e->mmax>hint->e1 && e->mmax<hint->e2 ) {
		e->mmax = hint->ae;
		e->max_adjusted = true;
	    }
	}
    }

    mpos = (int) ceil(e->m_cur);
    if ( mpos>e->mmax || mpos>=es->cnt ) {
	free(e);
return;
    }

    if ( e->m_cur!=ceil(e->m_cur) ) {
	/* bring the new edge up to its first scan line */
	e->t_cur = TOfNextMajor(e,es,ceil(e->m_cur));
	e->o_cur = ( ((osp->a*e->t_cur+osp->b)*e->t_cur+osp->c)*e->t_cur + osp->d ) * es->scale;
    }

    e->before = es->last;
    if ( es->last!=NULL )
	es->last->after = e;
    if ( es->last==NULL )
	es->splinesetfirst = e;
    es->last = e;

    if ( es->edges[mpos]==NULL || e->o_cur<es->edges[mpos]->o_cur ||
	    (e->o_cur==es->edges[mpos]->o_cur && SlopeLess(e,es->edges[mpos],es->other))) {
	e->esnext = es->edges[mpos];
	es->edges[mpos] = e;
    } else {
	for ( pr=es->edges[mpos]; pr->esnext!=NULL && pr->esnext->o_cur<e->o_cur ;
		pr = pr->esnext );
	/* When two splines share a vertex which is a local minimum, then */
	/*  o_cur will be equal for both (to the vertex's o value) and so */
	/*  the above code randomly picked one to go first. That screws up */
	/*  the overlap code, which wants them properly ordered from the */
	/*  start. so look at the end point, nope the end point isn't always */
	/*  meaningful, look at the slope... */
	if ( pr->esnext!=NULL && pr->esnext->o_cur==e->o_cur &&
		SlopeLess(e,pr->esnext,es->other)) {
	    pr = pr->esnext;
	}
	e->esnext = pr->esnext;
	pr->esnext = e;
    }
    if ( es->interesting ) {
	/* Mark the other end of the spline as interesting */
	es->interesting[(int) ceil(e->mmax)]=1;
    }
}

static void AddMajorEdge(EdgeList *es, Spline *sp) {
    Edge *e, *pr;
    real m1;
    Spline1D *msp = &sp->splines[es->major], *osp = &sp->splines[es->other];

    e = gcalloc(1,sizeof(Edge));
    e->spline = sp;

    e->mmin = e->mmax = m1 = msp->d * es->scale - es->mmin;
    e->t_mmin = 0;
    e->t_mmax = 1;
    e->up = false;
    e->o_mmin = osp->d * es->scale;
    e->o_mmax = ( osp->a + osp->b + osp->c + osp->d ) * es->scale;
    if ( e->o_mmin == e->o_mmax ) {	/* Just a point? */
	free(e);
return;
    }
    if ( e->mmin<0 )
	IError("Grg!");

    if ( ceil(e->m_cur)>e->mmax ) {
	free(e);
return;
    }

    if ( es->majors==NULL || es->majors->mmin>=m1 ) {
	e->esnext = es->majors;
	es->majors = e;
    } else {
	for ( pr=es->majors; pr->esnext!=NULL && pr->esnext->mmin<m1; pr = pr->esnext );
	e->esnext = pr->esnext;
	pr->esnext = e;
    }
}

static void AddSpline(EdgeList *es, Spline *sp ) {
    real t1=2, t2=2, t;
    real b2_fourac;
    real fm, tm;
    Spline1D *msp = &sp->splines[es->major], *osp = &sp->splines[es->other];

    /* Find the points of extrema on the curve discribing y behavior */
    if ( !RealNear(msp->a,0) ) {
	/* cubic, possibly 2 extrema (possibly none) */
	b2_fourac = 4*msp->b*msp->b - 12*msp->a*msp->c;
	if ( b2_fourac>=0 ) {
	    b2_fourac = sqrt(b2_fourac);
	    t1 = CheckExtremaForSingleBitErrors(msp,(-2*msp->b - b2_fourac) / (6*msp->a));
	    t2 = CheckExtremaForSingleBitErrors(msp,(-2*msp->b + b2_fourac) / (6*msp->a));
	    if ( t1>t2 ) { real temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = 2.0;

	    /* check for curves which have such a small slope they might */
	    /*  as well be horizontal */
	    fm = es->major==1?sp->from->me.y:sp->from->me.x;
	    tm = es->major==1?sp->to->me.y:sp->to->me.x;
	    if ( fm==tm ) {
		real m1, m2, d1, d2;
		m1 = m2 = fm;
		if ( t1>0 && t1<1 )
		    m1 = ((msp->a*t1+msp->b)*t1+msp->c)*t1 + msp->d;
		if ( t2>0 && t2<1 )
		    m2 = ((msp->a*t2+msp->b)*t2+msp->c)*t2 + msp->d;
		d1 = (m1-fm)*es->scale;
		d2 = (m2-fm)*es->scale;
		if ( d1>-.5 && d1<.5 && d2>-.5 && d2<.5 ) {
		    sp->ishorvert = true;
		    if ( es->genmajoredges )
			AddMajorEdge(es,sp);
return;		/* Pretend it's horizontal, ignore it */
		}
	    }
	}
    } else if ( !RealNear(msp->b,0) ) {
	/* Quadratic, at most one extremum */
	t1 = -msp->c/(2.0*msp->b);
    } else if ( !RealNear(msp->c,0) ) {
	/* linear, no points of extrema */
    } else {
	sp->ishorvert = true;
	if ( es->genmajoredges )
	    AddMajorEdge(es,sp);
return;		/* Horizontal line, ignore it */
    }

    if ( RealNear(t1,0)) t1=0;
    if ( RealNear(t1,1)) t1=1;
    if ( RealNear(t2,0)) t2=0;
    if ( RealNear(t2,1)) t2=1;
    if ( RealNear(t1,t2)) t2=2;
    t=0;
    if ( t1>0 && t1<1 ) {
	AddEdge(es,sp,0,t1);
	t = t1;
    }
    if ( t2>0 && t2<1 ) {
	AddEdge(es,sp,t,t2);
	t = t2;
    }
    AddEdge(es,sp,t,1.0);
    if ( es->interesting ) {
	/* Also store up points of extrema in X as interesting (we got the endpoints, just internals now)*/
	extended ot1, ot2;
	int mpos;
	SplineFindExtrema(osp,&ot1,&ot2);
	if ( ot1>0 && ot1<1 ) {
	    mpos = (int) ceil( ( ((msp->a*ot1+msp->b)*ot1+msp->c)*ot1+msp->d )*es->scale-es->mmin );
	    es->interesting[mpos] = 1;
	}
	if ( ot2>0 && ot2<1 ) {
	    mpos = (int) ceil( ( ((msp->a*ot2+msp->b)*ot2+msp->c)*ot2+msp->d )*es->scale-es->mmin );
	    es->interesting[mpos] = 1;
	}
    }
}

void FindEdgesSplineSet(SplinePointList *spl, EdgeList *es) {
    Spline *spline, *first;

    for ( ; spl!=NULL; spl = spl->next ) if ( spl->first->prev!=NULL && spl->first->prev->from!=spl->first ) {
	first = NULL;
	es->last = es->splinesetfirst = NULL;
	/* Set so there is no previous point!!! */
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    AddSpline(es,spline);
	    if ( first==NULL ) first = spline;
	}
	if ( es->last!=NULL ) {
	    es->splinesetfirst->before = es->last;
	    es->last->after = es->splinesetfirst;
	}
    }
}


#ifndef LUA_FF_LIB

static void FindEdges(SplineChar *sc, EdgeList *es) {
    RefChar *rf;

    for ( rf=sc->layers[ly_fore].refs; rf!=NULL; rf = rf->next )
	FindEdgesSplineSet(rf->layers[0].splines,es);

    FindEdgesSplineSet(sc->layers[ly_fore].splines,es);
}
#endif

Edge *ActiveEdgesInsertNew(EdgeList *es, Edge *active,int i) {
    Edge *apt, *pr, *npt;

    for ( pr=NULL, apt=active, npt=es->edges[(int) i]; apt!=NULL && npt!=NULL; ) {
	if ( npt->o_cur<apt->o_cur ) {
	    npt->aenext = apt;
	    if ( pr==NULL )
		active = npt;
	    else
		pr->aenext = npt;
	    pr = npt;
	    npt = npt->esnext;
	} else {
	    pr = apt;
	    apt = apt->aenext;
	}
    }
    while ( npt!=NULL ) {
	npt->aenext = NULL;
	if ( pr==NULL )
	    active = npt;
	else
	    pr->aenext = npt;
	pr = npt;
	npt = npt->esnext;
    }
return( active );
}

Edge *ActiveEdgesRefigure(EdgeList *es, Edge *active,real i) {
    Edge *apt, *pr;
    int any;

    /* first remove any entry which doesn't intersect the new scan line */
    /*  (ie. stopped on last line) */
    for ( pr=NULL, apt=active; apt!=NULL; apt = apt->aenext ) {
	if ( apt->mmax<i ) {
	    if ( pr==NULL )
		active = apt->aenext;
	    else
		pr->aenext = apt->aenext;
	} else
	    pr = apt;
    }
    /* then move the active list to the next line */
    for ( apt=active; apt!=NULL; apt = apt->aenext ) {
	Spline1D *osp = &apt->spline->splines[es->other];
	apt->t_cur = TOfNextMajor(apt,es,i);
	apt->o_cur = ( ((osp->a*apt->t_cur+osp->b)*apt->t_cur+osp->c)*apt->t_cur + osp->d ) * es->scale;
    }
    /* reorder list */
    if ( active!=NULL ) {
	any = true;
	while ( any ) {
	    any = false;
	    for ( pr=NULL, apt=active; apt->aenext!=NULL; ) {
		if ( apt->o_cur <= apt->aenext->o_cur ) {
		    /* still ordered */;
		    pr = apt;
		    apt = apt->aenext;
		} else if ( pr==NULL ) {
		    active = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    active->aenext = apt;
		    /* don't need to set any, since this reorder can't disorder the list */
		    pr = active;
		} else {
		    pr->aenext = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    pr->aenext->aenext = apt;
		    any = true;
		    pr = pr->aenext;
		}
	    }
	}
    }
    /* Insert new nodes */
    active = ActiveEdgesInsertNew(es,active,i);
return( active );
}

#ifndef LUA_FF_LIB
Edge *ActiveEdgesFindStem(Edge *apt, Edge **prev, real i) {
    int cnt=apt->up?1:-1;
    Edge *pr, *e;

    for ( pr=apt, e=apt->aenext; e!=NULL && cnt!=0; pr=e, e=e->aenext ) {
	if ( pr->up!=e->up )
	    cnt += (e->up?1:-1);
	else if ( (pr->before==e || pr->after==e ) &&
		(( pr->mmax==i && e->mmin==i ) ||
		 ( pr->mmin==i && e->mmax==i )) )
	    /* This just continues the line and doesn't change count */;
	else
	    cnt += (e->up?1:-1);
    }
    /* color a horizontal line that comes out of the last vertex */
    if ( e!=NULL && (e->before==pr || e->after==pr) &&
		(( pr->mmax==i && e->mmin==i ) ||
		 ( pr->mmin==i && e->mmax==i )) ) {
	pr = e;
	e = e->aenext;
    } else if ( e!=NULL && ((pr->up && !e->up) || (!pr->up && e->up)) &&
		    pr->spline!=e->spline &&
		    ((pr->after==e && pr->spline->to->next!=NULL &&
				pr->spline->to->next!=e->spline &&
				pr->spline->to->next->to->next==e->spline ) ||
			    (pr->before==e && pr->spline->from->prev!=NULL &&
				pr->spline->from->prev!=e->spline &&
				pr->spline->from->prev->from->prev!=e->spline )) &&
		    ((pr->mmax == i && e->mmax==i ) ||
		     (pr->mmin == i && e->mmin==i )) ) {
	pr = e;
    }
    *prev = pr;
return( e );
}

static int isvstem(EdgeList *es,real stem,int *vval) {
    Hints *hint;

    for ( hint=es->vhints; hint!=NULL ; hint=hint->next ) {
	if ( stem>=hint->b1 && stem<=hint->b2 ) {
	    *vval = hint->ab;
return( true );
	} else if ( stem>=hint->e1 && stem<=hint->e2 ) {
	    *vval = hint->ae;
return( true );
	}
    }
return( false );
}

static void FillChar(EdgeList *es) {
    Edge *active=NULL, *apt, *pr, *e, *prev;
    int i, k, end, width, oldk;
    uint8 *bpt;

    for ( i=0; i<es->cnt; ++i ) {
	active = ActiveEdgesRefigure(es,active,i);

	/* process scanline */
	bpt = es->bitmap+((es->cnt-1-i)*es->bytes_per_line);
	for ( apt=active; apt!=NULL; ) {
	    e = ActiveEdgesFindStem(apt,&prev,i);
	    pr = prev;
	    width = rint(pr->o_cur-apt->o_cur);
	    if ( width<=0 ) width=1;
	    k=rint(apt->o_cur-es->omin);
	    if ( k<0 ) k=0;
	    end =rint(pr->o_cur-es->omin);
	    if ( end-k > width-1 ) {
		int kval = -999999, eval= -999999;
		if ( isvstem(es,apt->o_cur,&kval) )
		    k = kval;
		if ( isvstem(es,pr->o_cur,&eval))
		    end = eval;
		if ( end-k > width-1 ) {
		    if ( k!=kval && (end==eval || (apt->o_cur-es->omin)-k > end-(pr->o_cur-es->omin) ))
			++k;
		    else if ( end!=eval )
			--end;
		}
	    }
	    oldk = k;
	    if ( apt->last_mpos==i-1 || pr->last_mpos==i-1 ) {
		int lx1=apt->last_opos, lx2=pr->last_opos;
		if ( apt->last_mpos!=i-1 ) lx1 = lx2;
		else if ( pr->last_mpos!=i-1 ) lx2 = lx1;
		if ( lx1>lx2 ) { int temp = lx1; lx1=lx2; lx2=temp; }
		if ( lx2<k-1 ) {
		    int mid = (lx2+k)/2;
		    for ( ; lx2<mid; ++lx2 )
			bpt[(lx2>>3)+es->bytes_per_line] |= (1<<(7-(lx2&7)));
		    for ( ; lx2<k; ++lx2 )
			bpt[(lx2>>3)] |= (1<<(7-(lx2&7)));
		} else if ( lx1>end+1 ) {
		    int mid = (lx1+end)/2;
		    for ( ; lx1<mid; --lx1 )
			bpt[(lx1>>3)+es->bytes_per_line] |= (1<<(7-(lx1&7)));
		    for ( ; lx1<end; --lx1 )
			bpt[(lx1>>3)] |= (1<<(7-(lx1&7)));
		}
	    }
	    for ( ; k<=end; ++k )
		bpt[k>>3] |= (1<<(7-(k&7)));
	    apt->last_mpos = pr->last_mpos = i;
	    apt->last_opos = oldk;
	    pr->last_opos = end;
	    apt = e;
	}
    }
}

static void InitializeHints(SplineChar *sc, EdgeList *es) {
    Hints *hint, *last;
    StemInfo *s;
    real t1, t2;
    int k,end,width;

    /* we only care about hstem hints, and only if they fail to cross a */
    /*  vertical pixel boundary. If that happens, adjust either the top */
    /*  or bottom position so that a boundary forcing is crossed.   Any */
    /*  vertexes at those points will be similarly adjusted later... */

    last = NULL; es->hhints = NULL;
    for ( s=sc->hstem; s!=NULL; s=s->next ) {
	hint = gcalloc(1,sizeof(Hints));
	hint->base = s->start; hint->width = s->width;
	if ( last==NULL ) es->hhints = hint;
	else last->next = hint;
	last = hint;
	hint->adjuste = hint->adjustb = false;
	t1 = hint->base*es->scale	-es->mmin;
	t2 = (hint->base+hint->width)*es->scale	-es->mmin;
	if ( floor(t1)==floor(t2) && t1!=floor(t1) && t2!=floor(t2) ) {
	    if ( t1<t2 ) {
		if ( t1-floor(t1) > ceil(t2)-t2 ) {
		    hint->adjuste = true;
		    hint->ae = ceil(t2);
		} else {
		    hint->adjustb = true;
		    hint->ab = floor(t1);
		}
	    } else {
		if ( t2-floor(t2) > ceil(t1)-t1 ) {
		    hint->adjustb = true;
		    hint->ab = ceil(t1);
		} else {
		    hint->adjuste = true;
		    hint->ae = floor(t2);
		}
	    }
	}
	hint->b1 = t1-.2; hint->b2 = t1 + .2;
	hint->e1 = t2-.2; hint->e2 = t2 + .2;
    }

    /* Nope. We care about vstems too now, but in a different case */
    last = NULL; es->vhints = NULL;
    for ( s=sc->vstem; s!=NULL; s=s->next ) {
	hint = gcalloc(1,sizeof(Hints));
	hint->base = s->start; hint->width = s->width;
	if ( last==NULL ) es->vhints = hint;
	else last->next = hint;
	last = hint;
	if ( hint->width<0 ) {
	    width = -rint(-hint->width*es->scale);
	    t1 = (hint->base+hint->width)*es->scale;
	    t2 = hint->base*es->scale;
	} else {
	    width = rint(hint->width*es->scale);
	    t1 = hint->base*es->scale;
	    t2 = (hint->base+hint->width)*es->scale;
	}
	k = rint(t1-es->omin); end = rint(t2-es->omin);
	if ( end-k > width-1 )
	    if ( end-k > width-1 ) {
		if ( (t1-es->omin)-k > end-(t2-es->omin) )
		    ++k;
		else
		    --end;
	    }
	hint->ab = k; hint->ae = end;
	hint->b1 = t1-.2; hint->b2 = t1 + .2;
	hint->e1 = t2-.2; hint->e2 = t2 + .2;
    }
}
#endif

/* After a bitmap has been compressed, it's sizes may not comply with the */
/*  expectations for saving images */
void BCRegularizeBitmap(BDFChar *bdfc) {
    int bpl =(bdfc->xmax-bdfc->xmin)/8+1;
    int i;

    if ( bdfc->bytes_per_line!=bpl ) {
	uint8 *bitmap = galloc(bpl*(bdfc->ymax-bdfc->ymin+1));
	for ( i=0; i<=(bdfc->ymax-bdfc->ymin); ++i )
	    memcpy(bitmap+i*bpl,bdfc->bitmap+i*bdfc->bytes_per_line,bpl);
	free(bdfc->bitmap);
	bdfc->bitmap= bitmap;
	bdfc->bytes_per_line = bpl;
    }
}

void BCRegularizeGreymap(BDFChar *bdfc) {
    int bpl = bdfc->xmax-bdfc->xmin+1;
    int i;

    if ( bdfc->bytes_per_line!=bpl ) {
	uint8 *bitmap = galloc(bpl*(bdfc->ymax-bdfc->ymin+1));
	for ( i=0; i<=(bdfc->ymax-bdfc->ymin); ++i )
	    memcpy(bitmap+i*bpl,bdfc->bitmap+i*bdfc->bytes_per_line,bpl);
	free(bdfc->bitmap);
	bdfc->bitmap= bitmap;
	bdfc->bytes_per_line = bpl;
    }
}

void BCCompressBitmap(BDFChar *bdfc) {
    /* Now we may have allocated a bit more than we need to the bitmap */
    /*  check to see if there are any unused rows or columns.... */
    int i,j,any, off, last;

    /* we can use the same code to lop off rows whether we deal with bytes or bits */
    for ( i=0; i<bdfc->ymax-bdfc->ymin; ++i ) {
	any = 0;
	for ( j=0; j<bdfc->bytes_per_line; ++j )
	    if ( bdfc->bitmap[i*bdfc->bytes_per_line+j]!=0 ) any = 1;
	if ( any )
    break;
    }
    if ( i!=0 ) {
	bdfc->ymax -= i;
	memmove(bdfc->bitmap,bdfc->bitmap+i*bdfc->bytes_per_line,(bdfc->ymax-bdfc->ymin+1)*bdfc->bytes_per_line );
    }

    for ( i=bdfc->ymax-bdfc->ymin; i>0; --i ) {
	any = 0;
	for ( j=0; j<bdfc->bytes_per_line; ++j )
	    if ( bdfc->bitmap[i*bdfc->bytes_per_line+j]!=0 ) any = 1;
	if ( any )
    break;
    }
    if ( i!=bdfc->ymax-bdfc->ymin ) {
	bdfc->ymin += bdfc->ymax-bdfc->ymin-i;
    }

    if ( !bdfc->byte_data ) {
	for ( j=0; j<bdfc->xmax-bdfc->xmin; ++j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) )
		    any = 1;
	    if ( any )
	break;
	}
	off = j;
	if ( off/8>0 ) {
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i ) {
		memmove(bdfc->bitmap+i*bdfc->bytes_per_line,
			bdfc->bitmap+i*bdfc->bytes_per_line+off/8,
			bdfc->bytes_per_line-off/8);
		memset(bdfc->bitmap+(i+1)*bdfc->bytes_per_line-off/8,
			0, off/8);
	    }
	    bdfc->xmin += off-off%8;
	    off %= 8;
	}
	if ( off!=0 ) {
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i ) {
		last = 0;
		for ( j=bdfc->bytes_per_line-1; j>=0; --j ) {
		    int index = i*bdfc->bytes_per_line+j;
		    int temp = bdfc->bitmap[index]>>(8-off);
		    bdfc->bitmap[index] = (bdfc->bitmap[index]<<off)|last;
		    last = temp;
		}
		if ( last!=0 )
		    IError("Sigh");
	    }
	    bdfc->xmin += off;
	}

	for ( j=bdfc->xmax-bdfc->xmin; j>0; --j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) )
		    any = 1;
	    if ( any )
	break;
	}
	if ( j!=bdfc->xmax+bdfc->xmin ) {
	    bdfc->xmax -= bdfc->xmax-bdfc->xmin-j;
	}
	BCRegularizeBitmap(bdfc);
    } else {
	for ( j=0; j<bdfc->xmax-bdfc->xmin; ++j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+j] != 0 )
		    any = 1;
	    if ( any )
	break;
	}
	off = j;
	if ( off!=0 ) {
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i ) {
		memmove(bdfc->bitmap+i*bdfc->bytes_per_line,
			bdfc->bitmap+i*bdfc->bytes_per_line+off,
			bdfc->bytes_per_line-off);
		memset(bdfc->bitmap+(i+1)*bdfc->bytes_per_line-off,
			0, off);
	    }
	    bdfc->xmin += off;
	}

	for ( j=bdfc->xmax-bdfc->xmin; j>0; --j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+j] != 0 )
		    any = 1;
	    if ( any )
	break;
	}
	if ( j!=bdfc->xmax+bdfc->xmin ) {
	    bdfc->xmax -= bdfc->xmax-bdfc->xmin-j;
	}
	BCRegularizeGreymap(bdfc);
    }
    if ( bdfc->xmax<bdfc->xmin || bdfc->ymax<bdfc->ymin ) {
	bdfc->ymax = bdfc->ymin-1;
	bdfc->xmax = bdfc->xmin-1;
    }
}

#ifndef LUA_FF_LIB
static void Bresenham(uint8 *bytemap,EdgeList *es,int x1,int x2,int y1,int y2,
	int grey) {
    int dx, dy, incr1, incr2, d, x, y;
    int incr3;
    int bytes_per_line = es->bytes_per_line<<3;
    int ymax = es->cnt;

    /* We are guarenteed x1<=x2 */
    dx = x2-x1;
    if ( (dy = y1-y2)<0 ) dy=-dy;
    if ( dx>=dy ) {
	d = 2 * dy - dx;
	incr1 = 2*dy;
	incr2 = 2*(dy-dx);
	incr3 = y2>y1 ? 1 : -1;
	x = x1;
	y = y1;
	if ( x>=0 && y>=0 && x<bytes_per_line && y<ymax )
	    bytemap[y*bytes_per_line+x] = grey;
	while ( x<x2 ) {
	    ++x;
	    if ( d<0 )
		d += incr1;
	    else {
		y += incr3;
		d += incr2;
	    }
	    if ( x>=0 && y>=0 && x<bytes_per_line && y<ymax )
		bytemap[y*bytes_per_line+x] = grey;
	}
    } else {
	if ( y1>y2 ) {
	    incr1 = y1; y1 = y2; y2 = incr1;
	    incr1 = x1; x1 = x2; x2 = incr1;
	}
	d = 2 * dx - dy;
	incr1 = 2*dx;
	incr2 = 2*(dx-dy);
	incr3 = x2>x1 ? 1 : -1;
	x = x1;
	y = y1;
	if ( x>=0 && y>=0 && x<bytes_per_line && y<ymax )
	    bytemap[y*bytes_per_line+x] = grey;
	while ( y<y2 ) {
	    ++y;
	    if ( d<0 )
		d += incr1;
	    else {
		x += incr3;
		d += incr2;
	    }
	    if ( x>=0 && y>=0 && x<bytes_per_line && y<ymax )
		bytemap[y*bytes_per_line+x] = grey;
	}
    }
}

static void StrokeLine(uint8 *bytemap,IPoint *from, IPoint *to,EdgeList *es,int grey,int width) {
    int x1, x2, y1, y2;
    int dx, dy;
    int i, w2;

    x1 = from->x - es->omin;
    x2 = to->x - es->omin;
    if ( (y1 = (es->cnt-1 - (from->y - es->mmin)))<0 ) y1=0;
    if ( (y2 = (es->cnt-1 - (to->y - es->mmin)))<0 ) y2=0;

    if ( x1>x2 ) {
	dx = x1; x1 = x2; x2 = dx;
	dy = y1; y1 = y2; y2 = dy;
    }

    if ( width>1 ) {
	w2 = width/2;
	dx = x2-x1;
	if ( (dy = y1-y2)<0 ) dy=-dy;
	if ( dy>2*dx ) {
	    x1 -= w2; x2 -= w2;
	    for ( i=0; i<width; ++i )
		Bresenham(bytemap,es,x1+i,x2+i,y1,y2,grey);
	} else if ( dx>2*dy ) {
	    y1 -= w2; y2 -= w2;
	    for ( i=0; i<width; ++i )
		Bresenham(bytemap,es,x1,x2,y1+i,y2+i,grey);
	} else if ( y2>y1 ) {
	    width *= 1.414; w2 = width/2;
	    x1-=w2/2; y1+=w2/2; x2-=w2/2; y2+=w2/2;
	    for ( i=0; 2*i<width; ++i ) {
		Bresenham(bytemap,es,x1+i,x2+i,y1-i,y2-i,grey);
		Bresenham(bytemap,es,x1+i+1,x2+i+1,y1-i,y2-i,grey);
	    }
	} else {
	    width *= 1.414; w2 = width/2;
	    x1-=w2/2; y1-=w2/2; x2-=w2/2; y2-=w2/2;
	    for ( i=0; 2*i<width; ++i ) {
		Bresenham(bytemap,es,x1+i,x2+i,y1+i,y2+i,grey);
		Bresenham(bytemap,es,x1+i+1,x2+i+1,y1+i,y2+i,grey);
	    }
	}
    } else
	Bresenham(bytemap,es,x1,x2,y1,y2,grey);
}

static void StrokeSS(uint8 *bytemap,EdgeList *es,int width,int grey,SplineSet *ss) {
    LinearApprox *lap;
    LineList *line, *prev;
    Spline *spline, *first;

    for ( ; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( spline = ss->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    lap = SplineApproximate(spline,es->scale);
	    if ( lap->lines!=NULL ) {
		for ( prev = lap->lines, line=prev->next; line!=NULL; prev = line, line=line->next )
		    StrokeLine(bytemap,&prev->here,&line->here,es,grey,width);
	    }
	    if ( first == NULL ) first = spline;
	}
    }
}

static void StrokeGlyph(uint8 *bytemap,EdgeList *es,real wid, SplineChar *sc) {
    RefChar *ref;
    int width = rint(wid*es->scale);

    StrokeSS(bytemap,es,width,0xff,sc->layers[ly_fore].splines);
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref = ref->next )
	StrokeSS(bytemap,es,width,0xff,ref->layers[0].splines);
}
#endif

#ifdef FONTFORGE_CONFIG_TYPE3
static void StrokePaths(uint8 *bytemap,EdgeList *es,Layer *layer,Layer *alt) {
    uint32 col;
    int width;
    int grey;

    if ( layer->stroke_pen.brush.col!=COLOR_INHERITED )
	col = layer->stroke_pen.brush.col;
    else if ( alt!=NULL && alt->stroke_pen.brush.col!=COLOR_INHERITED )
	col = alt->stroke_pen.brush.col;
    else
	col = 0x000000;
    if ( layer->stroke_pen.width!=WIDTH_INHERITED )
	width = rint(layer->stroke_pen.width*layer->stroke_pen.trans[0]*es->scale);
    else if ( alt!=NULL && alt->stroke_pen.width!=WIDTH_INHERITED )
	width = rint(alt->stroke_pen.width*alt->stroke_pen.trans[0]*es->scale);
    else
	width = 1;
    grey = ( ((col>>16)&0xff)*3 + ((col>>8)&0xff)*6 + (col&0xff) )/ 10;
    /* but our internal greymap convention is backwards */
    grey = 255-grey;

    StrokeSS(bytemap,es,width,grey,layer->splines);
}

static void SetByteMapToGrey(uint8 *bytemap,EdgeList *es,Layer *layer,Layer *alt) {
    uint32 col;
    int grey,i,j;
    uint8 *pt, *bpt;

    if ( layer->fill_brush.col!=COLOR_INHERITED )
	col = layer->fill_brush.col;
    else if ( alt!=NULL && alt->fill_brush.col!=COLOR_INHERITED )
	col = alt->fill_brush.col;
    else
	col = 0x000000;
    grey = ( ((col>>16)&0xff)*3 + ((col>>8)&0xff)*6 + (col&0xff) )/ 10;
    /* but our internal greymap convention is backwards */
    grey = 255-grey;

    for ( i=0; i<es->cnt; ++i ) {
	bpt = es->bitmap + i*es->bytes_per_line;
	pt = bytemap + i*8*es->bytes_per_line;
	for ( j=0; j<8*es->bytes_per_line; ++j ) {
	    if ( bpt[j>>3]&(0x80>>(j&7)) )
		pt[j] = grey;
	}
    }
}

#ifdef FONTFORGE_CONFIG_GTK
static void FillImages(uint8 *bytemap,EdgeList *es,ImageList *img,Layer *layer,Layer *alt) {
}
#else
static void FillImages(uint8 *bytemap,EdgeList *es,ImageList *img,Layer *layer,Layer *alt) {
    uint32 fillcol, col;
    int grey,i,j,x1,x2,y1,y2,jj,ii;

    if ( layer->fill_brush.col!=COLOR_INHERITED )
	fillcol = layer->fill_brush.col;
    else if ( alt!=NULL && alt->fill_brush.col!=COLOR_INHERITED )
	fillcol = alt->fill_brush.col;
    else
	fillcol = 0x000000;

    while ( img!=NULL ) {
	struct _GImage *base = img->image->list_len==0?
		img->image->u.image:img->image->u.images[0];

	y1 = es->cnt-1 - (img->yoff*es->scale - es->mmin);
	y2 = es->cnt-1 - ((img->yoff-base->height*img->yscale)*es->scale -es->mmin);
	x1 = img->xoff*es->scale - es->omin;
	x2 = (img->xoff+base->width*img->xscale)*es->scale - es->omin;
	if ( y1==y2 || x1==x2 )	/* too small to show */
    continue;
	for ( i=0; i<=y2-y1; ++i ) {
	    if ( i+y1<0 || i+y1>=es->cnt )	/* Shouldn't happen, rounding errors might gives us off by 1s though */
	continue;
	    ii = i*(base->height-1)/(y2-y1);
	    for ( j=0; j<x2-x1; ++j ) {
		if ( j+x1<0 || j+x1>=8*es->bytes_per_line )
	    continue;
		jj = j*(base->width-1)/(x2-x1);
		if ( base->image_type==it_true )
		    col = ((uint32 *) (base->data + ii*base->bytes_per_line))[jj];
		else if ( base->image_type==it_index ) {
		    col = (base->data + ii*base->bytes_per_line)[jj];
		    col = base->clut->clut[col];
		} else if ( layer->dofill && base->trans!=-1) {		/* Equivalent to imagemask */
		    if ( (base->trans==0 && !( (base->data + ii*base->bytes_per_line)[jj>>3]&(0x80>>(jj&7)) ) ) ||
			    (base->trans!=0 && ( (base->data + ii*base->bytes_per_line)[jj>>3]&(0x80>>(jj&7)) ) ))
	    continue;	/* transparent */
		    col = fillcol;
		} else {
		    int index = 0;
		    if ( (base->data + ii*base->bytes_per_line)[jj>>3]&(0x80>>(j&7)) )
			index = 1;
		    if ( base->clut!=NULL ) {
			col = base->clut->clut[index];
			if ( col==0xb0b0b0 ) col = 0xffffff;
		    } else if ( index==1 )
			col = 0xffffff;
		    else
			col = 0x000000;
		}
		grey = ( ((col>>16)&0xff)*3 + ((col>>8)&0xff)*6 + (col&0xff) )/ 10;
		/* but our internal greymap convention is backwards */
		grey = 255-grey;
		bytemap[(i+y1)*8*es->bytes_per_line + j+x1] = grey;
	    }
	}
	img = img->next;
    }
}
#endif

static void ProcessLayer(uint8 *bytemap,EdgeList *es,Layer *layer,
	Layer *alt) {
    ImageList *img;

    if ( !layer->fillfirst && layer->dostroke )
	StrokePaths(bytemap,es,layer,alt);
    if ( layer->dofill ) {
	memset(es->bitmap,0,es->cnt*es->bytes_per_line);
	FindEdgesSplineSet(layer->splines,es);
	FillChar(es);
	SetByteMapToGrey(bytemap,es,layer,alt);
	_FreeEdgeList(es);
    }
    for ( img = layer->images; img!=NULL; img=img->next )
	FillImages(bytemap,es,img,layer,alt);
    if ( layer->fillfirst && layer->dostroke )
	StrokePaths(bytemap,es,layer,alt);
}
#endif

#ifndef LUA_FF_LIB

static void FlattenBytemap(EdgeList *es,uint8 *bytemap) {
    int i,j;
    uint8 *bpt, *pt;

    memset(es->bitmap,0,es->cnt*es->bytes_per_line);
    for ( i=0; i<es->cnt; ++i ) {
	bpt = es->bitmap + i*es->bytes_per_line;
	pt = bytemap + i*8*es->bytes_per_line;
	for ( j=0; j<8*es->bytes_per_line; ++j )
	    if ( pt[j]>=128 )
		bpt[j>>3] |= (0x80>>(j&7));
    }
}

static int FigureBitmap(EdgeList *es,uint8 *bytemap, int is_aa) {
    if ( is_aa ) {
	free(es->bitmap);
	es->bitmap = bytemap;
return( 8 );
    } else {
	FlattenBytemap(es,bytemap);
	free(bytemap);
return( 0 );
    }
}


/* Yes, I really do want a double, even though it will almost always be an */
/*  integer value there are a few cases (fill pattern for charview) where */
/*  I need more precision and the pixelsize itself is largely irrelevant */
/*  (I care about the scale though) */
static BDFChar *_SplineCharRasterize(SplineChar *sc, double pixelsize, int is_aa) {
    EdgeList es;
    DBounds b;
    BDFChar *bdfc;
    int depth = 0;

    if ( sc==NULL )
return( NULL );
    memset(&es,'\0',sizeof(es));
    if ( sc==NULL ) {
	es.mmin = es.mmax = es.omin = es.omax = 0;
	es.bitmap = gcalloc(1,1);
	es.bytes_per_line = 1;
	is_aa = false;
    } else {
	SplineCharFindBounds(sc,&b);
	es.scale = (pixelsize-.1) / (real) (sc->parent->ascent+sc->parent->descent);
	es.mmin = floor(b.miny*es.scale);
	es.mmax = ceil(b.maxy*es.scale);
	es.omin = b.minx*es.scale;
	es.omax = b.maxx*es.scale;
	es.cnt = (int) (es.mmax-es.mmin) + 1;
	if ( es.cnt<4000 && es.omax-es.omin<4000 && es.cnt>1 ) {
	    es.edges = gcalloc(es.cnt,sizeof(Edge *));
	    es.sc = sc;
	    es.major = 1; es.other = 0;
	    es.bytes_per_line = ((int) ceil(es.omax-es.omin) + 8)/8;
	    es.bitmap = gcalloc(es.cnt*es.bytes_per_line,1);

	    InitializeHints(sc,&es);
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( sc->parent->multilayer ) {
		uint8 *bytemap = gcalloc(es.cnt*es.bytes_per_line*8,1);
		int layer, i;
		RefChar *rf;
		for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
		    ProcessLayer(bytemap,&es,&sc->layers[layer],NULL);

		    for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next ) {
			for ( i=0; i<rf->layer_cnt; ++i ) {
			    ProcessLayer(bytemap,&es,(Layer *) (&rf->layers[i]),
				    &sc->layers[layer]);
			}
		    }
		}
		depth = FigureBitmap(&es,bytemap,is_aa);
	    } else
#endif
	    if ( sc->parent->strokedfont ) {
		uint8 *bytemap = gcalloc(es.cnt*es.bytes_per_line*8,1);
		StrokeGlyph(bytemap,&es,sc->parent->strokewidth,sc);
		depth = FigureBitmap(&es,bytemap,is_aa);
	    } else {
		FindEdges(sc,&es);
		FillChar(&es);
		depth = 0;
	    }
	} else {
	    /* If they want a bitmap so enormous it threatens our memory */
	    /*  then just give 'em a blank. It's probably by mistake anyway */
	    es.mmin = es.mmax = es.omin = es.omax = 0;
	    es.bitmap = gcalloc(1,1);
	    es.bytes_per_line = 1;
	}
    }

    bdfc = chunkalloc(sizeof(BDFChar));
    bdfc->sc = sc;
    bdfc->xmin = rint(es.omin);
    bdfc->ymin = es.mmin;
    bdfc->xmax = (int) ceil(es.omax-es.omin) + bdfc->xmin;
    bdfc->ymax = es.mmax;
    if ( sc!=NULL ) {
	bdfc->width = rint(sc->width*pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	bdfc->vwidth = rint(sc->vwidth*pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	bdfc->orig_pos = sc->orig_pos;
    }
    bdfc->bitmap = es.bitmap;
    bdfc->depth = depth;
    bdfc->bytes_per_line = es.bytes_per_line;
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( depth==8 ) {
	bdfc->byte_data = true;
	bdfc->bytes_per_line *= 8;
    }
#endif
    BCCompressBitmap(bdfc);
    FreeEdges(&es);
return( bdfc );
}

BDFChar *SplineCharRasterize(SplineChar *sc, double pixelsize) {
return( _SplineCharRasterize(sc,pixelsize,false));
}

BDFFont *SplineFontToBDFHeader(SplineFont *_sf, int pixelsize, int indicate) {
    BDFFont *bdf = gcalloc(1,sizeof(BDFFont));
    int i;
    real scale;
    char size[40];
    char aa[200];
    int max;
    SplineFont *sf;	/* The complexity here is to pick the appropriate subfont of a CID font */

    sf = _sf;
    max = sf->glyphcnt;
    for ( i=0; i<_sf->subfontcnt; ++i ) {
	sf = _sf->subfonts[i];
	if ( sf->glyphcnt>max ) max = sf->glyphcnt;
    }
    scale = pixelsize / (real) (sf->ascent+sf->descent);

    if ( indicate ) {
	sprintf(size,_("%d pixels"), pixelsize );
	strcpy(aa,_("Generating bitmap font"));
	if ( sf->fontname!=NULL ) {
	    strcat(aa,": ");
	    strncat(aa,sf->fontname,sizeof(aa)-strlen(aa));
	    aa[sizeof(aa)-1] = '\0';
	}
	gwwv_progress_start_indicator(10,_("Rasterizing..."),
		aa,size,sf->glyphcnt,1);
	gwwv_progress_enable_stop(0);
    }
    bdf->sf = _sf;
    bdf->glyphcnt = bdf->glyphmax = max;
    bdf->pixelsize = pixelsize;
    bdf->glyphs = galloc(max*sizeof(BDFChar *));
    bdf->ascent = rint(sf->ascent*scale);
    bdf->descent = pixelsize-bdf->ascent;
    bdf->res = -1;
return( bdf );
}
#endif

#ifndef LUA_FF_LIB
#if 0
/* This code was an attempt to do better at rasterizing by making a big bitmap*/
/*  and shrinking it down. It did make curved edges look better, but it made */
/*  straight edges worse. So I don't think it's worth it. */
static int countsquare(BDFChar *bc, int i, int j, int linear_scale) {
    int ii,jj,jjj, cnt;
    uint8 *bpt;

    cnt = 0;
    for ( ii=0; ii<linear_scale; ++ii ) {
	if ( i*linear_scale+ii>bc->ymax-bc->ymin )
    break;
	bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	for ( jj=0; jj<linear_scale; ++jj ) {
	    jjj = j*linear_scale+jj;
	    if ( jjj>bc->xmax-bc->xmin )
	break;
	    if ( bpt[jjj>>3]& (1<<(7-(jjj&7))) )
		++cnt;
	}
    }
return( cnt );
}

static int makesleftedge(BDFChar *bc, int i, int j, int linear_scale) {
    int ii,jjj;
    uint8 *bpt;

    for ( ii=0; ii<linear_scale; ++ii ) {
	if ( i*linear_scale+ii>bc->ymax-bc->ymin )
return( false );
	bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	jjj = j*linear_scale;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
return( false );
    }
return( true );
}

static int makesbottomedge(BDFChar *bc, int i, int j, int linear_scale) {
    int jj,jjj;
    uint8 *bpt;

    for ( jj=0; jj<linear_scale; ++jj ) {
	jjj = j*linear_scale+jj;
	if ( jjj>bc->xmax-bc->xmin )
return( false );
	bpt = bc->bitmap + (i*linear_scale)*bc->bytes_per_line;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
return( false );
    }
return( true );
}

static int makesline(BDFChar *bc, int i, int j, int linear_scale) {
    int ii,jj,jjj;
    int across, any, alltop, allbottom;	/* alltop is sometimes allleft, and allbottom allright */
    uint8 *bpt;
    /* If we have a line that goes all the way across a square then we want */
    /*  to turn on the pixel that corresponds to that square. Exception: */
    /*  if that line is on an edge of the square, and the square adjacent to */
    /*  it has more pixels, then let the adjacent square get the pixel */
    /* if two squares are essentially equal, choose the top one */

    across = alltop = allbottom = true;
    for ( ii=0; ii<linear_scale; ++ii ) {
	if ( i*linear_scale+ii>bc->ymax-bc->ymin ) {
	    across = alltop = allbottom = false;
    break;
	}
	bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	jjj = j*linear_scale;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
	    allbottom = 0;
	jjj += linear_scale-1;
	if ( jjj>bc->xmax-bc->xmin )
	    alltop = false;
	else if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
	    alltop = 0;
	any = false;
	for ( jj=0; jj<linear_scale; ++jj ) {
	    jjj = j*linear_scale+jj;
	    if ( jjj>bc->xmax-bc->xmin )
	break;
	    if ( bpt[jjj>>3]& (1<<(7-(jjj&7))) )
		any = true;
	}
	if ( !any )
	    across = false;
    }
    if ( across ) {
	if ( (!alltop && !allbottom ) || (alltop && allbottom))
return( true );
	if ( allbottom ) {
	    if ( j==0 )
return( true );
	    if ( countsquare(bc,i,j-1,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
return( true );
	}
	if ( alltop ) {
	    if ( j==(bc->xmax-bc->xmin)/linear_scale )
return( true );
	    if ( countsquare(bc,i,j+1,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
	    if ( makesleftedge(bc,i,j+1,linear_scale))
return( false );
return( true );
	}
    }

    /* now the other dimension */
    across = alltop = allbottom = true;
    for ( jj=0; jj<linear_scale; ++jj ) {
	jjj = j*linear_scale+jj;
	if ( jjj>bc->xmax-bc->xmin ) {
	    across = alltop = allbottom = false;
    break;
	}
	bpt = bc->bitmap + (i*linear_scale)*bc->bytes_per_line;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
	    allbottom = 0;
	if ( i*linear_scale + linear_scale-1>bc->ymax-bc->ymin )
	    alltop = 0;
	else {
	    bpt = bc->bitmap + (i*linear_scale+linear_scale-1)*bc->bytes_per_line;
	    if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
		alltop = 0;
	}
	any = false;
	for ( ii=0; ii<linear_scale; ++ii ) {
	    if ( (i*linear_scale+ii)>bc->xmax-bc->xmin )
	break;
	    bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	    if ( bpt[jjj>>3]& (1<<(7-(jjj&7))) )
		any = true;
	}
	if ( !any )
	    across = false;
    }
    if ( across ) {
	if ( (!alltop && !allbottom ) || (alltop && allbottom))
return( true );
	if ( allbottom ) {
	    if ( i==0 )
return( true );
	    if ( countsquare(bc,i-1,j,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
return( true );
	}
	if ( alltop ) {
	    if ( i==(bc->ymax-bc->ymin)/linear_scale )
return( true );
	    if ( countsquare(bc,i+1,j,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
	    if ( makesbottomedge(bc,i+1,j,linear_scale))
return( false );
return( true );
	}
    }

return( false );		/* No line */
}

/* Make a much larger bitmap than we need, and then shrink it */
static void BDFCShrinkBitmap(BDFChar *bc, int linear_scale) {
    BDFChar new;
    int i,j, mid = linear_scale*linear_scale/2;
    int cnt;
    uint8 *pt;

    if ( bc==NULL )
return;

    memset(&new,'\0',sizeof(new));
    new.xmin = floor( ((real) bc->xmin)/linear_scale );
    new.ymin = floor( ((real) bc->ymin)/linear_scale );
    new.xmax = new.xmin + (bc->xmax-bc->xmin+linear_scale-1)/linear_scale;
    new.ymax = new.ymin + (bc->ymax-bc->ymin+linear_scale-1)/linear_scale;
    new.width = rint( ((real) bc->width)/linear_scale );

    new.bytes_per_line = (new.xmax-new.xmin+1);
    new.enc = bc->enc;
    new.sc = bc->sc;
    new.byte_data = true;
    new.bitmap = gcalloc( (new.ymax-new.ymin+1) * new.bytes_per_line, sizeof(uint8));
    for ( i=0; i<=new.ymax-new.ymin; ++i ) {
	pt = new.bitmap + i*new.bytes_per_line;
	for ( j=0; j<=new.xmax-new.xmin; ++j ) {
	    cnt = countsquare(bc,i,j,linear_scale);
	    if ( cnt>=mid )
		pt[j>>3] |= (1<<(7-(j&7)));
	    else if ( cnt>=linear_scale && makesline(bc,i,j,linear_scale))
		pt[j>>3] |= (1<<(7-(j&7)));
	}
    }
    free(bc->bitmap);
    *bc = new;
}

BDFChar *SplineCharSlowerRasterize(SplineChar *sc, int pixelsize) {
    BDFChar *bc;
    int linear_scale = 1;

    if ( pixelsize<=30 )
	linear_scale = 4;
    else if ( pixelsize<=40 )
	linear_scale = 3;
    else if ( pixelsize<=60 )
	linear_scale = 2;

    bc = SplineCharRasterize(sc,pixelsize*linear_scale);
    if ( linear_scale==1 )
return( bc );
    BDFCShrinkBitmap(bc,linear_scale);
return( bc );
}

BDFFont *SplineFontRasterize(SplineFont *_sf, int pixelsize, int indicate, int slower) {
#else
BDFFont *SplineFontRasterize(SplineFont *_sf, int pixelsize, int indicate) {
#endif
    BDFFont *bdf = SplineFontToBDFHeader(_sf,pixelsize,indicate);
    int i,k;
    SplineFont *sf=_sf;	/* The complexity here is to pick the appropriate subfont of a CID font */

    for ( i=0; i<bdf->glyphcnt; ++i ) {
	if ( _sf->subfontcnt!=0 ) {
	    for ( k=0; k<_sf->subfontcnt; ++k ) if ( _sf->subfonts[k]->glyphcnt>i ) {
		sf = _sf->subfonts[k];
		if ( SCWorthOutputting(sf->glyphs[i]))
	    break;
	    }
	}
#if 0
	bdf->glyphs[i] = slower ? SplineCharSlowerRasterize(sf->glyphs[i],pixelsize):
				SplineCharRasterize(sf->glyphs[i],pixelsize);
#else
	bdf->glyphs[i] = SplineCharRasterize(sf->glyphs[i],pixelsize);
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( indicate ) gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( indicate ) gwwv_progress_next();
#endif
    }
#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( indicate ) gwwv_progress_end_indicator();
#elif defined(FONTFORGE_CONFIG_GTK)
    if ( indicate ) gwwv_progress_end_indicator();
#endif
return( bdf );
}

void BDFCAntiAlias(BDFChar *bc, int linear_scale) {
    BDFChar new;
    int i,j, max = linear_scale*linear_scale-1;
    uint8 *bpt, *pt;

    if ( bc==NULL )
return;

    memset(&new,'\0',sizeof(new));
    new.xmin = floor( ((real) bc->xmin)/linear_scale );
    new.ymin = floor( ((real) bc->ymin)/linear_scale );
    new.xmax = new.xmin + (bc->xmax-bc->xmin+linear_scale-1)/linear_scale;
    new.ymax = new.ymin + (bc->ymax-bc->ymin+linear_scale-1)/linear_scale;
    new.width = rint( ((real) bc->width)/linear_scale );

    new.bytes_per_line = (new.xmax-new.xmin+1);
    new.orig_pos = bc->orig_pos;
    new.sc = bc->sc;
    new.byte_data = true;
    new.depth = max==3 ? 2 : max==15 ? 4 : 8;
    new.bitmap = gcalloc( (new.ymax-new.ymin+1) * new.bytes_per_line, sizeof(uint8));
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( bc->depth>1 ) {
	uint32 *sum = gcalloc(new.bytes_per_line,sizeof(uint32));
	for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
	    bpt = bc->bitmap + i*bc->bytes_per_line;
	    for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		sum[j/linear_scale] += bpt[j];
	    }
	    if ( (i+1)%linear_scale==0 ) {
		pt = new.bitmap + (i/linear_scale)*new.bytes_per_line;
		for ( j=(bc->xmax-bc->xmin)/linear_scale-1; j>=0 ; --j ) {
		    int val = rint( (sum[j]+128)/255 );
		    if ( val>max ) val = max;
		    pt[j] = val;
		}
		memset(sum,0,new.bytes_per_line*sizeof(uint32));
	    }
	}
    } else {
#else
    {
#endif
	for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
	    bpt = bc->bitmap + i*bc->bytes_per_line;
	    pt = new.bitmap + (i/linear_scale)*new.bytes_per_line;
	    for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		if ( bpt[(j>>3)] & (1<<(7-(j&7))) )
		    /* we can get values between 0..2^n we want values between 0..(2^n-1) */
		    if ( pt[ j/linear_scale ]!=max )
			++pt[ j/linear_scale ];
	    }
	}
    }
    free(bc->bitmap);
    *bc = new;
}
#endif

#ifndef LUA_FF_LIB
GClut *_BDFClut(int linear_scale) {
    int scale = linear_scale*linear_scale, i;
    Color bg = default_background;
    int bgr=((bg>>16)&0xff), bgg=((bg>>8)&0xff), bgb= (bg&0xff);
    GClut *clut;

    clut = gcalloc(1,sizeof(GClut));
    clut->clut_len = scale;
    clut->is_grey = (bgr==bgg && bgb==bgr);
    clut->trans_index = -1;
    for ( i=0; i<scale; ++i ) {
	clut->clut[i] =
		COLOR_CREATE( bgr- (i*(bgr))/(scale-1),
				bgg- (i*(bgg))/(scale-1),
				bgb- (i*(bgb))/(scale-1));
    }
    clut->clut[scale-1] = 0;	/* avoid rounding errors */
return( clut );
}

void BDFClut(BDFFont *bdf, int linear_scale) {
    bdf->clut = _BDFClut(linear_scale);
}
#endif

int BDFDepth(BDFFont *bdf) {
    if ( bdf->clut==NULL )
return( 1 );

return( bdf->clut->clut_len==256 ? 8 :
	bdf->clut->clut_len==16 ? 4 : 2);
}

#ifndef LUA_FF_LIB
BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize, int linear_scale) {
    BDFChar *bc;

    bc = _SplineCharRasterize(sc,pixelsize*linear_scale,true);
    if ( linear_scale!=1 )
	BDFCAntiAlias(bc,linear_scale);
    BCCompressBitmap(bc);
return( bc );
}

BDFFont *SplineFontAntiAlias(SplineFont *_sf, int pixelsize, int linear_scale) {
    BDFFont *bdf;
    int i,k;
    real scale;
    char size[40];
    char aa[200];
    int max;
    SplineFont *sf;	/* The complexity here is to pick the appropriate subfont of a CID font */

    if ( linear_scale==1 )
return( SplineFontRasterize(_sf,pixelsize,true));

    bdf = gcalloc(1,sizeof(BDFFont));
    sf = _sf;
    max = sf->glyphcnt;
    for ( i=0; i<_sf->subfontcnt; ++i ) {
	sf = _sf->subfonts[i];
	if ( sf->glyphcnt>max ) max = sf->glyphcnt;
    }
    scale = pixelsize / (real) (sf->ascent+sf->descent);

    sprintf(size,_("%d pixels"), pixelsize );
    strcpy(aa,_("Generating anti-alias font"));
    if ( sf->fontname!=NULL ) {
	strcat(aa,": ");
	strncat(aa,sf->fontname,sizeof(aa)-strlen(aa));
	aa[sizeof(aa)-1] = '\0';
    }
    gwwv_progress_start_indicator(10,_("Rasterizing..."),
	    aa,size,sf->glyphcnt,1);
    gwwv_progress_enable_stop(0);

    if ( linear_scale>16 ) linear_scale = 16;	/* can't deal with more than 256 levels of grey */
    if ( linear_scale<=1 ) linear_scale = 2;
    bdf->sf = _sf;
    bdf->glyphcnt = bdf->glyphmax = max;
    bdf->pixelsize = pixelsize;
    bdf->glyphs = galloc(max*sizeof(BDFChar *));
    bdf->ascent = rint(sf->ascent*scale);
    bdf->descent = pixelsize-bdf->ascent;
    bdf->res = -1;
    for ( i=0; i<max; ++i ) {
	if ( _sf->subfontcnt!=0 ) {
	    for ( k=0; k<_sf->subfontcnt; ++k ) if ( _sf->subfonts[k]->glyphcnt>i ) {
		sf = _sf->subfonts[k];
		if ( SCWorthOutputting(sf->glyphs[i]))
	    break;
	    }
	    scale = pixelsize / (real) (sf->ascent+sf->descent);
	}
	bdf->glyphs[i] = SplineCharRasterize(sf->glyphs[i],pixelsize*linear_scale);
	BDFCAntiAlias(bdf->glyphs[i],linear_scale);
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next();
#endif
    }
    BDFClut(bdf,linear_scale);
#if defined(FONTFORGE_CONFIG_GDRAW)
    gwwv_progress_end_indicator();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_end_indicator();
#endif
return( bdf );
}


BDFChar *BDFPieceMeal(BDFFont *bdf, int index) {
    SplineChar *sc;
    extern int use_freetype_to_rasterize_fv;

    if ( index==-1 )
return( NULL );
    if ( bdf->glyphcnt<bdf->sf->glyphcnt ) {
	if ( bdf->glyphmax<bdf->sf->glyphcnt )
	    bdf->glyphs = grealloc(bdf->glyphs,(bdf->glyphmax = bdf->sf->glyphmax)*sizeof(BDFChar *));
	memset(bdf->glyphs+bdf->glyphcnt,0,(bdf->glyphmax-bdf->glyphcnt)*sizeof(SplineChar *));
	bdf->glyphcnt = bdf->sf->glyphcnt;
    }
    sc = bdf->sf->glyphs[index];
    if ( sc==NULL )
return(NULL);
    if ( bdf->freetype_context )
#ifndef LUA_FF_LIB
	bdf->glyphs[index] = SplineCharFreeTypeRasterize(bdf->freetype_context,
		sc->orig_pos,bdf->truesize,bdf->clut?8:1);
#endif
    ; 
    else {
#ifndef LUA_FF_LIB
	if ( use_freetype_to_rasterize_fv && !sc->parent->multilayer &&
		!sc->parent->strokedfont )
	    bdf->glyphs[index] = SplineCharFreeTypeRasterizeNoHints(sc,
		    bdf->truesize,bdf->clut?4:1);
	else
#endif
	    bdf->glyphs[index] = NULL;
	if ( bdf->glyphs[index]==NULL ) {
	    if ( bdf->clut )
		bdf->glyphs[index] = SplineCharAntiAlias(sc,bdf->truesize,4);
	    else
		bdf->glyphs[index] = SplineCharRasterize(sc,bdf->truesize);
	}
    }
return( bdf->glyphs[index] );
}

/* Piecemeal fonts are only used as the display font in the fontview */
/*  as such they are simple fonts (ie. we only display the current cid subfont) */
BDFFont *SplineFontPieceMeal(SplineFont *sf,int pixelsize,int flags,void *ftc) {
    BDFFont *bdf = gcalloc(1,sizeof(BDFFont));
    real scale;
    int truesize = pixelsize;

    if ( flags&pf_bbsized ) {
	DBounds bb;
	SplineFontQuickConservativeBounds(sf,&bb);
	if ( bb.maxy<sf->ascent ) bb.maxy = sf->ascent;
	if ( bb.miny>-sf->descent ) bb.miny = -sf->descent;
	/* Ignore absurd values */
	if ( bb.maxy>10*(sf->ascent+sf->descent) ) bb.maxy = 2*(sf->ascent+sf->descent);
	if ( bb.maxx>10*(sf->ascent+sf->descent) ) bb.maxx = 2*(sf->ascent+sf->descent);
	if ( bb.miny<-10*(sf->ascent+sf->descent) ) bb.miny = -2*(sf->ascent+sf->descent);
	if ( bb.minx<-10*(sf->ascent+sf->descent) ) bb.minx = -2*(sf->ascent+sf->descent);
	scale = pixelsize/ (real) (bb.maxy-bb.miny);
	bdf->ascent = rint(bb.maxy*scale);
	truesize = rint( (sf->ascent+sf->descent)*scale );
    } else {
	scale = pixelsize / (real) (sf->ascent+sf->descent);
	bdf->ascent = rint(sf->ascent*scale);
    }

    bdf->sf = sf;
    bdf->glyphcnt = bdf->glyphmax = sf->glyphcnt;
    bdf->pixelsize = pixelsize;
    bdf->glyphs = gcalloc(sf->glyphcnt,sizeof(BDFChar *));
    bdf->descent = pixelsize-bdf->ascent;
    bdf->piecemeal = true;
    bdf->bbsized = (flags&pf_bbsized)?1:0;
    bdf->res = -1;
    bdf->truesize = truesize;
    bdf->freetype_context = ftc;
    if ( ftc && (flags&pf_antialias) )
	BDFClut(bdf,16);
    else if ( flags&pf_antialias )
	BDFClut(bdf,4);
return( bdf );
}
#endif

void BDFCharFree(BDFChar *bdfc) {
    if ( bdfc==NULL )
return;
    free(bdfc->bitmap);
    chunkfree(bdfc,sizeof(BDFChar));
}

void BDFPropsFree(BDFFont *bdf) {
    int i;

    for ( i=0; i<bdf->prop_cnt; ++i ) {
	free(bdf->props[i].name);
	if ( (bdf->props[i].type&~prt_property)==prt_string ||
		 (bdf->props[i].type&~prt_property)==prt_atom )
	     free(bdf->props[i].u.str);
     }
     free( bdf->props );
}

void BDFFontFree(BDFFont *bdf) {
    int i;

    if ( bdf==NULL )
return;
    for ( i=0; i<bdf->glyphcnt; ++i )
	BDFCharFree(bdf->glyphs[i]);
    free(bdf->glyphs);
    if ( bdf->clut!=NULL )
	free(bdf->clut);
#ifndef LUA_FF_LIB
    if ( bdf->freetype_context!=NULL )
	FreeTypeFreeContext(bdf->freetype_context);
#endif
    BDFPropsFree(bdf);
    free( bdf->foundry );
    free(bdf);
}
