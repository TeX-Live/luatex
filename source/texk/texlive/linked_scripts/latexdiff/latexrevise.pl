#!/usr/bin/env perl 
#   latexrevise - takes output file of latexdiff and removes either discarded
#                 or appended passages, then deletes all other latexdiff markup
#
#   Copyright (C) 2004  F J Tilmann (tilmann@gfz-potsdam.de, ftilmann@users.berlios.de)
#
# Project webpages:   http://latexdiff.berlios.de/
# CTAN page:          http://www.ctan.org/tex-archive/support/latexdiff
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Detailed usage information at the end of the file
# Note: version number now keeping up with latexdiff
# Version 1.0.2  Option --version
# Version 1.0.1   no changes to latexrevise
# Version 0.3   Updated for compatibility with latexdiff 0.3 output (DIFAUXCMD removal)
# Version 0.1   First public release

use Getopt::Long ;
use strict;
use warnings;

my $versionstring=<<EOF ;
This is LATEXREVISE 1.0.2
  (c) 2005-2012 F J Tilmann
EOF

# Markup strings (make sure that this are set to the same value as in 
# latexdiff)
# If at all possible, do not change these as parts of the program
# depend on the actual name.
# At the very least adapt subroutine postprocess to new tokens.

# note: have to use three backslashes to mean one literal one (reason
# unknown
my $ADDMARKOPEN='\\\DIFaddbegin(?:FL)? ';   # Token to mark begin of appended text
my $ADDMARKCLOSE='\\\DIFaddend(?:FL)?(?: |\z)';   # Token to mark end of appended text (|\z only deals with the case of \DIFaddend right before \end{document}
my $ADDOPEN='\\\DIFadd(?:FL)?{';  # To mark begin of added text passage
my $ADDCLOSE='}';        # To mark end of added text passage
my $ADDCOMMENT='DIF > ';   # To mark added comment line
my $DELMARKOPEN='\\\DIFdelbegin(?:FL)? ';   # Token to mark begin of deleted text
my $DELMARKCLOSE='\\\DIFdelend(?:FL)?(?: |\z)';   # Token to mark end of deleted text
my $DELOPEN='\\\DIFdel(?:FL)?{';  # To mark begin of deleted text passage
my $DELCLOSE='}';        # To mark end of deleted text passage
my $ALTEXT='FL';         # string which might be appended to above commands
my $DELCMDOPEN='%DIFDELCMD < ';  # To mark begin of deleted commands (must begin with %, i.e., be a comment
my $DELCMDCLOSE="%%%\n";    # To mark end of deleted commands (must end with a new line)
my $AUXCMD='%DIFAUXCMD' ; #  follows auxiliary commands put in by latexdiff to make difference file legal
my $DELCOMMENT='DIF < ';   # To mark deleted comment line

my $PREAMBLEXTBEG='^%DIF PREAMBLE EXTENSION ADDED BY LATEXDIFF$';
my $PREAMBLEXTEND='^%DIF END PREAMBLE EXTENSION ADDED BY LATEXDIFF$';


my $pat0 = '(?:[^{}]|\\\{|\\\})*';
my $pat1 = '(?:[^{}]|\\\{|\\\}|\{'.$pat0.'\})*';
my $pat2 = '(?:[^{}]|\\\{|\\\}|\{'.$pat1.'\})*';
my $pat3 = '(?:[^{}]|\\\{|\\\}|\{'.$pat2.'\})*';
my $pat4 = '(?:[^{}]|\\\{|\\\}|\{'.$pat3.'\})*';
my $brat0 = '(?:[^\[\]]|\\\[|\\\])*'; 


my ($input,$preamble,$body,$post);
my (@matches);
my ($cnt,$prematch,$postmatch);
my ($help,$version);
my ($verbose,$quiet,$accept,$decline,$simplify)=(0,0,0,0,0);
my ($comment,$comenv,$markup,$markenv);

# A word unlikely ever to be used in a real latex file
my $someword='gobbledegooksygook';

Getopt::Long::Configure('bundling');
GetOptions('accept|a' => \$accept,
	   'decline|d'=> \$decline,
	   'simplify|s' => \$simplify,
	   'comment|c=s' => \$comment,
	   'comment-environment|e=s' => \$comenv,
	   'markup|m=s' => \$markup,
	   'markup-environment|n=s' => \$markenv,
	   'no-warnings|q' => \$verbose,
           'version' => \$version,
	   'verbose|V' => \$verbose,
	   'help|h|H' => \$help);

if ( $help ) {
  usage() ;
}

if ( $version ) {
  die $versionstring ; 
}


if ( ($accept &&  $decline) || ($accept && $simplify) || ($decline && $simplify) ) {
  die '-a,-d and -s options are mutually axclusive. Type latexrevise -h to get more help.';
}



print STDERR "ACCEPT mode\n" if $verbose && $accept;
print STDERR "DECLINE mode\n" if $verbose && $decline;
print STDERR "SIMPLIFY mode.  WARNING: The output will not normally be valid latex,\n" if $verbose && $simplify;

# Slurp old and new files
{ 
  local $/ ; # locally set record operator to undefined, ie. enable whole-file mode
  $input=<>;
}

# split into parts
($preamble,$body,$post)=splitdoc($input,'\begin{document}','\end{document}');

if (length $preamble && ( $accept || $decline ) ) {
  #
  # WORK ON PREAMBLE
  #
  # (compare subroutine linediff in latexdiff to make sure correct strings are used)
  
  # remove extra commands added to preamble by latexdiff 
  $preamble =~ s/${PREAMBLEXTBEG}.*?${PREAMBLEXTEND}\n{0,1}//smg ;

  if ( $accept ) {
    # delete mark up in appended lines
    $preamble =~ s/^(.*) %DIF > $/$1/mg ;
  } elsif ( $decline ) {
    # delete appended lines
    #  $preamble =~ s/^(.*) %DIF > $//mg ;
    $preamble =~ s/^(.*) %DIF > \n//mg ;
    # delete markup in deleted lines
    $preamble =~ s/^%DIF < //mg ;
  }
  # remove any remaining DIF markups
  #$preamble =~ s/%DIF.*$//mg ;
  $preamble =~ s/%DIF.*?\n//sg ;
}
#print $preamble ;

#
# WORK ON BODY
#
if ($accept) {
  # remove ADDMARKOPEN, ADDMARKCLOSE tokens
  @matches= $body =~ m/${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}/sg;
  checkpure(@matches);
  $body =~ s/${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}/$1/sg;
  # remove text flanked by DELMARKOPEN, DELMARKCLOSE tokens
  @matches= $body =~ m/${DELMARKOPEN}(.*?)${DELMARKCLOSE}/sg;
  checkpure(@matches);
  $body =~ s/${DELMARKOPEN}(.*?)${DELMARKCLOSE}//sg;
  # remove markup of added comments
  $body =~ s/%${ADDCOMMENT}(.*?)$/%$1/mg ;
  # remove deleted comments (full line)
  $body =~ s/^%${DELCOMMENT}.*?\n//mg ;
  # remove deleted comments (part of line)
  $body =~ s/%${DELCOMMENT}.*?$//mg ;
} 
elsif ( $decline) {
  # remove DELMARKOPEN, DELMARKCLOSE tokens
  @matches= $body =~ m/${DELMARKOPEN}(.*?)${DELMARKCLOSE}/sg;
  checkpure(@matches);
  $body =~ s/${DELMARKOPEN}(.*?)${DELMARKCLOSE}/$1/sg;
  # remove text flanked by ADDMARKOPEN, ADDMARKCLOSE tokens
  # as latexdiff algorithm keeps the formatting and white spaces
  # of the new text, sometimes whitespace might be inserted or
  # removed inappropriately.  We try to guess whether this has
  # happened

  # Mop up tokens. This must be done already now as otherwise 
  # detection of white-space problems does not work
  $cnt = $body =~ s/${DELOPEN}($pat4)${DELCLOSE}/$1/sg;
  # remove markup of deleted commands 
  $cnt +=   $body =~ s/${DELCMDOPEN}(.*?)${DELCMDCLOSE}/$1/sg ;
  $cnt +=   $body =~ s/${DELCMDOPEN}//g ;
  # remove aux commands
  $cnt +=   $body =~ s/^.*${AUXCMD}$/${someword}/mg; $body =~ s/${someword}\n//g;

  while ( $body =~ m/${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}/s ) {
    $prematch=$`;
    $postmatch=$';
    checkpure($1);
    if ( $prematch =~ /\w$/s && $postmatch =~ /^\w/ ) {
      # apparently no white-space between word=>Insert white space
      $body =~ s/${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}/ /s ;
    }
    elsif ( $prematch =~ /\s$/s && $postmatch =~ /^[.,;:]/ ) {
      # space immediately before one of ".,:;" => remove this space
      $body =~ s/\s${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}//s ;
    }
    else {
      # do not insert or remove any extras
      $body =~ s/${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}//s;
    }
  }
# Alternative without special cases treatment
#  @matches= $body =~ m/${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}/sg;
#  checkpure(@matches);
#  $body =~ s/${ADDMARKOPEN}(.*?)${ADDMARKCLOSE}//sg;
  # remove markup of deleted comments
  $body =~ s/%${DELCOMMENT}(.*?)$/%$1/mg ;
  # remove added comments (full line)
  $body =~ s/^%${ADDCOMMENT}.*?\n//mg ;
  # remove added comments (part of line)
  $body =~ s/%${ADDCOMMENT}.*?$//mg ;
}

# remove any remaining tokens
if ( $accept || $decline || $simplify ) {
  # first substitution command deals with special case of added paragraph
  $cnt = $body =~ s/${ADDOPEN}($pat4)\n${ADDCLOSE}\n/$1\n/sg;
  $cnt += $body =~ s/${ADDOPEN}($pat4)${ADDCLOSE}/$1/sg;
  $cnt==0 || warn 'Remaining $ADDOPEN tokens in DECLINE mode\n' unless ( $quiet || $accept || $simplify );
}
if ($accept || $simplify ) {
  # Note: in decline mode these commands have already been removed above
  $cnt = $body =~ s/${DELOPEN}($pat4)${DELCLOSE}/$1/sg;
  #### remove markup of deleted commands 
  $cnt +=   $body =~ s/${DELCMDOPEN}(.*?)${DELCMDCLOSE}/$1/sg ;
  $cnt +=   $body =~ s/${DELCMDOPEN}//g ;
  # remove aux commands
  # $cnt +=   
  $body =~ s/^.*${AUXCMD}$/${someword}/mg; $body =~ s/${someword}\n//g;

  #### remove deleted comments 
  ###$cnt += $body =~ s/${DIFDELCMD}.*?$//mg ;
  $cnt==0 || warn 'Remaining $DELOPEN or $DIFDELCMD tokens in ACCEPT mode\n' unless ( $quiet || $simplify );
}

# Remove comment commands
if (defined($comment)) {
  print STDERR "Removing \\$comment\{..\} sequences ..." if $verbose;
  # protect $comments in comments by making them look different
  $body =~ s/(%.*)${comment}(.*)$/$1${someword}$2/mg ;
  # carry out the substitution
  $cnt = 0 + $body =~ s/\\${comment}(?:\[${brat0}\])?\{${pat4}\}//sg ;
  print STDERR "$cnt matches found and removed.\n" if $verbose;
  # and undo the protection substitution
  $body =~ s/(%.*)${someword}(.*)$/$1${comment}$2/mg ;
}
if (defined($comenv)) {
  print STDERR "Removing $comenv environments ..." if $verbose;
  $body =~ s/(%.*)${comenv}/$1${someword}/mg ;
##  $cnt = 0 + $body =~ s/\\begin(?:\[${brat0}\])?\{\$comenv\}.*?\\end\{\$comenv\}//sg ;
    $cnt = 0 + $body =~ s/\\begin(?:\[${brat0}\])?\{${comenv}\}.*?\\end\{${comenv}\}\s*?\n//sg ;
  print STDERR "$cnt matches found and removed.\n" if $verbose;
  $body =~ s/(%.*)${someword}/$1${comenv}/mg ;
}

if (defined($markup)) {
  print STDERR "Removing \\$markup\{..\} cpmmands ..." if $verbose;
  # protect $markups in comments by making them look different
  $body =~ s/(%.*)${markup}(.*)$/$1${someword}$2/mg ;
  # carry out the substitution
  $cnt = 0 + $body =~ s/\\${markup}(?:\[${brat0}\])?\{(${pat4})\}/$1/sg ;
  print STDERR "$cnt matches found and removed.\n" if $verbose;
  # and undo the protection substitution
  $body =~ s/(%.*)${someword}(.*)$/$1${markup}$2/mg ;
}
if (defined($markenv)) {
  print STDERR "Removing $markenv environments ..." if $verbose;
  $body =~ s/(%.*)${markenv}/$1${someword}/mg ;
  $cnt = 0 + $body =~ s/\\begin(?:\[${brat0}\])?\{${markenv}\}\n?//sg;
  $cnt += 0 + $body =~ s/\\end\{${markenv}\}\n?//sg;
  print STDERR $cnt/2, " matches found and removed.\n" if $verbose;
  $body =~ s/(%.*)${someword}/$1${markenv}/mg ;
}


if ( length $preamble ) {
  print "$preamble\\begin{document}${body}\\end{document}$post";
} else {
  print $body;
}

# checkpure(@matches)
# checks whether any of the strings in matches contains
# $ADDMARKOPEN, $ADDMARKCLOSE,$DELMARKOPEN, or $DELMARKCLOSE
# If so, die reporting nesting problems, otherwise return to caller
sub checkpure {
  while (defined($_=shift)) {
    if (  /$ADDMARKOPEN/ || /$ADDMARKCLOSE/
	  || /$DELMARKOPEN/ || /$DELMARKCLOSE/ ) {
      die <<EOF ;
There is a problem with nesting of  $ADDMARKOPEN, $ADDMARKCLOSE,
$DELMARKOPEN, and $DELMARKCLOSE tokens that prevents me from 
processing the file.  Exiting ...
EOF
     }
  }
}

# ($part1,$part2,$part3)=splitdoc($text,$word1,$word2)
# splits $text into 3 parts at $word1 and $word2.
# if neither $word1 nor $word2 exist, $part1 and $part3 are empty, $part2 is $text
# If only $word1 or $word2 exist but not the other, output an error message.
#
# same subroutine as in latexdiff
sub splitdoc {
  my ($text,$word1,$word2)=@_;
  my $l1 = length $word1 ;
  my $l2 = length $word2 ;

  my $i = index($text,$word1);
  my $j = index($text,$word2);

  my ($part1,$part2,$part3)=("","","");

  if ( $i<0 && $j<0) {
    # no $word1 or $word2
    print STDERR "Old Document not a complete latex file. Assuming it is a tex file with no preamble.\n";
    $part2 = $text;
  } elsif ( $i>=0 && $j>$i ) {
    $part1 = substr($text,0,$i) ; 
    $part2 = substr($text,$i+$l1,$j-$i-$l1);
    $part3 = substr($text,$j+$l2) unless $j+$l2 >= length $text;
  } else {
    die "$word1 or $word2 not in the correct order or not present as a pair."
  }
  return ($part1,$part2,$part3);
}



sub usage {
  die <<"EOF"; 
Usage: $0 [OPTIONS] [diff.tex] > revised.tex

Read a file diff.tex (output of latexdiff), and remove its markup. 
If no filename is given read from standard input. The command can be used
in ACCEPT, DECLINE, or SIMPLIFY mode, and be used to remove user-defined
latex commands from the input (see options -c, -e, -m, -n below). 
In ACCEPT mode, all appended text fragments  (or preamble lines)
are kept, and all discarded text fragments (or preamble lines) are
deleted.  
In DECLINE mode, all discarded text fragments are kept, and all appended 
text fragments are deleted.  
If you wish to keep some changes, edit the diff.tex file in
advance, and manually remove those tokens  which would otherwise be
deleted.  Note that latexrevise only pays attention to the \\DIFaddbegin,
\\DIFaddend, \\DIFdelbegin, and \\DIFdelend tokens and corresponding FL
varieties.  All \\DIFadd and \\DIFdel commands (but not their content) are 
simply deleted.   The commands added by latexdiff to the preamble are also
removed.
In SIMPLIFY mode all latexdiff markup is removed from the body of the text (after
\\begin{document}) except for \\DIFaddbegin, \\DIFaddend, \\DIFdelbegin, \\DIFdelend
tokens and the corresponding FL varieties of those commands.  The result
will not in general be valid latex-code but might be easier to read and edit in 
preparation for a subsequent run in ACCEPT or DECLINE mode.  
In SIMPLIFY mode the preamble is left unmodified.

-a
--accept          Run in ACCEPT mode (delete all blocks marked by \\DIFdelbegin
                  and \\DIFdelend).

-d
--decline         Run in DECLINE mode (delete all blocks marked by \\DIFaddbegin
                  and \\DIFaddend).

-s
--simplify        Run in SIMPLIFY mode (Keep all \\DIFaddbegin, \\DIFaddend, 
                  \\DIFdelbegin, \\DIFdelend tokens, but remove all other latexdiff
                  markup from body.  

Note that the three mode options are mutually exclusive. If no mode option is given,
latexrevise simply removes user annotations and markup according to the following four
options.


-c cmd
--comment=cmd     Remove \\cmd{...}.  cmd is supposed to mark some explicit 
                  anotations which should be removed from the file before 
                  release.

-e envir
--comment-environment=envir 
                  Remove explicit annotation environments from the text, i.e. remove
                  \\begin{envir}
                  ...
                  \\end{envir}
                  blocks.

-m cmd
--markup=cmd      Remove the markup command cmd but leave its argument, i.e.
                  turn \\cmd{abc} into abc.  

-n envir
--markup-environment=envir  
                  Similarly, remove \\begin{envir} and \\end{envir} commands,
                  but leave content of the environment in the text.
                  
-q
--no-warnings     Do not warn users about \\DIDadd{..} or \\DIFdel statements
                  which should not be there anymore

-V
--verbose         Verbose output

EOF
}

=head1 NAME

latexrevise - selectively remove markup and text from latexdiff output

=head1 SYNOPSIS

B<latexrevise> [ B<OPTIONS> ] [ F<diff.tex> ] > F<revised.tex>

=head1 DESCRIPTION

I<latexrevise> reads a file C<diff.tex> (output of I<latexdiff>), and remove the markup commands. 
If no filename is given the input is read from standard input. The command can be used
in I<ACCEPT>, I<DECLINE>, or I<SIMPLIFY> mode, or can be used to remove user-defined
latex commands from the input (see B<-c>, B<-e>, B<-m>, and B<-n> below). 
In I<ACCEPT> mode, all appended text fragments  (or preamble lines)
are kept, and all discarded text fragments (or preamble lines) are
deleted.  
In I<DECLINE> mode, all discarded text fragments are kept, and all appended 
text fragments are deleted.  
If you wish to keep some changes, edit the diff.tex file in
advance, and manually remove those tokens  which would otherwise be
deleted.  Note that I<latexrevise> only pays attention to the C<\DIFaddbegin>,
C<\DIFaddend>, C<\DIFdelbegin>, and C<\DIFdelend> tokens and corresponding FL
varieties.  All C<\DIFadd> and C<\DIFdel> commands (but not their contents) are 
simply deleted.   The commands added by latexdiff to the preamble are also
removed.
In I<SIMPLIFY> mode, C<\DIFaddbegin, \DIFaddend, \DIFdelbegin, \DIFdelend>
tokens and their corresponding C<FL> varieties are kept but all other markup (e.g. C<DIFadd> and <\DIFdel>) is removed.  The result
will not in general be valid latex-code but it will be easier to read and edit in 
preparation for a subsequent run in I<ACCEPT> or I<DECLINE> mode.  
In I<SIMPLIFY> mode the preamble is left unmodified.

=head1 OPTIONS

=over 4

=item B<-a> or B<--accept>

Run in I<ACCEPT> mode (delete all blocks marked by C<\DIFdelbegin> and C<\DIFdelend>).

=item B<-d> or B<--decline>

Run in I<DECLINE> mode (delete all blocks marked by C<\DIFaddbegin>
and C<\DIFaddend>).

=item B<-s> or B<--simplify>

Run in I<SIMPLIFY> mode (Keep all C<\DIFaddbegin>, C<\DIFaddend>, 
C<\DIFdelbegin>, C<\DIFdelend> tokens, but remove all other latexdiff
markup from body).  

=back

Note that the three mode options are mutually exclusive.  If no mode option is given,
I<latexrevise> simply removes user annotations and markup according to the following four
options.

=over 4

=item B<-c cmd> or B<--comment=cmd>

Remove C<\cmd{...}> sequences.  C<cmd> is supposed to mark some explicit 
anotations which should be removed from the file before 
release.

=item B<-e envir> or B<--comment-environment=envir> 

Remove explicit annotation environments from the text, i.e. remove

            \begin{envir}
            ...
            \end{envir}

blocks.

=item B<-m cmd> or B<--markup=cmd>

Remove the markup command C<\cmd> but leave its argument, i.e.
turn C<\cmd{abc}> into C<abc>.  

=item B<-n envir> or B<--markup-environment=envir>

Similarly, remove C<\begin{envir}> and C<\end{envir}> commands but 
leave content of the environment in the text.
                  

=item B<-V> or B<--verbose>

Verbose output

=item B<-q> or B<--no-warnings>

Do not warn users about C<\DIDadd{..}> or C<\DIFdel{..}> statements
which should have been removed already.

=back

=head1 BUGS

The current version is a beta version which has not yet been
extensively tested, but worked fine locally.  Please submit bug reports through
the latexdiff project page I<http://developer.berlios.de/projects/latexdiff/> or send
to I<tilmann@gfz-potsdam.de>.  Include the serial number of I<latexrevise>
(from comments at the top of the source).  If you come across latexdiff
output which is not processed correctly by I<latexrevise> please include the
problem file as well as the old and new files on which it is based,
ideally edited to only contain the offending passage as long as that still
reproduces the problem.

Note that I<latexrevise> gets confused by commented C<\begin{document}> or 
C<\end{document}> statements

=head1 SEE ALSO

L<latexdiff>

=head1 PORTABILITY

I<latexrevise> does not make use of external commands and thus should run
on any platform  supporting PERL v5 or higher. 

=head1 AUTHOR

Copyright (C) 2004 Frederik Tilmann

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License Version 3

=cut
