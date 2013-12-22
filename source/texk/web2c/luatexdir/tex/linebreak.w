% linebreak.w
%
% Copyright 2006-2008 Taco Hoekwater <taco@@luatex.org>
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

@ @c
static const char _svn_version[] =
    "$Id: linebreak.w 4679 2013-12-19 15:47:53Z luigi $"
    "$URL: https://foundry.supelec.fr/svn/luatex/trunk/source/texk/web2c/luatexdir/tex/linebreak.w $";

#include "ptexlib.h"

@ We come now to what is probably the most interesting algorithm of \TeX:
the mechanism for choosing the ``best possible'' breakpoints that yield
the individual lines of a paragraph. \TeX's line-breaking algorithm takes
a given horizontal list and converts it to a sequence of boxes that are
appended to the current vertical list. In the course of doing this, it
creates a special data structure containing three kinds of records that are
not used elsewhere in \TeX. Such nodes are created while a paragraph is
being processed, and they are destroyed afterwards; thus, the other parts
of \TeX\ do not need to know anything about how line-breaking is done.

The method used here is based on an approach devised by Michael F. Plass and
@^Plass, Michael Frederick@>
@^Knuth, Donald Ervin@>
the author in 1977, subsequently generalized and improved by the same two
people in 1980. A detailed discussion appears in {\sl SOFTWARE---Practice
\AM\ Experience \bf11} (1981), 1119--1184, where it is shown that the
line-breaking problem can be regarded as a special case of the problem of
computing the shortest path in an acyclic network. The cited paper includes
numerous examples and describes the history of line breaking as it has been
practiced by printers through the ages. The present implementation adds two
new ideas to the algorithm of 1980: Memory space requirements are considerably
reduced by using smaller records for inactive nodes than for active ones,
and arithmetic overflow is avoided by using ``delta distances'' instead of
keeping track of the total distance from the beginning of the paragraph to the
current point.

The |line_break| procedure should be invoked only in horizontal mode; it
leaves that mode and places its output into the current vlist of the
enclosing vertical mode (or internal vertical mode).
There is one explicit parameter:  |d| is true for partial paragraphs
preceding display math mode; in this case the amount of additional
penalty inserted before the final line is |display_widow_penalty|
instead of |widow_penalty|.

There are also a number of implicit parameters: The hlist to be broken
starts at |vlink(head)|, and it is nonempty. The value of |prev_graf| in the
enclosing semantic level tells where the paragraph should begin in the
sequence of line numbers, in case hanging indentation or \.{\\parshape}
are in use; |prev_graf| is zero unless this paragraph is being continued
after a displayed formula.  Other implicit parameters, such as the
|par_shape_ptr| and various penalties to use for hyphenation, etc., appear
in |eqtb|.

After |line_break| has acted, it will have updated the current vlist and the
value of |prev_graf|. Furthermore, the global variable |just_box| will
point to the final box created by |line_break|, so that the width of this
line can be ascertained when it is necessary to decide whether to use
|above_display_skip| or |above_display_short_skip| before a displayed formula.

@c
halfword just_box;              /* the |hlist_node| for the last line of the new paragraph */


@ In it's complete form, |line_break| is a rather lengthy
procedure---sort of a small world unto itself---we must build it up
little by little. Below you see only the general outline.

The main task performed here is to move the list from |head| to
|temp_head| and go into the enclosing semantic level. We also append
the \.{\\parfillskip} glue to the end of the paragraph, removing a
space (or other glue node) if it was there, since spaces usually
precede blank lines and instances of `\.{\$\$}'. The |par_fill_skip|
is preceded by an infinite penalty, so it will never be considered as
a potential breakpoint.

That code assumes that a |glue_node| and a |penalty_node| occupy the
same number of |mem|~words.
@^data structure assumptions@>

Most other processing is delegated to external functions.

@c
void line_break(boolean d, int line_break_context)
{
    int paragraph_dir = 0;      /* main direction of paragraph */
    halfword final_par_glue;
    halfword start_of_par;
    int callback_id;
    pack_begin_line = cur_list.ml_field;        /* this is for over/underfull box messages */
    vlink(temp_head) = vlink(cur_list.head_field);
    new_hyphenation(temp_head, cur_list.tail_field);
    cur_list.tail_field = new_ligkern(temp_head, cur_list.tail_field);
    if (is_char_node(cur_list.tail_field)) {
        tail_append(new_penalty(inf_penalty));
    } else if (type(cur_list.tail_field) != glue_node) {
        tail_append(new_penalty(inf_penalty));
    } else {
        type(cur_list.tail_field) = penalty_node;
        delete_glue_ref(glue_ptr(cur_list.tail_field));
        if (leader_ptr(cur_list.tail_field) != null)
            flush_node_list(leader_ptr(cur_list.tail_field));
        penalty(cur_list.tail_field) = inf_penalty;
    }
    final_par_glue = new_param_glue(par_fill_skip_code);
    couple_nodes(cur_list.tail_field, final_par_glue);
    cur_list.tail_field = vlink(cur_list.tail_field);
#ifdef DEBUG
    {
       int n = temp_head;
       fprintf(stdout, "pre_linebreak_filter status:\n");
       while (n) {
         fprintf(stdout, "  %s node %d\n",
            get_node_name(type(n), subtype(n)), (int) n);
         n = vlink(n);
       }
    }
#endif
    lua_node_filter(pre_linebreak_filter_callback,
                    line_break_context, temp_head,
                    addressof(cur_list.tail_field));
    last_line_fill = cur_list.tail_field;
    pop_nest();
    start_of_par = cur_list.tail_field;
#ifdef DEBUG
    {
       int n = temp_head;
       fprintf(stdout, "pre_linebreak_filter returned:\n");
       while (n) {
         fprintf(stdout, "  %s node %d\n",
            get_node_name(type(n), subtype(n)), (int) n);
         n = vlink(n);
       }
    }
#endif
    callback_id = callback_defined(linebreak_filter_callback);
    if (callback_id > 0) {
        callback_id =
            lua_linebreak_callback(d, temp_head,
                                   addressof(cur_list.tail_field));
        if (callback_id > 0) {
            /* find the correct value for the |just_box| */
	    halfword box_search = cur_list.tail_field;
            just_box  = null;
	    if (box_search != null) {
                do {
	            if (type(box_search) == hlist_node) {
                       just_box = box_search;
                    }
                    box_search = vlink(box_search);
                } while (box_search != null);
	    }
            if (just_box == null) {
                help3
                    ("A linebreaking routine should return a non-empty list of nodes",
                     "and at least one of those has to be a \\hbox.",
                     "Sorry, I cannot recover from this.");
                print_err("Invalid linebreak_filter");
                succumb();
            }
        } else {
            if (int_par(tracing_paragraphs_code) > 0) {
                begin_diagnostic();
                tprint_nl
                    ("Lua linebreak_filter failed, reverting to default on line ");
                print_int(line);
                end_diagnostic(true);
            }
        }
    }
    if (callback_id == 0) {
        if ((!is_char_node(vlink(temp_head)))
            && ((type(vlink(temp_head)) == whatsit_node)
                && (subtype(vlink(temp_head)) == local_par_node)))
            paragraph_dir = local_par_dir(vlink(temp_head));
        else
            assert(0);              /* |paragraph_dir = 0|; */
        ext_do_line_break(paragraph_dir,
                          int_par(pretolerance_code),
                          int_par(tracing_paragraphs_code),
                          int_par(tolerance_code),
                          dimen_par(emergency_stretch_code),
                          int_par(looseness_code),
                          int_par(hyphen_penalty_code),
                          int_par(ex_hyphen_penalty_code),
                          int_par(pdf_adjust_spacing_code),
                          equiv(par_shape_loc),
                          int_par(adj_demerits_code),
                          int_par(pdf_protrude_chars_code),
                          int_par(line_penalty_code),
                          int_par(last_line_fit_code),
                          int_par(double_hyphen_demerits_code),
                          int_par(final_hyphen_demerits_code),
                          dimen_par(hang_indent_code),
                          dimen_par(hsize_code),
                          int_par(hang_after_code),
                          glue_par(left_skip_code),
                          glue_par(right_skip_code),
                          dimen_par(pdf_each_line_height_code),
                          dimen_par(pdf_each_line_depth_code),
                          dimen_par(pdf_first_line_height_code),
                          dimen_par(pdf_last_line_depth_code),
                          equiv(inter_line_penalties_loc),
                          int_par(inter_line_penalty_code),
                          int_par(club_penalty_code),
                          equiv(club_penalties_loc),
			  (d ? equiv(display_widow_penalties_loc) : equiv(widow_penalties_loc)),
			  (d ? int_par(display_widow_penalty_code) : int_par(widow_penalty_code)),
                          int_par(broken_penalty_code),
                          final_par_glue, dimen_par(pdf_ignored_dimen_code));
    }
    lua_node_filter(post_linebreak_filter_callback,
                    line_break_context, start_of_par,
                    addressof(cur_list.tail_field));
    pack_begin_line = 0;
}


@ Glue nodes in a horizontal list that is being paragraphed are not supposed to
   include ``infinite'' shrinkability; that is why the algorithm maintains
   four registers for stretching but only one for shrinking. If the user tries to
   introduce infinite shrinkability, the shrinkability will be reset to finite
   and an error message will be issued. A boolean variable |no_shrink_error_yet|
   prevents this error message from appearing more than once per paragraph.

@c
#define check_shrinkage(a)                              \
  if ((shrink_order((a))!=normal)&&(shrink((a))!=0))    \
    a=finite_shrink((a))

static boolean no_shrink_error_yet;     /*have we complained about infinite shrinkage? */

static halfword finite_shrink(halfword p)
{                               /* recovers from infinite shrinkage */
    halfword q;                 /*new glue specification */
    const char *hlp[] = {
        "The paragraph just ended includes some glue that has",
        "infinite shrinkability, e.g., `\\hskip 0pt minus 1fil'.",
        "Such glue doesn't belong there---it allows a paragraph",
        "of any length to fit on one line. But it's safe to proceed,",
        "since the offensive shrinkability has been made finite.",
        NULL
    };
    if (no_shrink_error_yet) {
        no_shrink_error_yet = false;
        tex_error("Infinite glue shrinkage found in a paragraph", hlp);
    }
    q = new_spec(p);
    shrink_order(q) = normal;
    delete_glue_ref(p);
    return q;
}

@ A pointer variable |cur_p| runs through the given horizontal list as we look
   for breakpoints. This variable is global, since it is used both by |line_break|
   and by its subprocedure |try_break|.

   Another global variable called |threshold| is used to determine the feasibility
   of individual lines: breakpoints are feasible if there is a way to reach
   them without creating lines whose badness exceeds |threshold|.  (The
   badness is compared to |threshold| before penalties are added, so that
   penalty values do not affect the feasibility of breakpoints, except that
   no break is allowed when the penalty is 10000 or more.) If |threshold|
   is 10000 or more, all legal breaks are considered feasible, since the
   |badness| function specified above never returns a value greater than~10000.
   
   Up to three passes might be made through the paragraph in an attempt to find at
   least one set of feasible breakpoints. On the first pass, we have
   |threshold=pretolerance| and |second_pass=final_pass=false|.
   If this pass fails to find a
   feasible solution, |threshold| is set to |tolerance|, |second_pass| is set
   |true|, and an attempt is made to hyphenate as many words as possible.
   If that fails too, we add |emergency_stretch| to the background
   stretchability and set |final_pass=true|.

@c
static boolean second_pass;     /* is this our second attempt to break this paragraph? */
static boolean final_pass;      /*is this our final attempt to break this paragraph? */
static int threshold;           /* maximum badness on feasible lines */

/* maximum fill level for |hlist_stack|*/
#define max_hlist_stack 512     /* maybe good if larger than |2 *
                                   max_quarterword|, so that box nesting
                                   level would overflow first */

/* stack for |find_protchar_left()| and |find_protchar_right()| */
static halfword hlist_stack[max_hlist_stack];

/* fill level for |hlist_stack| */
static short hlist_stack_level = 0;

@ @c
static void push_node(halfword p)
{
    if (hlist_stack_level >= max_hlist_stack)
        pdf_error("push_node", "stack overflow");
    hlist_stack[hlist_stack_level++] = p;
}

static halfword pop_node(void)
{
    if (hlist_stack_level <= 0) /* would point to some bug */
        pdf_error("pop_node", "stack underflow (internal error)");
    return hlist_stack[--hlist_stack_level];
}

@ @c
static int max_stretch_ratio = 0;       /*maximal stretch ratio of expanded fonts */
static int max_shrink_ratio = 0;        /*maximal shrink ratio of expanded fonts */
static int cur_font_step = 0;   /*the current step of expanded fonts */


static boolean check_expand_pars(internal_font_number f)
{
    int m;

    if ((font_step(f) == 0)
        || ((font_max_stretch(f) == 0) && (font_max_shrink(f) == 0)))
        return false;
    if (cur_font_step < 0)
        cur_font_step = font_step(f);
    else if (cur_font_step != font_step(f))
        pdf_error("font expansion",
                  "using fonts with different step of expansion in one paragraph is not allowed");
    m = font_max_stretch(f);
    if (m != 0) {
        if (max_stretch_ratio < 0)
            max_stretch_ratio = m;
        else if (max_stretch_ratio != m)
            pdf_error("font expansion",
                      "using fonts with different limit of expansion in one paragraph is not allowed");
    }
    m = font_max_shrink(f);
    if (m != 0) {
        if (max_shrink_ratio < 0)
            max_shrink_ratio = -m;
        else if (max_shrink_ratio != -m)
            pdf_error("font expansion",
                      "using fonts with different limit of expansion in one paragraph is not allowed");
    }
    return true;
}

@ searches left to right from list head |l|, returns 1st non-skipable item

@c
/*public*/ halfword find_protchar_left(halfword l, boolean d)
{
    halfword t;
    boolean run;
    if ((vlink(l) != null) && (type(l) == hlist_node) && (width(l) == 0)
        && (height(l) == 0) && (depth(l) == 0) && (list_ptr(l) == null)) {
        l = vlink(l);           /*for paragraph start with \.{\\parindent} = 0pt */
    } else if (d) {
        while ((vlink(l) != null) && (!(is_char_node(l) || non_discardable(l)))) {
            l = vlink(l);       /* std.\ discardables at line break, \TeX book, p 95 */
        }
    }
    hlist_stack_level = 0;
    run = true;
    do {
        t = l;
        while (run && (type(l) == hlist_node) && (list_ptr(l) != null)) {
            push_node(l);
            l = list_ptr(l);
        }
        while (run && cp_skipable(l)) {
            while ((vlink(l) == null) && (hlist_stack_level > 0)) {
                l = pop_node(); /* don't visit this node again */
                run = false;
            }
            if (vlink(l) != null)
                l = vlink(l);
            else if (hlist_stack_level == 0)
                run = false;
        }
    } while (t != l);
    return l;
}


@ searches right to left from list tail |r| to head |l|, returns 1st non-skipable item 

@c
/*public*/ halfword find_protchar_right(halfword l, halfword r)
{
    halfword t;
    boolean run;
    if (r == null)
        return null;
    hlist_stack_level = 0;
    run = true;
    do {
        t = r;
        while (run && (type(r) == hlist_node) && (list_ptr(r) != null)) {
            push_node(l);
            push_node(r);
            l = list_ptr(r);
            r = l;
            while (vlink(r) != null) {
                halfword s = r;
                r = vlink(r);
                alink(r) = s;
            }
        }
        while (run && cp_skipable(r)) {
            while ((r == l) && (hlist_stack_level > 0)) {
                r = pop_node(); /* don't visit this node again */
                l = pop_node();
            }
            if ((r != l) && (r != null)) {
                if (alink(r) != null) {
                    assert(vlink(alink(r)) == r);
                    r = alink(r);
                } else {        /* this is the input: \.{\\leavevmode\\penalty-10000\\penalty-10000} (bug \#268) */
                    run = false;
                }
            } else if ((r == l) && (hlist_stack_level == 0))
                run = false;
        }
    } while (t != r);
    return r;
}

@ @c
#define left_pw(a) char_pw((a), left_side)
#define right_pw(a) char_pw((a), right_side)

@ When looking for optimal line breaks, \TeX\ creates a ``break node'' for
   each break that is {\sl feasible}, in the sense that there is a way to end
   a line at the given place without requiring any line to stretch more than
   a given tolerance. A break node is characterized by three things: the position
   of the break (which is a pointer to a |glue_node|, |math_node|, |penalty_node|,
   or |disc_node|); the ordinal number of the line that will follow this
   breakpoint; and the fitness classification of the line that has just
   ended, i.e., |tight_fit|, |decent_fit|, |loose_fit|, or |very_loose_fit|.

@c
typedef enum {
    very_loose_fit = 0,         /* fitness classification for lines stretching more than
                                   their stretchability */
    loose_fit,                  /* fitness classification for lines stretching 0.5 to 1.0 of their
                                   stretchability */
    decent_fit,                 /* fitness classification for all other lines */
    tight_fit                   /* fitness classification for lines shrinking 0.5 to 1.0 of their
                                   shrinkability */
} fitness_value;


@ The algorithm essentially determines the best possible way to achieve
   each feasible combination of position, line, and fitness. Thus, it answers
   questions like, ``What is the best way to break the opening part of the
   paragraph so that the fourth line is a tight line ending at such-and-such
   a place?'' However, the fact that all lines are to be the same length
   after a certain point makes it possible to regard all sufficiently large
   line numbers as equivalent, when the looseness parameter is zero, and this
   makes it possible for the algorithm to save space and time.
   
   An ``active node'' and a ``passive node'' are created in |mem| for each
   feasible breakpoint that needs to be considered. Active nodes are three
   words long and passive nodes are two words long. We need active nodes only
   for breakpoints near the place in the paragraph that is currently being
   examined, so they are recycled within a comparatively short time after
   they are created.

@ An active node for a given breakpoint contains six fields:
   
|vlink| points to the next node in the list of active nodes; the
last active node has |vlink=active|.

|break_node| points to the passive node associated with this
breakpoint.

|line_number| is the number of the line that follows this
breakpoint.

|fitness| is the fitness classification of the line ending at this
breakpoint.

|type| is either |hyphenated_node| or |unhyphenated_node|, depending on
whether this breakpoint is a |disc_node|.

|total_demerits| is the minimum possible sum of demerits over all
lines leading from the beginning of the paragraph to this breakpoint.

The value of |vlink(active)| points to the first active node on a vlinked list
of all currently active nodes. This list is in order by |line_number|,
except that nodes with |line_number>easy_line| may be in any order relative
to each other.

@c
void initialize_active(void)
{
    type(active) = hyphenated_node;
    line_number(active) = max_halfword;
    subtype(active) = 0;        /* the |subtype| is never examined */
}

@ The passive node for a given breakpoint contains EIGHT fields:

|vlink| points to the passive node created just before this one,
if any, otherwise it is |null|.

|cur_break| points to the position of this breakpoint in the
horizontal list for the paragraph being broken.

|prev_break| points to the passive node that should precede this
one in an optimal path to this breakpoint.

|serial| is equal to |n| if this passive node is the |n|th
one created during the current pass. (This field is used only when
printing out detailed statistics about the line-breaking calculations.)

|passive_pen_inter| holds the current \.{\\localinterlinepenalty}

|passive_pen_broken| holds the current \.{\\localbrokenpenalty}

There is a global variable called |passive| that points to the most
recently created passive node. Another global variable, |printed_node|,
is used to help print out the paragraph when detailed information about
the line-breaking computation is being displayed.

@c
static halfword passive;        /* most recent node on passive list */
static halfword printed_node;   /*most recent node that has been printed */
static halfword pass_number;    /*the number of passive nodes allocated on this pass */


@ The active list also contains ``delta'' nodes that help the algorithm
compute the badness of individual lines. Such nodes appear only between two
active nodes, and they have |type=delta_node|. If |p| and |r| are active nodes
and if |q| is a delta node between them, so that |vlink(p)=q| and |vlink(q)=r|,
then |q| tells the space difference between lines in the horizontal list that
start after breakpoint |p| and lines that start after breakpoint |r|. In
other words, if we know the length of the line that starts after |p| and
ends at our current position, then the corresponding length of the line that
starts after |r| is obtained by adding the amounts in node~|q|. A delta node
contains seven scaled numbers, since it must record the net change in glue
stretchability with respect to all orders of infinity. The natural width
difference appears in |mem[q+1].sc|; the stretch differences in units of
pt, sfi, fil, fill, and filll appear in |mem[q+2..q+6].sc|; and the shrink
difference appears in |mem[q+7].sc|. The |subtype| field of a delta node
is not used.

Actually, we have two more fields that are used by |pdftex|.


As the algorithm runs, it maintains a set of seven delta-like registers
for the length of the line following the first active breakpoint to the
current position in the given hlist. When it makes a pass through the
active list, it also maintains a similar set of seven registers for the
length following the active breakpoint of current interest. A third set
holds the length of an empty line (namely, the sum of \.{\\leftskip} and
\.{\\rightskip}); and a fourth set is used to create new delta nodes.

When we pass a delta node we want to do operations like
$$\hbox{\ignorespaces|for
k:=1 to 7 do cur_active_width[k]:=cur_active_width[k]+mem[q+k].sc|};$$ and we
want to do this without the overhead of |for| loops. The |do_all_six|
macro makes such six-tuples convenient.

@c
static scaled active_width[10] = { 0 }; /*distance from first active node to~|cur_p| */
static scaled background[10] = { 0 };   /*length of an ``empty'' line */
static scaled break_width[10] = { 0 };  /*length being computed after current break */

static boolean auto_breaking;   /*make |auto_breaking| accessible out of |line_break| */

@  Let's state the principles of the delta nodes more precisely and concisely,
   so that the following programs will be less obscure. For each legal
   breakpoint~|p| in the paragraph, we define two quantities $\alpha(p)$ and
   $\beta(p)$ such that the length of material in a line from breakpoint~|p|
   to breakpoint~|q| is $\gamma+\beta(q)-\alpha(p)$, for some fixed $\gamma$.
   Intuitively, $\alpha(p)$ and $\beta(q)$ are the total length of material from
   the beginning of the paragraph to a point ``after'' a break at |p| and to a
   point ``before'' a break at |q|; and $\gamma$ is the width of an empty line,
   namely the length contributed by \.{\\leftskip} and \.{\\rightskip}.
   
   Suppose, for example, that the paragraph consists entirely of alternating
   boxes and glue skips; let the boxes have widths $x_1\ldots x_n$ and
   let the skips have widths $y_1\ldots y_n$, so that the paragraph can be
   represented by $x_1y_1\ldots x_ny_n$. Let $p_i$ be the legal breakpoint
   at $y_i$; then $\alpha(p_i)=x_1+y_1+\cdots+x_i+y_i$, and $\beta(p_i)=
   x_1+y_1+\cdots+x_i$. To check this, note that the length of material from
   $p_2$ to $p_5$, say, is $\gamma+x_3+y_3+x_4+y_4+x_5=\gamma+\beta(p_5)
   -\alpha(p_2)$.

   The quantities $\alpha$, $\beta$, $\gamma$ involve glue stretchability and
   shrinkability as well as a natural width. If we were to compute $\alpha(p)$
   and $\beta(p)$ for each |p|, we would need multiple precision arithmetic, and
   the multiprecise numbers would have to be kept in the active nodes.
   \TeX\ avoids this problem by working entirely with relative differences
   or ``deltas.'' Suppose, for example, that the active list contains
   $a_1\,\delta_1\,a_2\,\delta_2\,a_3$, where the |a|'s are active breakpoints
   and the $\delta$'s are delta nodes. Then $\delta_1=\alpha(a_1)-\alpha(a_2)$
   and $\delta_2=\alpha(a_2)-\alpha(a_3)$. If the line breaking algorithm is
   currently positioned at some other breakpoint |p|, the |active_width| array
   contains the value $\gamma+\beta(p)-\alpha(a_1)$. If we are scanning through
   the list of active nodes and considering a tentative line that runs from
   $a_2$ to~|p|, say, the |cur_active_width| array will contain the value
   $\gamma+\beta(p)-\alpha(a_2)$. Thus, when we move from $a_2$ to $a_3$,
   we want to add $\alpha(a_2)-\alpha(a_3)$ to |cur_active_width|; and this
   is just $\delta_2$, which appears in the active list between $a_2$ and
   $a_3$. The |background| array contains $\gamma$. The |break_width| array
   will be used to calculate values of new delta nodes when the active
   list is being updated.


@  The heart of the line-breaking procedure is `|try_break|', a subroutine
   that tests if the current breakpoint |cur_p| is feasible, by running
   through the active list to see what lines of text can be made from active
   nodes to~|cur_p|.  If feasible breaks are possible, new break nodes are
   created.  If |cur_p| is too far from an active node, that node is
   deactivated.
   
   The parameter |pi| to |try_break| is the penalty associated
   with a break at |cur_p|; we have |pi=eject_penalty| if the break is forced,
   and |pi=inf_penalty| if the break is illegal.
   
   The other parameter, |break_type|, is set to |hyphenated_node| or |unhyphenated_node|,
   depending on whether or not the current break is at a |disc_node|. The
   end of a paragraph is also regarded as `|hyphenated_node|'; this case is
   distinguishable by the condition |cur_p=null|.


@c
static int internal_pen_inter;  /* running \.{\\localinterlinepenalty} */
static int internal_pen_broken; /* running \.{\\localbrokenpenalty} */
static halfword internal_left_box;      /* running \.{\\localleftbox} */
static int internal_left_box_width;     /* running \.{\\localleftbox} width */
static halfword init_internal_left_box; /* running \.{\\localleftbox} */
static int init_internal_left_box_width;        /* running \.{\\localleftbox} width */
static halfword internal_right_box;     /* running \.{\\localrightbox} */
static int internal_right_box_width;    /* running \.{\\localrightbox} width */

static scaled disc_width[10] = { 0 };   /* the length of discretionary material preceding a break */

@  As we consider various ways to end a line at |cur_p|, in a given line number
   class, we keep track of the best total demerits known, in an array with
   one entry for each of the fitness classifications. For example,
   |minimal_demerits[tight_fit]| contains the fewest total demerits of feasible
   line breaks ending at |cur_p| with a |tight_fit| line; |best_place[tight_fit]|
   points to the passive node for the break before~|cur_p| that achieves such
   an optimum; and |best_pl_line[tight_fit]| is the |line_number| field in the
   active node corresponding to |best_place[tight_fit]|. When no feasible break
   sequence is known, the |minimal_demerits| entries will be equal to
   |awful_bad|, which is $2^{30}-1$. Another variable, |minimum_demerits|,
   keeps track of the smallest value in the |minimal_demerits| array.

@c
static int minimal_demerits[4]; /* best total demerits known for current 
                                   line class and position, given the fitness */
static int minimum_demerits;    /* best total demerits known for current line class
                                   and position */
static halfword best_place[4];  /* how to achieve  |minimal_demerits| */
static halfword best_pl_line[4];        /*corresponding line number */



@  The length of lines depends on whether the user has specified
\.{\\parshape} or \.{\\hangindent}. If |par_shape_ptr| is not null, it
points to a $(2n+1)$-word record in |mem|, where the |vinfo| in the first
word contains the value of |n|, and the other $2n$ words contain the left
margins and line lengths for the first |n| lines of the paragraph; the
specifications for line |n| apply to all subsequent lines. If
|par_shape_ptr=null|, the shape of the paragraph depends on the value of
|n=hang_after|; if |n>=0|, hanging indentation takes place on lines |n+1|,
|n+2|, \dots, otherwise it takes place on lines 1, \dots, $\vert
n\vert$. When hanging indentation is active, the left margin is
|hang_indent|, if |hang_indent>=0|, else it is 0; the line length is
$|hsize|-\vert|hang_indent|\vert$. The normal setting is
|par_shape_ptr=null|, |hang_after=1|, and |hang_indent=0|.
Note that if |hang_indent=0|, the value of |hang_after| is irrelevant.
@^length of lines@> @^hanging indentation@>

@c
static halfword easy_line;      /*line numbers |>easy_line| are equivalent in break nodes */
static halfword last_special_line;      /*line numbers |>last_special_line| all have the same width */
static scaled first_width;      /*the width of all lines |<=last_special_line|, if
                                   no \.{\\parshape} has been specified */
static scaled second_width;     /*the width of all lines |>last_special_line| */
static scaled first_indent;     /*left margin to go with |first_width| */
static scaled second_indent;    /*left margin to go with |second_width| */

static halfword best_bet;       /*use this passive node and its predecessors */
static int fewest_demerits;     /*the demerits associated with |best_bet| */
static halfword best_line;      /*line number following the last line of the new paragraph */
static int actual_looseness;    /*the difference between |line_number(best_bet)|
                                   and the optimum |best_line| */
static int line_diff;           /*the difference between the current line number and
                                   the optimum |best_line| */



@  \TeX\ makes use of the fact that |hlist_node|, |vlist_node|,
   |rule_node|, |ins_node|, |mark_node|, |adjust_node|, 
   |disc_node|, |whatsit_node|, and |math_node| are at the low end of the
   type codes, by permitting a break at glue in a list if and only if the
   |type| of the previous node is less than |math_node|. Furthermore, a
   node is discarded after a break if its type is |math_node| or~more.

@c
#define do_all_six(a) a(1);a(2);a(3);a(4);a(5);a(6);a(7)
#define do_seven_eight(a) if (pdf_adjust_spacing > 1) { a(8);a(9); }
#define do_all_eight(a) do_all_six(a); do_seven_eight(a)
#define do_one_seven_eight(a) a(1); do_seven_eight(a)

#define store_background(a) {active_width[a]=background[a];}

#define kern_break() {  \
    if ((!is_char_node(vlink(cur_p))) && auto_breaking)  \
      if (type(vlink(cur_p))==glue_node)  \
	  ext_try_break(0,unhyphenated_node, line_break_dir, pdf_adjust_spacing,	\
                      par_shape_ptr, adj_demerits,  \
                      tracing_paragraphs, pdf_protrude_chars,  \
                      line_penalty, last_line_fit,  \
                      double_hyphen_demerits,  final_hyphen_demerits,first_p,cur_p);  \
    if (type(cur_p)!=math_node) active_width[1]+=width(cur_p);  \
    else                        active_width[1]+=surround(cur_p);  \
  }

#define clean_up_the_memory() {  \
    q=vlink(active);  \
    while (q!=active) {  \
      cur_p=vlink(q);  \
      if (type(q)==delta_node)         flush_node(q);  \
      else                        flush_node(q);  \
      q=cur_p;  \
    }  \
    q=passive;  \
    while (q!=null) {  \
      cur_p=vlink(q);  \
      flush_node(q);  \
      q=cur_p;  \
    }  \
  }

static boolean do_last_line_fit;        /* special algorithm for last line of paragraph? */
static scaled fill_width[4];    /* infinite stretch components of  |par_fill_skip| */
static scaled best_pl_short[4]; /* |shortfall|  corresponding to |minimal_demerits| */
static scaled best_pl_glue[4];  /*corresponding glue stretch or shrink */

#define reset_disc_width(a) disc_width[(a)] = 0

#define add_disc_width_to_break_width(a)     break_width[(a)] += disc_width[(a)]
#define sub_disc_width_from_active_width(a)  active_width[(a)] -= disc_width[(a)]

#define add_char_shrink(a,b)  a += char_shrink((b))
#define add_char_stretch(a,b) a += char_stretch((b))
#define sub_char_shrink(a,b)  a -= char_shrink((b))
#define sub_char_stretch(a,b) a -= char_stretch((b))

#define add_kern_shrink(a,b)  a += kern_shrink((b))
#define add_kern_stretch(a,b) a += kern_stretch((b))
#define sub_kern_shrink(a,b)  a -= kern_shrink((b))
#define sub_kern_stretch(a,b) a -= kern_stretch((b))


@ This function is used to add the width of a list of nodes
(from a discretionary) to one of the width arrays.


Replacement texts and discretionary texts are supposed to contain
only character nodes, kern nodes, and box or rule nodes. 

@c
static void add_to_widths(halfword s, int line_break_dir,
                          int pdf_adjust_spacing, scaled * widths)
{
    while (s != null) {
        if (is_char_node(s)) {
            widths[1] += pack_width(line_break_dir, dir_TRT, s, true);
            if ((pdf_adjust_spacing > 1) && check_expand_pars(font(s))) {
                set_prev_char_p(s);
                add_char_stretch(widths[8], s);
                add_char_shrink(widths[9], s);
            };
        } else {
            switch (type(s)) {
            case hlist_node:
            case vlist_node:
                widths[1] += pack_width(line_break_dir, box_dir(s), s, false);
                break;
            case kern_node:
                if ((pdf_adjust_spacing > 1) && (subtype(s) == normal)) {
                    add_kern_stretch(widths[8], s);
                    add_kern_shrink(widths[9], s);
                }
                /* fall through */
            case rule_node:
                widths[1] += width(s);
                break;
            case disc_node:    /* TH temp */
                break;
            default:
                confusion("add_disc_widths");
            }
        }
        s = vlink(s);
    }
}

@ This function is used to substract the width of a list of nodes
(from a discretionary) from one of the width arrays. 
It is used only once, but deserves it own function because of orthogonality 
with the |add_to_widths| function.

@c
static void sub_from_widths(halfword s, int line_break_dir,
                            int pdf_adjust_spacing, scaled * widths)
{
    while (s != null) {
        /* Subtract the width of node |s| from |break_width|; */
        if (is_char_node(s)) {
            widths[1] -= pack_width(line_break_dir, dir_TRT, s, true);
            if ((pdf_adjust_spacing > 1) && check_expand_pars(font(s))) {
                set_prev_char_p(s);
                sub_char_stretch(widths[8], s);
                sub_char_shrink(widths[9], s);
            }
        } else {
            switch (type(s)) {
            case hlist_node:
            case vlist_node:
                widths[1] -= pack_width(line_break_dir, box_dir(s), s, false);
                break;
            case kern_node:
                if ((pdf_adjust_spacing > 1) && (subtype(s) == normal)) {
                    sub_kern_stretch(widths[8], s);
                    sub_kern_shrink(widths[9], s);
                }
                /* fall through */
            case rule_node:
                widths[1] -= width(s);
                break;
            case disc_node:    /* TH temp */
                break;
            default:
                confusion("sub_disc_widths");
                break;
            }
        }
        s = vlink(s);
    }
}


@  When we insert a new active node for a break at |cur_p|, suppose this
   new node is to be placed just before active node |a|; then we essentially
   want to insert `$\delta\,|cur_p|\,\delta^\prime$' before |a|, where
   $\delta=\alpha(a)-\alpha(|cur_p|)$ and $\delta^\prime=\alpha(|cur_p|)-\alpha(a)$
   in the notation explained above.  The |cur_active_width| array now holds
   $\gamma+\beta(|cur_p|)-\alpha(a)$; so $\delta$ can be obtained by
   subtracting |cur_active_width| from the quantity $\gamma+\beta(|cur_p|)-
   \alpha(|cur_p|)$. The latter quantity can be regarded as the length of a
   line ``from |cur_p| to |cur_p|''; we call it the |break_width| at |cur_p|.
   
   The |break_width| is usually negative, since it consists of the background
   (which is normally zero) minus the width of nodes following~|cur_p| that are
   eliminated after a break. If, for example, node |cur_p| is a glue node, the
   width of this glue is subtracted from the background; and we also look
   ahead to eliminate all subsequent glue and penalty and kern and math
   nodes, subtracting their widths as well.
     
   Kern nodes do not disappear at a line break unless they are |explicit|.

@c
static void
compute_break_width(int break_type, int line_break_dir, int pdf_adjust_spacing,
                    halfword p
                    /*, halfword s */ )
{
    halfword s;                 /* glue and other 'whitespace' to be skipped after a break
                                 * used if unhyphenated, or |post_break==empty| */
    s = p;
    if (break_type > unhyphenated_node && p != null) {
        /*Compute the discretionary |break_width| values; */
        /* When |p| is a discretionary break, the length of a line
           ``from |p| to |p|'' has to be defined properly so
           that the other calculations work out.  Suppose that the
           pre-break text at |p| has length $l_0$, the post-break
           text has length $l_1$, and the replacement text has length
           |l|. Suppose also that |q| is the node following the
           replacement text. Then length of a line from |p| to |q|
           will be computed as $\gamma+\beta(q)-\alpha(|p|)$, where
           $\beta(q)=\beta(|p|)-l_0+l$. The actual length will be
           the background plus $l_1$, so the length from |p| to
           |p| should be $\gamma+l_0+l_1-l$.  If the post-break text
           of the discretionary is empty, a break may also discard~|q|;
           in that unusual case we subtract the length of~|q| and any
           other nodes that will be discarded after the discretionary
           break.

           TH: I don't quite understand the above remarks.

           The value of $l_0$ need not be computed, since |line_break|
           will put it into the global variable |disc_width| before
           calling |try_break|.
         */
        /* In case of nested discretionaries, we always follow the no-break
           path, as we are talking about the breaking on {\it this} position.
         */

        sub_from_widths(vlink_no_break(p), line_break_dir, pdf_adjust_spacing,
                        break_width);
        add_to_widths(vlink_post_break(p), line_break_dir, pdf_adjust_spacing,
                      break_width);
        do_one_seven_eight(add_disc_width_to_break_width);
        if (vlink_post_break(p) == null) {
            s = vlink(p);       /* no |post_break|: 'skip' any 'whitespace' following */
        } else {
            s = null;
        }
    }
    while (s != null) {
        switch (type(s)) {
        case glue_node:
            /*Subtract glue from |break_width|; */
            {
                halfword v = glue_ptr(s);
                break_width[1] -= width(v);
                break_width[2 + stretch_order(v)] -= stretch(v);
                break_width[7] -= shrink(v);
            }
            break;
        case penalty_node:
            break;
        case math_node:
            break_width[1] -= surround(s);
            break;
        case kern_node:
            if (subtype(s) != explicit)
                return;
            else
                break_width[1] -= width(s);
            break;
        default:
            return;
        };
        s = vlink(s);
    }
}

@ @c
static void
print_break_node(halfword q, fitness_value fit_class,
                 quarterword break_type, halfword cur_p)
{
    /* Print a symbolic description of the new break node */
    tprint_nl("@@@@");
    print_int(serial(passive));
    tprint(": line ");
    print_int(line_number(q) - 1);
    print_char('.');
    print_int(fit_class);
    if (break_type == hyphenated_node)
        print_char('-');
    tprint(" t=");
    print_int(total_demerits(q));
    if (do_last_line_fit) {
        /*Print additional data in the new active node; */
        tprint(" s=");
        print_scaled(active_short(q));
        if (cur_p == null)
            tprint(" a=");
        else
            tprint(" g=");
        print_scaled(active_glue(q));
    }
    tprint(" -> @@");
    if (prev_break(passive) == null)
        print_char('0');
    else
        print_int(serial(prev_break(passive)));
}

@ @c
static void
print_feasible_break(halfword cur_p, pointer r, halfword b, int pi,
                     int d, boolean artificial_demerits)
{
    /* Print a symbolic description of this feasible break; */
    if (printed_node != cur_p) {
        /* Print the list between |printed_node| and |cur_p|, then
           set |printed_node:=cur_p|; */
        tprint_nl("");
        if (cur_p == null) {
            short_display(vlink(printed_node));
        } else {
            halfword save_link = vlink(cur_p);
            vlink(cur_p) = null;
            tprint_nl("");
            short_display(vlink(printed_node));
            vlink(cur_p) = save_link;
        }
        printed_node = cur_p;
    }
    tprint_nl("@@");
    if (cur_p == null) {
        tprint_esc("par");
    } else if (type(cur_p) != glue_node) {
        if (type(cur_p) == penalty_node)
            tprint_esc("penalty");
        else if (type(cur_p) == disc_node)
            tprint_esc("discretionary");
        else if (type(cur_p) == kern_node)
            tprint_esc("kern");
        else
            tprint_esc("math");
    }
    tprint(" via @@");
    if (break_node(r) == null)
        print_char('0');
    else
        print_int(serial(break_node(r)));
    tprint(" b=");
    if (b > inf_bad)
        print_char('*');
    else
        print_int(b);
    tprint(" p=");
    print_int(pi);
    tprint(" d=");
    if (artificial_demerits)
        print_char('*');
    else
        print_int(d);
}

@ @c
#define add_disc_width_to_active_width(a)   active_width[a] += disc_width[a]
#define update_width(a) cur_active_width[a] += varmem[(r+(a))].cint

#define set_break_width_to_background(a) break_width[a]=background[(a)]

#define convert_to_break_width(a)  \
  varmem[(prev_r+(a))].cint = varmem[(prev_r+(a))].cint-cur_active_width[(a)]+break_width[(a)]

#define store_break_width(a)      active_width[(a)]=break_width[(a)]

#define new_delta_to_break_width(a)  \
  varmem[(q+(a))].cint=break_width[(a)]-cur_active_width[(a)]

#define new_delta_from_break_width(a)  \
  varmem[(q+(a))].cint=cur_active_width[(a)]-break_width[(a)]

#define copy_to_cur_active(a) cur_active_width[(a)]=active_width[(a)]

#define combine_two_deltas(a) varmem[(prev_r+(a))].cint += varmem[(r+(a))].cint
#define downdate_width(a) cur_active_width[(a)] -= varmem[(prev_r+(a))].cint
#define update_active(a) active_width[(a)]+=varmem[(r+(a))].cint

#define total_font_stretch cur_active_width[8]
#define total_font_shrink cur_active_width[9]

#define cal_margin_kern_var(a) {  \
  character(cp) = character((a));  \
  font(cp) = font((a));  \
  do_subst_font(cp, 1000);  \
  if (font(cp) != font((a)))  \
    margin_kern_stretch += (left_pw((a)) - left_pw(cp));        \
  font(cp) = font((a));  \
  do_subst_font(cp, -1000);  \
  if (font(cp) != font((a)))  \
    margin_kern_shrink += (left_pw(cp) - left_pw((a))); \
  }

static void
ext_try_break(int pi,
              quarterword break_type,
              int line_break_dir,
              int pdf_adjust_spacing,
              int par_shape_ptr,
              int adj_demerits,
              int tracing_paragraphs,
              int pdf_protrude_chars,
              int line_penalty,
              int last_line_fit,
              int double_hyphen_demerits,
              int final_hyphen_demerits, halfword first_p, halfword cur_p)
{
    /* labels: |CONTINUE,DEACTIVATE,FOUND,NOT_FOUND|; */
    pointer r;                  /* runs through the active list */
    scaled margin_kern_stretch;
    scaled margin_kern_shrink;
    halfword lp, rp, cp;
    halfword prev_r;            /* stays a step behind |r| */
    halfword prev_prev_r;       /*a step behind |prev_r|, if |type(prev_r)=delta_node| */
    halfword old_l;             /* maximum line number in current equivalence class of lines */
    boolean no_break_yet;       /* have we found a feasible break at |cur_p|? */
    halfword q;                 /*points to a new node being created */
    halfword l;                 /*line number of current active node */
    boolean node_r_stays_active;        /*should node |r| remain in the active list? */
    scaled line_width;          /*the current line will be justified to this width */
    fitness_value fit_class;    /*possible fitness class of test line */
    halfword b;                 /*badness of test line */
    int d;                      /*demerits of test line */
    boolean artificial_demerits;        /*has |d| been forced to zero? */

    scaled shortfall;           /*used in badness calculations */
    scaled g;                   /*glue stretch or shrink of test line, adjustment for last line */
    scaled cur_active_width[10] = { 0 };        /*distance from current active node */

    line_width = 0;
    g = 0;
    prev_prev_r = null;
    /*Make sure that |pi| is in the proper range; */
    if (pi >= inf_penalty) {
        return;                 /* this breakpoint is inhibited by infinite penalty */
    } else if (pi <= -inf_penalty) {
        pi = eject_penalty;     /*this breakpoint will be forced */
    }

    no_break_yet = true;
    prev_r = active;
    old_l = 0;
    do_all_eight(copy_to_cur_active);

    while (1) {
        r = vlink(prev_r);
        /* If node |r| is of type |delta_node|, update |cur_active_width|,
           set |prev_r| and |prev_prev_r|, then |goto continue|; */
        /* The following code uses the fact that |type(active)<>delta_node| */
        if (type(r) == delta_node) {
            do_all_eight(update_width); /* IMPLICIT ,r */
            prev_prev_r = prev_r;
            prev_r = r;
            continue;
        }
        /* If a line number class has ended, create new active nodes for
           the best feasible breaks in that class; then |return|
           if |r=active|, otherwise compute the new |line_width|; */
        /* The first part of the following code is part of \TeX's inner loop, so
           we don't want to waste any time. The current active node, namely node |r|,
           contains the line number that will be considered next. At the end of the
           list we have arranged the data structure so that |r=active| and
           |line_number(active)>old_l|.
         */
        l = line_number(r);
        if (l > old_l) {        /* now we are no longer in the inner loop */
            if ((minimum_demerits < awful_bad)
                && ((old_l != easy_line) || (r == active))) {
                /*Create new active nodes for the best feasible breaks just found */
                /* It is not necessary to create new active nodes having |minimal_demerits|
                   greater than
                   |minimum_demerits+abs(adj_demerits)|, since such active nodes will never
                   be chosen in the final paragraph breaks. This observation allows us to
                   omit a substantial number of feasible breakpoints from further consideration.
                 */
                if (no_break_yet) {
                    no_break_yet = false;
                    do_all_eight(set_break_width_to_background);
                    compute_break_width(break_type, line_break_dir,
                                        pdf_adjust_spacing, cur_p);
                }
                /* Insert a delta node to prepare for breaks at |cur_p|; */
                /* We use the fact that |type(active)<>delta_node|. */
                if (type(prev_r) == delta_node) {       /* modify an existing delta node */
                    do_all_eight(convert_to_break_width);       /* IMPLICIT |prev_r| */
                } else if (prev_r == active) {  /* no delta node needed at the beginning */
                    do_all_eight(store_break_width);
                } else {
                    q = new_node(delta_node, 0);
                    vlink(q) = r;
                    do_all_eight(new_delta_to_break_width);     /* IMPLICIT q */
                    vlink(prev_r) = q;
                    prev_prev_r = prev_r;
                    prev_r = q;
                }

                if (abs(adj_demerits) >= awful_bad - minimum_demerits)
                    minimum_demerits = awful_bad - 1;
                else
                    minimum_demerits += abs(adj_demerits);
                for (fit_class = very_loose_fit; fit_class <= tight_fit;
                     fit_class++) {
                    if (minimal_demerits[fit_class] <= minimum_demerits) {
                        /* Insert a new active node from |best_place[fit_class]|
                           to |cur_p|; */
                        /* When we create an active node, we also create the corresponding
                           passive node.
                         */
                        q = new_node(passive_node, 0);
                        vlink(q) = passive;
                        passive = q;
                        cur_break(q) = cur_p;
                        incr(pass_number);
                        serial(q) = pass_number;
                        prev_break(q) = best_place[fit_class];
                        /*Here we keep track of the subparagraph penalties in the break nodes */
                        passive_pen_inter(q) = internal_pen_inter;
                        passive_pen_broken(q) = internal_pen_broken;
                        passive_last_left_box(q) = internal_left_box;
                        passive_last_left_box_width(q) =
                            internal_left_box_width;
                        if (prev_break(q) != null) {
                            passive_left_box(q) =
                                passive_last_left_box(prev_break(q));
                            passive_left_box_width(q) =
                                passive_last_left_box_width(prev_break(q));
                        } else {
                            passive_left_box(q) = init_internal_left_box;
                            passive_left_box_width(q) =
                                init_internal_left_box_width;
                        }
                        passive_right_box(q) = internal_right_box;
                        passive_right_box_width(q) = internal_right_box_width;
                        q = new_node(break_type, fit_class);
                        break_node(q) = passive;
                        line_number(q) = best_pl_line[fit_class] + 1;
                        total_demerits(q) = minimal_demerits[fit_class];
                        if (do_last_line_fit) {
                            /*Store additional data in the new active node */
                            /* Here we save these data in the active node
                               representing a potential line break. */
                            active_short(q) = best_pl_short[fit_class];
                            active_glue(q) = best_pl_glue[fit_class];
                        }
                        vlink(q) = r;
                        vlink(prev_r) = q;
                        prev_r = q;
                        if (tracing_paragraphs > 0)
                            print_break_node(q, fit_class, break_type, cur_p);
                    }
                    minimal_demerits[fit_class] = awful_bad;
                }
                minimum_demerits = awful_bad;
                /* Insert a delta node to prepare for the next active node; */
                /* When the following code is performed, we will have just inserted at
                   least one active node before |r|, so |type(prev_r)<>delta_node|. 
                 */
                if (r != active) {
                    q = new_node(delta_node, 0);
                    vlink(q) = r;
                    do_all_eight(new_delta_from_break_width);   /* IMPLICIT q */
                    vlink(prev_r) = q;
                    prev_prev_r = prev_r;
                    prev_r = q;
                }
            }
            if (r == active)
                return;
            /*Compute the new line width; */
            /* When we come to the following code, we have just encountered
               the first active node~|r| whose |line_number| field contains
               |l|. Thus we want to compute the length of the $l\mskip1mu$th
               line of the current paragraph. Furthermore, we want to set
               |old_l| to the last number in the class of line numbers
               equivalent to~|l|.
             */
            if (l > easy_line) {
                old_l = max_halfword - 1;
                line_width = second_width;
            } else {
                old_l = l;
                if (l > last_special_line) {
                    line_width = second_width;
                } else if (par_shape_ptr == null) {
                    line_width = first_width;
                } else {
                    line_width = varmem[(par_shape_ptr + 2 * l + 1)].cint;
                }
            }
        }
        /* /If a line number class has ended, create new active nodes for
           the best feasible breaks in that class; then |return|
           if |r=active|, otherwise compute the new |line_width|; */

        /* Consider the demerits for a line from |r| to |cur_p|;
           deactivate node |r| if it should no longer be active;
           then |goto continue| if a line from |r| to |cur_p| is infeasible,
           otherwise record a new feasible break; */
        artificial_demerits = false;
        shortfall = line_width - cur_active_width[1];
        if (break_node(r) == null)
            shortfall -= init_internal_left_box_width;
        else
            shortfall -= passive_last_left_box_width(break_node(r));
        shortfall -= internal_right_box_width;
        if (pdf_protrude_chars > 1) {
            halfword l1, o;
            l1 = (break_node(r) == null) ? first_p : cur_break(break_node(r));
            if (cur_p == null) {
                o = null;
            } else {            /* TODO |if (is_character_node(alink(cur_p)))| */
                o = alink(cur_p);
                assert(vlink(o) == cur_p);
            }
            /* MAGIC: the disc could be a SELECT subtype, to we might need
             to get the last character as |pre_break| from either the
             |pre_break| list (if the previous INIT disc was taken), or the
             |no_break| (sic) list (if the previous INIT disc was not taken)
             
             BUT:
             the last characters (hyphenation character) if these two list
             should always be the same anyway, so we just look at |pre_break|.
             */
            /* let's look at the right margin first */
            if ((cur_p != null) && (type(cur_p) == disc_node)
                && (vlink_pre_break(cur_p) != null)) {
                /* a |disc_node| with non-empty |pre_break|, protrude the last char of |pre_break| */
                o = tlink_pre_break(cur_p);
            } else {
                o = find_protchar_right(l1, o);
            }
            /* now the left margin */
            if ((l1 != null) && (type(l1) == disc_node)
                && (vlink_post_break(l1) != null)) {
                /* FIXME: first 'char' could be a disc! */
                l1 = vlink_post_break(l1);        /* protrude the first char */
            } else {
                l1 = find_protchar_left(l1, true);
            }
            shortfall += (left_pw(l1) + right_pw(o));
        }
        if ((shortfall != 0) && (pdf_adjust_spacing > 1)) {
            margin_kern_stretch = 0;
            margin_kern_shrink = 0;
            if (pdf_protrude_chars > 1) {
                /* Calculate variations of marginal kerns; */
                lp = last_leftmost_char;
                rp = last_rightmost_char;
                cp = raw_glyph_node();
                if (lp != null) {
                    cal_margin_kern_var(lp);
                }
                if (rp != null) {
                    cal_margin_kern_var(rp);
                }
                flush_node(cp);
            }
            if ((shortfall > 0)
                && ((total_font_stretch + margin_kern_stretch) > 0)) {
                if ((total_font_stretch + margin_kern_stretch) > shortfall)
                    shortfall = ((total_font_stretch + margin_kern_stretch) /
                                 (max_stretch_ratio / cur_font_step)) / 2;
                else
                    shortfall -= (total_font_stretch + margin_kern_stretch);
            } else if ((shortfall < 0)
                       && ((total_font_shrink + margin_kern_shrink) > 0)) {
                if ((total_font_shrink + margin_kern_shrink) > -shortfall)
                    shortfall = -((total_font_shrink + margin_kern_shrink) /
                                  (max_shrink_ratio / cur_font_step)) / 2;
                else
                    shortfall += (total_font_shrink + margin_kern_shrink);
            }
        }
        if (shortfall > 0) {
            /* Set the value of |b| to the badness for stretching the line,
               and compute the corresponding |fit_class| */

            /* When a line must stretch, the available stretchability can be
               found in the subarray |cur_active_width[2..6]|, in units of
               points, sfi, fil, fill and filll.

               The present section is part of \TeX's inner loop, and it is
               most often performed when the badness is infinite; therefore
               it is worth while to make a quick test for large width excess
               and small stretchability, before calling the |badness|
               subroutine. */

            if ((cur_active_width[3] != 0) || (cur_active_width[4] != 0) ||
                (cur_active_width[5] != 0) || (cur_active_width[6] != 0)) {
                if (do_last_line_fit) {
                    if (cur_p == null) {        /* the last line of a paragraph */
                        /* Perform computations for last line and |goto found|; */

                        /* Here we compute the adjustment |g| and badness |b| for
                           a line from |r| to the end of the paragraph.  When any
                           of the criteria for adjustment is violated we fall
                           through to the normal algorithm.

                           The last line must be too short, and have infinite
                           stretch entirely due to |par_fill_skip|. */
                        if ((active_short(r) == 0) || (active_glue(r) <= 0))
                            /* previous line was neither stretched nor shrunk, or
                               was infinitely bad */
                            goto NOT_FOUND;
                        if ((cur_active_width[3] != fill_width[0]) ||
                            (cur_active_width[4] != fill_width[1]) ||
                            (cur_active_width[5] != fill_width[2]) ||
                            (cur_active_width[6] != fill_width[3]))
                            /* infinite stretch of this line not entirely due to
                               |par_fill_skip| */
                            goto NOT_FOUND;
                        if (active_short(r) > 0)
                            g = cur_active_width[2];
                        else
                            g = cur_active_width[7];
                        if (g <= 0)
                            /*no finite stretch resp.\ no shrink */
                            goto NOT_FOUND;
                        arith_error = false;
                        g = fract(g, active_short(r), active_glue(r),
                                  max_dimen);
                        if (last_line_fit < 1000)
                            g = fract(g, last_line_fit, 1000, max_dimen);
                        if (arith_error) {
                            if (active_short(r) > 0)
                                g = max_dimen;
                            else
                                g = -max_dimen;
                        }
                        if (g > 0) {
                            /*Set the value of |b| to the badness of the last line
                               for stretching, compute the corresponding |fit_class,
                               and |goto found|| */
                            /* These badness computations are rather similar to
                               those of the standard algorithm, with the adjustment
                               amount |g| replacing the |shortfall|. */
                            if (g > shortfall)
                                g = shortfall;
                            if (g > 7230584) {
                                if (cur_active_width[2] < 1663497) {
                                    b = inf_bad;
                                    fit_class = very_loose_fit;
                                    goto FOUND;
                                }
                            }
                            b = badness(g, cur_active_width[2]);
                            if (b > 99) {
                                fit_class = very_loose_fit;
                            } else if (b > 12) {
                                fit_class = loose_fit;
                            } else {
                                fit_class = decent_fit;
                            }
                            goto FOUND;
                        } else if (g < 0) {
                            /*Set the value of |b| to the badness of the last line
                               for shrinking, compute the corresponding |fit_class,
                               and |goto found||; */
                            if (-g > cur_active_width[7])
                                g = -cur_active_width[7];
                            b = badness(-g, cur_active_width[7]);
                            if (b > 12)
                                fit_class = tight_fit;
                            else
                                fit_class = decent_fit;
                            goto FOUND;
                        }
                    }
                  NOT_FOUND:
                    shortfall = 0;
                }
                b = 0;
                fit_class = decent_fit; /* infinite stretch */
            } else {
                if (shortfall > 7230584 && cur_active_width[2] < 1663497) {
                    b = inf_bad;
                    fit_class = very_loose_fit;
                } else {
                    b = badness(shortfall, cur_active_width[2]);
                    if (b > 99) {
                        fit_class = very_loose_fit;
                    } else if (b > 12) {
                        fit_class = loose_fit;
                    } else {
                        fit_class = decent_fit;
                    }
                }
            }
        } else {
            /* Set the value of |b| to the badness for shrinking the line,
               and compute the corresponding |fit_class|; */
            /* Shrinkability is never infinite in a paragraph; we can shrink
               the line from |r| to |cur_p| by at most
               |cur_active_width[7]|. */
            if (-shortfall > cur_active_width[7])
                b = inf_bad + 1;
            else
                b = badness(-shortfall, cur_active_width[7]);
            if (b > 12)
                fit_class = tight_fit;
            else
                fit_class = decent_fit;
        }
        if (do_last_line_fit) {
            /* Adjust the additional data for last line; */
            if (cur_p == null)
                shortfall = 0;
            if (shortfall > 0) {
                g = cur_active_width[2];
            } else if (shortfall < 0) {
                g = cur_active_width[7];
            } else {
                g = 0;
            }
        }
      FOUND:
        if ((b > inf_bad) || (pi == eject_penalty)) {
            /* Prepare to deactivate node~|r|, and |goto deactivate| unless
               there is a reason to consider lines of text from |r| to |cur_p| */
            /* During the final pass, we dare not lose all active nodes, lest we lose
               touch with the line breaks already found. The code shown here makes
               sure that such a catastrophe does not happen, by permitting overfull
               boxes as a last resort. This particular part of \TeX\ was a source of
               several subtle bugs before the correct program logic was finally
               discovered; readers who seek to ``improve'' \TeX\ should therefore
               think thrice before daring to make any changes here.
             */
            if (final_pass && (minimum_demerits == awful_bad) &&
                (vlink(r) == active) && (prev_r == active)) {
                artificial_demerits = true;     /* set demerits zero, this break is forced */
            } else if (b > threshold) {
                goto DEACTIVATE;
            }
            node_r_stays_active = false;
        } else {
            prev_r = r;
            if (b > threshold)
                continue;
            node_r_stays_active = true;
        }
        /* Record a new feasible break; */
        /* When we get to this part of the code, the line from |r| to |cur_p| is
           feasible, its badness is~|b|, and its fitness classification is
           |fit_class|.  We don't want to make an active node for this break yet,
           but we will compute the total demerits and record them in the
           |minimal_demerits| array, if such a break is the current champion among
           all ways to get to |cur_p| in a given line-number class and fitness
           class.
         */
        if (artificial_demerits) {
            d = 0;
        } else {
            /* Compute the demerits, |d|, from |r| to |cur_p|; */
            d = line_penalty + b;
            if (abs(d) >= 10000)
                d = 100000000;
            else
                d = d * d;
            if (pi != 0) {
                if (pi > 0) {
                    d += (pi * pi);
                } else if (pi > eject_penalty) {
                    d -= (pi * pi);
                }
            }
            if ((break_type == hyphenated_node) && (type(r) == hyphenated_node)) {
                if (cur_p != null)
                    d += double_hyphen_demerits;
                else
                    d += final_hyphen_demerits;
            }
            if (abs(fit_class - fitness(r)) > 1)
                d = d + adj_demerits;
        }
        if (tracing_paragraphs > 0)
            print_feasible_break(cur_p, r, b, pi, d, artificial_demerits);
        d += total_demerits(r); /*this is the minimum total demerits
                                   from the beginning to |cur_p| via |r| */
        if (d <= minimal_demerits[fit_class]) {
            minimal_demerits[fit_class] = d;
            best_place[fit_class] = break_node(r);
            best_pl_line[fit_class] = l;
            if (do_last_line_fit) {
                /* Store additional data for this feasible break; */
                /* For each feasible break we record the shortfall and glue stretch or
                   shrink (or adjustment). */
                best_pl_short[fit_class] = shortfall;
                best_pl_glue[fit_class] = g;
            }
            if (d < minimum_demerits)
                minimum_demerits = d;
        }
        /* /Record a new feasible break */
        if (node_r_stays_active)
            continue;           /*|prev_r| has been set to |r| */
      DEACTIVATE:
        /* Deactivate node |r|; */
        /* When an active node disappears, we must delete an adjacent delta node if
           the active node was at the beginning or the end of the active list, or
           if it was surrounded by delta nodes. We also must preserve the property
           that |cur_active_width| represents the length of material from
           |vlink(prev_r)| to~|cur_p|. */

        vlink(prev_r) = vlink(r);
        flush_node(r);
        if (prev_r == active) {
            /*Update the active widths, since the first active node has been
               deleted */
            /* The following code uses the fact that |type(active)<>delta_node|.
               If the active list has just become empty, we do not need to update the
               |active_width| array, since it will be initialized when an active
               node is next inserted.
             */
            r = vlink(active);
            if (type(r) == delta_node) {
                do_all_eight(update_active);    /* IMPLICIT r */
                do_all_eight(copy_to_cur_active);
                vlink(active) = vlink(r);
                flush_node(r);
            }
        } else if (type(prev_r) == delta_node) {
            r = vlink(prev_r);
            if (r == active) {
                do_all_eight(downdate_width);   /* IMPLICIT |prev_r| */
                vlink(prev_prev_r) = active;
                flush_node(prev_r);
                prev_r = prev_prev_r;
            } else if (type(r) == delta_node) {
                do_all_eight(update_width);     /* IMPLICIT ,r */
                do_all_eight(combine_two_deltas);       /* IMPLICIT r |prev_r| */
                vlink(prev_r) = vlink(r);
                flush_node(r);
            }
        }
    }
}

@ @c
void
ext_do_line_break(int paragraph_dir,
                  int pretolerance,
                  int tracing_paragraphs,
                  int tolerance,
                  scaled emergency_stretch,
                  int looseness,
                  int hyphen_penalty,
                  int ex_hyphen_penalty,
                  int pdf_adjust_spacing,
                  halfword par_shape_ptr,
                  int adj_demerits,
                  int pdf_protrude_chars,
                  int line_penalty,
                  int last_line_fit,
                  int double_hyphen_demerits,
                  int final_hyphen_demerits,
                  int hang_indent,
                  int hsize,
                  int hang_after,
                  halfword left_skip,
                  halfword right_skip,
                  int pdf_each_line_height,
                  int pdf_each_line_depth,
                  int pdf_first_line_height,
                  int pdf_last_line_depth,
                  halfword inter_line_penalties_ptr,
                  int inter_line_penalty,
                  int club_penalty,
                  halfword club_penalties_ptr,
                  halfword widow_penalties_ptr,
                  int widow_penalty,
                  int broken_penalty,
                  halfword final_par_glue, halfword pdf_ignored_dimen)
{
    /* DONE,DONE1,DONE2,DONE3,DONE4,DONE5,CONTINUE; */
    halfword cur_p, q, r, s;    /* miscellaneous nodes of temporary interest */
    int line_break_dir = paragraph_dir;

    /* Get ready to start ... */
    minimum_demerits = awful_bad;
    minimal_demerits[tight_fit] = awful_bad;
    minimal_demerits[decent_fit] = awful_bad;
    minimal_demerits[loose_fit] = awful_bad;
    minimal_demerits[very_loose_fit] = awful_bad;
                  
    /* We compute the values of |easy_line| and the other local variables relating
       to line length when the |line_break| procedure is initializing itself. */
    if (par_shape_ptr == null) {
        if (hang_indent == 0) {
            last_special_line = 0;
            second_width = hsize;
            second_indent = 0;
        } else {
            /*  Set line length parameters in preparation for hanging indentation */
            /* We compute the values of |easy_line| and the other local variables relating
               to line length when the |line_break| procedure is initializing itself. */
            last_special_line = abs(hang_after);
            if (hang_after < 0) {
                first_width = hsize - abs(hang_indent);
                if (hang_indent >= 0)
                    first_indent = hang_indent;
                else
                    first_indent = 0;
                second_width = hsize;
                second_indent = 0;
            } else {
                first_width = hsize;
                first_indent = 0;
                second_width = hsize - abs(hang_indent);
                if (hang_indent >= 0)
                    second_indent = hang_indent;
                else
                    second_indent = 0;
            }
        }
    } else {
        last_special_line = vinfo(par_shape_ptr + 1) - 1;
        second_indent =
            varmem[(par_shape_ptr + 2 * (last_special_line + 1))].cint;
        second_width =
            varmem[(par_shape_ptr + 2 * (last_special_line + 1) + 1)].cint;
    }
    if (looseness == 0)
        easy_line = last_special_line;
    else
        easy_line = max_halfword;

    no_shrink_error_yet = true;
    check_shrinkage(left_skip);
    check_shrinkage(right_skip);
    q = left_skip;
    r = right_skip;
    background[1] = width(q) + width(r);
    background[2] = 0;
    background[3] = 0;
    background[4] = 0;
    background[5] = 0;
    background[6] = 0;
    background[2 + stretch_order(q)] = stretch(q);
    background[2 + stretch_order(r)] += stretch(r);
    background[7] = shrink(q) + shrink(r);
    if (pdf_adjust_spacing > 1) {
        background[8] = 0;
        background[9] = 0;
        max_stretch_ratio = -1;
        max_shrink_ratio = -1;
        cur_font_step = -1;
        set_prev_char_p(null);
    }
    /* Check for special treatment of last line of paragraph; */
    /* The new algorithm for the last line requires that the stretchability
       |par_fill_skip| is infinite and the stretchability of |left_skip| plus
       |right_skip| is finite.
     */
    do_last_line_fit = false;
    if (last_line_fit > 0) {
        q = glue_ptr(last_line_fill);
        if ((stretch(q) > 0) && (stretch_order(q) > normal)) {
            if ((background[3] == 0) && (background[4] == 0) &&
                (background[5] == 0) && (background[6] == 0)) {
                do_last_line_fit = true;
                fill_width[0] = 0;
                fill_width[1] = 0;
                fill_width[2] = 0;
                fill_width[3] = 0;
                fill_width[stretch_order(q) - 1] = stretch(q);
            }
        }
    }
    /* DIR: Initialize |dir_ptr| for |line_break| */
    if (dir_ptr != null) {
        flush_node_list(dir_ptr);
        dir_ptr = null;
    }
#if 0
    push_dir(paragraph_dir,dir_ptr); /* TODO what was the point of this? */
#endif

    /* Find optimal breakpoints; */
    threshold = pretolerance;
    if (threshold >= 0) {
        if (tracing_paragraphs > 0) {
            begin_diagnostic();
            tprint_nl("@@firstpass");
        }
        second_pass = false;
        final_pass = false;
    } else {
        threshold = tolerance;
        second_pass = true;
        final_pass = (emergency_stretch <= 0);
        if (tracing_paragraphs > 0)
            begin_diagnostic();
    }

    while (1) {
        halfword first_p;
        halfword nest_stack[10];
        int nest_index = 0;
        if (threshold > inf_bad)
            threshold = inf_bad;
        /* Create an active breakpoint representing the beginning of the paragraph */
        q = new_node(unhyphenated_node, decent_fit);
        vlink(q) = active;
        break_node(q) = null;
        line_number(q) = cur_list.pg_field + 1;
        total_demerits(q) = 0;
        active_short(q) = 0;
        active_glue(q) = 0;
        vlink(active) = q;
        do_all_eight(store_background);
        passive = null;
        printed_node = temp_head;
        pass_number = 0;
        font_in_short_display = null_font;
        /* /Create an active breakpoint representing the beginning of the paragraph */
        auto_breaking = true;
        cur_p = vlink(temp_head);
        /* LOCAL: Initialize with first |local_paragraph| node */
        if ((cur_p != null) && (type(cur_p) == whatsit_node)
            && (subtype(cur_p) == local_par_node)) {
	    alink(cur_p) = temp_head; /* this used to be an assert, but may as well force it */
            internal_pen_inter = local_pen_inter(cur_p);
            internal_pen_broken = local_pen_broken(cur_p);
            init_internal_left_box = local_box_left(cur_p);
            init_internal_left_box_width = local_box_left_width(cur_p);
            internal_left_box = init_internal_left_box;
            internal_left_box_width = init_internal_left_box_width;
            internal_right_box = local_box_right(cur_p);
            internal_right_box_width = local_box_right_width(cur_p);
        } else {
            internal_pen_inter = 0;
            internal_pen_broken = 0;
            init_internal_left_box = null;
            init_internal_left_box_width = 0;
            internal_left_box = init_internal_left_box;
            internal_left_box_width = init_internal_left_box_width;
            internal_right_box = null;
            internal_right_box_width = 0;
        }
        /* /LOCAL: Initialize with first |local_paragraph| node */
        set_prev_char_p(null);
        first_p = cur_p;
        /* to access the first node of paragraph as the first active node
           has |break_node=null| */
        while ((cur_p != null) && (vlink(active) != active)) {
            /* |try_break| if |cur_p| is a legal breakpoint; on the 2nd pass, also look at |disc_node|s. */

            while (is_char_node(cur_p)) {
                /* Advance |cur_p| to the node following the present string of characters ; */
                /* The code that passes over the characters of words in a paragraph is part of
                   \TeX's inner loop, so it has been streamlined for speed. We use the fact that
                   `\.{\\parfillskip}' glue appears at the end of each paragraph; it is therefore
                   unnecessary to check if |vlink(cur_p)=null| when |cur_p| is a character node.
                 */
                active_width[1] +=
                    pack_width(line_break_dir, dir_TRT, cur_p, true);
                if ((pdf_adjust_spacing > 1) && check_expand_pars(font(cur_p))) {
                    set_prev_char_p(cur_p);
                    add_char_stretch(active_width[8], cur_p);
                    add_char_shrink(active_width[9], cur_p);
                }
                cur_p = vlink(cur_p);
                while (cur_p == null && nest_index > 0) {
                    cur_p = nest_stack[--nest_index];
#ifdef DEBUG
                    fprintf(stderr,"Node Pop  %d [%d]\n",nest_index,(int)cur_p); 
#endif
                }
            }
            if (cur_p == null) {        /* TODO */
                confusion("linebreak_tail");
            }
            /* Determine legal breaks: As we move through the hlist, we need to keep
               the |active_width| array up to date, so that the badness of individual
               lines is readily calculated by |try_break|. It is convenient to use the
               short name |active_width[1]| for the component of active width that represents
               real width as opposed to glue. */

            switch (type(cur_p)) {
            case hlist_node:
            case vlist_node:
                active_width[1] +=
                    pack_width(line_break_dir, box_dir(cur_p), cur_p, false);
                break;
            case rule_node:
                active_width[1] += width(cur_p);
                break;
            case whatsit_node:
                /* Advance past a whatsit node in the |line_break| loop; */
                switch (subtype(cur_p)) {
                case local_par_node:   /* LOCAL: Advance past a |local_paragraph| node; */
                    internal_pen_inter = local_pen_inter(cur_p);
                    internal_pen_broken = local_pen_broken(cur_p);
                    internal_left_box = local_box_left(cur_p);
                    internal_left_box_width = local_box_left_width(cur_p);
                    internal_right_box = local_box_right(cur_p);
                    internal_right_box_width = local_box_right_width(cur_p);
                    break;
                case dir_node: /* DIR: Adjust the dir stack for the |line_break| routine; */
                    if (dir_dir(cur_p) >= 0) {
                        line_break_dir = dir_dir(cur_p);
                        push_dir_node(cur_p,dir_ptr);   /* adds to |dir_ptr| */
                    } else {
                        pop_dir_node(dir_ptr);
                        if (dir_ptr != null)
                            line_break_dir = dir_dir(dir_ptr);
                    }
                    break;
                case pdf_refxform_node:
                case pdf_refximage_node:
                    active_width[1] += width(cur_p);
                }
                /* / Advance past a whatsit node in the |line_break| loop/; */
                break;
            case glue_node:
                /* If node |cur_p| is a legal breakpoint, call |try_break|;
                   then update the active widths by including the glue in
                   |glue_ptr(cur_p)|; */
                /* When node |cur_p| is a glue node, we look at the previous to
                   see whether or not a breakpoint is legal at |cur_p|, as
                   explained above. */
                /* *INDENT-OFF* */
                if (auto_breaking) {
                    halfword prev_p = alink(cur_p);
                    if (prev_p != temp_head &&
                        (is_char_node(prev_p) ||
                         precedes_break(prev_p) || 
                         ((type(prev_p) == kern_node)
                          && (subtype(prev_p) != explicit)))) {
                        ext_try_break(0, unhyphenated_node, line_break_dir, pdf_adjust_spacing,
                                      par_shape_ptr, adj_demerits,
                                      tracing_paragraphs, pdf_protrude_chars,
                                      line_penalty, last_line_fit,
                                      double_hyphen_demerits,
                                      final_hyphen_demerits, first_p, cur_p);
                    }
                }
                /* *INDENT-ON* */
                check_shrinkage(glue_ptr(cur_p));
                q = glue_ptr(cur_p);
                active_width[1] += width(q);
                active_width[2 + stretch_order(q)] += stretch(q);
                active_width[7] += shrink(q);
                break;
            case kern_node:
                if (subtype(cur_p) == explicit) {
                    kern_break();
                } else {
                    active_width[1] += width(cur_p);
                    if ((pdf_adjust_spacing > 1) && (subtype(cur_p) == normal)) {
                        add_kern_stretch(active_width[8], cur_p);
                        add_kern_shrink(active_width[9], cur_p);
                    }
                }
                break;
            case disc_node:
                /* |select_disc|s are handled by the leading |init_disc| */
                if (subtype(cur_p) == select_disc)
                    break;
                /* Try to break after a discretionary fragment, then |goto done5|; */
                /* The following code knows that discretionary texts contain
                   only character nodes, kern nodes, box nodes, and rule
                   nodes. */
                if (second_pass) {
                    int actual_penalty = hyphen_penalty;
                    if (subtype(cur_p) == automatic_disc)
                        actual_penalty = ex_hyphen_penalty;
                    s = vlink_pre_break(cur_p);
                    do_one_seven_eight(reset_disc_width);
                    if (s == null) {    /* trivial pre-break */
                        ext_try_break(actual_penalty, hyphenated_node,
                                      line_break_dir, pdf_adjust_spacing,
                                      par_shape_ptr, adj_demerits,
                                      tracing_paragraphs, pdf_protrude_chars,
                                      line_penalty, last_line_fit,
                                      double_hyphen_demerits,
                                      final_hyphen_demerits, first_p, cur_p);
                    } else {
                        add_to_widths(s, line_break_dir, pdf_adjust_spacing,
                                      disc_width);
                        do_one_seven_eight(add_disc_width_to_active_width);
                        ext_try_break(actual_penalty, hyphenated_node,
                                      line_break_dir, pdf_adjust_spacing,
                                      par_shape_ptr, adj_demerits,
                                      tracing_paragraphs, pdf_protrude_chars,
                                      line_penalty, last_line_fit,
                                      double_hyphen_demerits,
                                      final_hyphen_demerits, first_p, cur_p);
                        if (subtype(cur_p) == init_disc) {
                            /* we should at two break points after the one we
                             added above: 
                             \item1 which does a possible break in INIT's |post_break|
                             \item2 which means the |no_break| actually was broken
                             just a character later */
                            /* do the select-0 case 'f-f-i' */
                            assert(type(vlink(cur_p)) == disc_node &&
                                   subtype(vlink(cur_p)) == select_disc);
                            s = vlink_pre_break(vlink(cur_p));
                            add_to_widths(s, line_break_dir, pdf_adjust_spacing,
                                          disc_width);
                            ext_try_break(actual_penalty, hyphenated_node,
                                          line_break_dir, pdf_adjust_spacing,
                                          par_shape_ptr, adj_demerits,
                                          tracing_paragraphs,
                                          pdf_protrude_chars, line_penalty,
                                          last_line_fit, double_hyphen_demerits,
                                          final_hyphen_demerits, first_p,
                                          vlink(cur_p));
#if 0
                            /* TODO this does not work */
                            /* go back to the starting situation */
                            do_one_seven_eight
                                (sub_disc_width_from_active_width);
                            do_one_seven_eight(reset_disc_width);
                            /* add select |no_break| to |active_width| */
                            s = vlink_no_break(vlink(cur_p));
                            add_to_widths(s, line_break_dir, pdf_adjust_spacing,
                                          disc_width);
                            ext_try_break(actual_penalty, hyphenated_node,
                                          line_break_dir, pdf_adjust_spacing,
                                          par_shape_ptr, adj_demerits,
                                          tracing_paragraphs,
                                          pdf_protrude_chars, line_penalty,
                                          last_line_fit, double_hyphen_demerits,
                                          final_hyphen_demerits, first_p,
                                          vlink(cur_p));
#endif
                        }
                        do_one_seven_eight(sub_disc_width_from_active_width);
                    }
                }
                s = vlink_no_break(cur_p);
                add_to_widths(s, line_break_dir, pdf_adjust_spacing,
                              active_width);
                break;
            case math_node:
                auto_breaking = (subtype(cur_p) == after);
                kern_break();
                break;
            case penalty_node:
                ext_try_break(penalty(cur_p), unhyphenated_node, line_break_dir,
                              pdf_adjust_spacing, par_shape_ptr, adj_demerits,
                              tracing_paragraphs, pdf_protrude_chars,
                              line_penalty, last_line_fit,
                              double_hyphen_demerits, final_hyphen_demerits,
                              first_p, cur_p);
                break;
            case mark_node:
            case ins_node:
            case adjust_node:
                break;
            case glue_spec_node:
                fprintf(stdout, "\nfound a glue_spec in a paragraph!");
                break;
            default:
                fprintf(stdout, "\ntype=%d", type(cur_p));
                confusion("paragraph");
            }
            cur_p = vlink(cur_p);
            while (cur_p == null && nest_index > 0) {
                cur_p = nest_stack[--nest_index];
#ifdef DEBUG
                fprintf(stderr,"Node Pop  %d [%d]\n",nest_index,(int)cur_p);
#endif
            }
        }
        if (cur_p == null) {
            /* Try the final line break at the end of the paragraph,
               and |goto done| if the desired breakpoints have been found */

            /* The forced line break at the paragraph's end will reduce the list of
               breakpoints so that all active nodes represent breaks at |cur_p=null|.
               On the first pass, we insist on finding an active node that has the
               correct ``looseness.'' On the final pass, there will be at least one active
               node, and we will match the desired looseness as well as we can.

               The global variable |best_bet| will be set to the active node for the best
               way to break the paragraph, and a few other variables are used to
               help determine what is best.
             */

            ext_try_break(eject_penalty, hyphenated_node, line_break_dir,
                          pdf_adjust_spacing, par_shape_ptr, adj_demerits,
                          tracing_paragraphs, pdf_protrude_chars, line_penalty,
                          last_line_fit, double_hyphen_demerits,
                          final_hyphen_demerits, first_p, cur_p);
            if (vlink(active) != active) {
                /* Find an active node with fewest demerits; */
                r = vlink(active);
                fewest_demerits = awful_bad;
                do {
                    if (type(r) != delta_node) {
                        if (total_demerits(r) < fewest_demerits) {
                            fewest_demerits = total_demerits(r);
                            best_bet = r;
                        }
                    }
                    r = vlink(r);
                } while (r != active);
                best_line = line_number(best_bet);

                /* /Find an active node with fewest demerits; */
                if (looseness == 0)
                    goto DONE;
                /*Find the best active node for the desired looseness; */

                /* The adjustment for a desired looseness is a slightly more complicated
                   version of the loop just considered. Note that if a paragraph is broken
                   into segments by displayed equations, each segment will be subject to the
                   looseness calculation, independently of the other segments.
                 */
                r = vlink(active);
                actual_looseness = 0;
                do {
                    if (type(r) != delta_node) {
                        line_diff = line_number(r) - best_line;
                        if (((line_diff < actual_looseness)
                             && (looseness <= line_diff))
                            || ((line_diff > actual_looseness)
                                && (looseness >= line_diff))) {
                            best_bet = r;
                            actual_looseness = line_diff;
                            fewest_demerits = total_demerits(r);
                        } else if ((line_diff == actual_looseness) &&
                                   (total_demerits(r) < fewest_demerits)) {
                            best_bet = r;
                            fewest_demerits = total_demerits(r);
                        }
                    }
                    r = vlink(r);
                } while (r != active);
                best_line = line_number(best_bet);

                /* /Find the best active node for the desired looseness; */
                if ((actual_looseness == looseness) || final_pass)
                    goto DONE;
            }
        }

        /* Clean up the memory by removing the break nodes; */
        clean_up_the_memory();
        /* /Clean up the memory by removing the break nodes; */

        if (!second_pass) {
            if (tracing_paragraphs > 0)
                tprint_nl("@@secondpass");
            threshold = tolerance;
            second_pass = true;
            final_pass = (emergency_stretch <= 0);
        } else {                /* if at first you do not succeed, \dots */
            if (tracing_paragraphs > 0)
                tprint_nl("@@emergencypass");
            background[2] += emergency_stretch;
            final_pass = true;
        }
    }

  DONE:
    if (tracing_paragraphs > 0) {
        end_diagnostic(true);
        normalize_selector();
    }
    if (do_last_line_fit) {
        /* Adjust the final line of the paragraph; */
        /* Here we either reset |do_last_line_fit| or adjust the |par_fill_skip| glue.
         */
        if (active_short(best_bet) == 0) {
            do_last_line_fit = false;
        } else {
            q = new_spec(glue_ptr(last_line_fill));
            delete_glue_ref(glue_ptr(last_line_fill));
            width(q) += (active_short(best_bet) - active_glue(best_bet));
            stretch(q) = 0;
            glue_ptr(last_line_fill) = q;
        }
    }

    /* Break the paragraph at the chosen...; */
    /* Once the best sequence of breakpoints has been found (hurray), we call on the
       procedure |post_line_break| to finish the remainder of the work.
       (By introducing this subprocedure, we are able to keep |line_break|
       from getting extremely long.)
     */

    /* first thing |ext_post_line_break| does is reset |dir_ptr| */
    flush_node_list(dir_ptr);
    dir_ptr = null;
    ext_post_line_break(paragraph_dir,
                        right_skip,
                        left_skip,
                        pdf_protrude_chars,
                        par_shape_ptr,
                        pdf_adjust_spacing,
                        pdf_each_line_height,
                        pdf_each_line_depth,
                        pdf_first_line_height,
                        pdf_last_line_depth,
                        inter_line_penalties_ptr,
                        inter_line_penalty,
                        club_penalty,
                        club_penalties_ptr,
                        widow_penalties_ptr,
                        widow_penalty,
                        broken_penalty,
                        final_par_glue,
                        best_bet,
                        last_special_line,
                        second_width,
                        second_indent, first_width, first_indent, best_line,
                        pdf_ignored_dimen);
    /* /Break the paragraph at the chosen... */
    /* Clean up the memory by removing the break nodes; */
    clean_up_the_memory();
}

@ @c
void get_linebreak_info (int *f, int *a) 
{
    *f = fewest_demerits;
    *a = actual_looseness;
}
