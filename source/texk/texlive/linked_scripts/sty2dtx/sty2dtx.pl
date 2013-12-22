#!/usr/bin/env perl
use strict;
use warnings;
use File::Basename qw(dirname);

=head1 NAME

sty2dtx -- Converts a LaTeX .sty file to a documented .dtx file

=head1 VERSION

Version: v2.3

=head1 COPYRIGHT

Copyright (c) 2010-2012 Martin Scharrer <martin@scharrer-online.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


=head1 DESCRIPTION

Converts a .sty file (LaTeX package) to .dtx format (documented LaTeX source),
by surrounding macro definitions with 'macro' and 'macrocode' environments.
The macro name is automatically inserted as an argument to the 'macro'
environemnt.
Code lines outside macro definitions are wrapped only in 'macrocode'
environments. Empty lines are removed.
The script is not thought to be fool proof and 100% accurate but rather
as a good start to convert undocumented style files to .dtx files.


=head2 Basic Usage

     perl sty2dtx.pl infile [infile ...] outfile

or

     perl sty2dtx.pl < file.sty > file.dtx

=head2 Supported Definitions

The following macro definitions are detected when they are at the start of a
line (can be prefixed by \global, \long, \protected and/or \outer):

    \def   \edef   \gdef   \xdef
    \newcommand{\name}     \newcommand*{\name}
    \newcommand\name       \newcommand*\name
    \renewcommand{\name}   \renewcommand*{\name}
    \renewcommand\name     \renewcommand*\name
    \providecommand{\name} \providecommand*{\name}
    \providecommand\name   \providecommand*\name
    \@namedef{\name}       \@namedef\name

The following environment definitions are detected when they are at the start
of a line:

    \newenvironment{name}  \renewenvironemnt{name}  \provideenvironment{name}

The macro and environment definition must either end at the same line or with
a 'C<}>' on its own on a line.


=head1 USAGE

  sty2dtx [<options>] [--<VAR>=<VALUE> ...] [--] [<infile(s)>] [<outfile>]

=head2 Files

=over 2

=item *
  can be 'C<->' for STDIN or STDOUT, which is the default if no files are given

=item *
  multiple input files are merged to one output file

=back

=head2 Variables

Variables can be defined using

  --<VAR>=<VALUE>

or

  --<VAR> <VALUE>

and will be used for substitutions in the template file.

=head3 Common variables:

      author, email, maintainer, year (for copyright),
      version, date, description (of package/class),
      type (either 'package' default or 'class'),
      filebase (automatically set from output or input file name),


=head2 Options

=over 8

=item B<-h> S<          >
   Print this help text

=item B<-H> S<          >
 Print extended help

=item B<-V> S<          >
 Print version and copyright

=item B<-v> S<          >
  Be verbose

=item B<-o> F<output> S<   >
  Use given file as output

=item B<-O> S<          >
  Overwrite already existing output file(s)

=item B<-B> S<          >
  Use basename of single input file for output file

=item B<-I> S<          >
  Also create .ins (install) file

=item B<-c> S<          >
  Only use code section (like v1.0)

=item B<-r> S<          >
  Remove existing 'macro', 'macrocode', etc. environments.

=item B<-R> S<          >
  Do not remove existing 'macro', 'macrocode', etc. environments.

=item B<-i> F<ins-file> S< >
  Create .ins file with given name

=item B<-t> F<template> S< >
  Use this file as template instead of the default one

=item B<-T> F<template> S< >
  Use this file as template for the .ins file

=item B<-e> F<file> S<     >
  Export default .dtx template to file and exit

=item B<-E> F<file> S<     >
  Export default .ins template to file and exit

=item B<-D> S<          >
  Use current date as file date

=item B<-F> F<file> S<     >
  Read more options and variables from file.

=item B<-N> S<          >
  Do not read default config file; must be the first option

=back


=head2 Config files

A default config file either named 'sty2dtx.cfg' or '.sty2dtx.cfg' is searched
in the current directory, the users home directory and the directory of this
script, in this order. The first one found is loaded.  If none is found the
'texmf' tree is searched for a 'sty2dtx.cfg' config file.  As with -F files the
config file should contain one option or variable per line.
Lines starting with 'C<%>' or 'C<#>' are ignored.


=head1 Examples

Produce 'file.dtx' from 'file.sty':

    sty2dtx.pl < file.sty > file.dtx

or

    sty2dtx.pl file.sty file.dtx

or

    sty2dtx.pl -B file.sty

Produce 'file.dtx' and 'file.ins' from 'file.sty':

    sty2dtx.pl -I file.sty file.dtx

or

    sty2dtx.pl file.sty -i file.sty file.dtx

or

    sty2dtx.pl -IB file.sty

Set custom variable values:

    sty2dtx.pl --author Me --email me@there.com mypkg.sty mypkg.dtx

Produce DTX file for a class:

    sty2dtx.pl --type class mycls.sty mycls.dtx

=head1 AUTHOR

Martin Scharrer 

E-mail: L<martin@scharrer-online.de>

WWW: L<http://www.scharrer-online.de>

=cut

################################################################################
use Pod::Usage;

my $VERSION = "v2.3";
$VERSION =~ tr/-/\//;

my $TITLE = << "EOT";
  sty2dtx -- Converts a LaTeX .sty file to a documented .dtx file
  Version: $VERSION
EOT

sub usage {
    print "Version: $VERSION\n\n";
    pod2usage(-message => $TITLE, -sections => 'USAGE', -verbose=>99 );
}

my $ERROR = "sty2dtx: Error:";

# Used as format string of printf so that the '%' must be doubled:
my $macrostart = <<'EOT';
%%
%% \begin{macro}{\%s}
%s%%    \begin{macrocode}
EOT

my $keystart = <<'EOT';
%%
%% \begin{key}{%s}{%s}
%s%%    \begin{macrocode}
EOT

my $environmentstart = <<'EOT';
%%
%% \begin{environment}{%s}
%s%%    \begin{macrocode}
EOT

my $macrodescription = <<'EOT';
%%
%% \DescribeMacro{\%s}
%%
EOT

my $keydescription = <<'EOT';
%%
%% \DescribeKey{\%s}{\%s}
%%
EOT

my $envdescription = <<'EOT';
%%
%% \DescribeEnv{%s}
%%
EOT

# Printed normally:
my $macrostop = <<'EOT';
%    \end{macrocode}
% \end{macro}
%
EOT

my $keystop = <<'EOT';
%    \end{macrocode}
% \end{key}
%
EOT

my $environmentstop = <<'EOT';
%    \end{macrocode}
% \end{environment}
%
EOT

my $macrocodestart = <<'EOT';
%    \begin{macrocode}
EOT

my $macrocodestop = <<'EOT';
%    \end{macrocode}
EOT

my $USAGE = q();    # Store macro names for usage section
my $IMPL  = q();    # Store implementation section

my $mode = 0;
# 0 = outside of macro or macrocode environments
# 1 = inside 'macrocode' environment
# 2 = inside 'macro' environment
# 3 = inside 'environment' environment
# 4 = inside 'key' environment

# RegExs for macro names and defintion:
my $rmacroname = qr/[a-zA-Z\@:]+/xms;    # Add ':' for LaTeX3 style macros
my $renvname   = qr/[a-zA-Z\@:*]+/xms;   #
my $rusermacro = qr/[a-zA-Z]+/xms;       # Macros intended for users
my $rmacrodef  = qr/
    ^                                                        # Begin of line (no whitespaces!)
     (
       (?:(?:\\global|\\long|\\protected|\\outer)\s*)*       # Prefixes (maybe with whitespace between them)
     )
    \\(
          [gex]?def \s* \\                                   # TeX definitions
        | (?:new|renew|provide)command\s* \*? \s* {? \s* \\  # LaTeX definitions
        | \@namedef{?                                        # Definition by name only
     )
     ($rmacroname)                                           # Macro name without backslash
     \s* }?                                                  # Potential closing brace
     (.*)                                                    # Rest of line
    /xms;

my $rkeydef  = qr/
    ^                                                        # Begin of line (no whitespaces!)
    \\
    (define\@[a-z]*key)
    \s*
    {([^}]+)}                                                # Key family
    \s*
    {([^}]+)}                                                # Key name
     (.*)                                                    # Rest of line
    /xms;

my $renvdef = qr/
    ^                                                        # Begin of line (no whitespaces!)
     \\(
        (?:new|renew|provide)environment\s* { \s*            # LaTeX definitions
     )
     ($renvname)                                             # Environment names follow same rules as macro names
     \s* }                                                   # closing brace
     (.*)                                                    # Rest of line
    /xms;

my $comments = q();

# Print end of environment, if one is open
sub close_environment {
    if ( $mode == 1 ) {
        $IMPL .= $macrocodestop;
    }
    elsif ( $mode == 2 ) {
        # Happens only if closing brace is not on a line by its own.
        $IMPL .= $macrostop;
    }
    elsif ( $mode == 3 ) {
        $IMPL .= $environmentstop;
    }
    elsif ( $mode == 4 ) {
        $IMPL .= $keystop;
    }
}

my ( $mday, $mon, $year ) = ( localtime(time) )[ 3 .. 5 ];
$mon = sprintf( "%02d", $mon + 1 );
$year += 1900;

my @files;
my $outfile    = q();
my $verbose    = 0;
my $codeonly   = 0;
my $install    = 0;
my $usebase    = 0;
my $overwrite  = 0;
my $removeenvs = 0;
my $installfile;
my $templfile;
my $installtempl;
my $checksum = 0;
my $config_file;

# Holds the variables for the templates, is initiated with default values:
my %vars = (
    type  => 'package',
    class => 'ltxdoc',
    year  => "$year",
);

my $template = *DATA;
my %OUTFILES;

# Handle options
sub option {
    my $opt = shift;
    if ( $opt eq 'h' ) {
        usage();
    }
    elsif ( $opt eq 'H' ) {
        pod2usage(-message => $TITLE, -sections => 'DESCRIPTION', -verbose=>99 );
    }
    elsif ( $opt eq 'r' ) {
        $removeenvs = 1;
    }
    elsif ( $opt eq 'R' ) {
        $removeenvs = 0;
    }
    elsif ( $opt eq 'c' ) {
        $codeonly = 1;
    }
    elsif ( $opt eq 'B' ) {
        $usebase = 1;
    }
    elsif ( $opt eq 't' ) {
        $templfile = shift @ARGV;
    }
    elsif ( $opt eq 'e' ) {
        my $templ = shift @ARGV;
        if ( $templ ne '-' ) {
            die "$ERROR '$templ' already exists\n" if -e $templ;
            open( STDOUT, '>', $templ )
              or die "$ERROR Couldn't open new template file '$templ' ($!)\n";
        }
        while (<DATA>) {
            last if /^__INS__$/;
            print;
        }
        print STDERR
          "Exported default template for .dtx files to file '$templ'\n"
          if $verbose;
        exit(0);
    }
    elsif ( $opt eq 'E' ) {
        my $templ = shift @ARGV;
        if ( $templ ne '-' ) {
            die "$ERROR '$templ' already exists\n" if -e $templ;
            open( STDOUT, '>', $templ )
              or die "$ERROR Couldn't open new template file '$templ' ($!)\n";
        }
        while (<DATA>) {
            last if /^__INS__$/;
        }
        while (<DATA>) {
            print;
        }
        print STDERR
          "Exported default template for .ins files to file '$templ'\n"
          if $verbose;
        exit(0);
    }
    elsif ( $opt eq 'v' ) {
        $verbose++;
    }
    elsif ( $opt eq 'I' ) {
        $install = 1;
    }
    elsif ( $opt eq 'i' ) {
        $installfile = shift @ARGV;
        $install     = 1;
    }
    elsif ( $opt eq 'T' ) {
        $installtempl = shift @ARGV;
    }
    elsif ( $opt eq 'V' ) {
        pod2usage(-message => $TITLE, -sections => 'COPYRIGHT', -verbose=>99 );
    }
    elsif ( $opt eq 'N' ) {
        print STDERR "sty2dtx warning: '-N' option used after default config file was already loaded.\n"
            if $config_file && $verbose;
    }
    elsif ( $opt eq 'F' ) {
        my $optfile = shift @ARGV;

        # Read more options and variables from file
        open( my $OPT, '<', $optfile )
          or die("Couldn't open options file '$optfile' ($!)!\n");
        while ( my $line = <$OPT> ) {
            chomp $line;

            # Skip comment lines
            next if $line =~ /^\s*[#%]/;

            # Handle variables
            # Split at '=' or space and add as two arguments
            if ( $line =~ /\A\s*(--[a-zA-Z0-9]+?)(?:\s+|=)(.*)/xms ) {
                my $var = $1;
                (my $val = $2) =~ s/^["']|["']$//g;
                unshift @ARGV, $var, $val;
            }
            # Handle options with and without values
            # as well sets of options
            elsif ($line =~ /\A\s*(-[a-zA-Z0-9]+)(?:\s+(.*))?/xms) {
                my $var = $1;
                my $val = $2;
                if (defined $val) {
                    $val =~ s/^["']|["']$//g;
                    unshift @ARGV, $var, $val;
                }
                else {
                    unshift @ARGV, $var;
                }
            }
            else {
                die "$ERROR Could not recognize '$line' in '$optfile'\n";
            }
        }
        close $OPT;
    }
    elsif ( $opt eq 'D' ) {
        $vars{date} = "$year/$mon/$mday";
    }
    elsif ( $opt eq 'o' ) {
        $outfile = shift @ARGV;
    }
    elsif ( $opt eq 'O' ) {
        $overwrite = 1;
    }
    elsif ( $opt eq 'A' ) {
        $template = shift @ARGV;
    }
    elsif ( $opt eq 'a' ) {
        my $outname = shift @ARGV;
        $OUTFILES{$outname} = $template;
    }
    else {
        print STDERR "sty2dtx: unknown option '-$opt'!\n";
        exit(2);
    }
}

# Count number of backslashes in code for file checksum
sub addtochecksum {
    my $line = shift;
    $checksum += $line =~ tr{\\}{\\};
}

################################################################################

my $CONFIG_NAME = "sty2dtx.cfg";

if (!@ARGV || $ARGV[0] !~ /^-[^-]*N/) {

PATH:
foreach my $path ('.', $ENV{HOME}, dirname($0)) {
    foreach my $file ($path.'/'.$CONFIG_NAME, $path.'/.'.$CONFIG_NAME) {
        if (-e $file) {
            $config_file = $file;
            last PATH;
        }
    }
}
if (!$config_file) {
    # Find the config file in the TeX tree.
    $config_file = `kpsewhich -must-exist $CONFIG_NAME`;
}
if ($config_file) {
    chomp $config_file;
    print STDERR "sty2dtx info: loading config file '$config_file'.\n"
        if ($verbose);
    unshift @ARGV, '-F', $config_file;
}

}

# Parse arguments
while (@ARGV) {
    my $arg = shift;

    # '--' Marks rest of arguments as files
    if ( $arg eq '--' ) {
        push @files, @ARGV;
        last;
    }

    # Options and variables
    elsif ( $arg =~ /^(-+)(.+)$/ ) {
        my $dashes = $1;
        my $name   = $2;

        # Single dash => option
        if ( length($dashes) == 1 ) {
            foreach my $opt ( split //, $name ) {
                option($opt);
            }
        }
        # Douple Dash => Variable
        # Form "--var=value"
        elsif ($name =~ /^([^=]+)=(.*)$/) {
                $vars{lc($1)} = $2;
        }
        # Form "--var value"
        else {
            if ($name eq "help") {
                usage();
            }
            else {
                $vars{ lc($name) } = shift;
            }
        }
    }
    # Files
    else {
        push @files, $arg;
    }
}


# Last (but not only) argument is output file, except if it is '-' (=STDOUT)
if ( $outfile || @files > 1 ) {
    $outfile = pop @files unless $outfile;
    $vars{filebase} = substr( $outfile, 0, rindex( $outfile, '.' ) )
      if not exists $vars{filebase};
}
elsif ( @files == 1 ) {
    my $infile = $files[0];
    $vars{filebase} = substr( $infile, 0, rindex( $infile, '.' ) )
      if not exists $vars{filebase};
    if ($usebase) {
        $outfile = $vars{filebase} . '.dtx';
    }
}
if ( $outfile && $outfile ne '-' ) {
    if ( !$overwrite && -e $outfile && $outfile ne '/dev/null' ) {
        die(    "$ERROR output file '$outfile' does already exists!"
              . " Use the -O option to overwrite.\n" );
    }
    open( OUTPUT, '>', $outfile )
      or die("$ERROR Could not open output file '$outfile' ($!)!");
    select OUTPUT;
}


################################################################################
# Read input files
@ARGV = @files;
while (<>) {
    # Test for macro definition command
    if (/$rmacrodef/) {
        my $pre  = $1 || "";    # before command
        my $cmd  = $2;          # definition command
        my $name = $3;          # macro name
        my $rest = $4;          # rest of line

        MACRO:

        # Add to usage section if it is a user level macro
        if ( $name =~ /^$rusermacro$/i ) {
            $USAGE .= sprintf( $macrodescription, $name );
        }

        close_environment();

        # Print 'macro' environment with current line.
        $IMPL .= sprintf( $macrostart, $name, $comments );
        addtochecksum($_);
        $IMPL .= $_;
        $comments = '';

        # Inside macro mode
        $mode = 2;

        # Test for one line definitions.
        # $pre is tested to handle '{\somecatcodechange\gdef\name{short}}' lines
        my $prenrest = $pre . $rest;
        if ( $prenrest =~ tr/{/{/ == $prenrest =~ tr/}/}/ ) {
            $IMPL .= $macrostop;
            # Outside mode
            $mode = 0;
        }
    }
    elsif (/$rkeydef/) {
        my $pre  = "";    # before command
        my $cmd  = $1;        # definition command
        my $keyfamily = $2;   # macro name
        my $keyname   = $3;   # macro name
        my $rest = $4;        # rest of line

        # Add to usage section if it is a user level macro
        $USAGE .= sprintf( $keydescription, $keyfamily, $keyname );

        close_environment();

        # Print 'key' environment with current line.
        $IMPL .= sprintf( $keystart, $keyfamily, $keyname, $comments );
        addtochecksum($_);
        $IMPL .= $_;
        $comments = '';

        # Inside key mode
        $mode = 4;

        # Test for one line definitions.
        # $pre is tested to handle '{\somecatcodechange\gdef\name{short}}' lines
        my $prenrest = $pre . $rest;
        if ( $prenrest =~ tr/{/{/ == $prenrest =~ tr/}/}/ ) {
            $IMPL .= $keystop;
            # Outside mode
            $mode = 0;
        }
    }
    # Test for environment definition command
    elsif (/$renvdef/) {
        my $cmd  = $1;    # definition command
        my $name = $2;    # macro name
        my $rest = $3;    # rest of line

        # Add to usage section if it is a user level environment
        # Can use the same RegEx as for macro names
        if ( $name =~ /^$rusermacro$/i ) {
            $USAGE .= sprintf( $envdescription, $name );
        }

        close_environment();

        # Print 'environment' environment with current line.
        $IMPL .= sprintf( $environmentstart, $name, $comments );
        addtochecksum($_);
        $IMPL .= $_;
        $comments = '';

        # Inside environment mode
        $mode = 3;

        # Test for one line definitions.
        my $nopen = ( $rest =~ tr/{/{/ );
        if ( $nopen >= 2 && $nopen == ( $rest =~ tr/}/}/ ) ) {
            $IMPL .= $environmentstop;
            # Outside mode
            $mode = 0;
        }
    }
    # Collect comment lines, might be inserted as macro or environment description
    # Real comments are either: 1) starting with a '%' at SOL or 2) are followed
    # by at least one whitespace. This exclude (most) commented out code.
    elsif (/^%|^\s*%\s/) {
        if (!$removeenvs || !/^%\s+\\(?:begin|end){(?:macro|environment|macrocode|key)}/) {
            $_ =~ s/^\s*//;
            if ($comments || !/^%\s*$/){
                $comments .= $_;
            }
        } 
        if ( $mode == 1 ) {
            $IMPL .= $macrocodestop;
            $mode = 0;
        }
    }
    # Remove empty lines (mostly between macros)
    elsif (/^$/) {
        if ($comments) {
            # Flush collected outside comments
            $IMPL .= $comments . "%\n";
            $comments = '';
        }
    }
    else {
        addtochecksum($_);
        # If inside an environment
        if ($mode) {
            if ($comments) {
                $IMPL .= $macrocodestop . $comments . $macrocodestart;
                $comments = '';
            }
            $IMPL .= $_;
            # A single '}' on a line ends a 'macro' or 'environment' environment
            if ( $mode > 1 && /^\}\s*$/ ) {
                $IMPL .= ( $mode == 2 ) ? $macrostop : 
                         ( $mode == 3 ) ? $environmentstop : $keystop;
                $mode = 0;
            }
        }
        else {
            # Start macrocode environment
            $IMPL .= $comments . $macrocodestart . $_;
            $mode     = 1;
            $comments = '';
        }
    }
}

close_environment();

################################################################################
# Set extra/auto variables
$vars{IMPLEMENTATION} = $IMPL;
$vars{USAGE}      = $USAGE;
$vars{type}       = "\L$vars{type}";
$vars{Type}       = "\L\u$vars{type}";
$vars{extension}  = $vars{type} eq 'class' ? 'cls' : 'sty';
$vars{checksum}   = $checksum if not exists $vars{checksum}; # Allow user to overwrite
$vars{maintainer} = $vars{author}
  if not exists $vars{maintainer} and exists $vars{author};

################################################################################
# Write DTX file
if ($codeonly) {
    print $IMPL;
    if ($verbose) {
        print STDERR "Generated DTX file";
        print STDERR " '$outfile'" if $outfile and $outfile ne '-';
        print STDERR " (code only).\n";
    }
}
else {
    if ($templfile) {
        open( DTX, '<', $templfile )
            or die "$ERROR Couldn't open template file '$templfile' ($!)\n";
    }
    else {
        *DTX = *DATA;
    }
    while (<DTX>) {
        last if !$templfile and /^__INS__$/;
        # Substitute template variables
        s/<\+([^+]+)\+>/exists $vars{$1} ? $vars{$1} : "<+$1+>"/eg;
        print;
    }

    if ($verbose) {
        print STDERR "Generated DTX file";
        print STDERR " '$outfile'" if $outfile and $outfile ne '-';
        print STDERR " using template '$templfile'" if $templfile;
        print STDERR ".\n";
    }
}
select STDOUT;
################################################################################
# Write INS file if requested
if ($install) {

    if ( ( !$outfile || $outfile eq '-' ) && !$installfile ) {
        print STDERR
          "Warning: Did not generate requested .ins file because main file\n";
        print STDERR "         was written to STDOUT and no -i option was given.\n";
        exit(1);
    }

    if ($installtempl) {
        open( DATA, '<', $installtempl )
          or die "$ERROR Could't open template '$installtempl' for .ins file! ($!)\n";
    }
    elsif ($codeonly || $templfile) {
        # If DATA template was not used for main file go forward to correct position
        while (<DATA>) {
            last if /^__INS__$/;
        }
    }

    $installfile = $vars{filebase} . '.ins' unless defined $installfile;
    if ( !$overwrite && -e $installfile && $installfile ne '/dev/null' ) {
        die(    "$ERROR Output file '$installfile' does already exists!"
              . " Use the -O option to overwrite.\n" );
    }
    open( INS, '>', $installfile )
      or die "$ERROR Could't open new .ins file '$installfile' ($!)\n";

    while (<DATA>) {
        # Substitute template variables
        s/<\+([^+]+)\+>/exists $vars{$1} ? $vars{$1} : "<+$1+>"/eg;
        print INS $_;
    }

    if ($verbose) {
        print STDERR "Generated INS file '$installfile'";
        print STDERR " using template '$installtempl'" if $installtempl;
        print STDERR ".\n";
    }

}

################################################################################
# Write additional files if requested
while (my ($outfile, $template) = each %OUTFILES) {
    my $TEMPLATEHANDLE;
    if (ref $template eq 'GLOB') {
        $TEMPLATEHANDLE = $template;
    }
    # TODO: add support for DTX and INS templates in DATA
    else {
        open( $TEMPLATEHANDLE, '<', $template )
            or die "$ERROR Could't open template '$template' for file '$outfile' ($!)!\n";
    }
    open( my $OUTFILEHANDLE, '>', $outfile )
      or die "$ERROR Could't create output file '$outfile' ($!)!\n";

    while (<$TEMPLATEHANDLE>) {
        # Substitute template variables
        s/<\+([^+]+)\+>/exists $vars{$1} ? $vars{$1} : "<+$1+>"/eg;
        print {$OUTFILEHANDLE} $_;
    }
    close $TEMPLATEHANDLE;
    close $OUTFILEHANDLE;

    if ($verbose) {
        print STDERR "Generated file '$outfile' using template '$template'.\n";
    }
}
################################################################################
exit (0);
################################################################################
# The templates for the DTX file and INS file
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Adepted from the skeleton file provided by the 'dtxtut' (DTX tuturial).
# The '<+var+>' format was choosen because it is used by the latex suite for Vim.
# Therfore all variables which are not expanded are easily accessible to the
# user using a certain feature in the latex suite.
#
# Perl modules like 'Template' were not used to support minimal Perl installation
# for typical LaTeX installations.
#
__DATA__
% \iffalse meta-comment
%
% Copyright (C) <+year+> by <+author+> <<+email+>>
% ---------------------------------------------------------------------------
% This work may be distributed and/or modified under the
% conditions of the LaTeX Project Public License, either version 1.3
% of this license or (at your option) any later version.
% The latest version of this license is in
%   http://www.latex-project.org/lppl.txt
% and version 1.3 or later is part of all distributions of LaTeX
% version 2005/12/01 or later.
%
% This work has the LPPL maintenance status `maintained'.
%
% The Current Maintainer of this work is <+maintainer+>.
%
% This work consists of the files <+filebase+>.dtx and <+filebase+>.ins
% and the derived filebase <+filebase+>.<+extension+>.
%
% \fi
%
% \iffalse
%<*driver>
\ProvidesFile{<+filebase+>.dtx}
%</driver>
%<<+type+>>\NeedsTeXFormat{LaTeX2e}[1999/12/01]
%<<+type+>>\Provides<+Type+>{<+filebase+>}
%<*<+type+>>
    [<+date+> <+version+> <+description+>]
%</<+type+>>
%
%<*driver>
\documentclass{ltxdoc}
\usepackage{<+filebase+>}[<+date+>]
\EnableCrossrefs
\CodelineIndex
\RecordChanges
\begin{document}
  \DocInput{<+filebase+>.dtx}
  \PrintChanges
  \PrintIndex
\end{document}
%</driver>
% \fi
%
% \CheckSum{<+checksum+>}
%
% \CharacterTable
%  {Upper-case    \A\B\C\D\E\F\G\H\I\J\K\L\M\N\O\P\Q\R\S\T\U\V\W\X\Y\Z
%   Lower-case    \a\b\c\d\e\f\g\h\i\j\k\l\m\n\o\p\q\r\s\t\u\v\w\x\y\z
%   Digits        \0\1\2\3\4\5\6\7\8\9
%   Exclamation   \!     Double quote  \"     Hash (number) \#
%   Dollar        \$     Percent       \%     Ampersand     \&
%   Acute accent  \'     Left paren    \(     Right paren   \)
%   Asterisk      \*     Plus          \+     Comma         \,
%   Minus         \-     Point         \.     Solidus       \/
%   Colon         \:     Semicolon     \;     Less than     \<
%   Equals        \=     Greater than  \>     Question mark \?
%   Commercial at \@     Left bracket  \[     Backslash     \\
%   Right bracket \]     Circumflex    \^     Underscore    \_
%   Grave accent  \`     Left brace    \{     Vertical bar  \|
%   Right brace   \}     Tilde         \~}
%
%
% \changes{<+version+>}{<+date+>}{Converted to DTX file}
%
% \DoNotIndex{\newcommand,\newenvironment}
%
% \providecommand*{\url}{\texttt}
% \GetFileInfo{<+filebase+>.dtx}
% \title{The \textsf{<+filebase+>} package}
% \author{<+author+> \\ \url{<+email+>}}
% \date{\fileversion~from \filedate}
%
% \maketitle
%
% \section{Introduction}
%
% Put text here.
%
% \section{Usage}
%
% Put text here.
%
<+USAGE+>
%
% \StopEventually{}
%
% \section{Implementation}
%
% \iffalse
%<*<+type+>>
% \fi
%
<+IMPLEMENTATION+>
%
% \iffalse
%</<+type+>>
% \fi
%
% \Finale
\endinput
__INS__
%% Copyright (C) <+year+> by <+author+> <<+email+>>
%% --------------------------------------------------------------------------
%% This work may be distributed and/or modified under the
%% conditions of the LaTeX Project Public License, either version 1.3
%% of this license or (at your option) any later version.
%% The latest version of this license is in
%%   http://www.latex-project.org/lppl.txt
%% and version 1.3 or later is part of all distributions of LaTeX
%% version 2005/12/01 or later.
%%
%% This work has the LPPL maintenance status `maintained'.
%%
%% The Current Maintainer of this work is <+maintainer+>.
%%
%% This work consists of the files <+filebase+>.dtx and <+filebase+>.ins
%% and the derived filebase <+filebase+>.<+extension+>.
%%

\input docstrip.tex
\keepsilent

\usedir{tex/latex/<+filebase+>}

\preamble

This is a generated file.

Copyright (C) <+year+> by <+author+> <<+email+>>
--------------------------------------------------------------------------
This work may be distributed and/or modified under the
conditions of the LaTeX Project Public License, either version 1.3
of this license or (at your option) any later version.
The latest version of this license is in
  http://www.latex-project.org/lppl.txt
and version 1.3 or later is part of all distributions of LaTeX
version 2005/12/01 or later.

\endpreamble

\generate{\file{<+filebase+>.<+extension+>}{\from{<+filebase+>.dtx}{<+type+>}}}

\obeyspaces
\Msg{*************************************************************}
\Msg{*                                                           *}
\Msg{* To finish the installation you have to move the following *}
\Msg{* file into a directory searched by TeX:                    *}
\Msg{*                                                           *}
\Msg{*     <+filebase+>.<+extension+>                                          *}
\Msg{*                                                           *}
\Msg{* To produce the documentation run the file <+filebase+>.dtx    *}
\Msg{* through LaTeX.                                            *}
\Msg{*                                                           *}
\Msg{* Happy TeXing!                                             *}
\Msg{*                                                           *}
\Msg{*************************************************************}

\endbatchfile
