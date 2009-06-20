#ifndef _UTYPE_H
#define _UTYPE_H

#define ____L	0x1
#define ____U	0x2
#define ____TITLE	0x4
#define ____D	0x8
#define ____S	0x10
#define ____P	0x20
#define ____X	0x40
#define ____ZW	0x80
#define ____L2R	0x100
#define ____R2L	0x200
#define ____ENUM	0x400
#define ____ANUM	0x800
#define ____ENS	0x1000
#define ____CS	0x2000
#define ____ENT	0x4000
#define ____COMBINE	0x8000
#define ____BB	0x10000
#define ____BA	0x20000
#define ____NS	0x40000
#define ____NE	0x80000
#define ____UB	0x100000
#define ____NB	0x8000000
#define ____AL	0x200000
#define ____ID	0x400000
#define ____INITIAL	0x800000
#define ____MEDIAL	0x1000000
#define ____FINAL	0x2000000
#define ____ISOLATED 0x4000000
#define ____DECOMPNORM 0x10000000

#define ____COMBININGCLASS 0xff
#define ____ABOVE	0x100
#define ____BELOW	0x200
#define ____OVERSTRIKE	0x400
#define ____LEFT	0x800
#define ____RIGHT	0x1000
#define ____JOINS2	0x2000
#define ____CENTERLEFT	0x4000
#define ____CENTERRIGHT	0x8000
#define ____CENTEREDOUTSIDE	0x10000
#define ____OUTSIDE		0x20000
#define ____LEFTEDGE	0x80000
#define ____RIGHTEDGE	0x40000
#define ____TOUCHING	0x100000
#define ____COMBININGPOSMASK	0x1fff00

#if 0
extern const unsigned short ____tolower[];
extern const unsigned short ____toupper[];
#endif
extern const unsigned int  ____utype[];

#if 0
#define tolower(ch) (____tolower[(ch)+1])
#define toupper(ch) (____toupper[(ch)+1])
#else
/* ASCII style */
#define tolower(ch) (ch+'A'-'a')
#endif
#define islower(ch) (____utype[(ch)+1]&____L)
#define isupper(ch) (____utype[(ch)+1]&____U)
#define istitle(ch) (____utype[(ch)+1]&____TITLE)
#define isalpha(ch) (____utype[(ch)+1]&(____L|____U|____TITLE|____AL))
#define isdigit(ch) (____utype[(ch)+1]&____D)
#define isalnum(ch) (____utype[(ch)+1]&(____L|____U|____TITLE|____AL|____D))
#define isideographic(ch) (____utype[(ch)+1]&____ID)
#define isideoalpha(ch) (____utype[(ch)+1]&(____ID|____L|____U|____TITLE|____AL))
#define isspace(ch) (____utype[(ch)+1]&____S)
#define ispunct(ch) (____utype[(ch)+1]&_____P)
#define ishexdigit(ch) (____utype[(ch)+1]&____X)
#define iszerowidth(ch) (____utype[(ch)+1]&____ZW)
#define islefttoright(ch) (____utype[(ch)+1]&____L2R)
#define isrighttoleft(ch) (____utype[(ch)+1]&____R2L)
#define iseuronumeric(ch) (____utype[(ch)+1]&____ENUM)
#define isarabnumeric(ch) (____utype[(ch)+1]&____ANUM)
#define iseuronumsep(ch) (____utype[(ch)+1]&____ENS)
#define iscommonsep(ch) (____utype[(ch)+1]&____CS)
#define iseuronumterm(ch) (____utype[(ch)+1]&____ENT)
#define iscombining(ch) (____utype[(ch)+1]&____COMBINE)
#define isbreakbetweenok(ch1,ch2) (((____utype[(ch1)+1]&____BA) && !(____utype[(ch2)+1]&____NS)) || ((____utype[(ch2)+1]&____BB) && !(____utype[(ch1)+1]&____NE)) || (!(____utype[(ch2)+1]&____D) && ch1=='/'))
#define isnobreak(ch) (____utype[(ch)+1]&____NB)
#define isarabinitial(ch) (____utype[(ch)+1]&____INITIAL)
#define isarabmedial(ch) (____utype[(ch)+1]&____MEDIAL)
#define isarabfinal(ch) (____utype[(ch)+1]&____FINAL)
#define isarabisolated(ch) (____utype[(ch)+1]&____ISOLATED)

#define isdecompositionnormative(ch) (____utype[(ch)+1]&____DECOMPNORM)

extern struct arabicforms {
    unsigned short initial, medial, final, isolated;
    unsigned int isletter: 1;
    unsigned int joindual: 1;
    unsigned int required_lig_with_alef: 1;
} ArabicForms[256];	/* for chars 0x600-0x6ff, subtract 0x600 to use array */

#define _SOFT_HYPHEN	0xad

#define _DOUBLE_S	0xdf

#endif
