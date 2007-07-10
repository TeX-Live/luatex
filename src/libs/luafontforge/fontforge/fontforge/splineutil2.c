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
#include <math.h>
#include "ustring.h"
#include "chardata.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "views.h"		/* For SCCharChangedUpdate */

/*#define DEBUG	1*/

int RealNear(real a,real b) {
    real d;

#if 0 /* def FONTFORGE_CONFIG_USE_DOUBLE*/
    if ( a==0 )
return( b>-1e-8 && b<1e-8 );
    if ( b==0 )
return( a>-1e-8 && a<1e-8 );

    d = a/(1024*1024.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#else		/* For floats */
    if ( a==0 )
return( b>-1e-5 && b<1e-5 );
    if ( b==0 )
return( a>-1e-5 && a<1e-5 );

    d = a/(1024*64.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#endif
}

int RealNearish(real a,real b) {

    if ( a-b<.001 && a-b>-.001 )
return( true );
return( false );
}

int RealApprox(real a,real b) {

    if ( a==0 ) {
	if ( b<.0001 && b>-.0001 )
return( true );
    } else if ( b==0 ) {
	if ( a<.0001 && a>-.0001 )
return( true );
    } else {
	a /= b;
	if ( a>=.95 && a<=1.05 )
return( true );
    }
return( false );
}

int RealWithin(real a,real b,real fudge) {

return( b>=a-fudge && b<=a+fudge );
}

int RealRatio(real a,real b,real fudge) {

    if ( b==0 )
return( RealWithin(a,b,fudge));

return( RealWithin(a/b,1.0,fudge));
}

static int MinMaxWithin(Spline *spline) {
    extended dx, dy;
    int which;
    extended t1, t2;
    extended w;
    /* We know that this "spline" is basically one dimensional. As long as its*/
    /*  extrema are between the start and end points on that line then we can */
    /*  treat it as a line. If the extrema are way outside the line segment */
    /*  then it's a line that backtracks on itself */

    if ( (dx = spline->to->me.x - spline->from->me.x)<0 ) dx = -dx;
    if ( (dy = spline->to->me.y - spline->from->me.y)<0 ) dy = -dy;
    which = dx<dy;
    SplineFindExtrema(&spline->splines[which],&t1,&t2);
    if ( t1==-1 )
return( true );
    w = ((spline->splines[which].a*t1 + spline->splines[which].b)*t1
	     + spline->splines[which].c)*t1 + spline->splines[which].d;
    if ( RealNear(w, (&spline->to->me.x)[which]) || RealNear(w, (&spline->from->me.x)[which]) )
	/* Close enough */;
    else if ( (w<(&spline->to->me.x)[which] && w<(&spline->from->me.x)[which]) ||
	    (w>(&spline->to->me.x)[which] && w>(&spline->from->me.x)[which]) )
return( false );		/* Outside */

    w = ((spline->splines[which].a*t2 + spline->splines[which].b)*t2
	     + spline->splines[which].c)*t2 + spline->splines[which].d;
    if ( RealNear(w, (&spline->to->me.x)[which]) || RealNear(w, (&spline->from->me.x)[which]) )
	/* Close enough */;
    else if ( (w<(&spline->to->me.x)[which] && w<(&spline->from->me.x)[which]) ||
	    (w>(&spline->to->me.x)[which] && w>(&spline->from->me.x)[which]) )
return( false );		/* Outside */

return( true );
}

int SplineIsLinear(Spline *spline) {
    double t1,t2, t3,t4;
    int ret;

    if ( spline->knownlinear )
return( true );
    if ( spline->knowncurved )
return( false );

    if ( spline->splines[0].a==0 && spline->splines[0].b==0 &&
	    spline->splines[1].a==0 && spline->splines[1].b==0 )
return( true );

    /* Something is linear if the control points lie on the line between the */
    /*  two base points */

    /* Vertical lines */
    if ( RealNear(spline->from->me.x,spline->to->me.x) ) {
	ret = RealNear(spline->from->me.x,spline->from->nextcp.x) &&
	    RealNear(spline->from->me.x,spline->to->prevcp.x);
	if ( ! ((spline->from->nextcp.y >= spline->from->me.y &&
		  spline->from->nextcp.y <= spline->to->me.y &&
		  spline->to->prevcp.y >= spline->from->me.y &&
		  spline->to->prevcp.y <= spline->to->me.y ) ||
		 (spline->from->nextcp.y <= spline->from->me.y &&
		  spline->from->nextcp.y >= spline->to->me.y &&
		  spline->to->prevcp.y <= spline->from->me.y &&
		  spline->to->prevcp.y >= spline->to->me.y )) )
	    ret = MinMaxWithin(spline);
    /* Horizontal lines */
    } else if ( RealNear(spline->from->me.y,spline->to->me.y) ) {
	ret = RealNear(spline->from->me.y,spline->from->nextcp.y) &&
	    RealNear(spline->from->me.y,spline->to->prevcp.y);
	if ( ! ((spline->from->nextcp.x >= spline->from->me.x &&
		  spline->from->nextcp.x <= spline->to->me.x &&
		  spline->to->prevcp.x >= spline->from->me.x &&
		  spline->to->prevcp.x <= spline->to->me.x) ||
		 (spline->from->nextcp.x <= spline->from->me.x &&
		  spline->from->nextcp.x >= spline->to->me.x &&
		  spline->to->prevcp.x <= spline->from->me.x &&
		  spline->to->prevcp.x >= spline->to->me.x)) )
	    ret = MinMaxWithin(spline);
    } else {
	ret = true;
	t1 = (spline->from->nextcp.y-spline->from->me.y)/(spline->to->me.y-spline->from->me.y);
	t2 = (spline->from->nextcp.x-spline->from->me.x)/(spline->to->me.x-spline->from->me.x);
	t3 = (spline->to->me.y-spline->to->prevcp.y)/(spline->to->me.y-spline->from->me.y);
	t4 = (spline->to->me.x-spline->to->prevcp.x)/(spline->to->me.x-spline->from->me.x);
	ret = (RealApprox(t1,t2) || (RealApprox(t1,0) && RealApprox(t2,0))) &&
		(RealApprox(t3,t4) || (RealApprox(t3,0) && RealApprox(t4,0)));
	if ( ret ) {
	    if ( t1<0 || t2<0 || t3<0 || t4<0 ||
		    t1>1 || t2>1 || t3>1 || t4>1 )
		ret = MinMaxWithin(spline);
	}
    }
    spline->knowncurved = !ret;
    spline->knownlinear = ret;
    if ( ret ) {
	/* A few places that if the spline is knownlinear then its splines[?] */
	/*  are linear. So give the linear version and not that suggested by */
	/*  the control points */
	spline->splines[0].a = spline->splines[0].b = 0;
	spline->splines[0].d = spline->from->me.x;
	spline->splines[0].c = spline->to->me.x-spline->from->me.x;
	spline->splines[1].a = spline->splines[1].b = 0;
	spline->splines[1].d = spline->from->me.y;
	spline->splines[1].c = spline->to->me.y-spline->from->me.y;
    }
return( ret );
}

int SplineIsLinearMake(Spline *spline) {

    if ( spline->islinear )
return( true );
    if ( SplineIsLinear(spline)) {
	spline->islinear = spline->from->nonextcp = spline->to->noprevcp = true;
	spline->from->nextcp = spline->from->me;
	if ( spline->from->nonextcp && spline->from->noprevcp )
	    spline->from->pointtype = pt_corner;
	else if ( spline->from->pointtype == pt_curve )
	    spline->from->pointtype = pt_tangent;
	spline->to->prevcp = spline->to->me;
	if ( spline->to->nonextcp && spline->to->noprevcp )
	    spline->to->pointtype = pt_corner;
	else if ( spline->to->pointtype == pt_curve )
	    spline->to->pointtype = pt_tangent;
	SplineRefigure(spline);
    }
return( spline->islinear );
}

typedef struct spline1 {
    Spline1D sp;
    real s0, s1;
    real c0, c1;
} Spline1;

static void FigureSpline1(Spline1 *sp1,bigreal t0, bigreal t1, Spline1D *sp ) {
    bigreal s = (t1-t0);
    if ( sp->a==0 && sp->b==0 ) {
	sp1->sp.d = sp->d + t0*sp->c;
	sp1->sp.c = s*sp->c;
	sp1->sp.b = sp1->sp.a = 0;
    } else {
	sp1->sp.d = sp->d + t0*(sp->c + t0*(sp->b + t0*sp->a));
	sp1->sp.c = s*(sp->c + t0*(2*sp->b + 3*sp->a*t0));
	sp1->sp.b = s*s*(sp->b+3*sp->a*t0);
	sp1->sp.a = s*s*s*sp->a;
#if 0		/* Got invoked once on a perfectly good spline */
	sp1->s1 = sp1->sp.a+sp1->sp.b+sp1->sp.c+sp1->sp.d;
	if ( ((sp1->s1>.001 || sp1->s1<-.001) && !RealNear((double) sp1->sp.a+sp1->sp.b+sp1->sp.c+sp1->sp.d,sp1->s1)) ||
		!RealNear(sp1->sp.d,sp1->s0))
	    IError( "Created spline does not work in FigureSpline1");
#endif
    }
    sp1->c0 = sp1->sp.c/3 + sp1->sp.d;
    sp1->c1 = sp1->c0 + (sp1->sp.b+sp1->sp.c)/3;
}

SplinePoint *SplineBisect(Spline *spline, extended t) {
    Spline1 xstart, xend;
    Spline1 ystart, yend;
    Spline *spline1, *spline2;
    SplinePoint *mid;
    SplinePoint *old0, *old1;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];
    int order2 = spline->order2;

#ifdef DEBUG
    if ( t<=1e-3 || t>=1-1e-3 )
	IError("Bisection to create a zero length spline");
#endif
    xstart.s0 = xsp->d; ystart.s0 = ysp->d;
    xend.s1 = (extended) xsp->a+xsp->b+xsp->c+xsp->d;
    yend.s1 = (extended) ysp->a+ysp->b+ysp->c+ysp->d;
    xstart.s1 = xend.s0 = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
    ystart.s1 = yend.s0 = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;
    FigureSpline1(&xstart,0,t,xsp);
    FigureSpline1(&xend,t,1,xsp);
    FigureSpline1(&ystart,0,t,ysp);
    FigureSpline1(&yend,t,1,ysp);

    mid = chunkalloc(sizeof(SplinePoint));
    mid->me.x = xstart.s1;	mid->me.y = ystart.s1;
    if ( order2 ) {
	mid->nextcp.x = xend.sp.d + xend.sp.c/2;
	mid->nextcp.y = yend.sp.d + yend.sp.c/2;
	mid->prevcp.x = xstart.sp.d + xstart.sp.c/2;
	mid->prevcp.y = ystart.sp.d + ystart.sp.c/2;
    } else {
	mid->nextcp.x = xend.c0;	mid->nextcp.y = yend.c0;
	mid->prevcp.x = xstart.c1;	mid->prevcp.y = ystart.c1;
    }
    if ( mid->me.x==mid->nextcp.x && mid->me.y==mid->nextcp.y )
	mid->nonextcp = true;
    if ( mid->me.x==mid->prevcp.x && mid->me.y==mid->prevcp.y )
	mid->noprevcp = true;

    old0 = spline->from; old1 = spline->to;
    if ( order2 ) {
	old0->nextcp = mid->prevcp;
	old1->prevcp = mid->nextcp;
    } else {
	old0->nextcp.x = xstart.c0;	old0->nextcp.y = ystart.c0;
	old1->prevcp.x = xend.c1;	old1->prevcp.y = yend.c1;
    }
    old0->nonextcp = (old0->nextcp.x==old0->me.x && old0->nextcp.y==old0->me.y);
    old1->noprevcp = (old1->prevcp.x==old1->me.x && old1->prevcp.y==old1->me.y);
    old0->nextcpdef = false;
    old1->prevcpdef = false;
    SplineFree(spline);

    spline1 = chunkalloc(sizeof(Spline));
    spline1->splines[0] = xstart.sp;	spline1->splines[1] = ystart.sp;
    spline1->from = old0;
    spline1->to = mid;
    spline1->order2 = order2;
    old0->next = spline1;
    mid->prev = spline1;
    if ( SplineIsLinear(spline1)) {
	spline1->islinear = spline1->from->nonextcp = spline1->to->noprevcp = true;
	spline1->from->nextcp = spline1->from->me;
	spline1->to->prevcp = spline1->to->me;
    }
    SplineRefigure(spline1);

    spline2 = chunkalloc(sizeof(Spline));
    spline2->splines[0] = xend.sp;	spline2->splines[1] = xend.sp;
    spline2->from = mid;
    spline2->to = old1;
    spline2->order2 = order2;
    mid->next = spline2;
    old1->prev = spline2;
    if ( SplineIsLinear(spline2)) {
	spline2->islinear = spline2->from->nonextcp = spline2->to->noprevcp = true;
	spline2->from->nextcp = spline2->from->me;
	spline2->to->prevcp = spline2->to->me;
    }
    SplineRefigure(spline2);
return( mid );
}

Spline *SplineSplit(Spline *spline, extended ts[3]) {
    /* Split the current spline in up to 3 places */
    Spline1 splines[2][4];
    int i,cnt;
    bigreal base;
    SplinePoint *last, *sp;
    Spline *new;
    int order2 = spline->order2;

    memset(splines,0,sizeof(splines));
    base=0;
    for ( i=cnt=0; i<3 && ts[i]!=-1; ++i ) {
	if ( base>1-1e-3 )			/* Avoid tiny splines */
    break;
	else if ( base<ts[i]-1e-3 ) {
	    FigureSpline1(&splines[0][cnt],base,ts[i],&spline->splines[0]);
	    FigureSpline1(&splines[1][cnt++],base,ts[i],&spline->splines[1]);
	    base = ts[i];
	}
    }
    if ( base==0 )
return( spline );

    FigureSpline1(&splines[0][cnt],base,1.0,&spline->splines[0]);
    FigureSpline1(&splines[1][cnt],base,1.0,&spline->splines[1]);

    last = spline->from;
    for ( i=0; i<=cnt; ++i ) {
	if ( order2 ) {
	    last->nextcp.x = splines[0][i].sp.d+splines[0][i].sp.c/2;
	    last->nextcp.y = splines[1][i].sp.d+splines[1][i].sp.c/2;
	} else {
	    last->nextcp.x = splines[0][i].c0;
	    last->nextcp.y = splines[1][i].c0;
	}
	if ( i==cnt )
	    sp = spline->to;
	else {
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = splines[0][i+1].sp.d;
	    sp->me.y = splines[1][i+1].sp.d;
	}
	if ( order2 ) {
	    sp->prevcp = last->nextcp;
	    SplineMake2(last,sp);
	} else {
	    sp->prevcp.x = splines[0][i].c1;
	    sp->prevcp.y = splines[1][i].c1;
	    SplineMake3(last,sp);
	}
	last = sp;
    }

    new = spline->from->next;
    SplineFree(spline);
return( new );
}

static Spline *IsLinearApprox(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt, int order2) {
    double vx, vy, slope;
    int i;

    vx = to->me.x-from->me.x; vy = to->me.y-from->me.y;
    if ( vx==0 && vy==0 ) {
	for ( i=0; i<cnt; ++i )
	    if ( mid[i].x != from->me.x || mid[i].y != from->me.y )
return( NULL );
    } else if ( fabs(vx)>fabs(vy) ) {
	slope = vy/vx;
	for ( i=0; i<cnt; ++i )
	    if ( !RealWithin(mid[i].y,from->me.y+slope*(mid[i].x-from->me.x),.7) )
return( NULL );
    } else {
	slope = vx/vy; 
	for ( i=0; i<cnt; ++i )
	    if ( !RealWithin(mid[i].x,from->me.x+slope*(mid[i].y-from->me.y),.7) )
return( NULL );
    }
    from->nonextcp = to->noprevcp = true;
return( SplineMake(from,to,order2) );
}

#if 0
static void CleanupDir(BasePoint *newcp,BasePoint *oldcp,BasePoint *base) {
    double orig, new;
    orig = atan2(oldcp->y-base->y,oldcp->x-base->x);
    new  = atan2(newcp->y-base->y,newcp->x-base->x);
    if ( !RealNearish(orig,new)) {
	double c = cos(orig), s = sin(orig);
	/*double len = sqrt((newcp->x-base->x)*(newcp->x-base->x) + (newcp->y-base->y)*(newcp->y-base->y));*/
	/* use dot product rather than length */
	double len;
	if ( c<1e-6 && c>-1e-6 ) { c=0; if ( s<0 ) s=-1; else s=1; }
	if ( s<1e-6 && s>-1e-6 ) { s=0; if ( c<0 ) c=-1; else c=1; }
	len = (newcp->x-base->x)*c + (newcp->y-base->y)*s;
	newcp->x = len*c; newcp->y = len*s;
	if ( newcp->x*(oldcp->x-base->x) + newcp->y*(oldcp->y-base->y)<0 ) {
	    IError( "Control points in wrong direction" );
	    newcp->x = -newcp->x; newcp->y = -newcp->y;
	}
	newcp->x += base->x; newcp->y += base->y;
    }
}
#endif

/* Least squares tells us that:
	| S(xi*ti^3) |	 | S(ti^6) S(ti^5) S(ti^4) S(ti^3) |   | a |
	| S(xi*ti^2) | = | S(ti^5) S(ti^4) S(ti^3) S(ti^2) | * | b |
	| S(xi*ti)   |	 | S(ti^4) S(ti^3) S(ti^2) S(ti)   |   | c |
	| S(xi)	     |   | S(ti^3) S(ti^2) S(ti)   n       |   | d |
 and the definition of a spline tells us:
	| x1         | = |   1        1       1       1    | * (a b c d)
	| x0         | = |   0        0       0       1    | * (a b c d)
So we're a bit over specified. Let's use the last two lines of least squares
and the 2 from the spline defn. So d==x0. Now we've got three unknowns
and only three equations...

For order2 splines we've got
	| S(xi*ti^2) |	 | S(ti^4) S(ti^3) S(ti^2) |   | b |
	| S(xi*ti)   | = | S(ti^3) S(ti^2) S(ti)   | * | c |
	| S(xi)	     |   | S(ti^2) S(ti)   n       |   | d |
 and the definition of a spline tells us:
	| x1         | = |   1       1       1    | * (b c d)
	| x0         | = |   0       0       1    | * (b c d)
=>
    d = x0
    b+c = x1-x0
    S(ti^2)*b + S(ti)*c = S(xi)-n*x0
    S(ti^2)*b + S(ti)*(x1-x0-b) = S(xi)-n*x0
    [ S(ti^2)-S(ti) ]*b = S(xi)-S(ti)*(x1-x0) - n*x0
*/
static int _ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt, BasePoint *nextcp, BasePoint *prevcp,
	int order2) {
    double tt, ttn;
    int i, j, ret;
    double vx[3], vy[3], m[3][3];
    double ts[7], xts[4], yts[4];
    BasePoint nres, pres;
    int nrescnt=0, prescnt=0;
    double nmin, nmax, pmin, pmax, test, ptest;
    double bx, by, cx, cy;

    memset(&nres,0,sizeof(nres)); memset(&pres,0,sizeof(pres));

    /* Add the initial and end points */
    ts[0] = 2; for ( i=1; i<7; ++i ) ts[i] = 1;
    xts[0] = from->me.x+to->me.x; yts[0] = from->me.y+to->me.y;
    xts[3] = xts[2] = xts[1] = to->me.x; yts[3] = yts[2] = yts[1] = to->me.y;
    nmin = pmin = 0; nmax = pmax = (to->me.x-from->me.x)*(to->me.x-from->me.x)+(to->me.y-from->me.y)*(to->me.y-from->me.y);
    for ( i=0; i<cnt; ++i ) {
	xts[0] += mid[i].x;
	yts[0] += mid[i].y;
	++ts[0];
	tt = mid[i].t;
	xts[1] += tt*mid[i].x;
	yts[1] += tt*mid[i].y;
	ts[1] += tt;
	ts[2] += (ttn=tt*tt);
	xts[2] += ttn*mid[i].x;
	yts[2] += ttn*mid[i].y;
	ts[3] += (ttn*=tt);
	xts[3] += ttn*mid[i].x;
	yts[3] += ttn*mid[i].y;
	ts[4] += (ttn*=tt);
	ts[5] += (ttn*=tt);
	ts[6] += (ttn*=tt);

	test = (mid[i].x-from->me.x)*(to->me.x-from->me.x) + (mid[i].y-from->me.y)*(to->me.y-from->me.y);
	if ( test<nmin ) nmin=test;
	if ( test>nmax ) nmax=test;
	test = (mid[i].x-to->me.x)*(from->me.x-to->me.x) + (mid[i].y-to->me.y)*(from->me.y-to->me.y);
	if ( test<pmin ) pmin=test;
	if ( test>pmax ) pmax=test;
    }
    pmin *= 1.2; pmax *= 1.2; nmin *= 1.2; nmax *= 1.2;

    for ( j=0; j<3; ++j ) {
	if ( order2 ) {
	    if ( RealNear(ts[j+2],ts[j+1]) )
    continue;
	    /* This produces really bad results!!!! But I don't see what I can do to improve it */
	    bx = (xts[j]-ts[j+1]*(to->me.x-from->me.x) - ts[j]*from->me.x) / (ts[j+2]-ts[j+1]);
	    by = (yts[j]-ts[j+1]*(to->me.y-from->me.y) - ts[j]*from->me.y) / (ts[j+2]-ts[j+1]);
	    cx = to->me.x-from->me.x-bx;
	    cy = to->me.y-from->me.y-by;

	    nextcp->x = from->me.x + cx/2;
	    nextcp->y = from->me.y + cy/2;
	    *prevcp = *nextcp;
	} else {
	    vx[0] = xts[j+1]-ts[j+1]*from->me.x;
	    vx[1] = xts[j]-ts[j]*from->me.x;
	    vx[2] = to->me.x-from->me.x;		/* always use the defn of spline */

	    vy[0] = yts[j+1]-ts[j+1]*from->me.y;
	    vy[1] = yts[j]-ts[j]*from->me.y;
	    vy[2] = to->me.y-from->me.y;

	    m[0][0] = ts[j+4]; m[0][1] = ts[j+3]; m[0][2] = ts[j+2];
	    m[1][0] = ts[j+3]; m[1][1] = ts[j+2]; m[1][2] = ts[j+1];
	    m[2][0] = 1;  m[2][1] = 1;  m[2][2] = 1;

	    /* Remove a terms from rows 0 and 1 */
	    vx[0] -= ts[j+4]*vx[2];
	    vy[0] -= ts[j+4]*vy[2];
	    m[0][0] = 0; m[0][1] -= ts[j+4]; m[0][2] -= ts[j+4];
	    vx[1] -= ts[j+3]*vx[2];
	    vy[1] -= ts[j+3]*vy[2];
	    m[1][0] = 0; m[1][1] -= ts[j+3]; m[1][2] -= ts[j+3];

	    if ( fabs(m[1][1])<fabs(m[0][1]) ) {
		double temp;
		temp = vx[1]; vx[1] = vx[0]; vx[0] = temp;
		temp = vy[1]; vy[1] = vy[0]; vy[0] = temp;
		temp = m[1][1]; m[1][1] = m[0][1]; m[0][1] = temp;
		temp = m[1][2]; m[1][2] = m[0][2]; m[0][2] = temp;
	    }
	    /* remove b terms from rows 0 and 2 (first normalize row 1 so m[1][1] is 1*/
	    vx[1] /= m[1][1];
	    vy[1] /= m[1][1];
	    m[1][2] /= m[1][1];
	    m[1][1] = 1;
	    vx[0] -= m[0][1]*vx[1];
	    vy[0] -= m[0][1]*vy[1];
	    m[0][2] -= m[0][1]*m[1][2]; m[0][1] = 0;
	    vx[2] -= m[2][1]*vx[1];
	    vy[2] -= m[2][1]*vy[1];
	    m[2][2] -= m[2][1]*m[1][2]; m[2][1] = 0; 

	    vx[0] /= m[0][2];			/* This is cx */
	    vy[0] /= m[0][2];			/* This is cy */
	    /*m[0][2] = 1;*/

	    vx[1] -= m[1][2]*vx[0];		/* This is bx */
	    vy[1] -= m[1][2]*vy[0];		/* This is by */
	    /* m[1][2] = 0; */
	    vx[2] -= m[2][2]*vx[0];		/* This is ax */
	    vy[2] -= m[2][2]*vy[0];		/* This is ay */
	    /* m[2][2] = 0; */

	    nextcp->x = from->me.x + vx[0]/3;
	    nextcp->y = from->me.y + vy[0]/3;
	    prevcp->x = nextcp->x + (vx[0]+vx[1])/3;
	    prevcp->y = nextcp->y + (vy[0]+vy[1])/3;
	}

	test = (nextcp->x-from->me.x)*(to->me.x-from->me.x) +
		(nextcp->y-from->me.y)*(to->me.y-from->me.y);
	ptest = (prevcp->x-to->me.x)*(from->me.x-to->me.x) +
		(prevcp->y-to->me.y)*(from->me.y-to->me.y);
	if ( order2 &&
		(test<nmin || test>nmax || ptest<pmin || ptest>pmax))
    continue;
	if ( test>=nmin && test<=nmax ) {
	    nres.x += nextcp->x; nres.y += nextcp->y;
	    ++nrescnt;
	}
	if ( test>=pmin && test<=pmax ) {
	    pres.x += prevcp->x; pres.y += prevcp->y;
	    ++prescnt;
	}
	if ( nrescnt==1 && prescnt==1 )
    break;
    }

    ret = 0;
    if ( nrescnt>0 ) {
	ret |= 1;
	nextcp->x = nres.x/nrescnt;
	nextcp->y = nres.y/nrescnt;
    } else
	*nextcp = from->nextcp;
    if ( prescnt>0 ) {
	ret |= 2;
	prevcp->x = pres.x/prescnt;
	prevcp->y = pres.y/prescnt;
    } else
	*prevcp = to->prevcp;
    if ( order2 && ret!=3 ) {
	nextcp->x = (nextcp->x + prevcp->x)/2;
	nextcp->y = (nextcp->y + prevcp->y)/2;
    }
    if ( order2 )
	*prevcp = *nextcp;
return( ret );
}

static void TestForLinear(SplinePoint *from,SplinePoint *to) {
    BasePoint off, cpoff, cpoff2;
    double len, co, co2;

    /* Did we make a line? */
    off.x = to->me.x-from->me.x; off.y = to->me.y-from->me.y;
    len = sqrt(off.x*off.x + off.y*off.y);
    if ( len!=0 ) {
	off.x /= len; off.y /= len;
	cpoff.x = from->nextcp.x-from->me.x; cpoff.y = from->nextcp.y-from->me.y;
	len = sqrt(cpoff.x*cpoff.x + cpoff.y*cpoff.y);
	if ( len!=0 ) {
	    cpoff.x /= len; cpoff.y /= len;
	}
	cpoff2.x = to->prevcp.x-from->me.x; cpoff2.y = to->prevcp.y-from->me.y;
	len = sqrt(cpoff2.x*cpoff2.x + cpoff2.y*cpoff2.y);
	if ( len!=0 ) {
	    cpoff2.x /= len; cpoff2.y /= len;
	}
	co = cpoff.x*off.y - cpoff.y*off.x; co2 = cpoff2.x*off.y - cpoff2.y*off.x;
	if ( co<.05 && co>-.05 && co2<.05 && co2>-.05 ) {
	    from->nextcp = from->me; from->nonextcp = true;
	    to->prevcp = to->me; to->noprevcp = true;
	} else {
	    Spline temp;
	    memset(&temp,0,sizeof(temp));
	    temp.from = from; temp.to = to;
	    SplineRefigure(&temp);
	    if ( SplineIsLinear(&temp)) {
		from->nextcp = from->me; from->nonextcp = true;
		to->prevcp = to->me; to->noprevcp = true;
	    }
	}
    }
}

/* Find a spline which best approximates the list of intermediate points we */
/*  are given. No attempt is made to fix the slopes */
Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt, int order2) {
    int ret;
    Spline *spline;
    BasePoint nextcp, prevcp;

    if ( (spline = IsLinearApprox(from,to,mid,cnt,order2))!=NULL )
return( spline );

    ret = _ApproximateSplineFromPoints(from,to,mid,cnt,&nextcp,&prevcp,order2);

    if ( ret&1 ) {
	from->nextcp = nextcp;
	from->nonextcp = false;
    } else {
	from->nextcp = from->me;
	from->nonextcp = true;
    }
    if ( ret&2 ) {
	to->prevcp = prevcp;
	to->noprevcp = false;
    } else {
	to->prevcp = to->me;
	to->noprevcp = true;
    }
    TestForLinear(from,to);
    spline = SplineMake(from,to,order2);
return( spline );
}

static double ClosestSplineSolve(Spline1D *sp,double sought,double close_to_t) {
    /* We want to find t so that spline(t) = sought */
    /*  find the value which is closest to close_to_t */
    /* on error return closetot */
    Spline1D temp;
    extended ts[3];
    int i;
    double t, best, test;

    temp = *sp;
    temp.d -= sought;
    CubicSolve(&temp,ts);
    best = 9e20; t= close_to_t;
    for ( i=0; i<3; ++i ) if ( ts[i]!=-1 ) {
	if ( (test=ts[i]-close_to_t)<0 ) test = -test;
	if ( test<best ) {
	    best = test;
	    t = ts[i];
	}
    }

return( t );
}

struct dotbounds {
    BasePoint unit;
    BasePoint base;
    double len;
    double min,max;		/* If min<0 || max>len the spline extends beyond its endpoints */
};

static double SigmaDeltas(Spline *spline,TPoint *mid, int cnt, DBounds *b, struct dotbounds *db) {
    int i, lasti;
    double xdiff, ydiff, sum, temp, t, lastt;
    SplinePoint *to = spline->to, *from = spline->from;
    extended ts[2], x,y;
    struct dotbounds db2;
    double dot;

    if ( (xdiff = to->me.x-from->me.x)<0 ) xdiff = -xdiff;
    if ( (ydiff = to->me.y-from->me.y)<0 ) ydiff = -ydiff;

    sum = 0; lastt = -1; lasti = -1;
    for ( i=0; i<cnt; ++i ) {
	if ( ydiff>2*xdiff ) {
	    t = ClosestSplineSolve(&spline->splines[1],mid[i].y,mid[i].t);
	} else if ( xdiff>2*ydiff ) {
	    t = ClosestSplineSolve(&spline->splines[0],mid[i].x,mid[i].t);
	} else {
	    t = (ClosestSplineSolve(&spline->splines[1],mid[i].y,mid[i].t) +
		    ClosestSplineSolve(&spline->splines[0],mid[i].x,mid[i].t))/2;
	}
	if ( t==lastt )		/* These last* values appear to be debugging */
	    t = lastt + (mid[i].t - mid[lasti].t);
	else {
	    lastt = t;
	    lasti = i;
	}
	temp = mid[i].x - ( ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d );
	sum += temp*temp;
	temp = mid[i].y - ( ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d );
	sum += temp*temp;
    }

    /* Ok, we've got distances from a set of points on the old spline to the */
    /*  new one. Let's do the reverse: find the extrema of the new and see how*/
    /*  close they get to the bounding box of the old */
    /* And get really unhappy if the spline extends beyond the end-points */
    db2.min = 0; db2.max = db->len;
    SplineFindExtrema(&spline->splines[0],&ts[0],&ts[1]);
    for ( i=0; i<2; ++i ) {
	if ( ts[i]!=-1 ) {
	    x = ((spline->splines[0].a*ts[i]+spline->splines[0].b)*ts[i]+spline->splines[0].c)*ts[i] + spline->splines[0].d;
	    y = ((spline->splines[1].a*ts[i]+spline->splines[1].b)*ts[i]+spline->splines[1].c)*ts[i] + spline->splines[1].d;
	    if ( x<b->minx )
		sum += (x-b->minx)*(x-b->minx);
	    else if ( x>b->maxx )
		sum += (x-b->maxx)*(x-b->maxx);
	    dot = (x-db->base.x)*db->unit.x + (y-db->base.y)*db->unit.y;
	    if ( dot<db2.min ) db2.min = dot;
	    if ( dot>db2.max ) db2.max = dot;
	}
    }
    SplineFindExtrema(&spline->splines[1],&ts[0],&ts[1]);
    for ( i=0; i<2; ++i ) {
	if ( ts[i]!=-1 ) {
	    x = ((spline->splines[0].a*ts[i]+spline->splines[0].b)*ts[i]+spline->splines[0].c)*ts[i] + spline->splines[0].d;
	    y = ((spline->splines[1].a*ts[i]+spline->splines[1].b)*ts[i]+spline->splines[1].c)*ts[i] + spline->splines[1].d;
	    if ( y<b->miny )
		sum += (y-b->miny)*(y-b->miny);
	    else if ( y>b->maxy )
		sum += (y-b->maxy)*(y-b->maxy);
	    dot = (x-db->base.x)*db->unit.x + (y-db->base.y)*db->unit.y;
	    if ( dot<db2.min ) db2.min = dot;
	    if ( dot>db2.max ) db2.max = dot;
	}
    }

    /* Big penalty for going beyond the range of the desired spline */
    if ( db->min==0 && db2.min<0 )
	sum += 10000 + db2.min*db2.min;
    else if ( db2.min<db->min )
	sum += 100 + (db2.min-db->min)*(db2.min-db->min);
    if ( db->max==db->len && db2.max>db->len )
	sum += 10000 + (db2.max-db->max)*(db2.max-db->max);
    else if ( db2.max>db->max )
	sum += 100 + (db2.max-db->max)*(db2.max-db->max);

return( sum );
}

static void ApproxBounds(DBounds *b,TPoint *mid, int cnt, struct dotbounds *db) {
    int i;
    double dot;

    b->minx = b->maxx = mid[0].x;
    b->miny = b->maxy = mid[0].y;
    db->min = 0; db->max = db->len;
    for ( i=1; i<cnt; ++i ) {
	if ( mid[i].x>b->maxx ) b->maxx = mid[i].x;
	if ( mid[i].x<b->minx ) b->minx = mid[i].x;
	if ( mid[i].y>b->maxy ) b->maxy = mid[i].y;
	if ( mid[i].y<b->miny ) b->miny = mid[i].y;
	dot = (mid[i].x-db->base.x)*db->unit.x + (mid[i].y-db->base.y)*db->unit.y;
	if ( dot<db->min ) db->min = dot;
	if ( dot>db->max ) db->max = dot;
    }
}

static int GoodCurve(SplinePoint *sp, int check_prev ) {
    double dx, dy, lenx, leny;

    if ( sp->pointtype!=pt_curve )
return( false );
    if ( check_prev ) {
	dx = sp->me.x - sp->prevcp.x;
	dy = sp->me.y - sp->prevcp.y;
    } else {
	dx = sp->me.x - sp->nextcp.x;
	dy = sp->me.y - sp->nextcp.y;
    }
    /* If the cp is very close to the base point the point might as well be a corner */
    if ( dx<0 ) dx = -dx;
    if ( dy<0 ) dy = -dy;
    if ( dx+dy<1 )
return( false );

    if ( check_prev ) {
	if ( sp->prev==NULL )
return( true );
	lenx = sp->me.x - sp->prev->from->me.x;
	leny = sp->me.y - sp->prev->from->me.y;
    } else {
	if ( sp->next==NULL )
return( true );
	lenx = sp->me.x - sp->next->to->me.x;
	leny = sp->me.y - sp->next->to->me.y;
    }
    if ( lenx<0 ) lenx = -lenx;
    if ( leny<0 ) leny = -leny;
    if ( 50*(dx+dy) < lenx+leny )
return( false );

return( true );
}

#if 0
static int totcnt_cnt, nocnt_cnt, incr_cnt, curdiff_cnt;
#endif

/* I used to do a least squares aproach adding two more to the above set of equations */
/*  which held the slopes constant. But that didn't work very well. So instead*/
/*  Then I tried doing the approximation, and then forcing the control points */
/*  to be in line (witht the original slopes), getting a better approximation */
/*  to "t" for each data point and then calculating an error array, approximating*/
/*  it, and using that to fix up the final result */
/* Then I tried checking various possible cp lengths in the desired directions*/
/*  finding the best one or two, and doing a 2D binary search using that as a */
/*  starting point. */
/* And sometimes a least squares approach will give us the right answer, so   */
/*  try that too. */
/* This still isn't as good as I'd like it... But I haven't been able to */
/*  improve it further yet */
#define TRY_CNT		2
#define DECIMATION	5
Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt, int order2) {
    BasePoint tounit, fromunit, ftunit;
    double flen,tlen,ftlen;
    Spline *spline, temp;
    BasePoint nextcp;
    int bettern, betterp;
    double offn, offp, incrn, incrp, trylen;
    int nocnt = 0, totcnt;
    double curdiff, bestdiff[TRY_CNT];
    int i,j,besti[TRY_CNT],bestj[TRY_CNT],k,l;
    double fdiff, tdiff, fmax, tmax, fdotft, tdotft;
    DBounds b;
    struct dotbounds db;
    double offn_, offp_, finaldiff;

    /* If all the selected points are at the same spot, and one of the */
    /*  end-points is also at that spot, then just copy the control point */
    /* But our caller seems to have done that for us */
    
    /* If the two end-points are corner points then allow the slope to vary */
    /* Or if one end-point is a tangent but the point defining the tangent's */
    /*  slope is being removed then allow the slope to vary */
    /* Except if the slope is horizontal or vertical then keep it fixed */
    if ( ( !from->nonextcp && ( from->nextcp.x==from->me.x || from->nextcp.y==from->me.y)) ||
	    (!to->noprevcp && ( to->prevcp.x==to->me.x || to->prevcp.y==to->me.y)) )
	/* Preserve the slope */;
    else if ( ((from->pointtype == pt_corner && from->nonextcp) ||
		(from->pointtype == pt_tangent &&
			((from->nonextcp && from->noprevcp) || !from->noprevcp))) &&
	    ((to->pointtype == pt_corner && to->noprevcp) ||
		(to->pointtype == pt_tangent &&
			((to->nonextcp && to->noprevcp) || !to->nonextcp))) ) {
	from->pointtype = to->pointtype = pt_corner;
return( ApproximateSplineFromPoints(from,to,mid,cnt,order2) );
    }

    /* If we are going to honour the slopes of a quadratic spline, there is */
    /*  only one possibility */
    if ( order2 ) {
	if ( from->nonextcp || to->noprevcp )
return( ApproximateSplineFromPoints(from,to,mid,cnt,order2) );
	else if ( !IntersectLines(&nextcp,&from->nextcp,&from->me,&to->prevcp,&to->me) ) {
	    /* If the slopes don't intersect then use a line */
	    /*  (or if the intersection is patently absurd) */
	    from->nonextcp = to->noprevcp = true;
	    from->nextcp = from->me;
	    to->prevcp = to->me;
	    TestForLinear(from,to);
	} else {
	    from->nextcp = to->prevcp = nextcp;
	    from->nonextcp = to->noprevcp = false;
	}
return( SplineMake2(from,to));
    }
    /* From here down we are only working with cubic splines */

    if ( !to->noprevcp || to->pointtype == pt_corner ) {
	tounit.x = to->prevcp.x-to->me.x; tounit.y = to->prevcp.y-to->me.y;
    } else {
	tounit.x = to->me.x-to->nextcp.x; tounit.y = to->me.y-to->nextcp.y;
    }
    tlen = sqrt(tounit.x*tounit.x + tounit.y*tounit.y);
    if ( !from->nonextcp || from->pointtype == pt_corner ) {
	fromunit.x = from->nextcp.x-from->me.x; fromunit.y = from->nextcp.y-from->me.y;
    } else {
	fromunit.x = from->me.x-from->prevcp.x; fromunit.y = from->me.y-from->prevcp.y;
    }
    flen = sqrt(fromunit.x*fromunit.x + fromunit.y*fromunit.y);
    if ( tlen==0 || flen==0 ) {
	if ( from->next!=NULL )
	    temp = *from->next;
	else {
	    memset(&temp,0,sizeof(temp));
	    temp.from = from; temp.to = to;
	    SplineRefigure(&temp);
	    from->next = to->prev = NULL;
	}
    }
    if ( tlen==0 ) {
	if ( to->pointtype==pt_curve && to->next && !to->nonextcp ) {
	    tounit.x = to->me.x-to->nextcp.x; tounit.y = to->me.y-to->nextcp.y;
/* Doesn't work
	} else if ( to->pointtype==pt_tangent && to->next ) {
	    tounit.x = to->me.x-to->next->to->me.x; tounit.y = to->me.y-to->next->to->me.y;
*/
	} else {
	    tounit.x = -( (3*temp.splines[0].a*.9999+2*temp.splines[0].b)*.9999+temp.splines[0].c );
	    tounit.y = -( (3*temp.splines[1].a*.9999+2*temp.splines[1].b)*.9999+temp.splines[1].c );
	}
	tlen = sqrt(tounit.x*tounit.x + tounit.y*tounit.y);
    }
    tounit.x /= tlen; tounit.y /= tlen;

    if ( flen==0 ) {
	if ( from->pointtype==pt_curve && from->prev && !from->noprevcp ) {
	    fromunit.x = from->me.x-from->prevcp.x; fromunit.y = from->me.y-from->prevcp.y;
/*
	} else if ( from->pointtype==pt_tangent && from->prev ) {
	    fromunit.x = from->me.x-from->prev->from->me.x; fromunit.y = from->me.y-from->prev->from->me.y;
*/
	} else {
	    fromunit.x = ( (3*temp.splines[0].a*.0001+2*temp.splines[0].b)*.0001+temp.splines[0].c );
	    fromunit.y = ( (3*temp.splines[1].a*.0001+2*temp.splines[1].b)*.0001+temp.splines[1].c );
	}
	flen = sqrt(fromunit.x*fromunit.x + fromunit.y*fromunit.y);
    }
    fromunit.x /= flen; fromunit.y /= flen;

    trylen = (to->me.x-from->me.x)*fromunit.x + (to->me.y-from->me.y)*fromunit.y;
    if ( trylen>flen ) flen = trylen;
    
    trylen = (from->me.x-to->me.x)*tounit.x + (from->me.y-to->me.y)*tounit.y;
    if ( trylen>tlen ) tlen = trylen;

    ftunit.x = (to->me.x-from->me.x); ftunit.y = (to->me.y-from->me.y);
    ftlen = sqrt(ftunit.x*ftunit.x + ftunit.y*ftunit.y);
    if ( ftlen!=0 ) {
	ftunit.x /= ftlen; ftunit.y /= ftlen;
    }
    fdotft = fromunit.x*ftunit.x + fromunit.y*ftunit.y;
    fmax = fdotft>0 ? ftlen/fdotft : 1e10;
    tdotft = -tounit.x*ftunit.x - tounit.y*ftunit.y;
    tmax = tdotft>0 ? ftlen/tdotft : 1e10;
    /* At fmax, tmax the control points will stretch beyond the other endpoint*/
    /*  when projected along the line between the two endpoints */

    db.base = from->me;
    db.unit.x = (to->me.x-from->me.x); db.unit.y = (to->me.y-from->me.y);
    db.len = sqrt(db.unit.x*db.unit.x + db.unit.y*db.unit.y);
    if ( db.len!=0 ) {
	db.unit.x /= db.len;
	db.unit.y /= db.len;
    }
    ApproxBounds(&b,mid,cnt,&db);

    for ( k=0; k<TRY_CNT; ++k ) {
	bestdiff[k] = 1e20;
	besti[k] = -1; bestj[k] = -1;
    }
    fdiff = flen/DECIMATION;
    tdiff = tlen/DECIMATION;
    from->nextcp = from->me;
    from->nonextcp = false;
    to->noprevcp = false;
    memset(&temp,0,sizeof(Spline));
    temp.from = from; temp.to = to;
    for ( i=1; i<DECIMATION; ++i ) {
	from->nextcp.x += fdiff*fromunit.x; from->nextcp.y += fdiff*fromunit.y;
	to->prevcp = to->me;
	for ( j=1; j<DECIMATION; ++j ) {
	    to->prevcp.x += tdiff*tounit.x; to->prevcp.y += tdiff*tounit.y;
	    SplineRefigure(&temp);
	    curdiff = SigmaDeltas(&temp,mid,cnt,&b,&db);
	    for ( k=0; k<TRY_CNT; ++k ) {
		if ( curdiff<bestdiff[k] ) {
		    for ( l=TRY_CNT-1; l>k; --l ) {
			bestdiff[l] = bestdiff[l-1];
			besti[l] = besti[l-1];
			bestj[l] = bestj[l-1];
		    }
		    bestdiff[k] = curdiff;
		    besti[k] = i; bestj[k]=j;
	    break;
		}
	    }
	}
    }

    finaldiff = 1e20;
    offn_ = offp_ = -1;
    spline = SplineMake(from,to,false);
    for ( k=-1; k<TRY_CNT; ++k ) {
	if ( k<0 ) {
	    BasePoint nextcp, prevcp;
	    double temp1, temp2;
	    int ret = _ApproximateSplineFromPoints(from,to,mid,cnt,&nextcp,&prevcp,false);
	    /* sometimes least squares gives us the right answer */
	    if ( !(ret&1) || !(ret&2))
    continue;
	    temp1 = (prevcp.x-to->me.x)*tounit.x + (prevcp.y-to->me.y)*tounit.y;
	    temp2 = (nextcp.x-from->me.x)*fromunit.x + (nextcp.y-from->me.y)*fromunit.y;
	    if ( temp1<=0 || temp2<=0 )		/* A nice solution... but the control points are diametrically opposed to what they should be */
    continue;
	    tlen = temp1; flen = temp2;
	} else {
	    if ( bestj[k]<0 || besti[k]<0 )
    continue;
	    tlen = bestj[k]*tdiff; flen = besti[k]*fdiff;
	}
	to->prevcp.x = to->me.x + tlen*tounit.x; to->prevcp.y = to->me.y + tlen*tounit.y;
	from->nextcp.x = from->me.x + flen*fromunit.x; from->nextcp.y = from->me.y + flen*fromunit.y;
	SplineRefigure(spline);

	bettern = betterp = false;
	incrn = tdiff/2.0; incrp = fdiff/2.0;
	offn = flen; offp = tlen;
	nocnt = 0;
	curdiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	totcnt = 0;
	forever {
	    double fadiff, fsdiff;
	    double tadiff, tsdiff;

	    from->nextcp.x = from->me.x + (offn+incrn)*fromunit.x; from->nextcp.y = from->me.y + (offn+incrn)*fromunit.y;
	    to->prevcp.x = to->me.x + offp*tounit.x; to->prevcp.y = to->me.y + offp*tounit.y;
	    SplineRefigure(spline);
	    fadiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    from->nextcp.x = from->me.x + (offn-incrn)*fromunit.x; from->nextcp.y = from->me.y + (offn-incrn)*fromunit.y;
	    SplineRefigure(spline);
	    fsdiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    from->nextcp.x = from->me.x + offn*fromunit.x; from->nextcp.y = from->me.y + offn*fromunit.y;
	    if ( offn-incrn<=0 )
		fsdiff += 1e10;

	    to->prevcp.x = to->me.x + (offp+incrp)*tounit.x; to->prevcp.y = to->me.y + (offp+incrp)*tounit.y;
	    SplineRefigure(spline);
	    tadiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    to->prevcp.x = to->me.x + (offp-incrp)*tounit.x; to->prevcp.y = to->me.y + (offp-incrp)*tounit.y;
	    SplineRefigure(spline);
	    tsdiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    to->prevcp.x = to->me.x + offp*tounit.x; to->prevcp.y = to->me.y + offp*tounit.y;
	    if ( offp-incrp<=0 )
		tsdiff += 1e10;

	    if ( offn>=incrn && fsdiff<curdiff &&
		    (fsdiff<fadiff && fsdiff<tsdiff && fsdiff<tadiff)) {
		offn -= incrn;
		if ( bettern>0 )
		    incrn /= 2;
		bettern = -1;
		nocnt = 0;
		curdiff = fsdiff;
	    } else if ( offn+incrn<fmax && fadiff<curdiff &&
		    (fadiff<=fsdiff && fadiff<tsdiff && fadiff<tadiff)) {
		offn += incrn;
		if ( bettern<0 )
		    incrn /= 2;
		bettern = 1;
		nocnt = 0;
		curdiff = fadiff;
	    } else if ( offp>=incrp && tsdiff<curdiff &&
		    (tsdiff<=fsdiff && tsdiff<=fadiff && tsdiff<tadiff)) {
		offp -= incrp;
		if ( betterp>0 )
		    incrp /= 2;
		betterp = -1;
		nocnt = 0;
		curdiff = tsdiff;
	    } else if ( offp+incrp<tmax && tadiff<curdiff &&
		    (tadiff<=fsdiff && tadiff<=fadiff && tadiff<=tsdiff)) {
		offp += incrp;
		if ( betterp>0 )
		    incrp /= 2;
		betterp = 1;
		nocnt = 0;
		curdiff = tadiff;
	    } else {
		if ( ++nocnt > 6 )
	break;
		incrn /= 2;
		incrp /= 2;
	    }
	    if ( curdiff<1 )
	break;
	    if ( incrp<tdiff/1024 || incrn<fdiff/1024 )
	break;
	    if ( ++totcnt>200 )
	break;
	    if ( offn<0 || offp<0 ) {
		IError("Approximation got inverse control points");
	break;
	    }
	}
#if 0
 if ( nocnt>6 )
  nocnt_cnt++;
 else if ( curdiff<1 )
  curdiff_cnt++;
 else if ( totcnt>200 )
  totcnt_cnt++;
 else
  incr_cnt++;
#endif
	if ( curdiff<finaldiff ) {
	    finaldiff = curdiff;
	    offn_ = offn;
	    offp_ = offp;
	}
    }

    to->noprevcp = offp_==0;
    from->nonextcp = offn_==0;
    to->prevcp.x = to->me.x + offp_*tounit.x; to->prevcp.y = to->me.y + offp_*tounit.y;
    from->nextcp.x = from->me.x + offn_*fromunit.x; from->nextcp.y = from->me.y + offn_*fromunit.y;
    /* I used to check for a spline very close to linear (and if so, make it */
    /*  linear). But in when stroking a path with an eliptical pen we transform*/
    /*  the coordinate system and our normal definitions of "close to linear" */
    /*  don't apply */
    /*TestForLinear(from,to);*/
    SplineRefigure(spline);

return( spline );
}
#undef TRY_CNT
#undef DECIMATION

    /* calculating the actual length of a spline is hard, this gives a very */
    /*  rough (but quick) approximation */
static double SplineLenApprox(Spline *spline) {
    double len, slen, temp;

    if ( (temp = spline->to->me.x-spline->from->me.x)<0 ) temp = -temp;
    len = temp;
    if ( (temp = spline->to->me.y-spline->from->me.y)<0 ) temp = -temp;
    len += temp;
    if ( !spline->to->noprevcp || !spline->from->nonextcp ) {
	if ( (temp = spline->from->nextcp.x-spline->from->me.x)<0 ) temp = -temp;
	slen = temp;
	if ( (temp = spline->from->nextcp.y-spline->from->me.y)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->prevcp.x-spline->from->nextcp.x)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->prevcp.y-spline->from->nextcp.y)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->me.x-spline->to->prevcp.x)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->me.y-spline->to->prevcp.y)<0 ) temp = -temp;
	slen += temp;
	len = (len + slen)/2;
    }
return( len );
}

double SplineLength(Spline *spline) {
    /* I ignore the constant term. It's just an unneeded addition */
    double len, t;
    double lastx = 0, lasty = 0;
    double curx, cury;

    len = 0;
    for ( t=1.0/128; t<=1.0001 ; t+=1.0/128 ) {
	curx = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t;
	cury = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t;
	len += sqrt( (curx-lastx)*(curx-lastx) + (cury-lasty)*(cury-lasty) );
	lastx = curx; lasty = cury;
    }
return( len );
}

static TPoint *SplinesFigureTPsBetween(SplinePoint *from, SplinePoint *to,
	int *tot) {
    int cnt, i, j, pcnt;
    double len, slen, lbase;
    SplinePoint *np;
    TPoint *tp;
    double _lens[10], *lens = _lens;
    int _cnts[10], *cnts = _cnts;
    /* I used just to give every spline 10 points. But that gave much more */
    /*  weight to small splines than to big ones */

    cnt = 0;
    for ( np = from->next->to; ; np = np->next->to ) {
	++cnt;
	if ( np==to )
    break;
    }
    if ( cnt>10 ) {
	lens = galloc(cnt*sizeof(double));
	cnts = galloc(cnt*sizeof(int));
    }
    cnt = 0; len = 0;
    for ( np = from->next->to; ; np = np->next->to ) {
	lens[cnt] = SplineLenApprox(np->prev);
	len += lens[cnt];
	++cnt;
	if ( np==to )
    break;
    }
    if ( len!=0 ) {
	pcnt = 0;
	for ( i=0; i<cnt; ++i ) {
	    int pnts = rint( (10*cnt*lens[i])/len );
	    if ( pnts<2 ) pnts = 2;
	    cnts[i] = pnts;
	    pcnt += pnts;
	}
    } else
	pcnt = 2*cnt;

    tp = galloc((pcnt+1)*sizeof(TPoint)); i = 0;
    if ( len==0 ) {
	for ( i=0; i<=pcnt; ++i ) {
	    tp[i].t = i/(pcnt);
	    tp[i].x = from->me.x;
	    tp[i].y = from->me.y;
	}
    } else {
	lbase = 0;
	for ( i=cnt=0, np = from->next->to; ; np = np->next->to, ++cnt ) {
	    slen = SplineLenApprox(np->prev);
	    for ( j=0; j<cnts[cnt]; ++j ) {
		double t = j/(double) cnts[cnt];
		tp[i].t = (lbase+ t*slen)/len;
		tp[i].x = ((np->prev->splines[0].a*t+np->prev->splines[0].b)*t+np->prev->splines[0].c)*t + np->prev->splines[0].d;
		tp[i++].y = ((np->prev->splines[1].a*t+np->prev->splines[1].b)*t+np->prev->splines[1].c)*t + np->prev->splines[1].d;
	    }
	    lbase += slen;
	    if ( np==to )
	break;
	}
    }
    if ( cnts!=_cnts ) free(cnts);
    if ( lens!=_lens ) free(lens);

    *tot = i;
	
return( tp );
}

void SplinesRemoveBetween(SplineChar *sc, SplinePoint *from, SplinePoint *to,int type) {
    int tot;
    TPoint *tp;
    SplinePoint *np, oldfrom;
    Spline *sp;
    int order2 = from->next->order2;

    oldfrom = *from;
    tp = SplinesFigureTPsBetween(from,to,&tot);

    if ( type==1 )
	ApproximateSplineFromPointsSlopes(from,to,tp,tot-1,order2);
    else
	ApproximateSplineFromPoints(from,to,tp,tot-1,order2);

    /* Have to do the frees after the approximation because the approx */
    /*  uses the splines to determine slopes */
    for ( sp = oldfrom.next; ; ) {
	np = sp->to;
	SplineFree(sp);
	if ( np==to )
    break;
	sp = np->next;
	SplinePointMDFree(sc,np);
    }
    
    free(tp);

    SplinePointCatagorize(from);
    SplinePointCatagorize(to);
}

static void RemoveZeroLengthSplines(SplineSet *spl, int onlyselected, double bound) {
    SplinePoint *curp, *next, *prev;
    double plen, nlen;

    bound *= bound;

    for ( curp = spl->first, prev=NULL; curp!=NULL ; curp=next ) {
	next = NULL;
	if ( curp->next!=NULL )
	    next = curp->next->to;
	if ( curp==next )	/* Once we've worked a contour down to a single point we can't do anything more here. Someone else will have to free the contour */
return;
	/* Zero length splines give us NaNs */
	if ( curp!=NULL && (curp->selected || !onlyselected) ) {
	    plen = nlen = 1e10;
	    if ( curp->prev!=NULL )
		plen = (curp->me.x-curp->prev->from->me.x)*(curp->me.x-curp->prev->from->me.x) +
			(curp->me.y-curp->prev->from->me.y)*(curp->me.y-curp->prev->from->me.y);
	    if ( curp->next!=NULL )
		nlen = (curp->me.x-next->me.x)*(curp->me.x-next->me.x) +
			(curp->me.y-next->me.y)*(curp->me.y-next->me.y);
	    if (( curp->prev!=NULL && plen<=bound && plen<nlen ) ||
		    (curp->next!=NULL && nlen<bound && nlen<plen )) {
		if ( curp->prev!=NULL && plen<=bound && plen<nlen ) {
		    SplinePoint *other = curp->prev->from;
		    other->nextcp = curp->nextcp;
		    other->nonextcp = curp->nonextcp;
		    other->nextcpdef = curp->nextcpdef;
		    other->next = curp->next;
		    if ( curp->next!=NULL ) other->next->from = other;
		    SplineFree(curp->prev);
		} else {
		    SplinePoint *other = next;
		    other->prevcp = curp->prevcp;
		    other->noprevcp = curp->noprevcp;
		    other->prevcpdef = curp->prevcpdef;
		    other->prev = curp->prev;
		    if ( curp->prev!=NULL ) other->prev->to = other;
		    SplineFree(curp->next);
		}
		SplinePointFree(curp);
		if ( spl->first==curp ) {
		    spl->first = next;
		    if ( spl->last==curp )
			spl->last = next;
		} else if ( spl->last==curp )
		    spl->last = prev;
	    } else
		prev = curp;
	} else
	    prev = curp;
	if ( next==spl->first )
    break;
    }
}

SplineSet *SSRemoveZeroLengthSplines(SplineSet *base) {
    SplineSet *spl, *prev, *next;

    for ( prev = NULL, spl=base; spl!=NULL; spl=next ) {
	next = spl->next;
	if ( spl->first->next!=NULL && spl->first->next->to==spl->first &&
		spl->first->nonextcp && spl->first->noprevcp ) {
	    if ( prev==NULL )
		base = next;
	    else
		prev->next = next;
	    spl->next = NULL;
	    SplinePointListFree(spl);
	} else {
	    RemoveZeroLengthSplines(spl,false,0);
	    prev = spl;
	}
    }
return( base );
}

static void RemoveStupidControlPoints(SplineSet *spl) {
    double len, normal, dir;
    Spline *s, *first;
    BasePoint unit, off;

    /* Also remove really stupid control points: Tiny offsets pointing in */
    /*  totally the wrong direction. Some of the TeX fonts we get have these */
    /* We get equally bad results with a control point that points beyond the */
    /*  other end point */
    first = NULL;
    for ( s = spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	unit.x = s->to->me.x-s->from->me.x;
	unit.y = s->to->me.y-s->from->me.y;
	len = sqrt(unit.x*unit.x+unit.y*unit.y);
	if ( len!=0 ) {
	    int refigure = false;
	    unit.x /= len; unit.y /= len;
	    if ( !s->from->nonextcp ) {
		off.x = s->from->nextcp.x-s->from->me.x;
		off.y = s->from->nextcp.y-s->from->me.y;
		if ((normal = off.x*unit.y - off.y*unit.x)<0 ) normal = -normal;
		dir = off.x*unit.x + off.y*unit.y;
		if (( normal<dir && normal<1 && dir<0 ) || (normal<.5 && dir<-.5) ||
			(normal<.1 && dir>len)) {
		    s->from->nextcp = s->from->me;
		    s->from->nonextcp = true;
		    refigure = true;
		}
	    }
	    if ( !s->to->noprevcp ) {
		off.x = s->to->me.x - s->to->prevcp.x;
		off.y = s->to->me.y - s->to->prevcp.y;
		if ((normal = off.x*unit.y - off.y*unit.x)<0 ) normal = -normal;
		dir = off.x*unit.x + off.y*unit.y;
		if (( normal<-dir && normal<1 && dir<0 ) || (normal<.5 && dir>-.5 && dir<0) ||
			(normal<.1 && dir>len)) {
		    s->to->prevcp = s->to->me;
		    s->to->noprevcp = true;
		    refigure = true;
		}
	    }
	    if ( refigure )
		SplineRefigure(s);
	}
	if ( first==NULL ) first = s;
    }
}

void SSRemoveStupidControlPoints(SplineSet *base) {
    SplineSet *spl;

    for (spl=base; spl!=NULL; spl=spl->next )
	RemoveStupidControlPoints(spl);
}

static void OverlapClusterCpAngles(SplineSet *spl,double within) {
    double len, nlen, plen;
    double startoff, endoff;
    SplinePoint *sp, *nsp, *psp;
    BasePoint *nbp, *pbp;
    BasePoint pdir, ndir, fpdir, fndir;
    int nbad, pbad;

    /* If we have a junction point (right mid of 3) where the two splines */
    /*  are almost, but not quite moving in the same direction as they go */
    /*  away from the point, and if there is a tiny overlap because of this */
    /*  "almost" same, then we will probably get a bit confused in remove */
    /*  overlap */

    for ( sp = spl->first; ; ) {
	if ( sp->next==NULL )
    break;
	nsp = sp->next->to;
	if (( !sp->nonextcp || !sp->noprevcp ) && sp->prev!=NULL ) {
	    psp = sp->prev->from; 
	    nbp = !sp->nonextcp ? &sp->nextcp : !nsp->noprevcp ? &nsp->prevcp : &nsp->me;
	    pbp = !sp->noprevcp ? &sp->prevcp : !psp->nonextcp ? &psp->nextcp : &psp->me;

	    pdir.x = pbp->x-sp->me.x; pdir.y = pbp->y-sp->me.y;
	    ndir.x = nbp->x-sp->me.x; ndir.y = nbp->y-sp->me.y;
	    fpdir.x = psp->me.x-sp->me.x; fpdir.y = psp->me.y-sp->me.y;
	    fndir.x = nsp->me.x-sp->me.x; fndir.y = nsp->me.y-sp->me.y;

	    plen = sqrt(pdir.x*pdir.x+pdir.y*pdir.y);
	    if ( plen!=0 ) {
		pdir.x /= plen; pdir.y /= plen;
	    }

	    nlen = sqrt(ndir.x*ndir.x+ndir.y*ndir.y);
	    if ( nlen!=0 ) {
		ndir.x /= nlen; ndir.y /= nlen;
	    }

	    nbad = pbad = false;
	    if ( !sp->nonextcp && plen!=0 && nlen!=0 ) {
		len = sqrt(fndir.x*fndir.x+fndir.y*fndir.y);
		if ( len!=0 ) {
		    fndir.x /= len; fndir.y /= len;
		    startoff = ndir.x*pdir.y - ndir.y*pdir.x;
		    endoff = fndir.x*pdir.y - fndir.y*pdir.x;
		    if (( (startoff<0 && endoff>0) || (startoff>0 && endoff<0)) &&
			    startoff > -within && startoff < within )
			nbad = true;
		}
	    }
	    if ( !sp->noprevcp && plen!=0 && nlen!=0 ) {
		len = sqrt(fpdir.x*fpdir.x+fpdir.y*fpdir.y);
		if ( len!=0 ) {
		    fpdir.x /= len; fpdir.y /= len;
		    startoff = pdir.x*ndir.y - pdir.y*ndir.x;
		    endoff = fpdir.x*ndir.y - fpdir.y*ndir.x;
		    if (( (startoff<0 && endoff>0) || (startoff>0 && endoff<0)) &&
			    startoff > -within && startoff < within )
			pbad = true;
		}
	    }
	    if ( nbad && pbad ) {
		if ( ndir.x==0 || ndir.y==0 )
		    nbad = false;
		else if ( pdir.x==0 || pdir.y==0 )
		    pbad = false;
	    }
	    if ( nbad && pbad ) {
		if ( ndir.x*pdir.x + ndir.y*pdir.y > 0 ) {
		    ndir.x = pdir.x = (ndir.x + pdir.x)/2;
		    ndir.y = pdir.y = (ndir.x + pdir.x)/2;
		} else {
		    ndir.x = (ndir.x - pdir.x)/2;
		    ndir.y = (ndir.y - pdir.y)/2;
		    pdir.x = -ndir.x; pdir.y = -ndir.y;
		}
		sp->nextcp.x = sp->me.x + nlen*ndir.x;
		sp->nextcp.y = sp->me.y + nlen*ndir.y;
		sp->prevcp.x = sp->me.x + plen*pdir.x;
		sp->prevcp.y = sp->me.y + plen*pdir.y;
		SplineRefigure(sp->next); SplineRefigure(sp->prev);
	    } else if ( nbad ) {
		if ( ndir.x*pdir.x + ndir.y*pdir.y < 0 ) {
		    pdir.x = -pdir.x;
		    pdir.y = -pdir.y;
		}
		sp->nextcp.x = sp->me.x + nlen*pdir.x;
		sp->nextcp.y = sp->me.y + nlen*pdir.y;
		SplineRefigure(sp->next);
	    } else if ( pbad ) {
		if ( ndir.x*pdir.x + ndir.y*pdir.y < 0 ) {
		    ndir.x = -ndir.x;
		    ndir.y = -ndir.y;
		}
		sp->prevcp.x = sp->me.x + plen*ndir.x;
		sp->prevcp.y = sp->me.y + plen*ndir.y;
		SplineRefigure(sp->prev);
	    }
	}
	if ( nsp==spl->first )
    break;
	sp = nsp;
    }
}

void SSOverlapClusterCpAngles(SplineSet *base,double within) {
    SplineSet *spl;

    for (spl=base; spl!=NULL; spl=spl->next )
	OverlapClusterCpAngles(spl,within);
}

static SplinePointList *SplinePointListMerge(SplineChar *sc, SplinePointList *spl,int type) {
    Spline *spline, *first;
    SplinePoint *nextp, *curp, *selectme;
    int all;

    /* If the entire splineset is selected, it should merge into oblivion */
    first = NULL;
    all = spl->first->selected;
    for ( spline = spl->first->next; spline!=NULL && spline!=first && all; spline=spline->to->next ) {
	if ( !spline->to->selected ) all = false;
	if ( first==NULL ) first = spline;
    }
    if ( spl->first->next!=NULL && spl->first->next->to==spl->first &&
	    spl->first->nonextcp && spl->first->noprevcp )
	all = true;		/* Merge away any splines which are just dots */
    if ( all )
return( NULL );			/* Some one else should free it and reorder the spline set list */
    RemoveZeroLengthSplines(spl,true,.3);

    if ( spl->first!=spl->last ) {
	/* If the spline isn't closed, then any selected points at the ends */
	/*  get deleted */
	while ( spl->first->selected ) {
	    nextp = spl->first->next->to;
	    SplineFree(spl->first->next);
	    SplinePointMDFree(sc,spl->first);
	    spl->first = nextp;
	    nextp->prev = NULL;
	}
	while ( spl->last->selected ) {
	    nextp = spl->last->prev->from;
	    SplineFree(spl->last->prev);
	    SplinePointMDFree(sc,spl->last);
	    spl->last = nextp;
	    nextp->next = NULL;
	}
    } else {
	while ( spl->first->selected ) {
	    spl->first = spl->first->next->to;
	    spl->last = spl->first;
	}
    }

    /* when we get here spl->first is not selected */
    if ( spl->first->selected ) IError( "spl->first is selected in SplinePointListMerge");
    curp = spl->first;
    selectme = NULL;
    while ( 1 ) {
	while ( !curp->selected ) {
	    if ( curp->next==NULL )
		curp = NULL;
	    else
		curp = curp->next->to;
	    if ( curp==spl->first || curp==NULL )
	break;
	}
	if ( curp==NULL || !curp->selected )
    break;
	for ( nextp=curp->next->to; nextp->selected; nextp = nextp->next->to );
	/* we don't need to check for the end of the splineset here because */
	/*  we know that spl->last is not selected */
	SplinesRemoveBetween(sc,curp->prev->from,nextp,type);
	curp = nextp;
	selectme = nextp;
    }
    if ( selectme!=NULL ) selectme->selected = true;
return( spl );
}

void SplineCharMerge(SplineChar *sc,SplineSet **head,int type) {
    SplineSet *spl, *prev=NULL, *next;

    for ( spl = *head; spl!=NULL; spl = next ) {
	next = spl->next;
	if ( SplinePointListMerge(sc,spl,type)==NULL ) {
	    if ( prev==NULL )
		*head = next;
	    else
		prev->next = next;
	    chunkfree(spl,sizeof(*spl));
	} else
	    prev = spl;
    }
}

static int SPisExtremum(SplinePoint *sp) {
    BasePoint *prev, *next;
    SplinePoint *psp, *nsp;

    if ( sp->prev==NULL || sp->next==NULL )
return( true );

    nsp = sp->next->to;
    psp = sp->prev->from;

    /* A point that changes from curved to straight horizontal/vertical should*/
    /*  be treated as an extremum */
    if (( !sp->next->knownlinear && sp->prev->knownlinear &&
		(RealWithin(sp->me.x,sp->prev->from->me.x,.02) ||
		 RealWithin(sp->me.y,sp->prev->from->me.y,.02))) ||
	    ( !sp->prev->knownlinear && sp->next->knownlinear &&
		(RealWithin(sp->me.x,sp->next->to->me.x,.02) ||
		 RealWithin(sp->me.y,sp->next->to->me.y,.02))))
return( true );

    if ( sp->prev->knownlinear )
	prev = &psp->me;
    else if ( !sp->noprevcp )
	prev = &sp->prevcp;
    else
	prev = &psp->nextcp;
    if ( sp->next->knownlinear )
	next = &nsp->me;
    else if ( !sp->nonextcp )
	next = &sp->nextcp;
    else
	next = &nsp->prevcp;

    if ( sp->next->knownlinear && sp->prev->knownlinear &&
	    ((sp->me.x==nsp->me.x && sp->me.x==psp->me.x &&
	      ((sp->me.y<=nsp->me.y && psp->me.y<=sp->me.y) ||
	       (sp->me.y>=nsp->me.y && psp->me.y>=sp->me.y))) ||
	     (sp->me.y==nsp->me.y && sp->me.y==psp->me.y &&
	      ((sp->me.x<=nsp->me.x && psp->me.x<=sp->me.x) ||
	       (sp->me.x>=nsp->me.x && psp->me.x>=sp->me.x)))) )
return( false );	/* A point in the middle of a horizontal/vertical line */
			/*  is not an extrema and can be removed */

    if ( prev->x==sp->me.x && next->x==sp->me.x ) {
	if ( prev->y==sp->me.y && next->y==sp->me.y )
return( false );		/* this should be caught above */
#if 0	/* no matter what the control points look like this guy is either an */
	/*  an extremum or a point of inflection, so we don't need to check */
	if (( prev->y<=sp->me.y && sp->me.y <= next->y ) ||
		(prev->y>=sp->me.y && sp->me.y >= next->y ))
#endif
return( true );
    } else if ( prev->y==sp->me.y && next->y==sp->me.y ) {
return( true );
    } else if (( prev->x<=sp->me.x && next->x<=sp->me.x ) ||
	    (prev->x>=sp->me.x && next->x>=sp->me.x ))
return( true );
    else if (( prev->y<=sp->me.y && next->y<=sp->me.y ) ||
	    (prev->y>=sp->me.y && next->y>=sp->me.y ))
return( true );

return( false );
}

/* Almost exactly the same as SplinesRemoveBetween, but this one is conditional */
/*  the intermediate points/splines are removed only if we have a good match */
/*  used for simplify */
static int SplinesRemoveBetweenMaybe(SplineChar *sc,
	SplinePoint *from, SplinePoint *to, int flags, double err) {
    int i,tot;
    SplinePoint *afterfrom, *sp, *next;
    TPoint *tp, *tp2;
    BasePoint test;
    int good;
    BasePoint fncp, tpcp;
    int fpt, tpt;
    int order2 = from->next->order2;

    afterfrom = from->next->to;
    fncp = from->nextcp; tpcp = to->prevcp;
    fpt = from->pointtype; tpt = to->pointtype;

    if ( afterfrom==to || from==to )
return( false );

    tp = SplinesFigureTPsBetween(from,to,&tot);
    tp2 = galloc((tot+1)*sizeof(TPoint));
    memcpy(tp2,tp,tot*sizeof(TPoint));

    if ( !(flags&sf_ignoreslopes) )
	ApproximateSplineFromPointsSlopes(from,to,tp,tot-1,order2);
    else {
	ApproximateSplineFromPoints(from,to,tp,tot-1,order2);
    }

    i = tot;

    good = true;
    while ( --i>0 && good ) {
	/* tp[0] is the same as from (easier to include it), but the SplineNear*/
	/*  routine will sometimes reject the end-points of the spline */
	/*  so just don't check it */
	test.x = tp2[i].x; test.y = tp2[i].y;
	good = SplineNearPoint(from->next,&test,err)!= -1;
    }

    free(tp);
    free(tp2);
    if ( good ) {
	SplineFree(afterfrom->prev);
	for ( sp=afterfrom; sp!=to; sp=next ) {
	    next = sp->next->to;
	    SplineFree(sp->next);
	    SplinePointMDFree(sc,sp);
	}
	SplinePointCatagorize(from);
	SplinePointCatagorize(to);
    } else {
	SplineFree(from->next);
	from->next = afterfrom->prev;
	from->nextcp = fncp;
	from->nonextcp = ( fncp.x==from->me.x && fncp.y==from->me.y);
	from->pointtype = fpt;
	for ( sp=afterfrom; sp->next->to!=to; sp=sp->next->to );
	to->prev = sp->next;
	to->prevcp = tpcp;
	to->noprevcp = ( tpcp.x==to->me.x && tpcp.y==to->me.y);
	to->pointtype = tpt;
    }
return( good );
}

/* In truetype we can interpolate away an on curve point. Try this */
static int Spline2Interpolate(SplinePoint *mid, double err) {
    SplinePoint *from, *to;
    BasePoint midme, test;
    int i,tot, good;
    TPoint *tp;

    midme = mid->me;
    from = mid->prev->from; to = mid->next->to;
    tp = SplinesFigureTPsBetween(from,to,&tot);

    mid->me.x = (mid->prevcp.x + mid->nextcp.x)/2;
    mid->me.y = (mid->prevcp.y + mid->nextcp.y)/2;
    SplineRefigure(mid->next);
    SplineRefigure(mid->prev);

    i = tot;
    good = true;
    while ( --i>0 && good ) {
	/* tp[0] is the same as from (easier to include it), but the SplineNear*/
	/*  routine will sometimes reject the end-points of the spline */
	/*  so just don't check it */
	test.x = tp[i].x; test.y = tp[i].y;
	if ( i>tot/2 )
	    good = SplineNearPoint(mid->next,&test,err)!= -1 ||
		    SplineNearPoint(mid->prev,&test,err)!= -1;
	else
	    good = SplineNearPoint(mid->prev,&test,err)!= -1 ||
		    SplineNearPoint(mid->next,&test,err)!= -1;
    }
    if ( !good ) {
	mid->me = midme;
	SplineRefigure(mid->next);
	SplineRefigure(mid->prev);
    }
    free(tp);
return( good );
}

int SPInterpolate(SplinePoint *sp) {
    /* Using truetype rules, can we interpolate this point? */
return( !sp->dontinterpolate && !sp->nonextcp && !sp->noprevcp &&
	    !sp->roundx && !sp->roundy &&
	    (RealWithin(sp->me.x,(sp->nextcp.x+sp->prevcp.x)/2,.1) &&
	     RealWithin(sp->me.y,(sp->nextcp.y+sp->prevcp.y)/2,.1)) );
}

static int _SplinesRemoveMidMaybe(SplineChar *sc,SplinePoint *mid, int flags,
	double err, double lenmax2) {
    SplinePoint *from, *to;
#if 0		/* See comment below */
    BasePoint fncp, tpcp, fncp2, tpcp2;
    int fpt, tpt;
#endif

    if ( mid->prev==NULL || mid->next==NULL )
return( false );

    from = mid->prev->from; to = mid->next->to;

    /* Retain points which are horizontal or vertical, because postscript says*/
    /*  type1 fonts should always have a point at the extrema (except for small*/
    /*  things like serifs), and the extrema occur at horizontal/vertical points*/
    /* tt says something similar */
    if ( !(flags&sf_ignoreextremum) && SPisExtremum(mid) )
return( false );

    if ( !(flags&sf_mergelines) && (mid->pointtype==pt_corner ||
	    mid->prev->knownlinear || mid->next->knownlinear) ) {
	/* Be very careful about merging straight lines. Generally they should*/
	/*  remain straight... */
	/* Actually it's not that the lines are straight, the significant thing */
	/*  is that if there is a abrupt change in direction at this point */
	/*  (a corner) we don't want to get rid of it */
	BasePoint prevu, nextu;
	double plen, nlen;

	if ( mid->next->knownlinear || mid->nonextcp ) {
	    nextu.x = to->me.x-mid->me.x;
	    nextu.y = to->me.y-mid->me.y;
	} else {
	    nextu.x = mid->nextcp.x-mid->me.x;
	    nextu.y = mid->nextcp.y-mid->me.y;
	}
	if ( mid->prev->knownlinear || mid->noprevcp ) {
	    prevu.x = from->me.x-mid->me.x;
	    prevu.y = from->me.y-mid->me.y;
	} else {
	    prevu.x = mid->prevcp.x-mid->me.x;
	    prevu.y = mid->prevcp.y-mid->me.y;
	}
	nlen = sqrt(nextu.x*nextu.x + nextu.y*nextu.y);
	plen = sqrt(prevu.x*prevu.x + prevu.y*prevu.y);
	if ( nlen==0 || plen==0 )
	    /* Not a real corner */;
	else if ( (nextu.x*prevu.x + nextu.y*prevu.y)/(nlen*plen)>((nlen+plen>20)?-.98:-.95) ) {
	    /* If the cos if the angle between the two segments is too far */
	    /*  from parallel then don't try to smooth the point into oblivion */
	    double flen, tlen;
	    flen = (from->me.x-mid->me.x)*(from->me.x-mid->me.x) +
		    (from->me.y-mid->me.y)*(from->me.y-mid->me.y);
	    tlen = (to->me.x-mid->me.x)*(to->me.x-mid->me.x) +
		    (to->me.y-mid->me.y)*(to->me.y-mid->me.y);
	    if ( (flen<.7 && tlen<.7) || flen<.25 || tlen<.25 )
		/* Too short to matter */;
	    else
return( false );
	}

	if ( mid->prev->knownlinear && mid->next->knownlinear ) {
	    /* Special checks for horizontal/vertical lines */
	    /* don't let the smoothing distort them */
	    if ( from->me.x==to->me.x ) {
		if ( mid->me.x!=to->me.x )
return( false );
	    } else if ( from->me.y==to->me.y ) {
		if ( mid->me.y!=to->me.y )
return( false );
	    } else if ( !RealRatio((from->me.y-to->me.y)/(from->me.x-to->me.x),
				(mid->me.y-to->me.y)/(mid->me.x-to->me.x),
			        .05) ) {
return( false );
	    }
	} else if ( mid->prev->knownlinear ) {
	    if ( (mid->me.x-from->me.x)*(mid->me.x-from->me.x) + (mid->me.y-from->me.y)*(mid->me.y-from->me.y)
		    > lenmax2 )
return( false );
	} else {
	    if ( (mid->me.x-to->me.x)*(mid->me.x-to->me.x) + (mid->me.y-to->me.y)*(mid->me.y-to->me.y)
		    > lenmax2 )
return( false );
	}
    }

#if 0		/* Used to try to enforce tangentness of a point */
		/*  I now think it's better to change the point to a curve */
    fncp2 = fncp = from->nextcp; tpcp2 = tpcp = to->prevcp;
    fpt = from->pointtype; tpt = to->pointtype;

    /* if from or to is a tangent then we can only remove mid if it's on the */
    /*  line between them */
    if (( from->pointtype==pt_tangent && !from->noprevcp) ||
	    ( to->pointtype==pt_tangent && !to->nonextcp)) {
	if ( from->me.x==to->me.x ) {
	    if ( mid->me.x!=to->me.x )
return( false );
	} else if ( !RealNear((from->me.y-to->me.y)/(from->me.x-to->me.x),
			    (mid->me.y-to->me.y)/(mid->me.x-to->me.x)) )
return( false );
    }
#endif

    if ( mid->next->order2 ) {
	if ( SPInterpolate(from) && SPInterpolate(to) && SPInterpolate(mid))
return( false );
    }

return ( SplinesRemoveBetweenMaybe(sc,from,to,flags,err));
}

/* A wrapper to SplinesRemoveBetweenMaybe to handle some extra checking for a */
/*  common case */
static int SplinesRemoveMidMaybe(SplineChar *sc,SplinePoint *mid, int flags,
	double err, double lenmax2) {
    int changed1 = false;
    if ( mid->next->order2 ) {
	if ( !mid->dontinterpolate && !mid->nonextcp && !mid->noprevcp &&
		!(RealWithin(mid->me.x,(mid->nextcp.x+mid->prevcp.x)/2,.1) &&
		  RealWithin(mid->me.y,(mid->nextcp.y+mid->prevcp.y)/2,.1)) )
	  changed1 = Spline2Interpolate(mid,err);
    }
return( _SplinesRemoveMidMaybe(sc,mid,flags,err,lenmax2) || changed1 );
}

#ifndef LUA_FF_LIB
void SPLNearlyHvCps(SplineChar *sc,SplineSet *ss,double err) {
    Spline *s, *first=NULL;
    int refresh;
    SplinePoint *from, *to;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	refresh = false;
	from = s->from; to = s->to;
	if ( !from->nonextcp && from->nextcp.x-from->me.x<err && from->nextcp.x-from->me.x>-err ) {
	    from->nextcp.x = from->me.x;
	    if ( s->order2 ) to->prevcp = from->nextcp;
	    if ( from->nextcp.y==from->me.y ) from->nonextcp = true;
	    refresh = true;
	} else if ( !from->nonextcp && from->nextcp.y-from->me.y<err && from->nextcp.y-from->me.y>-err ) {
	    from->nextcp.y = from->me.y;
	    if ( s->order2 ) to->prevcp = from->nextcp;
	    if ( from->nextcp.x==from->me.x ) from->nonextcp = true;
	    refresh = true;
	}
	if ( !to->noprevcp && to->prevcp.x-to->me.x<err && to->prevcp.x-to->me.x>-err ) {
	    to->prevcp.x = to->me.x;
	    if ( s->order2 ) from->nextcp = to->prevcp;
	    if ( to->prevcp.y==to->me.y ) to->noprevcp = true;
	    refresh = true;
	} else if ( !to->noprevcp && to->prevcp.y-to->me.y<err && to->prevcp.y-to->me.y>-err ) {
	    to->prevcp.y = to->me.y;
	    if ( s->order2 ) from->nextcp = to->prevcp;
	    if ( to->prevcp.x==to->me.x ) to->noprevcp = true;
	    refresh = true;
	}
	if ( refresh )
	    SplineRefigure(s);
    }
}
#endif

void SPLNearlyHvLines(SplineChar *sc,SplineSet *ss,double err) {
    Spline *s, *first=NULL;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	if ( s->knownlinear ) {
	    if ( s->to->me.x-s->from->me.x<err && s->to->me.x-s->from->me.x>-err ) {
		s->to->nextcp.x += (s->from->me.x-s->to->me.x);
		s->to->me.x = s->from->me.x;
		s->to->prevcp = s->to->me;
		s->from->nextcp = s->from->me;
		SplineRefigure(s);
		if ( s->to->next != NULL )
		    SplineRefigure(s->to->next);
	    } else if ( s->to->me.y-s->from->me.y<err && s->to->me.y-s->from->me.y>-err ) {
		s->to->nextcp.y += (s->from->me.y-s->to->me.y);
		s->to->me.y = s->from->me.y;
		s->to->prevcp = s->to->me;
		s->from->nextcp = s->from->me;
		SplineRefigure(s);
		if ( s->to->next != NULL )
		    SplineRefigure(s->to->next);
	    }
	}
    }
}

#ifndef LUA_FF_LIB
/* Does this spline deviate from a straight line between its endpoints by more*/
/*  than err?								      */
/* Rotate the spline so that the line between the endpoints is horizontal,    */
/*  then find the maxima/minima of the y spline (this is the deviation)	      */
/*  check that that is less than err					      */
static int SplineCloseToLinear(Spline *s, double err) {
    double angle;
    extended co,si, t1, t2, y;
    SplinePoint from, to;
    Spline sp;
    BasePoint bp;

    from = *s->from; to = *s->to;
    to.me.x -= from.me.x; to.me.y -= from.me.y;
    to.prevcp.x -= from.me.x; to.prevcp.y -= from.me.y;
    from.nextcp.x -= from.me.x ; from.nextcp.y -= from.me.y;
    from.me.x = from.me.y = 0;
    angle = atan2(to.me.y, to.me.x);

    si = sin(angle); co = cos(angle);
    bp.x =  to.me.x*co + to.me.y*si;
    bp.y = -to.me.x*si + to.me.y*co;
    to.me = bp;

    bp.x =  to.prevcp.x*co + to.prevcp.y*si;
    bp.y = -to.prevcp.x*si + to.prevcp.y*co;
    to.prevcp = bp;

    bp.x =  from.nextcp.x*co + from.nextcp.y*si;
    bp.y = -from.nextcp.x*si + from.nextcp.y*co;
    from.nextcp = bp;

    memset(&sp,0,sizeof(Spline));
    sp.from = &from; sp.to = &to;
    sp.order2 = s->order2;
    SplineRefigure(&sp);
    SplineFindExtrema(&sp.splines[1],&t1,&t2);

    if ( t1==-1 )
return( true );

    y = ((sp.splines[1].a*t1 + sp.splines[1].b)*t1 + sp.splines[1].c)*t1 + sp.splines[1].d;
    if ( y>err || y<-err )
return( false );

    if ( t2==-1 )
return( true );
    y = ((sp.splines[1].a*t2 + sp.splines[1].b)*t2 + sp.splines[1].c)*t2 + sp.splines[1].d;
return( y<=err && y>=-err );
}

int SPLNearlyLines(SplineChar *sc,SplineSet *ss,double err) {
    Spline *s, *first=NULL;
    int changed = false;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	if ( s->islinear )
	    /* Nothing to be done */;
	else if ( s->knownlinear || SplineCloseToLinear(s,err)) {
	    s->from->nextcp = s->from->me;
	    s->from->nonextcp = true;
	    s->to->prevcp = s->to->me;
	    s->to->noprevcp = true;
	    SplineRefigure(s);
	    changed = true;
	}
    }
return( changed );
}
#endif

static void SPLForceLines(SplineChar *sc,SplineSet *ss,double bump_size) {
    Spline *s, *first=NULL;
    SplinePoint *sp;
    int any;
    BasePoint unit;
    double len, minlen = (sc->parent->ascent+sc->parent->descent)/20.0;
    double diff, xoff, yoff, len2;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	if ( s->knownlinear ) {
	    unit.x = s->to->me.x-s->from->me.x;
	    unit.y = s->to->me.y-s->from->me.y;
	    len = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( len<minlen )
    continue;
	    unit.x /= len; unit.y /= len;
	    do {
		any = false;
		if ( s->from->prev!=NULL && s->from->prev!=s ) {
		    sp = s->from->prev->from;
		    len2 = sqrt((sp->me.x-s->from->me.x)*(sp->me.x-s->from->me.x) + (sp->me.y-s->from->me.y)*(sp->me.y-s->from->me.y));
		    diff = (sp->me.x-s->from->me.x)*unit.y - (sp->me.y-s->from->me.y)*unit.x;
		    if ( len2<len && fabs(diff)<=bump_size ) {
			xoff = diff*unit.y; yoff = -diff*unit.x;
			sp->me.x -= xoff; sp->me.y -= yoff;
			sp->prevcp.x -= xoff; sp->prevcp.y -= yoff;
			sp->nextcp = sp->me; sp->nonextcp = true;
			if ( sp->next==first ) first = NULL;
			SplineFree(sp->next);
			if ( s->from==ss->first ) {
			    if ( ss->first==ss->last ) ss->last = sp;
			    ss->first = sp;
			}
			SplinePointMDFree(sc,s->from);
			sp->next = s; s->from = sp;
			SplineRefigure(s);
			if ( sp->prev!=NULL )
			    SplineRefigure(sp->prev);
			sp->pointtype = pt_corner;
			any = true;

			/* We must recalculate the length each time, we */
			/*  might have shortened it. */
			unit.x = s->to->me.x-s->from->me.x;
			unit.y = s->to->me.y-s->from->me.y;
			len = sqrt(unit.x*unit.x + unit.y*unit.y);
			if ( len<minlen )
	    break;
			unit.x /= len; unit.y /= len;
		    }
		}
		if ( s->to->next!=NULL && s->to->next!=s ) {
		    sp = s->to->next->to;
		    /* If the next spline is a longer line than we are, then don't */
		    /*  merge it to us, rather merge us into it next time through the loop */
		    /* Hmm. Don't merge out the bump in general if the "bump" is longer than we are */
		    len2 = sqrt((sp->me.x-s->to->me.x)*(sp->me.x-s->to->me.x) + (sp->me.y-s->to->me.y)*(sp->me.y-s->to->me.y));
		    diff = (sp->me.x-s->to->me.x)*unit.y - (sp->me.y-s->to->me.y)*unit.x;
		    if ( len2<len && fabs(diff)<=bump_size ) {
			xoff = diff*unit.y; yoff = -diff*unit.x;
			sp->me.x -= xoff; sp->me.y -= yoff;
			sp->nextcp.x -= xoff; sp->nextcp.y -= yoff;
			sp->prevcp = sp->me; sp->noprevcp = true;
			if ( sp->prev==first ) first = NULL;
			SplineFree(sp->prev);
			if ( s->to==ss->last ) {
			    if ( ss->first==ss->last ) ss->first = sp;
			    ss->last = sp;
			}
			SplinePointMDFree(sc,s->to);
			sp->prev = s; s->to = sp;
			SplineRefigure(s);
			if ( sp->next!=NULL )
			    SplineRefigure(sp->next);
			sp->pointtype = pt_corner;
			any = true;

			unit.x = s->to->me.x-s->from->me.x;
			unit.y = s->to->me.y-s->from->me.y;
			len = sqrt(unit.x*unit.x + unit.y*unit.y);
			if ( len<minlen )
	    break;
			unit.x /= len; unit.y /= len;
		    }
		}
	    } while ( any );
	}
    }
}

static int SPLSmoothControlPoints(SplineSet *ss,double tan_bounds,int vert_check) {
    SplinePoint *sp;
    /* If a point has control points, and if those cps are in nearly the same */
    /*  direction (within tan_bounds) then adjust them so that they are in the*/
    /*  same direction */
    BasePoint unit, unit2;
    double len, len2, para, norm, tn;
    int changed=false, found;

    for ( sp = ss->first; ; ) {
	if (( !sp->nonextcp && !sp->noprevcp && sp->pointtype==pt_corner ) ||
		((sp->pointtype==pt_corner || sp->pointtype==pt_curve) &&
		 (( !sp->nonextcp && sp->noprevcp && sp->prev!=NULL && sp->prev->knownlinear ) ||
		  ( !sp->noprevcp && sp->nonextcp && sp->next!=NULL && sp->next->knownlinear )))) {
	    BasePoint *next = sp->nonextcp ? &sp->next->to->me : &sp->nextcp;
	    BasePoint *prev = sp->noprevcp ? &sp->prev->to->me : &sp->prevcp;
	    unit.x = next->x-sp->me.x;
	    unit.y = next->y-sp->me.y;
	    len = sqrt(unit.x*unit.x + unit.y*unit.y);
	    unit.x /= len; unit.y /= len;
	    para = (sp->me.x-prev->x)*unit.x + (sp->me.y-prev->y)*unit.y;
	    norm = (sp->me.x-prev->x)*unit.y - (sp->me.y-prev->y)*unit.x;
	    if ( para==0 )
		tn = 1000;
	    else
		tn = norm/para;
	    if ( tn<0 ) tn = -tn;
	    if ( tn<tan_bounds && para>0 ) {
		found = 0;
		unit2.x = sp->me.x-sp->prevcp.x;
		unit2.y = sp->me.y-sp->prevcp.y;
		len2 = sqrt(unit2.x*unit2.x + unit2.y*unit2.y);
		unit2.x /= len2; unit2.y /= len2;
		if ( vert_check ) {
		    if ( fabs(unit.x)>fabs(unit.y) ) {
			/* Closer to horizontal */
			if ( (unit.y<=0 && unit2.y>=0) || (unit.y>=0 && unit2.y<=0) ) {
			    unit2.x = unit2.x<0 ? -1 : 1; unit2.y = 0;
			    found = 1;
			}
		    } else {
			if ( (unit.x<=0 && unit2.x>=0) || (unit.x>=0 && unit2.x<=0) ) {
			    unit2.y = unit2.y<0 ? -1 : 1; unit2.x = 0;
			    found = 1;
			}
		    }
		}
		/* If we're next to a line, we must extend the line. No choice */
		if ( sp->nonextcp ) {
		    if ( len<len2 )
    goto nextpt;
		    found = true;
		    unit2 = unit;
		} else if ( sp->noprevcp ) {
		    if ( len2<len )
    goto nextpt;
		    found = true;
		} else if ( !found ) {
		    unit2.x = (unit.x*len + unit2.x*len2)/(len+len2);
		    unit2.y = (unit.y*len + unit2.y*len2)/(len+len2);
		}
		sp->nextcp.x = sp->me.x + len*unit2.x;
		sp->nextcp.y = sp->me.y + len*unit2.y;
		sp->prevcp.x = sp->me.x - len2*unit2.x;
		sp->prevcp.y = sp->me.y - len2*unit2.y;
		sp->pointtype = pt_curve;
		if ( sp->prev )
		    SplineRefigure(sp->prev);
		if ( sp->next )
		    SplineRefigure(sp->next);
		changed = true;
	    }
	}
    nextpt:
	if ( sp->next==NULL )
    break;
	sp = sp->next->to;
	if ( sp==ss->first )
    break;
    }
return( changed );
}

static void GetNextUnitVector(SplinePoint *sp,BasePoint *uv) {
    double len;

    if ( sp->next==NULL ) {
	uv->x = uv->y = 0;
    } else if ( sp->next->knownlinear ) {
	uv->x = sp->next->to->me.x - sp->me.x;
	uv->y = sp->next->to->me.y - sp->me.y;
    } else if ( sp->nonextcp ) {
	uv->x = sp->next->to->prevcp.x - sp->me.x;
	uv->y = sp->next->to->prevcp.y - sp->me.y;
    } else {
	uv->x = sp->nextcp.x - sp->me.x;
	uv->y = sp->nextcp.y - sp->me.y;
    }
    len = sqrt(uv->x*uv->x + uv->y*uv->y );
    if ( len!= 0 ) {
	uv->x /= len;
	uv->y /= len;
    }
}

static int isExtreme(SplinePoint *sp) {
    if ( sp->next==NULL || sp->prev==NULL )
return( 2 );		/* End point. Always extreme */
    if ( sp->nonextcp ) {
	if ( !sp->next->knownlinear )
return( false );
	if ( sp->next->to->me.x == sp->me.x || sp->next->to->me.y == sp->me.y )
	    /* Ok, horizontal or vertical line, that'll do */;
	else
return( false );
    } else {
	if ( sp->nextcp.x != sp->me.x && sp->nextcp.y != sp->me.y )
return( false );
    }
    if ( sp->noprevcp ) {
	if ( !sp->prev->knownlinear )
return( false );
	if ( sp->prev->from->me.x == sp->me.x || sp->prev->from->me.y == sp->me.y )
	    /* Ok, horizontal or vertical line, that'll do */;
	else
return( false );
    } else {
	if ( sp->prevcp.x != sp->me.x && sp->prevcp.y != sp->me.y )
return( false );
    }
return( true );
}

/* If the start point of a contour is not at an extremum, and the contour has */
/*  a point which is at an extremum, then make the start point be that point  */
/* leave it unchanged if start point is already extreme, or no extreme point  */
/*  could be found							      */
static void SPLStartToExtremum(SplineChar *sc,SplinePointList *spl) {
    SplinePoint *sp;

    while ( spl!=NULL ) {
	if ( spl->first==spl->last ) {		/* It's closed */
	    for ( sp=spl->first; !isExtreme(sp); ) {
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    if ( sp!=spl->first )
		spl->first = spl->last = sp;
	}
	spl = spl->next;
    }
}

/* Cleanup just turns splines with control points which happen to trace out */
/*  lines into simple lines */
/* it also checks for really nasty control points which point in the wrong */
/*  direction but are very close to the base point. We get these from some */
/*  TeX fonts. I assume they are due to rounding errors (or just errors) in*/
/*  some autotracer */
void SplinePointListSimplify(SplineChar *sc,SplinePointList *spl,
	struct simplifyinfo *smpl) {
    SplinePoint *first, *next, *sp, *nsp;
    BasePoint suv, nuv;
    double lenmax2 = smpl->linelenmax*smpl->linelenmax;

    if ( spl==NULL )
return;

    RemoveZeroLengthSplines(spl,false,0.1);
    RemoveStupidControlPoints(spl);
    SSRemoveBacktracks(spl);
    if ( smpl->flags!=sf_cleanup && (smpl->flags&sf_setstart2extremum))
	SPLStartToExtremum(sc,spl);
    if ( spl->first->next!=NULL && spl->first->next->to==spl->first &&
	    spl->first->nonextcp && spl->first->noprevcp )
return;		/* Ignore any splines which are just dots */

    if ( smpl->flags!=sf_cleanup && (smpl->flags&sf_forcelines)) {
	SPLNearlyHvLines(sc,spl,smpl->linefixup);
	SPLForceLines(sc,spl,smpl->linefixup);
    }

    if ( smpl->flags!=sf_cleanup && spl->first->prev!=NULL && spl->first->prev!=spl->first->next ) {
	/* first thing to try is to remove everything between two extrema */
	/* We do this even if they checked ignore extrema. After this pass */
	/*  we'll come back and check every point individually */
	/* However, if we turn through more than 90 degrees we can't approximate */
	/*  a good match, and it takes us forever to make the attempt and fail*/
	/*  We take a dot product to prevent that */
	for ( sp = spl->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    if ( SPisExtremum(sp) ) {
		GetNextUnitVector(sp,&suv);
		for ( nsp=sp->next->to; nsp!=sp; nsp = nsp->next->to ) {
		    if ( nsp->next==NULL )
		break;
		    if ( nsp->prev->knownlinear &&
			    (nsp->me.x-nsp->prev->from->me.x)*(nsp->me.x-nsp->prev->from->me.x) +
			    (nsp->me.y-nsp->prev->from->me.y)*(nsp->me.y-nsp->prev->from->me.y)
			    >= lenmax2 )
	      goto nogood;
		    GetNextUnitVector(nsp,&nuv);
		    if ( suv.x*nuv.x + suv.y*nuv.y < 0 ) {
			if ( suv.x*nuv.x + suv.y*nuv.y > -.1 )
		break;
	      goto nogood;
		    }
		    if ( SPisExtremum(nsp) || nsp==spl->first)
		break;
		}
		/* nsp is something we don't want to remove */
		if ( nsp==sp )
	break;
		else if ( sp->next->to == nsp )
	      goto nogood;		/* Nothing to remove */
		if ( SplinesRemoveBetweenMaybe(sc,sp,nsp,smpl->flags,smpl->err)) {
		    if ( spl->last==spl->first )
			spl->last = spl->first = sp;	/* We know this point didn't get removed */
		}
	      nogood:
		sp = nsp;
	    } else
		sp = sp->next->to;
	    if ( sp == spl->first )
	break;
	}

	while ( 1 ) {
	    first = spl->first->prev->from;
	    if ( first->prev == first->next )
return;
	    if ( !SplinesRemoveMidMaybe(sc,spl->first,smpl->flags,smpl->err,lenmax2))
	break;
	    if ( spl->first==spl->last )
		spl->last = first;
	    spl->first = first;
	}
    }

	/* Special case checks for paths containing only one point */
	/*  else we get lots of nans (or only two points) */
    if ( spl->first->next == NULL )
return;
    for ( sp = spl->first->next->to; sp->next!=NULL; sp = next ) {
	SplineIsLinearMake(sp->prev);		/* First see if we can turn it*/
				/* into a line, then try to merge two splines */
	next = sp->next->to;
	if ( sp->prev == sp->next ||
		(sp->next!=NULL && sp->next->to->next!=NULL &&
		    sp->next->to->next->to == sp ))
return;
	if ( smpl->flags!=sf_cleanup ) {
	    if ( SplinesRemoveMidMaybe(sc,sp,smpl->flags,smpl->err,lenmax2) ) {
		if ( spl->first==sp )
		    spl->first = next;
		if ( spl->last==sp )
		    spl->last = next;
	    }
	} else {
	    while ( sp->me.x==next->me.x && sp->me.y==next->me.y &&
		    sp->nextcp.x>sp->me.x-1 && sp->nextcp.x<sp->me.x+1 &&
		    sp->nextcp.y>sp->me.y-1 && sp->nextcp.y<sp->me.y+1 &&
		    next->prevcp.x>next->me.x-1 && next->prevcp.x<next->me.x+1 &&
		    next->prevcp.y>next->me.y-1 && next->prevcp.y<next->me.y+1 ) {
		SplineFree(sp->next);
		sp->next = next->next;
		if ( sp->next!=NULL )
		    sp->next->from = sp;
		sp->nextcp = next->nextcp;
		sp->nonextcp = next->nonextcp;
		sp->nextcpdef = next->nextcpdef;
		SplinePointMDFree(sc,next);
		if ( sp->next!=NULL )
		    next = sp->next->to;
		else {
		    next = NULL;
	    break;
		}
	    }
	    if ( next==NULL )
    break;
	}
	if ( next->prev!=NULL && next->prev->from==spl->last )
    break;
    }
    if ( smpl->flags!=sf_cleanup && (smpl->flags&sf_smoothcurves))
	SPLSmoothControlPoints(spl,smpl->tan_bounds,smpl->flags&sf_choosehv);
}

/* cleanup may be: -1 => lines become lines, 0 => simplify & retain slopes, 1=> simplify and discard slopes, 2=>discard extrema */
SplineSet *SplineCharSimplify(SplineChar *sc,SplineSet *head,
	struct simplifyinfo *smpl) {
    SplineSet *spl, *prev, *snext;
    int anysel=0, wassingleton;

    if ( smpl->check_selected_contours ) {
	for ( spl = head; spl!=NULL && !anysel; spl = spl->next ) {
	    anysel = PointListIsSelected(spl);
	}
    }

    prev = NULL;
    for ( spl = head; spl!=NULL; spl = snext ) {
	snext = spl->next;
	if ( !anysel || PointListIsSelected(spl)) {
	    wassingleton = spl->first->prev==spl->first->next &&
		    (spl->first->prev==NULL ||
		     (spl->first->noprevcp && spl->first->nonextcp));
	    SplinePointListSimplify(sc,spl,smpl);
	    /* remove any singleton points */
	    if (( !wassingleton ||
		    (smpl->flags!=sf_cleanup && (smpl->flags&sf_rmsingletonpoints))) &&
		   spl->first->prev==spl->first->next &&
		   (spl->first->prev==NULL ||
		     (spl->first->noprevcp && spl->first->nonextcp))) {
		if ( prev==NULL )
		    head = snext;
		else
		    prev->next = snext;
		spl->next = NULL;
		SplinePointListMDFree(sc,spl);
	    } else
		prev = spl;
	}
    }
    SplineSetsRemoveAnnoyingExtrema(head,.3);
    SPLCatagorizePoints(head);
#if 0
 printf( "nocnt=%d totcnt=%d curdif=%d incr=%d\n", nocnt_cnt, totcnt_cnt, curdiff_cnt, incr_cnt );
#endif
return( head );
}

#ifndef LUA_FF_LIB
/* If the start point of a contour to be the left most point on it.  If there */
/*  are several points with that same value, use the one closest to the base */
/*  line */
void SPLStartToLeftmost(SplineChar *sc,SplinePointList *spl, int *changed) {
    SplinePoint *sp;
    SplinePoint *best;

    if ( spl->first==spl->last ) {		/* It's closed */
	best = spl->first;
	for ( sp=spl->first; ; ) {
	    if ( sp->me.x < best->me.x ||
		    (sp->me.x==best->me.x && (fabs(sp->me.y)<fabs(best->me.y))) )
		best = sp;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( best!=spl->first ) {
	    if ( !*changed ) {
		SCPreserveState(sc,false);
		*changed = true;
	    }
	    spl->first = spl->last = best;
	}
    }
}

void SPLsStartToLeftmost(SplineChar *sc) {
    int changed = 0;
    int layer;
    SplineSet *ss;

    if ( sc==NULL )
return;

    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next )
	    SPLStartToLeftmost(sc,ss,&changed);
    }
    if ( changed )
	SCCharChangedUpdate(sc);
}

struct contourinfo {
    SplineSet *ss;
    BasePoint *min;
};

static int order_contours(const void *_c1, const void *_c2) {
    const struct contourinfo *c1 = _c1, *c2 = _c2;

    if ( c1->min->x<c2->min->x )
return( -1 );
    else if ( c1->min->x>c2->min->x )
return( 1 );
    else if ( fabs(c1->min->y)<fabs(c2->min->y) )
return( -1 );
    else if ( fabs(c1->min->y)>fabs(c2->min->y) )
return( 1 );
    else
return( 0 );
}

void CanonicalContours(SplineChar *sc) {
    int changed;
    int layer;
    SplineSet *ss;
    SplinePoint *sp, *best;
    int contour_cnt, contour_max=0, i, diff;
    struct contourinfo *ci;

    if ( sc==NULL )
return;

    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	contour_cnt = 0;
	for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next )
	    ++contour_cnt;
	if ( contour_cnt>contour_max )
	    contour_max = contour_cnt;
    }

    if ( contour_max<=1 )	/* Can't be out of order */
return;

    changed = 0;
    ci = gcalloc(contour_max,sizeof(struct contourinfo));
    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	contour_cnt = 0;
	for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	    best = ss->first;
	    for ( sp=ss->first; ; ) {
		if ( sp->me.x < best->me.x ||
			(sp->me.x==best->me.x && (fabs(sp->me.y)<fabs(best->me.y))) )
		    best = sp;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	    ci[contour_cnt].ss = ss;
	    ci[contour_cnt++].min = &best->me;
	}
	qsort(ci,contour_cnt,sizeof(struct contourinfo),order_contours);
	diff = 0;
	for ( i=0, ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	    if ( ci[i].ss != ss ) {
		diff = true;
	break;
	    }
	}
	if ( diff && !changed ) {
	    SCPreserveState(sc,false);
	    changed = true;
	}
	if ( diff ) {
	    sc->layers[layer].splines = ci[0].ss;
	    for ( i=1; i<contour_cnt; ++i )
		ci[i-1].ss->next = ci[i].ss;
	    ci[contour_cnt-1].ss->next = NULL;
	}
    }
    free(ci);
    if ( changed )
	SCCharChangedUpdate(sc);
}
#endif

static int SplineSetMakeLoop(SplineSet *spl,real fudge) {
    if ( spl->first!=spl->last &&
	    (spl->first->me.x >= spl->last->me.x-fudge &&
		spl->first->me.x <= spl->last->me.x+fudge &&
		spl->first->me.y >= spl->last->me.y-fudge &&
		spl->first->me.y <= spl->last->me.y+fudge )) {
	spl->first->prev = spl->last->prev;
	spl->first->prev->to = spl->first;
	spl->first->prevcp = spl->last->prevcp;
	spl->first->noprevcp = spl->last->noprevcp;
	spl->first->prevcpdef = spl->last->prevcpdef;
	SplinePointFree(spl->last);
	spl->last = spl->first;
return( true );
    }
return( false );
}

SplineSet *SplineSetJoin(SplineSet *start,int doall,real fudge,int *changed) {
    SplineSet *spl, *spl2, *prev;

    *changed = false;
    for ( spl=start; spl!=NULL; spl=spl->next ) {
	if ( spl->first->prev==NULL &&
		(doall || PointListIsSelected(spl)) ) {
	    if ( SplineSetMakeLoop(spl,fudge) ) {
		*changed = true;
	    } else {
		prev = NULL;
		for ( spl2=start ; spl2!=NULL; prev = spl2, spl2=spl2->next ) if ( spl2!=spl ) {
		    if (!( spl->first->me.x >= spl2->last->me.x-fudge &&
			    spl->first->me.x <= spl2->last->me.x+fudge &&
			    spl->first->me.y >= spl2->last->me.y-fudge &&
			    spl->first->me.y <= spl2->last->me.y+fudge )) {
			if (( spl->last->me.x >= spl2->last->me.x-fudge &&
				spl->last->me.x <= spl2->last->me.x+fudge &&
				spl->last->me.y >= spl2->last->me.y-fudge &&
				spl->last->me.y <= spl2->last->me.y+fudge ) ||
			    ( spl->last->me.x >= spl2->first->me.x-fudge &&
				spl->last->me.x <= spl2->first->me.x+fudge &&
				spl->last->me.y >= spl2->first->me.y-fudge &&
				spl->last->me.y <= spl2->first->me.y+fudge ))
			    SplineSetReverse(spl);
		    }
		    if ( spl->first->me.x >= spl2->first->me.x-fudge &&
			    spl->first->me.x <= spl2->first->me.x+fudge &&
			    spl->first->me.y >= spl2->first->me.y-fudge &&
			    spl->first->me.y <= spl2->first->me.y+fudge )
			SplineSetReverse(spl2);
		    if ( spl->first->me.x >= spl2->last->me.x-fudge &&
			    spl->first->me.x <= spl2->last->me.x+fudge &&
			    spl->first->me.y >= spl2->last->me.y-fudge &&
			    spl->first->me.y <= spl2->last->me.y+fudge ) {
			spl->first->prev = spl2->last->prev;
			spl->first->prev->to = spl->first;
			spl->first->prevcp = spl2->last->prevcp;
			spl->first->noprevcp = spl2->last->noprevcp;
			spl->first->prevcpdef = spl2->last->prevcpdef;
			SplinePointFree(spl2->last);
			spl->first = spl2->first;
			spl2->first = spl2->last = NULL;
			if ( prev!=NULL )
			    prev->next = spl2->next;
			else
			    start = spl2->next;
			chunkfree(spl2,sizeof(SplineSet));
			SplineSetMakeLoop(spl,fudge);
			*changed = true;
		break;
		    }
		}
	    }
	}
    }
return(start);
}

#ifndef LUA_FF_LIB
SplineSet *SplineCharRemoveTiny(SplineChar *sc,SplineSet *head) {
    SplineSet *spl, *snext, *pr;
    Spline *spline, *next, *first;
    const double err = 1.0/64.0;

    for ( spl = head, pr=NULL; spl!=NULL; spl = snext ) {
	first = NULL;
	for ( spline=spl->first->next; spline!=NULL && spline!=first; spline=next ) {
	    next = spline->to->next;
	    if ( spline->from->me.x-spline->to->me.x>-err && spline->from->me.x-spline->to->me.x<err &&
		    spline->from->me.y-spline->to->me.y>-err && spline->from->me.y-spline->to->me.y<err &&
		    (spline->from->nonextcp || spline->to->noprevcp) &&
		    spline->from->prev!=NULL ) {
		if ( spline->from==spline->to )
	    break;
		if ( spl->last==spline->from ) spl->last = NULL;
		if ( spl->first==spline->from ) spl->first = NULL;
		if ( first==spline->from->prev ) first=NULL;
		/*SplinesRemoveBetween(sc,spline->from->prev->from,spline->to);*/
		spline->to->prevcp = spline->from->prevcp;
		spline->to->noprevcp = spline->from->noprevcp;
		spline->to->prevcpdef = spline->from->prevcpdef;
		spline->from->prev->to = spline->to;
		spline->to->prev = spline->from->prev;
		SplineRefigure(spline->from->prev);
		SplinePointFree(spline->from);
		SplineFree(spline);
		if ( first==NULL ) first = next->from->prev;
		if ( spl->first==NULL ) spl->first = next->from;
		if ( spl->last==NULL ) spl->last = next->from;
	    } else {
		if ( first==NULL ) first = spline;
	    }
	}
	snext = spl->next;
	if ( spl->first->next==spl->first->prev ) {
	    spl->next = NULL;
	    SplinePointListMDFree(sc,spl);
	    if ( pr==NULL )
		head = snext;
	    else
		pr->next = snext;
	} else
	    pr = spl;
    }
return( head );
}
#endif

#if 0
Spline *SplineAddPointsOfInflection(Spline *s) {
    double ts[2];
    int cnt;
    SplinePoint *sp;

    forever {
	if ( s->knownlinear )
return( s );
	cnt = Spline2DFindPointsOfInflection(s,ts);
	if ( cnt==0 )
return( s );
	if ( cnt==2 && ts[0]>ts[1] )
	    ts[0] = ts[1];
	sp = SplineBisect(s,ts[0]);
	s = sp->next;
	if ( cnt==1 )
return( s );
	/* As with add extrema, if we had more than one, loop back to look */
	/*  at any others. The spline is different now, easier to compute the */
	/*  new value than to fix up the old */
    }
}

void SplineSetAddPointsOfInflection(SplineChar *sc, SplineSet *ss) {
    Spline *s, *first;

    first = NULL;
    for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	s = SplineAddPointsOfInflection(s);
	if ( first==NULL ) first = s;
    }
}

void SplineCharAddPointsOfInflection(SplineChar *sc, SplineSet *head) {
    SplineSet *ss;

    for ( ss=head; ss!=NULL; ss=ss->next ) {
	SplineSetAddPointsOfInflection(sc,ss,between_selected,sf);
    }
}
#endif

Spline *SplineAddExtrema(Spline *s,int always,real lenbound, real offsetbound,
	DBounds *b) {
    /* First find the extrema, if any */
    bigreal t[4], min;
    uint8 rmfrom[4], rmto[4];
    int p, i,j, p_s, mini;
    SplinePoint *sp;
    real len = 0;

    if ( !always ) {
	real xlen, ylen;
	xlen = (s->from->me.x-s->to->me.x);
	ylen = (s->from->me.y-s->to->me.y);
	len = xlen*xlen + ylen*ylen;
	lenbound *= lenbound;
	if ( len < lenbound ) {
	    len = SplineLength(s);
	    len *= len;
	}
    }

    memset(rmfrom,0,sizeof(rmfrom));
    memset(rmto,0,sizeof(rmto));

    forever {
	if ( s->knownlinear )
return(s);
	p = 0;
	if ( s->splines[0].a!=0 ) {
	    double d = 4*s->splines[0].b*s->splines[0].b-4*3*s->splines[0].a*s->splines[0].c;
	    if ( d>0 ) {
		d = sqrt(d);
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[0],(-2*s->splines[0].b+d)/(2*3*s->splines[0].a));
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[0],(-2*s->splines[0].b-d)/(2*3*s->splines[0].a));
	    }
	} else if ( s->splines[0].b!=0 )
	    t[p++] = -s->splines[0].c/(2*s->splines[0].b);
	if ( !always ) {
	    /* Generally we are only interested in extrema on long splines, or */
	    /* extrema which are extrema for the entire contour, not just this */
	    /* spline */
	    /* Also extrema which are very close to one of the end-points can */
	    /*  be ignored. */
	    /* No they can't. But we need to remove the original point in this*/
	    /*  case */
	    for ( i=0; i<p; ++i ) {
		real x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d;
		real y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d;
		int close_from = ( x-s->from->me.x<offsetbound && x-s->from->me.x>-offsetbound) &&
				( y-s->from->me.y<10*offsetbound && y-s->from->me.y>-10*offsetbound );
		int close_to = ( x-s->to->me.x<offsetbound && x-s->to->me.x>-offsetbound) &&
				( y-s->to->me.y<10*offsetbound && y-s->to->me.y>-10*offsetbound );
		int remove_from = close_from  && GoodCurve(s->from,true);
		int remove_to = close_to  && GoodCurve(s->to,false);
		if (( x>b->minx && x<b->maxx  && len<lenbound ) ||
			(close_from && !remove_from) || (close_to && !remove_to) ) {
		    --p;
		    for ( j=i; j<p; ++j )
			t[j] = t[j+1];
		    --i;
		} else {
		    rmfrom[i] = remove_from;
		    rmto[i] = remove_to;
		}
	    }
	}

	p_s = p;
	if ( s->splines[1].a!=0 ) {
	    double d = 4*s->splines[1].b*s->splines[1].b-4*3*s->splines[1].a*s->splines[1].c;
	    if ( d>0 ) {
		d = sqrt(d);
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[1],(-2*s->splines[1].b+d)/(2*3*s->splines[1].a));
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[1],(-2*s->splines[1].b-d)/(2*3*s->splines[1].a));
	    }
	} else if ( s->splines[1].b!=0 )
	    t[p++] = -s->splines[1].c/(2*s->splines[1].b);
	if ( !always ) {
	    for ( i=p_s; i<p; ++i ) {
		real x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d;
		real y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d;
		int close_from =( y-s->from->me.y<offsetbound && y-s->from->me.y>-offsetbound ) &&
			( x-s->from->me.x<offsetbound && x-s->from->me.x>-offsetbound);
		int close_to = ( y-s->to->me.y<offsetbound && y-s->to->me.y>-offsetbound ) &&
			( x-s->to->me.x<offsetbound && x-s->to->me.x>-offsetbound);
		int remove_from = close_from  && GoodCurve(s->from,true);
		int remove_to = close_to  && GoodCurve(s->to,false);
		if (( y>b->miny && y<b->maxy && len<lenbound ) ||
			(close_from && !remove_from) || (close_to && !remove_to) ) {
		    --p;
		    for ( j=i; j<p; ++j )
			t[j] = t[j+1];
		    --i;
		} else {
		    rmfrom[i] = remove_from;
		    rmto[i] = remove_to;
		}
	    }
	}

	/* Throw out any t values which are not between 0 and 1 */
	/*  (we do a little fudging near the endpoints so we don't get confused */
	/*   by rounding errors) */
	for ( i=0; i<p; ++i ) {
	    if ( t[i]>0 && t[i]<.05 ) {
		BasePoint test;
		/* Expand strong gets very confused on zero-length splines so */
		/*  don't let that happen */
		test.x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d;
		test.y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d;
		if ( test.x== s->from->me.x && test.y==s->from->me.y )
		    t[i] = 0;		/* Throw it out */
	    }
	    if ( t[i]<1 && t[i]>.95 ) {
		BasePoint test;
		test.x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d;
		test.y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d;
		if ( test.x== s->to->me.x && test.y==s->to->me.y )
		    t[i] = 1.0;		/* Throw it out */
	    }
		
	    if ( t[i]<.0001 || t[i]>.9999 ) {
		--p;
		for ( j=i; j<p; ++j ) {
		    t[j] = t[j+1];
		    rmfrom[j] = rmfrom[j+1];
		    rmto[j] = rmto[j+1];
		}
		--i;
	    }
	}

	if ( p==0 )
return(s);

	/* Find the smallest of all the interesting points */
	min = t[0]; mini = 0;
	for ( i=1; i<p; ++i ) {
	    if ( t[i]<min ) {
		min=t[i];
		mini = i;
	    }
	}
	sp = SplineBisect(s,min);
/* On the mac we get rounding errors in the bisect routine */
	{ double dx, dy;
	    if ( (dx = sp->me.x - sp->prevcp.x)<0 ) dx=-dx;
	    if ( (dy = sp->me.y - sp->prevcp.y)<0 ) dy=-dy;
	    if ( dx!=0 && dy!=0 ) {
		if ( dx<dy )
		    sp->prevcp.x = sp->me.x;
		else
		    sp->prevcp.y = sp->me.y;
	    }
	    if ( (dx = sp->me.x - sp->nextcp.x)<0 ) dx=-dx;
	    if ( (dy = sp->me.y - sp->nextcp.y)<0 ) dy=-dy;
	    if ( dx!=0 && dy!=0 ) {
		if ( dx<dy )
		    sp->nextcp.x = sp->me.x;
		else
		    sp->nextcp.y = sp->me.y;
	    }
	}

	if ( rmfrom[mini] ) sp->prev->from->ticked = true;
	if ( rmto[mini] ) sp->next->to->ticked = true;
	s = sp->next;
	if ( p==1 )
return( s );
	/* Don't try to use any other computed t values, it is easier to */
	/*  recompute them than to try and figure out what they map to on the */
	/*  new spline */
    }
}

void SplineSetAddExtrema(SplineChar *sc, SplineSet *ss,enum ae_type between_selected, int emsize) {
    Spline *s, *first;
    DBounds b;
    int always = true;
    real lenbound = 0;
    real offsetbound=0;
    SplinePoint *sp, *nextp;

    if ( between_selected==ae_only_good ) {
	SplineSetQuickBounds(ss,&b);
	lenbound = (emsize)/32.0;
	always = false;
	offsetbound = .5;
	between_selected = ae_only_good_rm_later;
	for ( sp = ss->first; ; ) {
	    sp->ticked = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    first = NULL;
    for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	if ( between_selected!=ae_between_selected ||
		(s->from->selected && s->to->selected))
	    s = SplineAddExtrema(s,always,lenbound,offsetbound,&b);
	if ( first==NULL ) first = s;
    }
    if ( between_selected == ae_only_good_rm_later ) {
	for ( sp = ss->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    nextp = sp->next->to;
	    if ( sp->ticked ) {
		if ( sp==ss->first )
		    ss->first = ss->last = nextp;
		SplinesRemoveBetween(sc,sp->prev->from,nextp,1);
	    }
	    sp = nextp;
	    if ( sp==ss->first )
	break;
	}
    }
}

void SplineCharAddExtrema(SplineChar *sc, SplineSet *head,enum ae_type between_selected,int emsize) {
    SplineSet *ss;

    for ( ss=head; ss!=NULL; ss=ss->next ) {
	SplineSetAddExtrema(sc,ss,between_selected,emsize);
    }
}

char *GetNextUntitledName(void) {
    static int untitled_cnt=1;
    char buffer[80];

    sprintf( buffer, "Untitled%d", untitled_cnt++ );
return( copy(buffer));
}

SplineFont *SplineFontEmpty(void) {
    extern int default_fv_row_count, default_fv_col_count;
    time_t now;
    SplineFont *sf;

    sf = gcalloc(1,sizeof(SplineFont));
    sf->pfminfo.fstype = -1;
    sf->top_enc = -1;
    sf->macstyle = -1;
    sf->desired_row_cnt = default_fv_row_count; sf->desired_col_cnt = default_fv_col_count;
    sf->display_antialias = default_fv_antialias;
    sf->display_bbsized = default_fv_bbsized;
    sf->display_size = -default_fv_font_size;
    sf->pfminfo.winascent_add = sf->pfminfo.windescent_add = true;
    sf->pfminfo.hheadascent_add = sf->pfminfo.hheaddescent_add = true;
    sf->pfminfo.typoascent_add = sf->pfminfo.typodescent_add = true;
    if ( TTFFoundry!=NULL )
	strncpy(sf->pfminfo.os2_vendor,TTFFoundry,4);
    else
	memcpy(sf->pfminfo.os2_vendor,"PfEd",4);
    sf->for_new_glyphs = DefaultNameListForNewFonts();
    time(&now);
    sf->creationtime = sf->modificationtime = now;
return( sf );
}

SplineFont *SplineFontBlank(int charcnt) {
    SplineFont *sf;
    char buffer[200];
    time_t now;
    struct tm *tm;
    const char *author = GetAuthor();

    sf = SplineFontEmpty();
    sf->fontname = GetNextUntitledName();
    sf->fullname = copy(sf->fontname);
    sf->familyname = copy(sf->fontname);
    sprintf( buffer, "%s.sfd", sf->fontname);
    sf->origname = copy(buffer);
    sf->weight = copy("Medium");
    if ( author!=NULL )
	sprintf( buffer, "Created by %.50s with FontForge 2.0 (http://fontforge.sf.net)", author );
    else
	strcpy( buffer, "Created with FontForge 2.0 (http://fontforge.sf.net)" );
    sf->copyright = copy(buffer);
    if ( xuid!=NULL ) {
	sf->xuid = galloc(strlen(xuid)+20);
	sprintf(sf->xuid,"[%s %d]", xuid, (rand()&0xffffff));
    }
    time(&now);
    tm = localtime(&now);
    sprintf( buffer, "%d-%d-%d: Created.", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
    sf->comments = copy(buffer);
    sf->version = copy("001.000");
    sf->ascent = rint(new_em_size*.8); sf->descent = new_em_size-sf->ascent;
    sf->upos = -rint(new_em_size*.1); sf->uwidth = rint(new_em_size*.05);		/* defaults for cff */
    sf->glyphcnt = 0;
    sf->glyphmax = charcnt;
    sf->glyphs = gcalloc(charcnt,sizeof(SplineChar *));
    sf->pfminfo.fstype = -1;
    sf->order2 = false;
    sf->use_typo_metrics = true;
return( sf );
}

SplineFont *SplineFontNew(void) {
    SplineFont *sf;
    int enclen = default_encoding->char_cnt;

    sf = SplineFontBlank(enclen);
    sf->onlybitmaps = true;
    sf->new = true;
    sf->order2 = new_fonts_are_order2;

    sf->map = EncMapNew(enclen,enclen,default_encoding);
return( sf );
}

static void SFChangeXUID(SplineFont *sf, int random) {
    char *pt, *new, *npt;
    int val;

    if ( sf->xuid==NULL )
return;
    pt = strrchr(sf->xuid,' ');
    if ( pt==NULL )
	pt = strchr(sf->xuid,'[');
    if ( pt==NULL )
	pt = sf->xuid;
    else
	++pt;
    if ( random )
	val = rand()&0xffffff;
    else {
	val = strtol(pt,NULL,10);
	val = (val+1)&0xffffff;
    }

    new = galloc(pt-sf->xuid+12);
    strncpy(new,sf->xuid,pt-sf->xuid);
    npt = new + (pt-sf->xuid);
    if ( npt==new ) *npt++ = '[';
    sprintf(npt, "%d]", val );
    free(sf->xuid); sf->xuid = new;
    sf->changed = true;
    sf->changed_since_xuidchanged = false;
}

void SFIncrementXUID(SplineFont *sf) {
    SFChangeXUID(sf,false);
}

void SFRandomChangeXUID(SplineFont *sf) {
    SFChangeXUID(sf,true);
}

void SPWeightedAverageCps(SplinePoint *sp) {
    double pangle, nangle, angle, plen, nlen, c, s;
    if ( sp->noprevcp || sp->nonextcp )
	/*SPAverageCps(sp)*/;		/* Expand Stroke wants this case to hold still */
    else if ( sp->pointtype==pt_curve && sp->prev && sp->next ) {
	pangle = atan2(sp->me.y-sp->prevcp.y,sp->me.x-sp->prevcp.x);
	nangle = atan2(sp->nextcp.y-sp->me.y,sp->nextcp.x-sp->me.x);
	if ( pangle<0 && nangle>0 && nangle-pangle>=3.1415926 )
	    pangle += 2*3.1415926535897932;
	else if ( pangle>0 && nangle<0 && pangle-nangle>=3.1415926 )
	    nangle += 2*3.1415926535897932;
	plen = sqrt((sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y) +
		(sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x));
	nlen = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) +
		(sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
	if ( plen+nlen==0 )
	    angle = (nangle+pangle)/2;
	else
	    angle = (plen*pangle + nlen*nangle)/(plen+nlen);
	plen = -plen;
	c = cos(angle); s=sin(angle);
	sp->nextcp.x = c*nlen + sp->me.x;
	sp->nextcp.y = s*nlen + sp->me.y;
	sp->prevcp.x = c*plen + sp->me.x;
	sp->prevcp.y = s*plen + sp->me.y;
	SplineRefigure(sp->prev);
	SplineRefigure(sp->next);
    } else
	SPAverageCps(sp);
}

void SPAverageCps(SplinePoint *sp) {
    double pangle, nangle, angle, plen, nlen, c, s;
    if ( sp->pointtype==pt_curve && sp->prev && sp->next ) {
	if ( sp->noprevcp )
	    pangle = atan2(sp->me.y-sp->prev->from->me.y,sp->me.x-sp->prev->from->me.x);
	else
	    pangle = atan2(sp->me.y-sp->prevcp.y,sp->me.x-sp->prevcp.x);
	if ( sp->nonextcp )
	    nangle = atan2(sp->next->to->me.y-sp->me.y,sp->next->to->me.x-sp->me.x);
	else
	    nangle = atan2(sp->nextcp.y-sp->me.y,sp->nextcp.x-sp->me.x);
	if ( pangle<0 && nangle>0 && nangle-pangle>=3.1415926 )
	    pangle += 2*3.1415926535897932;
	else if ( pangle>0 && nangle<0 && pangle-nangle>=3.1415926 )
	    nangle += 2*3.1415926535897932;
	angle = (nangle+pangle)/2;
	plen = -sqrt((sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y) +
		(sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x));
	nlen = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) +
		(sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
	c = cos(angle); s=sin(angle);
	sp->nextcp.x = c*nlen + sp->me.x;
	sp->nextcp.y = s*nlen + sp->me.y;
	sp->prevcp.x = c*plen + sp->me.x;
	sp->prevcp.y = s*plen + sp->me.y;
	SplineRefigure(sp->prev);
	SplineRefigure(sp->next);
    } else if ( sp->pointtype==pt_tangent && sp->prev && sp->next ) {
	if ( !sp->noprevcp ) {
	    nangle = atan2(sp->next->to->me.y-sp->me.y,sp->next->to->me.x-sp->me.x);
	    plen = -sqrt((sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y) +
		    (sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x));
	    c = cos(nangle); s=sin(nangle);
	    sp->prevcp.x = c*plen + sp->me.x;
	    sp->prevcp.y = s*plen + sp->me.y;
	SplineRefigure(sp->prev);
	}
	if ( !sp->nonextcp ) {
	    pangle = atan2(sp->me.y-sp->prev->from->me.y,sp->me.x-sp->prev->from->me.x);
	    nlen = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) +
		    (sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
	    c = cos(pangle); s=sin(pangle);
	    sp->nextcp.x = c*nlen + sp->me.x;
	    sp->nextcp.y = s*nlen + sp->me.y;
	    SplineRefigure(sp->next);
	}
    }
}

void SPLAverageCps(SplinePointList *spl) {
    SplinePoint *sp;

    while ( spl!=NULL ) {
	for ( sp=spl->first ; ; ) {
	    SPAverageCps(sp);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	spl = spl->next;
    }
}

void SplineCharTangentNextCP(SplinePoint *sp) {
    double len;
    BasePoint *bp, unit;

    if ( sp->prev==NULL )
return;
    bp = &sp->prev->from->me;

    unit.y = sp->me.y-bp->y; unit.x = sp->me.x-bp->x;
    len = sqrt( unit.x*unit.x + unit.y*unit.y );
    if ( len!=0 ) {
	unit.x /= len;
	unit.y /= len;
    }
    len = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) + (sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
    sp->nextcp.x = sp->me.x + len*unit.x;
    sp->nextcp.y = sp->me.y + len*unit.y;
    sp->nextcp.x = rint(sp->nextcp.x*1024)/1024;
    sp->nextcp.y = rint(sp->nextcp.y*1024)/1024;
    if ( sp->next!=NULL && sp->next->order2 )
	sp->next->to->prevcp = sp->nextcp;
}

void SplineCharTangentPrevCP(SplinePoint *sp) {
    double len;
    BasePoint *bp, unit;

    if ( sp->next==NULL )
return;
    bp = &sp->next->to->me;

    unit.y = sp->me.y-bp->y; unit.x = sp->me.x-bp->x;
    len = sqrt( unit.x*unit.x + unit.y*unit.y );
    if ( len!=0 ) {
	unit.x /= len;
	unit.y /= len;
    }
    len = sqrt((sp->prevcp.y-sp->me.y)*(sp->prevcp.y-sp->me.y) + (sp->prevcp.x-sp->me.x)*(sp->prevcp.x-sp->me.x));
    sp->prevcp.x = sp->me.x + len*unit.x;
    sp->prevcp.y = sp->me.y + len*unit.y;
    sp->prevcp.x = rint(sp->prevcp.x*1024)/1024;
    sp->prevcp.y = rint(sp->prevcp.y*1024)/1024;
    if ( sp->prev!=NULL && sp->prev->order2 )
	sp->prev->from->nextcp = sp->prevcp;
}

#define NICE_PROPORTION	.39
void SplineCharDefaultNextCP(SplinePoint *base) {
    SplinePoint *prev=NULL, *next;
    double len, plen, ulen;
    BasePoint unit;

    if ( base->next==NULL )
return;
    if ( base->next->order2 ) {
	SplineRefigureFixup(base->next);
return;
    }
    if ( !base->nextcpdef ) {
	if ( base->pointtype==pt_tangent )
	    SplineCharTangentNextCP(base);
return;
    }
    next = base->next->to;
    if ( base->prev!=NULL )
	prev = base->prev->from;

    len = NICE_PROPORTION * sqrt((base->me.x-next->me.x)*(base->me.x-next->me.x) +
	    (base->me.y-next->me.y)*(base->me.y-next->me.y));
    unit.x = next->me.x - base->me.x;
    unit.y = next->me.y - base->me.y;
    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
    if ( ulen!=0 )
	unit.x /= ulen, unit.y /= ulen;
    base->nonextcp = false;

    if ( base->pointtype == pt_curve ) {
	if ( prev!=NULL && (base->prevcpdef || base->noprevcp)) {
	    unit.x = next->me.x - prev->me.x;
	    unit.y = next->me.y - prev->me.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 )
		unit.x /= ulen, unit.y /= ulen;
	    plen = sqrt((base->prevcp.x-base->me.x)*(base->prevcp.x-base->me.x) +
		    (base->prevcp.y-base->me.y)*(base->prevcp.y-base->me.y));
	    base->prevcp.x = base->me.x - plen*unit.x;
	    base->prevcp.y = base->me.y - plen*unit.y;
	    SplineRefigureFixup(base->prev);
	} else if ( prev!=NULL ) {
	    /* The prev control point is fixed. So we've got to use the same */
	    /*  angle it uses */
	    unit.x = base->me.x-base->prevcp.x;
	    unit.y = base->me.y-base->prevcp.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 )
		unit.x /= ulen, unit.y /= ulen;
	} else {
	    base->prevcp = base->me;
	    base->noprevcp = true;
	    base->prevcpdef = true;
	}
    } else if ( base->pointtype == pt_corner ) {
	if ( next->pointtype != pt_curve ) {
	    base->nonextcp = true;
	}
    } else /* tangent */ {
	if ( next->pointtype != pt_curve ) {
	    base->nonextcp = true;
	} else {
	    if ( prev!=NULL ) {
		if ( !base->noprevcp ) {
		    plen = sqrt((base->prevcp.x-base->me.x)*(base->prevcp.x-base->me.x) +
			    (base->prevcp.y-base->me.y)*(base->prevcp.y-base->me.y));
		    base->prevcp.x = base->me.x - plen*unit.x;
		    base->prevcp.y = base->me.y - plen*unit.y;
		    SplineRefigureFixup(base->prev);
		}
		unit.x = base->me.x-prev->me.x;
		unit.y = base->me.y-prev->me.y;
		ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
		if ( ulen!=0 )
		    unit.x /= ulen, unit.y /= ulen;
	    }
	}
    }
    if ( base->nonextcp )
	base->nextcp = base->me;
    else {
	base->nextcp.x = base->me.x + len*unit.x;
	base->nextcp.y = base->me.y + len*unit.y;
	base->nextcp.x = rint(base->nextcp.x*1024)/1024;
	base->nextcp.y = rint(base->nextcp.y*1024)/1024;
	if ( base->next != NULL )
	    SplineRefigureFixup(base->next);
    }
}

void SplineCharDefaultPrevCP(SplinePoint *base) {
    SplinePoint *next=NULL, *prev;
    double len, nlen, ulen;
    BasePoint unit;

    if ( base->prev==NULL )
return;
    if ( base->prev->order2 ) {
	SplineRefigureFixup(base->prev);
return;
    }
    if ( !base->prevcpdef ) {
	if ( base->pointtype==pt_tangent )
	    SplineCharTangentPrevCP(base);
return;
    }
    prev = base->prev->from;
    if ( base->next!=NULL )
	next = base->next->to;

    len = NICE_PROPORTION * sqrt((base->me.x-prev->me.x)*(base->me.x-prev->me.x) +
	    (base->me.y-prev->me.y)*(base->me.y-prev->me.y));
    unit.x = prev->me.x - base->me.x;
    unit.y = prev->me.y - base->me.y;
    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
    if ( ulen!=0 )
	unit.x /= ulen, unit.y /= ulen;
    base->noprevcp = false;

    if ( base->pointtype == pt_curve ) {
	if ( next!=NULL && (base->nextcpdef || base->nonextcp)) {
	    unit.x = prev->me.x - next->me.x;
	    unit.y = prev->me.y - next->me.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 ) 
		unit.x /= ulen, unit.y /= ulen;
	    nlen = sqrt((base->nextcp.x-base->me.x)*(base->nextcp.x-base->me.x) +
		    (base->nextcp.y-base->me.y)*(base->nextcp.y-base->me.y));
	    base->nextcp.x = base->me.x - nlen*unit.x;
	    base->nextcp.y = base->me.y - nlen*unit.y;
	    SplineRefigureFixup(base->next);
	} else if ( next!=NULL ) {
	    /* The next control point is fixed. So we got to use the same */
	    /*  angle it uses */
	    unit.x = base->me.x-base->nextcp.x;
	    unit.y = base->me.y-base->nextcp.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 )
		unit.x /= ulen, unit.y /= ulen;
	} else {
	    base->nextcp = base->me;
	    base->nonextcp = true;
	    base->nextcpdef = true;
	}
    } else if ( base->pointtype == pt_corner ) {
	if ( prev->pointtype != pt_curve ) {
	    base->noprevcp = true;
	}
    } else /* tangent */ {
	if ( prev->pointtype != pt_curve ) {
	    base->noprevcp = true;
	} else {
	    if ( next!=NULL ) {
		if ( !base->nonextcp ) {
		    nlen = sqrt((base->nextcp.x-base->me.x)*(base->nextcp.x-base->me.x) +
			    (base->nextcp.y-base->me.y)*(base->nextcp.y-base->me.y));
		    base->nextcp.x = base->me.x - nlen*unit.x;
		    base->nextcp.y = base->me.y - nlen*unit.y;
		    SplineRefigureFixup(base->next);
		}
		unit.x = base->me.x-next->me.x;
		unit.y = base->me.y-next->me.y;
		ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
		if ( ulen!=0 )
		    unit.x /= ulen, unit.y /= ulen;
	    }
	}
    }
    if ( base->noprevcp )
	base->prevcp = base->me;
    else {
	base->prevcp.x = base->me.x + len*unit.x;
	base->prevcp.y = base->me.y + len*unit.y;
	base->prevcp.x = rint(base->prevcp.x*1024)/1024;
	base->prevcp.y = rint(base->prevcp.y*1024)/1024;
	if ( base->prev!=NULL )
	    SplineRefigureFixup(base->prev);
    }
}

void SPSmoothJoint(SplinePoint *sp) {
    BasePoint unitn, unitp;
    double len, dot, dotn, dotp;
    if ( sp->prev==NULL || sp->next==NULL || sp->pointtype==pt_corner )
return;

    if ( sp->pointtype==pt_curve && !sp->nonextcp && !sp->noprevcp ) {
	unitn.x = sp->nextcp.x-sp->me.x;
	unitn.y = sp->nextcp.y-sp->me.y;
	len = sqrt(unitn.x*unitn.x + unitn.y*unitn.y);
	if ( len==0 )
return;
	unitn.x /= len; unitn.y /= len;
	unitp.x = sp->me.x - sp->prevcp.x;
	unitp.y = sp->me.y - sp->prevcp.y;
	len = sqrt(unitp.x*unitp.x + unitp.y*unitp.y);
	if ( len==0 )
return;
	unitp.x /= len; unitp.y /= len;
	dotn = unitp.y*(sp->nextcp.x-sp->me.x) - unitp.x*(sp->nextcp.y-sp->me.y);
	dotp = unitn.y*(sp->me.x - sp->prevcp.x) - unitn.x*(sp->me.y - sp->prevcp.y);
	sp->nextcp.x -= dotn*unitp.y/2;
	sp->nextcp.y -= -dotn*unitp.x/2;
	sp->prevcp.x += dotp*unitn.y/2;
	sp->prevcp.y += -dotp*unitn.x/2;
	SplineRefigure(sp->prev); SplineRefigure(sp->next);
    }
    if ( sp->pointtype==pt_tangent && !sp->nonextcp ) {
	unitp.x = sp->me.x - sp->prev->from->me.x;
	unitp.y = sp->me.y - sp->prev->from->me.y;
	len = sqrt(unitp.x*unitp.x + unitp.y*unitp.y);
	if ( len!=0 ) {
	    unitp.x /= len; unitp.y /= len;
	    dot = unitp.y*(sp->nextcp.x-sp->me.x) - unitp.x*(sp->nextcp.y-sp->me.y);
	    sp->nextcp.x -= dot*unitp.y;
	    sp->nextcp.y -= -dot*unitp.x;
	    SplineRefigure(sp->next);
	}
    }
    if ( sp->pointtype==pt_tangent && !sp->noprevcp ) {
	unitn.x = sp->nextcp.x-sp->me.x;
	unitn.y = sp->nextcp.y-sp->me.y;
	len = sqrt(unitn.x*unitn.x + unitn.y*unitn.y);
	if ( len!=0 ) {
	    unitn.x /= len; unitn.y /= len;
	    dot = unitn.y*(sp->me.x-sp->prevcp.x) - unitn.x*(sp->me.y-sp->prevcp.y);
	    sp->prevcp.x += dot*unitn.y;
	    sp->prevcp.y += -dot*unitn.x;
	    SplineRefigure(sp->prev);
	}
    }
}

void SplinePointListSet(SplinePointList *tobase, SplinePointList *frombase) {
    SplinePoint *tsp, *first, *fsp;
    Spline *tspline, *firstsp, *fspline;

    for ( ; tobase!=NULL && frombase!=NULL; tobase = tobase->next , frombase=frombase->next ) {
	first = NULL;
	for ( tsp = tobase->first, fsp=frombase->first; tsp!=first; tsp=tsp->next->to, fsp=fsp->next->to ) {
	    tsp->me = fsp->me;
	    tsp->nextcp = fsp->nextcp;
	    tsp->prevcp = fsp->prevcp;
	    tsp->noprevcp = fsp->noprevcp;
	    tsp->nonextcp = fsp->nonextcp;
	    if ( first==NULL ) first = tsp;
	    if ( tsp->next==NULL )
	break;
	}
	firstsp = NULL;
	for ( tspline = tobase->first->next, fspline=frombase->first->next;
		tspline!=NULL && tspline!=firstsp;
		tspline=tspline->to->next, fspline=fspline->to->next ) {
	    tspline->splines[0] = fspline->splines[0];
	    tspline->splines[1] = fspline->splines[1];
	    LinearApproxFree(tspline->approx);
	    tspline->approx = NULL;
	    if ( firstsp==NULL ) firstsp = tspline;
	}
    }
}

int PointListIsSelected(SplinePointList *spl) {
    int anypoints = 0;
    Spline *spline, *first;

    first = NULL;
    if ( spl->first->selected ) anypoints = true;
    for ( spline=spl->first->next; spline!=NULL && spline!=first && !anypoints; spline = spline->to->next ) {
	if ( spline->to->selected ) anypoints = true;
	if ( first == NULL ) first = spline;
    }
return( anypoints );
}

SplineSet *SplineSetReverse(SplineSet *spl) {
    Spline *spline, *first, *next;
    BasePoint tp;
    SplinePoint *temp;
    int bool;
    /* reverse the splineset so that what was the start point becomes the end */
    /*  and vice versa. This entails reversing every individual spline, and */
    /*  each point */

    first = NULL;
    spline = spl->first->next;
    if ( spline==NULL )
return( spl );			/* Only one point, reversal is meaningless */

    tp = spline->from->nextcp;
    spline->from->nextcp = spline->from->prevcp;
    spline->from->prevcp = tp;
    bool = spline->from->nonextcp;
    spline->from->nonextcp = spline->from->noprevcp;
    spline->from->noprevcp = bool;
    bool = spline->from->nextcpdef;
    spline->from->nextcpdef = spline->from->prevcpdef;
    spline->from->prevcpdef = bool;

    for ( ; spline!=NULL && spline!=first; spline=next ) {
	next = spline->to->next;

	if ( spline->to!=spl->first ) {		/* On a closed spline don't want to reverse the first point twice */
	    tp = spline->to->nextcp;
	    spline->to->nextcp = spline->to->prevcp;
	    spline->to->prevcp = tp;
	    bool = spline->to->nonextcp;
	    spline->to->nonextcp = spline->to->noprevcp;
	    spline->to->noprevcp = bool;
	    bool = spline->to->nextcpdef;
	    spline->to->nextcpdef = spline->to->prevcpdef;
	    spline->to->prevcpdef = bool;
	}

	temp = spline->to;
	spline->to = spline->from;
	spline->from = temp;
	spline->from->next = spline;
	spline->to->prev = spline;
	SplineRefigure(spline);
	if ( first==NULL ) first = spline;
    }

    if ( spl->first!=spl->last ) {
	temp = spl->first;
	spl->first = spl->last;
	spl->last = temp;
	spl->first->prev = NULL;
	spl->last->next = NULL;
    }
return( spl );
}

void SplineSetsUntick(SplineSet *spl) {
    Spline *spline, *first;
    
    while ( spl!=NULL ) {
	first = NULL;
	spl->first->isintersection = false;
	for ( spline=spl->first->next; spline!=first && spline!=NULL; spline = spline->to->next ) {
	    spline->isticked = false;
	    spline->isneeded = false;
	    spline->isunneeded = false;
	    spline->ishorvert = false;
	    spline->to->isintersection = false;
	    if ( first==NULL ) first = spline;
	}
	spl = spl->next;
    }
}

static void SplineSetTick(SplineSet *spl) {
    Spline *spline, *first;
    
    first = NULL;
    for ( spline=spl->first->next; spline!=first && spline!=NULL; spline = spline->to->next ) {
	spline->isticked = true;
	if ( first==NULL ) first = spline;
    }
}

static SplineSet *SplineSetOfSpline(SplineSet *spl,Spline *search) {
    Spline *spline, *first;
    
    while ( spl!=NULL ) {
	first = NULL;
	for ( spline=spl->first->next; spline!=first && spline!=NULL; spline = spline->to->next ) {
	    if ( spline==search )
return( spl );
	    if ( first==NULL ) first = spline;
	}
	spl = spl->next;
    }
return( NULL );
}

int SplineInSplineSet(Spline *spline, SplineSet *spl) {
    Spline *first, *s;

    first = NULL;
    for ( s = spl->first->next; s!=NULL && s!=first; s = s->to->next ) {
	if ( s==spline )
return( true );
	if ( first==NULL ) first = s;
    }
return( false );
}

#include "edgelist.h"

static void EdgeListReverse(EdgeList *es, SplineSet *spl) {
    int i;

    if ( es->edges!=NULL ) {
	for ( i=0; i<es->cnt; ++i ) {
	    Edge *e;
	    for ( e = es->edges[i]; e!=NULL; e = e->esnext ) {
		if ( SplineInSplineSet(e->spline,spl)) {
		    e->up = !e->up;
		    e->t_mmin = 1-e->t_mmin;
		    e->t_mmax = 1-e->t_mmax;
		    e->t_cur = 1-e->t_cur;
		}
	    }
	}
    }
}

static int SSCheck(SplineSet *base,Edge *active, int up, EdgeList *es,int *changed) {
    SplineSet *spl;
    if ( active->spline->isticked )
return( 0 );
    spl = SplineSetOfSpline(base,active->spline);
    if ( active->up!=up ) {
	SplineSetReverse(spl);
	*changed = true;
	EdgeListReverse(es,spl);
    }
    SplineSetTick(spl);
return( 1 );
}

SplineSet *SplineSetsExtractOpen(SplineSet **tbase) {
    SplineSet *spl, *openhead=NULL, *openlast=NULL, *prev=NULL, *snext;

    for ( spl= *tbase; spl!=NULL; spl = snext ) {
	snext = spl->next;
	if ( spl->first->prev==NULL ) {
	    if ( prev==NULL )
		*tbase = snext;
	    else
		prev->next = snext;
	    if ( openhead==NULL )
		openhead = spl;
	    else
		openlast->next = spl;
	    openlast = spl;
	    spl->next = NULL;
	} else
	    prev = spl;
    }
return( openhead );
}

void SplineSetsInsertOpen(SplineSet **tbase,SplineSet *open) {
    SplineSet *e, *p, *spl, *next;

    for ( p=NULL, spl=*tbase, e=open; e!=NULL; e = next ) {
	next = e->next;
	while ( spl!=NULL && spl->first->ttfindex<e->first->ttfindex ) {
	    p = spl;
	    spl = spl->next;
	}
	if ( p==NULL )
	    *tbase = e;
	else
	    p->next = e;
	e->next = spl;
	p = e;
    }
}

/* The idea behind SplineSetsCorrect is simple. However there are many splinesets */
/*  where it is impossible, so bear in mind that this only works for nice */
/*  splines. Figure 8's, interesecting splines all cause problems */
/* The outermost spline should be clockwise (up), the next splineset we find */
/*  should be down, if it isn't reverse it (if it's already been dealt with */
/*  then ignore it) */
SplineSet *SplineSetsCorrect(SplineSet *base,int *changed) {
    SplineSet *spl;
    int sscnt, check_cnt;
    EdgeList es;
    DBounds b;
    Edge *active=NULL, *apt, *pr, *e;
    int i, winding;

    *changed = false;

    SplineSetsUntick(base);
    for (sscnt=0,spl=base; spl!=NULL; spl=spl->next, ++sscnt );

    SplineSetFindBounds(base,&b);
    memset(&es,'\0',sizeof(es));
    es.scale = 1.0;
    es.mmin = floor(b.miny*es.scale);
    es.mmax = ceil(b.maxy*es.scale);
    es.omin = b.minx*es.scale;
    es.omax = b.maxx*es.scale;

/* Give up if we are given unreasonable values (ie. if rounding errors might screw us up) */
    if ( es.mmin<1e5 && es.mmax>-1e5 && es.omin<1e5 && es.omax>-1e5 ) {
	es.cnt = (int) (es.mmax-es.mmin) + 1;
	es.edges = gcalloc(es.cnt,sizeof(Edge *));
	es.interesting = gcalloc(es.cnt,sizeof(char));
	es.sc = NULL;
	es.major = 1; es.other = 0;
	FindEdgesSplineSet(base,&es);

	check_cnt = 0;
	for ( i=0; i<es.cnt && check_cnt<sscnt; ++i ) {
	    active = ActiveEdgesRefigure(&es,active,i);
	    if ( es.edges[i]!=NULL )
	continue;			/* Just too hard to get the edges sorted when we are at a start vertex */
	    if ( /*es.edges[i]==NULL &&*/ !es.interesting[i] &&
		    !(i>0 && es.interesting[i-1]) && !(i>0 && es.edges[i-1]!=NULL) &&
		    !(i<es.cnt-1 && es.edges[i+1]!=NULL) &&
		    !(i<es.cnt-1 && es.interesting[i+1]))	/* interesting things happen when we add (or remove) entries */
	continue;			/* and where we have extrema */
	    for ( apt=active; apt!=NULL; apt = e) {
		check_cnt += SSCheck(base,apt,true,&es,changed);
		winding = apt->up?1:-1;
		for ( pr=apt, e=apt->aenext; e!=NULL && winding!=0; pr=e, e=e->aenext ) {
		    if ( !e->spline->isticked )
			check_cnt += SSCheck(base,e,winding<0,&es,changed);
		    if ( pr->up!=e->up )
			winding += (e->up?1:-1);
		    else if ( (pr->before==e || pr->after==e ) &&
			    (( pr->mmax==i && e->mmin==i ) ||
			     ( pr->mmin==i && e->mmax==i )) )
			/* This just continues the line and doesn't change count */;
		    else
			winding += (e->up?1:-1);
		}
		/* color a horizontal line that comes out of the last vertex */
		if ( e!=NULL && (e->before==pr || e->after==pr) &&
			    (( pr->mmax==i && e->mmin==i ) ||
			     ( pr->mmin==i && e->mmax==i )) ) {
		    pr = e;
		    e = e->aenext;
		}
	    }
	}
	FreeEdges(&es);
    }
return( base );
}

SplineSet *SplineSetsAntiCorrect(SplineSet *base) {
    int changed;
    SplineSet *spl;

    SplineSetsCorrect(base,&changed);
    for ( spl = base; spl!=NULL; spl = spl->next )
	SplineSetReverse(spl);
return( base );
}

/* This is exactly the same as SplineSetsCorrect, but instead of correcting */
/*  problems we merely search for them and if we find any return the first */
SplineSet *SplineSetsDetectDir(SplineSet **_base,int *_lastscan) {
    SplineSet *ret, *base;
    EIList el;
    EI *active=NULL, *apt, *pr, *e;
    int i, winding,change,waschange;
    int lastscan = *_lastscan;
    SplineChar dummy;
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer layers[2];
#endif

    base = *_base;

    memset(&el,'\0',sizeof(el));
    memset(&dummy,'\0',sizeof(dummy));
#ifdef FONTFORGE_CONFIG_TYPE3
    memset(layers,0,sizeof(layers));
    dummy.layers = layers;
#endif
    dummy.layer_cnt = 2;
    dummy.layers[ly_fore].splines = base;
    ELFindEdges(&dummy,&el);
    if ( el.coordmax[1]-el.coordmin[1] > 1.e6 ) {
	LogError( _("Warning: Unreasonably big splines. They will be ignored.\n") );
return( NULL );
    }
    el.major = 1;
    ELOrder(&el,el.major);

    ret = NULL;
    waschange = false;
    for ( i=0; i<el.cnt && ret==NULL; ++i ) {
	active = EIActiveEdgesRefigure(&el,active,i,1,&change);
	if ( i<=lastscan )
    continue;
	if ( el.ordered[i]!=NULL || el.ends[i] ) {
	    waschange = change;
    continue;			/* Just too hard to get the edges sorted when we are at a start vertex */
	}
	if ( !( waschange || change || el.ends[i] || el.ordered[i]!=NULL ||
		(i!=el.cnt-1 && (el.ends[i+1] || el.ordered[i+1]!=NULL)) ))
    continue;
	waschange = change;
	for ( apt=active; apt!=NULL && ret==NULL; apt = e) {
	    if ( EISkipExtremum(apt,i+el.low,1)) {
		e = apt->aenext->aenext;
	continue;
	    }
	    if ( !apt->up ) {
		ret = SplineSetOfSpline(base,active->spline);
	break;
	    }
	    winding = apt->up?1:-1;
	    for ( pr=apt, e=apt->aenext; e!=NULL && winding!=0; pr=e, e=e->aenext ) {
		if ( EISkipExtremum(e,i+el.low,1)) {
		    e = e->aenext;
	    continue;
		}
		if ( pr->up!=e->up ) {
		    if ( (winding<=0 && !e->up) || (winding>0 && e->up )) {
			ret = SplineSetOfSpline(base,active->spline);
		break;
		    }
		    winding += (e->up?1:-1);
		} else if ( EISameLine(pr,e,i+el.low,1) )
		    /* This just continues the line and doesn't change count */;
		else {
		    if ( (winding<=0 && !e->up) || (winding>0 && e->up )) {
			ret = SplineSetOfSpline(base,active->spline);
		break;
		    }
		    winding += (e->up?1:-1);
		}
	    }
	}
    }
    free(el.ordered);
    free(el.ends);
    ElFreeEI(&el);
    *_base = base;
    *_lastscan = i;
return( ret );
}

int SplinePointListIsClockwise(const SplineSet *spl) {
    EIList el;
    EI *active=NULL, *apt, *e;
    int i, change,waschange;
    SplineChar dummy;
    SplineSet *next;
    int ret = -1, maybe=-1;
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer layers[2];
#endif

    if ( spl->first!=spl->last || spl->first->next == NULL )
return( -1 );		/* Open paths, (open paths with only one point are a special case) */

    memset(&el,'\0',sizeof(el));
    memset(&dummy,'\0',sizeof(dummy));
#ifdef FONTFORGE_CONFIG_TYPE3
    memset(layers,0,sizeof(layers));
    dummy.layers = layers;
#endif
    dummy.layer_cnt = 2;
    dummy.layers[ly_fore].splines = (SplineSet *) spl;
    next = spl->next; ((SplineSet *) spl)->next = NULL;
    ELFindEdges(&dummy,&el);
    if ( el.coordmax[1]-el.coordmin[1] > 1.e6 ) {
	LogError( _("Warning: Unreasonably big splines. They will be ignored.\n") );
return( -1 );
    }
    el.major = 1;
    ELOrder(&el,el.major);

    waschange = false;
    for ( i=0; i<el.cnt && ret==-1; ++i ) {
	active = EIActiveEdgesRefigure(&el,active,i,1,&change);
	if ( el.ordered[i]!=NULL || el.ends[i] || waschange || change ) {
	    waschange = change;
	    if ( active!=NULL )
		maybe = active->up;
    continue;			/* Just too hard to get the edges sorted when we are at a start vertex */
	}
	waschange = change;
	for ( apt=active; apt!=NULL && ret==-1; apt = e) {
	    if ( EISkipExtremum(apt,i+el.low,1)) {
		e = apt->aenext->aenext;
	continue;
	    }
	    ret = apt->up;
	break;
	}
    }
    free(el.ordered);
    free(el.ends);
    ElFreeEI(&el);
    ((SplineSet *) spl)->next = next;
    if ( ret==-1 )
	ret = maybe;
return( ret );
}

#if 0
void SFFigureGrid(SplineFont *sf) {
    /* Look for any horizontal/vertical lines in the grid splineset */
    int hsnaps[40], hcnt=0, vsnaps[40], vcnt=0, i;
    SplineSet *ss;
    Spline *s, *first;

    for ( ss = sf->gridsplines; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    if ( s->knownlinear ) {
		if ( s->from->me.x==s->to->me.x && hcnt<40 )
		    hsnaps[hcnt++] = s->from->me.x;
		if ( s->from->me.y==s->to->me.y && vcnt<40 )
		    vsnaps[vcnt++] = s->from->me.y;
	    }
	    if ( first==NULL ) first = s;
	}
    }

    if ( sf->hsnaps!=NULL ) {
	for ( i=0; i<hcnt && sf->hsnaps[i]==hsnaps[i]; ++i );
	if ( i!=hcnt || sf->hsnaps[i]!=0x80000000 ) {
	    free( sf->hsnaps );
	    sf->hsnaps = NULL;
	}
    }
    if ( sf->vsnaps!=NULL ) {
	for ( i=0; i<vcnt && sf->vsnaps[i]==vsnaps[i]; ++i );
	if ( i!=vcnt || sf->vsnaps[i]!=0x80000000 ) {
	    free( sf->vsnaps );
	    sf->vsnaps = NULL;
	}
    }

    if ( hcnt!=0 && sf->hsnaps==NULL ) {
	sf->hsnaps = galloc((hcnt+1)*sizeof(int));
	memcpy(sf->hsnaps,hsnaps,hcnt*sizeof(int));
	sf->hsnaps[hcnt] = 0x80000000;
    }
    if ( vcnt!=0 && sf->vsnaps==NULL ) {
	sf->vsnaps = galloc((vcnt+1)*sizeof(int));
	memcpy(sf->vsnaps,vsnaps,vcnt*sizeof(int));
	sf->vsnaps[vcnt] = 0x80000000;
    }
}
#endif
