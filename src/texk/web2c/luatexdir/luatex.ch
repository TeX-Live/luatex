@x
  \def\?##1]{\hbox to 1in{\hfil##1.\ }}
  }
@y
  \def\?##1]{\hbox{Changes to \hbox to 1em{\hfil##1}.\ }}
  }
\let\maybe=\iftrue
@z

@x
@d banner==luaTeX_banner
@y
@d luaTeXk_banner=='This is luaTeXk, Version 3.141592',luatex_version_string
@#
@d banner==luaTeX_banner
@d banner_k==luaTeXk_banner
@z

@x
Actually the heading shown here is not quite normal: The |program| line
does not mention any |output| file, because \ph\ would ask the \TeX\ user
to specify a file name if |output| were specified here.
@^system dependencies@>
@y
@z

@x
program TEX; {all file names are defined dynamically}
label @<Labels in the outer block@>@/
@y
program TEX; {all file names are defined dynamically}
@z

@x [1.4] initialize the "stack base" value early
  begin @<Initialize whatever \TeX\ might access@>@;
@y
  begin stack_base:=stringcast(addressof(i));
    @<Initialize whatever \TeX\ might access@>@;
@z

@x
@ Three labels must be declared in the main program, so we give them
symbolic names.

@d start_of_TEX=1 {go here when \TeX's variables are initialized}
@d end_of_TEX=9998 {go here to close files and terminate gracefully}
@d final_end=9999 {this label marks the ending of the program}

@<Labels in the out...@>=
start_of_TEX@t\hskip-2pt@>, end_of_TEX@t\hskip-2pt@>,@,final_end;
  {key control points}
@y
@ For Web2c, labels are not declared in the main program, but
we still have to declare the symbolic names.

@d start_of_TEX=1 {go here when \TeX's variables are initialized}
@d final_end=9999 {this label marks the ending of the program}
@z

@x
@d debug==@{ {change this to `$\\{debug}\equiv\null$' when debugging}
@d gubed==@t@>@} {change this to `$\\{gubed}\equiv\null$' when debugging}
@y
@d debug== ifdef('TEXMF_DEBUG')
@d gubed== endif('TEXMF_DEBUG')
@z

@x
@d stat==@{ {change this to `$\\{stat}\equiv\null$' when gathering
  usage statistics}
@d tats==@t@>@} {change this to `$\\{tats}\equiv\null$' when gathering
  usage statistics}
@y
@d stat==ifdef('STAT')
@d tats==endif('STAT')
@z

@x
the codewords `$|init|\ldots|tini|$'.

@d init== {change this to `$\\{init}\equiv\.{@@\{}$' in the production version}
@d tini== {change this to `$\\{tini}\equiv\.{@@\}}$' in the production version}
@y
the codewords `$|init|\ldots|tini|$' for declarations and by the codewords
`$|Init|\ldots|Tini|$' for executable code.  This distinction is helpful for
implementations where a run-time switch differentiates between the two
versions of the program.

@d init==ifdef('INITEX')
@d tini==endif('INITEX')
@d Init==init if ini_version then begin
@d Tini==end;@+tini
@f Init==begin
@f Tini==end
@z

@x
@!init @<Initialize table entries (done by \.{INITEX} only)@>@;@+tini
@y
@!Init @<Initialize table entries (done by \.{INITEX} only)@>@;@+Tini
@z

@x
@<Constants...@>=
@!buf_size=500; {maximum number of characters simultaneously present in
  current lines of open files and in control sequences between
  \.{\\csname} and \.{\\endcsname}; must not exceed |max_halfword|}
@!error_line=72; {width of context lines on terminal error messages}
@!half_error_line=42; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line=79; {width of longest text lines output; should be at least 60}
@!stack_size=200; {maximum number of simultaneous input sources}
@!max_in_open=6; {maximum number of input files and error insertions that
  can be going on simultaneously}
@!param_size=60; {maximum number of simultaneous macro parameters}
@!nest_size=40; {maximum number of semantic levels simultaneously active}
@!max_strings=3000; {maximum number of strings; must not exceed |max_halfword|}
@!string_vacancies=8000; {the minimum number of characters that should be
  available for the user's control sequences and font names,
  after \TeX's own error messages are stored}
@!pool_size=32000; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all fonts and
  control sequences; must exceed |string_vacancies| by the total
  length of \TeX's own strings, which is currently about 23000}
@!save_size=600; {space for saving values outside of current group; must be
  at most |max_halfword|}
@!trie_size=8000; {space for hyphenation patterns; should be larger for
  \.{INITEX} than it is in production versions of \TeX}
@!trie_op_size=500; {space for ``opcodes'' in the hyphenation patterns}
@!dvi_buf_size=800; {size of the output buffer; must be a multiple of 8}
@!file_name_size=40; {file names shouldn't be longer than this}
@!pool_name='TeXformats:TEX.POOL                     ';
  {string of length |file_name_size|; tells where the string pool appears}
@y
@d stack_limit=@"400000
@d file_name_size == max_halfword-1 { has to be big enough to force namelength into integer }
@d ssup_error_line = 255
@d ssup_max_strings == 262143
{Larger values than 65536 cause the arrays consume much more memory.}
@d ssup_trie_opcode == 65535
@d ssup_trie_size == @"3FFFFFFF

@d ssup_hyph_size == 65535 {Changing this requires changing (un)dumping!}
@d iinf_hyphen_size == 610 {Must be not less than |hyph_prime|!}

@<Constants...@>=
@!hash_offset=514; {smallest index in hash array, i.e., |hash_base| }
  {Use |hash_offset=0| for compilers which cannot decrement pointers.}
@!trie_op_size=35111; {space for ``opcodes'' in the hyphenation patterns;
  best if relatively prime to 313, 361, and 1009.}
@!neg_trie_op_size=-35111; {for lower |trie_op_hash| array bound;
  must be equal to |-trie_op_size|.}
@!min_trie_op=0; {first possible trie op code for any language}
@!max_trie_op=ssup_trie_opcode; {largest possible trie opcode for any language}
@!engine_name='luatex'; {the name of this engine}
@#

@!inf_main_memory = 2000000;
@!sup_main_memory = 32000000;

@!inf_trie_size = 80000;
@!sup_trie_size = ssup_trie_size;

@!inf_max_strings = 100000;
@!sup_max_strings = ssup_max_strings;
@!inf_strings_free = 100;
@!sup_strings_free = sup_max_strings;

@!inf_buf_size = 500;
@!sup_buf_size = 100000000;

@!inf_nest_size = 40;
@!sup_nest_size = 4000;

@!inf_max_in_open = 6;
@!sup_max_in_open = 127;

@!inf_param_size = 60;
@!sup_param_size = 6000;

@!inf_save_size = 600;
@!sup_save_size = 80000;

@!inf_stack_size = 200;
@!sup_stack_size = 30000;

@!inf_dvi_buf_size = 800;
@!sup_dvi_buf_size = 65536;

@!inf_pool_size = 128000;
@!sup_pool_size = 40000000;
@!inf_pool_free = 1000;
@!sup_pool_free = sup_pool_size;
@!inf_string_vacancies = 8000;
@!sup_string_vacancies = sup_pool_size - 23000;

@!sup_hash_extra = sup_max_strings;
@!inf_hash_extra = 0;

@!sup_hyph_size = ssup_hyph_size;
@!inf_hyph_size = iinf_hyphen_size; {Must be not less than |hyph_prime|!}
@!sup_ocp_list_size = 1000000;
@!inf_ocp_list_size = 1000;
@!sup_ocp_buf_size = 1000000;
@!inf_ocp_buf_size = 1000;
@!sup_ocp_stack_size = 1000000;
@!inf_ocp_stack_size = 1000;
@z

@x
@d font_base=0 {smallest internal font number; must not be less
  than |min_quarterword|}
@d hash_size=2100 {maximum number of control sequences; it should be at most
  about |(fix_mem_max-fix_mem_min)/10|}
@d hash_prime=1777 {a prime number equal to about 85\pct! of |hash_size|}
@d hyph_size=307 {another prime; the number of \.{\\hyphenation} exceptions}
@y
@d font_base=0 {smallest internal font number; must not be less
  than |min_quarterword|}
@d hash_size=65536 {maximum number of control sequences; it should be at most
  about |(mem_max-mem_min)/10|}
@d hash_prime=55711 {a prime number equal to about 85\pct! of |hash_size|}
@d hyph_prime=607 {another prime for hashing \.{\\hyphenation} exceptions;
                if you change this, you should also change |iinf_hyphen_size|.}
@z

@x
@d incr(#) == #:=#+1 {increase a variable by unity}
@d decr(#) == #:=#-1 {decrease a variable by unity}
@y
@z

@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x
@!nameoffile:packed array[1..file_name_size] of char;@;@/
  {on some systems this may be a \&{record} variable}
@y
@!nameoffile:^packed_ASCII_code;
@z

@x
@ The \ph\ compiler with which the present version of \TeX\ was prepared has
extended the rules of \PASCAL\ in a very convenient way. To open file~|f|,
we can write
$$\vbox{\halign{#\hfil\qquad&#\hfil\cr
|reset(f,@t\\{name}@>,'/O')|&for input;\cr
|rewrite(f,@t\\{name}@>,'/O')|&for output.\cr}}$$
The `\\{name}' parameter, which is of type `{\bf packed array
$[\langle\\{any}\rangle]$ of \\{char}}', stands for the name of
the external file that is being opened for input or output.
Blank spaces that might appear in \\{name} are ignored.

The `\.{/O}' parameter tells the operating system not to issue its own
error messages if something goes wrong. If a file of the specified name
cannot be found, or if such a file cannot be opened for some other reason
(e.g., someone may already be trying to write the same file), we will have
|@!erstat(f)<>0| after an unsuccessful |reset| or |rewrite|.  This allows
\TeX\ to undertake appropriate corrective action.
@:PASCAL H}{\ph@>
@^system dependencies@>

\TeX's file-opening procedures return |false| if no file identified by
|nameoffile| could be opened.

@d reset_OK(#)==erstat(#)=0
@d rewrite_OK(#)==erstat(#)=0

@p function a_open_in(var f:alpha_file):boolean;
  {open a text file for input}
begin reset(f,nameoffile,'/O'); a_open_in:=reset_OK(f);
end;
@#
function a_open_out(var f:alpha_file):boolean;
  {open a text file for output}
begin rewrite(f,nameoffile,'/O'); a_open_out:=rewrite_OK(f);
end;
@#
function b_open_in(var f:byte_file):boolean;
  {open a binary file for input}
begin reset(f,nameoffile,'/O'); b_open_in:=reset_OK(f);
end;
@#
function b_open_out(var f:byte_file):boolean;
  {open a binary file for output}
begin rewrite(f,nameoffile,'/O'); b_open_out:=rewrite_OK(f);
end;
@#
function w_open_in(var f:word_file):boolean;
  {open a word file for input}
begin reset(f,nameoffile,'/O'); w_open_in:=reset_OK(f);
end;
@#
function w_open_out(var f:word_file):boolean;
  {open a word file for output}
begin rewrite(f,nameoffile,'/O'); w_open_out:=rewrite_OK(f);
end;
@y
@ All of the file opening functions are defined in C.
@z

@x
@ Files can be closed with the \ph\ routine `|close(f)|', which
@^system dependencies@>
should be used when all input or output with respect to |f| has been completed.
This makes |f| available to be opened again, if desired; and if |f| was used for
output, the |close| operation makes the corresponding external file appear
on the user's area, ready to be read.

These procedures should not generate error messages if a file is
being closed before it has been successfully opened.

@p procedure a_close(var f:alpha_file); {close a text file}
begin close(f);
end;
@#
procedure b_close(var f:byte_file); {close a binary file}
begin close(f);
end;
@#
procedure w_close(var f:word_file); {close a word file}
begin close(f);
end;
@y
@ And all the file closing routines as well.
@z

@x
@!buffer:array[0..buf_size] of packed_ASCII_code; {lines of characters being read}
@y
@!buffer:^packed_ASCII_code; {lines of characters being read}
@z

@x
@p function input_ln(var f:alpha_file;@!bypass_eoln:boolean):boolean;
  {inputs the next line or returns |false|}
var last_nonblank:0..buf_size; {|last| with trailing blanks removed}
begin if bypass_eoln then if not eof(f) then get(f);
  {input the first character of the line into |f^|}
last:=first; {cf.\ Matthew 19\thinspace:\thinspace30}
if eof(f) then input_ln:=false
else  begin last_nonblank:=first;
  while not eoln(f) do
    begin if last>=max_buf_stack then
      begin max_buf_stack:=last+1;
      if max_buf_stack=buf_size then
        @<Report overflow of the input buffer, and abort@>;
      end;
    buffer[last]:=f^; get(f); incr(last);
    if buffer[last-1]<>" " then last_nonblank:=last;
    end;
  last:=last_nonblank; input_ln:=true;
  end;
end;
@y
We define |input_ln| in C, for efficiency. Nevertheless we quote the module
`Report overflow of the input buffer, and abort' here in order to make
\.{WEAVE} happy, since part of that module is needed by e-TeX.

@p @{ @<Report overflow of the input buffer, and abort@> @}
@z

@x
@<Glob...@>=
@!term_in:alpha_file; {the terminal as an input file}
@!term_out:alpha_file; {the terminal as an output file}
@y
@d term_in==stdin {the terminal as an input file}
@d term_out==stdout {the terminal as an output file}

@<Glob...@>=
@!init
@!ini_version:boolean; {are we \.{INITEX}?}
@!dump_option:boolean; {was the dump name option used?}
@!dump_line:boolean; {was a \.{\%\AM format} line seen?}
tini@/
@#
@!bound_default:integer; {temporary for setup}
@!bound_name:^char; {temporary for setup}
@#
@!error_line:integer; {width of context lines on terminal error messages}
@!half_error_line:integer; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line:integer;
  {width of longest text lines output; should be at least 60}
@!ocp_list_size:integer;
@!ocp_buf_size:integer;
@!ocp_stack_size:integer;
@!max_strings:integer; {maximum number of strings; must not exceed |max_halfword|}
@!strings_free:integer; {strings available after format loaded}
@!string_vacancies:integer; {the minimum number of characters that should be
  available for the user's control sequences and font names,
  after \TeX's own error messages are stored}
@!pool_size:integer; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all fonts and
  control sequences; must exceed |string_vacancies| by the total
  length of \TeX's own strings, which is currently about 23000}
@!pool_free:integer;{pool space free after format loaded}
@!font_k:integer; {loop variable for initialization}
@!hyph_size:integer; {maximun number of hyphen exceptions}
@!trie_size:integer; {space for hyphenation patterns; should be larger for
  \.{INITEX} than it is in production versions of \TeX.  50000 is
  needed for English, German, and Portuguese.}
@!buf_size:integer; {maximum number of characters simultaneously present in
  current lines of open files and in control sequences between
  \.{\\csname} and \.{\\endcsname}; must not exceed |max_halfword|}
@!stack_size:integer; {maximum number of simultaneous input sources}
@!max_in_open:integer; {maximum number of input files and error insertions that
  can be going on simultaneously}
@!param_size:integer; {maximum number of simultaneous macro parameters}
@!nest_size:integer; {maximum number of semantic levels simultaneously active}
@!save_size:integer; {space for saving values outside of current group; must be
  at most |max_halfword|}
@!dvi_buf_size:integer; {size of the output buffer; must be a multiple of 8}
@!parsefirstlinep:cinttype; {parse the first line for options}
@!filelineerrorstylep:cinttype; {format messages as file:line:error}
@!haltonerrorp:cinttype; {stop at first error}
@!quoted_filename:boolean; {current filename is quoted}
{Variables for source specials}
@!srcspecialsp : boolean;{Whether |src_specials| are enabled at all}
@!insertsrcspecialauto : boolean;
@!insertsrcspecialeverypar : boolean;
@!insertsrcspecialeveryparend : boolean;
@!insertsrcspecialeverycr : boolean;
@!insertsrcspecialeverymath : boolean;
@!insertsrcspecialeveryhbox : boolean;
@!insertsrcspecialeveryvbox : boolean;
@!insertsrcspecialeverydisplay : boolean;
{for stack overflow check}
@!stack_base : ^char;
@z

@x
@ Here is how to open the terminal files
in \ph. The `\.{/I}' switch suppresses the first |get|.
@^system dependencies@>

@d t_open_in==reset(term_in,'TTY:','/O/I') {open the terminal for text input}
@d t_open_out==rewrite(term_out,'TTY:','/O') {open the terminal for text output}
@y
@ Here is how to open the terminal files.  |t_open_out| does nothing.
|t_open_in|, on the other hand, does the work of ``rescanning,'' or getting
any command line arguments the user has provided.  It's defined in C.

@d t_open_out == {output already open for text output}
@z

@x
these operations can be specified in \ph:
@^system dependencies@>

@d update_terminal == break(term_out) {empty the terminal output buffer}
@d clear_terminal == break_in(term_in,true) {clear the terminal input buffer}
@y
these operations can be specified with {\mc UNIX}.  |update_terminal|
does an |fflush|. |clear_terminal| is redefined
to do nothing, since the user should control the terminal.
@^system dependencies@>

@d update_terminal == fflush (term_out)
@d clear_terminal == do_nothing
@z

@x
@<Report overflow of the input buffer, and abort@>=
if format_ident=0 then
  begin writeln(term_out,'Buffer size exceeded!'); goto final_end;
@.Buffer size exceeded@>
  end
else 
  check_buffer_overflow(buf_size+10) { need  a little bit more}
@y
@<Report overflow of the input buffer, and abort@>=
  check_buffer_overflow(buf_size+10) { need  a little bit more}
@z

@x
@ The following program does the required initialization
without retrieving a possible command line.
It should be clear how to modify this routine to deal with command lines,
if the system permits them.
@^system dependencies@>

@p function init_terminal:boolean; {gets the terminal input started}
label exit;
begin t_open_in;
@y
@ The following program does the required initialization.
Iff anything has been specified on the command line, then |t_open_in|
will return with |last > first|.
@^system dependencies@>

@p function init_terminal:boolean; {gets the terminal input started}
label exit;
begin t_open_in;
if last > first then
  begin loc := first;
  while (loc < last) and (buffer[loc]=' ') do incr(loc);
  if loc < last then
    begin init_terminal := true; goto exit;
    end;
  end;
@z

@x
    write(term_out,'! End of file on the terminal... why?');
@y
    writeln(term_out,'! End of file on the terminal... why?');
@z

@x
@!str_pool:packed array[pool_pointer] of packed_ASCII_code; {the characters}
@!str_start : array[str_number] of pool_pointer; {the starting pointers}
@y
@!str_pool: ^packed_ASCII_code; {the characters}
@!str_start : ^pool_pointer; {the starting pointers}
@z

@x
@p @!init function get_strings_started:boolean; {initializes the string pool,
@y
@p @t\4@>@<Declare additional routines for string recycling@>@/

@!init function get_strings_started:boolean; {initializes the string pool,
@z

@x
would like string @'32 to be the single character @'32 instead of the
@y
would like string @'32 to be printed as the single character @'32
instead of the
@z

@x
@!trick_buf:array[0..error_line] of packed_ASCII_code; {circular buffer for
@y
@!trick_buf:array[0..ssup_error_line] of packed_ASCII_code; {circular buffer for
@z

@x
@d error_stop_mode=3 {stops at every opportunity to interact}
@y
@d error_stop_mode=3 {stops at every opportunity to interact}
@d unspecified_mode=4 {extra value for command-line switch}
@z

@x
  print_nl("! "); print(#);
@y
  if filelineerrorstylep then print_file_line
  else print_nl("! ");
  print(#);
@z

@x
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@y
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@!interactionoption:batch_mode..unspecified_mode; {set from command line}
@z

@x
@ @<Set init...@>=interaction:=error_stop_mode;
@y
@ @<Set init...@>=if interactionoption=unspecified_mode then
  interaction:=error_stop_mode
else
  interaction:=interactionoption;
@z

@x
@<Error hand...@>=
procedure jump_out;
begin goto end_of_TEX;
end;
@y
@d do_final_end==begin
   update_terminal;
   ready_already:=0;
   if (history <> spotless) and (history <> warning_issued) then
       uexit(1)
   else
       uexit(0);
   end

@<Error hand...@>=
procedure jump_out;
begin
close_files_and_terminate;
do_final_end;
end;
@z

@x
show_context;
@y
show_context;
if (haltonerrorp) then begin
  history:=fatal_error_stop; jump_out;
end;
@z

@x
line ready to be edited. But such an extension requires some system
wizardry, so the present implementation simply types out the name of the
file that should be
edited and the relevant line number.
@^system dependencies@>

There is a secret `\.D' option available when the debugging routines haven't
been commented~out.
@^debugging@>
@y
line ready to be edited.
We do this by calling the external procedure |calledit| with a pointer to
the filename, its length, and the line number.
However, here we just set up the variables that will be used as arguments,
since we don't want to do the switch-to-editor until after TeX has closed
its files.
@^system dependencies@>

There is a secret `\.D' option available when the debugging routines haven't
been commented~out.
@^debugging@>
@d edit_file==input_stack[base_ptr]
@z

@x
"E": if base_ptr>0 then
  begin print_nl("You want to edit file ");
@.You want to edit file x@>
  slow_print(input_stack[base_ptr].name_field);
  print(" at line "); print_int(line);
  interaction:=scroll_mode; jump_out;
@y
"E": if base_ptr>0 then
    begin edit_name_start:=str_start_macro(edit_file.name_field);
    edit_name_length:=str_start_macro(edit_file.name_field+1) -
                      str_start_macro(edit_file.name_field);
    edit_line:=line;
    jump_out;
@z

@x
|remainder|, holds the remainder after a division.

@<Glob...@>=
@y
|remainder|, holds the remainder after a division.

@d remainder==tex_remainder

@<Glob...@>=
@z

@x
@!glue_ratio=real; {one-word representation of a glue expansion factor}
@y
@z

@x
macros are simplified in the obvious way when |min_quarterword=0|.
@^inner loop@>@^system dependencies@>

@d qi(#)==#+min_quarterword
  {to put an |eight_bits| item into a quarterword}
@d qo(#)==#-min_quarterword
  {to take an |eight_bits| item out of a quarterword}
@d hi(#)==#+min_halfword
  {to put a sixteen-bit item into a halfword}
@d ho(#)==#-min_halfword
  {to take a sixteen-bit item from a halfword}
@y
macros are simplified in the obvious way when |min_quarterword=0|.
So they have been simplified here in the obvious way.
@^inner loop@>@^system dependencies@>

@d qi(#)==# {to put an |eight_bits| item into a quarterword}
@d qo(#)==# {to take an |eight_bits| item from a quarterword}
@d hi(#)==# {to put a sixteen-bit item into a halfword}
@d ho(#)==# {to take a sixteen-bit item from a halfword}
@z

@x
@!quarterword = min_quarterword..max_quarterword; {1/4 of a word}
@!halfword=min_halfword..max_halfword; {1/2 of a word}
@!two_choices = 1..2; {used when there are two variants in a record}
@!four_choices = 1..4; {used when there are four variants in a record}
@!two_halves = packed record@;@/
  @!rh:halfword;
  case two_choices of
  1: (@!lh:halfword);
  2: (@!b0:quarterword; @!b1:quarterword);
  end;
@!four_quarters = packed record@;@/
  @!b0:quarterword;
  @!b1:quarterword;
  @!b2:quarterword;
  @!b3:quarterword;
  end;
@!memory_word = record@;@/
  case four_choices of
  1: (@!int:integer);
  2: (@!gr:glue_ratio);
  3: (@!hh:two_halves);
  4: (@!qqqq:four_quarters);
  end;
@y
@!quarterword = min_quarterword..max_quarterword;
@!halfword = min_halfword..max_halfword;
@!two_choices = 1..2; {used when there are two variants in a record}
@!four_choices = 1..4; {used when there are four variants in a record}
@=#include "texmfmem.h";@>
@z

@x
  if abs(vmem(p+glue_offset).int)<@'4000000 then print("?.?")
  else if abs(g)>float_constant(20000) then
@y
  { The Unix |pc| folks removed this restriction with a remark that
    invalid bit patterns were vanishingly improbable, so we follow
    their example without really understanding it.
  |if abs(vmem(p+glue_offset).int)<@'4000000 then print('?.?')|
  |else| }
  if fabs(g)>float_constant(20000) then
@z

@x
begin if m>0 then
  case m div (max_command+1) of
  0:print("vertical");
  1:print("horizontal");
  2:print("display math");
  end
else if m=0 then print("no")
else  case (-m) div (max_command+1) of
  0:print("internal vertical");
  1:print("restricted horizontal");
  2:print("math");
  end;
print(" mode");
end;
@y
begin if m>0 then
  case m div (max_command+1) of
  0:print("vertical mode");
  1:print("horizontal mode");
  2:print("display math mode");
  end
else if m=0 then print("no mode")
else  case (-m) div (max_command+1) of
  0:print("internal vertical mode");
  1:print("restricted horizontal mode");
  2:print("math mode");
  end;
end;

procedure print_in_mode(@!m:integer); {prints the mode represented by |m|}
begin if m>0 then
  case m div (max_command+1) of
  0:print("' in vertical mode");
  1:print("' in horizontal mode");
  2:print("' in display math mode");
  end
else if m=0 then print("' in no mode")
else  case (-m) div (max_command+1) of
  0:print("' in internal vertical mode");
  1:print("' in restricted horizontal mode");
  2:print("' in math mode");
  end;
end;
@z

@x
@!nest:array[0..nest_size] of list_state_record;
@y
@!nest:^list_state_record;
@z

@x
prev_graf:=0; shown_mode:=0;
dir_save:=null; dir_math_save:=false;
local_par:=null; local_par_bool:=false;
@<Start a new current page@>;
@y
prev_graf:=0; shown_mode:=0;
dir_save:=null; dir_math_save:=false;
local_par:=null; local_par_bool:=false;
@/{The following piece of code is a copy of module 991:}
page_contents:=empty; page_tail:=page_head; {|link(page_head):=null;|}@/
last_glue:=max_halfword; last_penalty:=0; last_kern:=0;
page_depth:=0; page_max_depth:=0;
@z

@x
    print_int(nest[p].pg_field); print(" line");
    if nest[p].pg_field<>1 then print_char("s");
@y
    print_int(nest[p].pg_field);
    if nest[p].pg_field<>1 then print(" lines")
    else print(" line");
@z

@x
@d frozen_null_font=frozen_control_sequence+12
  {permanent `\.{\\nullfont}'}
@y
@d frozen_special=frozen_control_sequence+12
  {permanent `\.{\\special}'}
@d frozen_null_font=frozen_control_sequence+13
  {permanent `\.{\\nullfont}'}
@z

@x
for k:=active_base to undefined_control_sequence-1 do
  eqtb[k]:=eqtb[undefined_control_sequence];
@y
for k:=active_base to eqtb_top do
  eqtb[k]:=eqtb[undefined_control_sequence];
@z

@x
@ The following procedure, which is called just before \TeX\ initializes its
input and output, establishes the initial values of the date and time.
@^system dependencies@>
Since standard \PASCAL\ cannot provide such information, something special
is needed. The program here simply specifies July 4, 1776, at noon; but
users probably want a better approximation to the truth.

@p procedure fix_date_and_time;
begin time:=12*60; {minutes since midnight}
day:=4; {fourth day of the month}
month:=7; {seventh month of the year}
year:=1776; {Anno Domini}
end;
@y
@ The following procedure, which is called just before \TeX\ initializes its
input and output, establishes the initial values of the date and time.
It calls a macro-defined |date_and_time| routine.  |date_and_time|
in turn is a C macro, which calls |get_date_and_time|, passing
it the addresses of the day, month, etc., so they can be set by the
routine.  |get_date_and_time| also sets up interrupt catching if that
is conditionally compiled in the C code.
@^system dependencies@>

@d fix_date_and_time==dateandtime(time,day,month,year)
@z

@x
else if n<glue_base then @<Show equivalent |n|, in region 1 or 2@>
@y
else if (n<glue_base) or ((n>eqtb_size)and(n<=eqtb_top)) then
  @<Show equivalent |n|, in region 1 or 2@>
@z

@x
@!eqtb:array[active_base..eqtb_size] of memory_word;
@y
@!zeqtb:^memory_word;
@z

@x
@!hash: array[hash_base..undefined_control_sequence-1] of two_halves;
  {the hash table}
@!hash_used:pointer; {allocation pointer for |hash|}
@y
@!hash: ^two_halves; {the hash table}
@!yhash: ^two_halves; {auxiliary pointer for freeing hash}
@!hash_used:pointer; {allocation pointer for |hash|}
@!hash_extra:pointer; {|hash_extra=hash| above |eqtb_size|}
@!hash_top:pointer; {maximum of the hash array}
@!eqtb_top:pointer; {maximum of the |eqtb|}
@!hash_high:pointer; {pointer to next high hash location}
@z

@x
next(hash_base):=0; text(hash_base):=0;
for k:=hash_base+1 to undefined_control_sequence-1 do hash[k]:=hash[hash_base];
@y
@z

@x
hash_used:=frozen_control_sequence; {nothing is used}
@y
hash_used:=frozen_control_sequence; {nothing is used}
hash_high:=0;
@z

@x
@ @<Insert a new control...@>=
begin if text(p)>0 then
  begin repeat if hash_is_full then overflow("hash size",hash_size);
@:TeX capacity exceeded hash size}{\quad hash size@>
  decr(hash_used);
  until text(hash_used)=0; {search for an empty location in |hash|}
  next(p):=hash_used; p:=hash_used;
  end;
@y
@ @<Insert a new control...@>=
begin if text(p)>0 then
  begin if hash_high<hash_extra then
      begin incr(hash_high);
      next(p):=hash_high+eqtb_size; p:=hash_high+eqtb_size;
      end
    else begin
      repeat if hash_is_full then overflow("hash size",hash_size+hash_extra);
@:TeX capacity exceeded hash size}{\quad hash size@>
      decr(hash_used);
      until text(hash_used)=0; {search for an empty location in |hash|}
    next(p):=hash_used; p:=hash_used;
    end;
  end;
@z

@x
else if p>=undefined_control_sequence then print_esc("IMPOSSIBLE.")
@y
else if ((p>=undefined_control_sequence)and(p<=eqtb_size))or(p>eqtb_top) then
  print_esc("IMPOSSIBLE.")
@z

@x
else if (text(p)<0)or(text(p)>=str_ptr) then print_esc("NONEXISTENT.")
@y
else if (text(p)>=str_ptr) then print_esc("NONEXISTENT.")
@z

@x
@!save_stack : array[0..save_size] of memory_word;
@y
@!save_stack : ^memory_word;
@z

@x
if p<int_base then
@y
if (p<int_base)or(p>eqtb_size) then
@z

@x
if cs_token_flag+undefined_control_sequence>max_halfword then bad:=21;
@y
if cs_token_flag+eqtb_size+hash_extra>max_halfword then bad:=21;
if (hash_offset<0)or(hash_offset>hash_base) then bad:=42;
@z

@x
@!input_stack : array[0..stack_size] of in_state_record;
@y
@!input_stack : ^in_state_record;
@z

@x
@!input_file : array[1..max_in_open] of alpha_file;
@!line : integer; {current line number in the current source file}
@!line_stack : array[1..max_in_open] of integer;
@y
@!input_file : ^alpha_file;
@!line : integer; {current line number in the current source file}
@!line_stack : ^integer;
@!source_filename_stack : ^str_number;
@!full_source_filename_stack : ^str_number;
@z

@x
  begin print_nl("Runaway ");
@.Runaway...@>
  case scanner_status of
  defining: begin print("definition"); p:=def_ref;
    end;
  matching: begin print("argument"); p:=temp_token_head;
    end;
  aligning: begin print("preamble"); p:=hold_token_head;
    end;
  absorbing: begin print("text"); p:=def_ref;
    end;
  end; {there are no other cases}
@y
  begin
@.Runaway...@>
  case scanner_status of
  defining: begin print_nl("Runaway definition"); p:=def_ref;
    end;
  matching: begin print_nl("Runaway argument"); p:=temp_token_head;
    end;
  aligning: begin print_nl("Runaway preamble"); p:=hold_token_head;
    end;
  absorbing: begin print_nl("Runaway text"); p:=def_ref;
    end;
  end; {there are no other cases}
@z

@x
@!param_stack:array [0..param_size] of pointer;
  {token list pointers for parameters}
@y
@!param_stack: ^pointer;
  {token list pointers for parameters}
@z

@x
incr(in_open); push_input; index:=in_open;
@y
incr(in_open); push_input; index:=in_open;
source_filename_stack[index]:=0;full_source_filename_stack[index]:=0;
@z

@x
begin input_ptr:=0; max_in_stack:=0;
@y
begin input_ptr:=0; max_in_stack:=0;
source_filename_stack[0]:=0;full_source_filename_stack[0]:=0;
@z

@x
print(" while scanning ");
@y
@z

@x
defining:begin print("definition"); info(p):=right_brace_token+"}";
  end;
matching:begin print("use"); info(p):=par_token; long_state:=outer_call;
  end;
aligning:begin print("preamble"); info(p):=right_brace_token+"}"; q:=p;
  p:=get_avail; link(p):=q; info(p):=cs_token_flag+frozen_cr;
  align_state:=-1000000;
  end;
absorbing:begin print("text"); info(p):=right_brace_token+"}";
@y
defining:begin print(" while scanning definition"); info(p):=right_brace_token+"}";
  end;
matching:begin print(" while scanning use"); info(p):=par_token; long_state:=outer_call;
  end;
aligning:begin print(" while scanning preamble"); info(p):=right_brace_token+"}"; q:=p;
  p:=get_avail; link(p):=q; info(p):=cs_token_flag+frozen_cr;
  align_state:=-1000000;
  end;
absorbing:begin print(" while scanning text"); info(p):=right_brace_token+"}";
@z

@x [25.366] stack overflow protection
begin cv_backup:=cur_val; cvl_backup:=cur_val_level; radix_backup:=radix;
@y
begin if abs(stringcast(addressof(t)) - stack_base) > stack_limit
  then overflow("stack", stack_limit);
cv_backup:=cur_val; cvl_backup:=cur_val_level; radix_backup:=radix;
@z


@x
if_eof_code: begin scan_four_bit_int; b:=(read_open[cur_val]=closed);
  end;
@y
if_eof_code: begin scan_four_bit_int_or_18;
  if cur_val=18 then b:=not shellenabledp
  else b:=(read_open[cur_val]=closed);
  end;
@z

@x
@ The file names we shall deal with for illustrative purposes have the
following structure:  If the name contains `\.>' or `\.:', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@y
@ The file names we shall deal with have the
following structure:  If the name contains `\./' or `\.:'
(for Amiga only), the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the last
`\..' to the end, otherwise the file extension is null.
@z

@x
@!area_delimiter:pool_pointer; {the most recent `\.>' or `\.:', if any}
@!ext_delimiter:pool_pointer; {the relevant `\..', if any}
@y
@!area_delimiter:pool_pointer; {the most recent `\./', if any}
@!ext_delimiter:pool_pointer; {the most recent `\..', if any}
@z

@x
@d TEX_area=="TeXinputs:"
@.TeXinputs@>
@d TEX_font_area=="TeXfonts:"
@.TeXfonts@>
@y
In C, the default paths are specified separately.
@z

@x
begin area_delimiter:=0; ext_delimiter:=0;
@y
begin area_delimiter:=0; ext_delimiter:=0; quoted_filename:=false;
@z

@x
begin if c=" " then more_name:=false
@y
begin if (c=" ") and stop_at_space and (not quoted_filename) then
  more_name:=false
else  if c="""" then begin
  quoted_filename:=not quoted_filename;
  more_name:=true;
  end
@z

@x
  if (c=">")or(c=":") then
@y
  if ISDIRSEP(c) then
@z

@x
  else if (c=".")and(ext_delimiter=0) then ext_delimiter:=cur_length;
@y
  else if c="." then ext_delimiter:=cur_length;
@z

@x
@ The third.
@^system dependencies@>

@p procedure end_name;
@y
@ The third.
@^system dependencies@>
If a string is already in the string pool, the function
|slow_make_string| does not create a new string but returns this string
number, thus saving string space.  Because of this new property of the
returned string number it is not possible to apply |flush_string| to
these strings.

@p procedure end_name;
var temp_str: str_number; {result of file name cache lookups}
@!j,@!s,@!t: pool_pointer; {running indices}
@!must_quote:boolean; {whether we need to quote a string}
@z

@x
@:TeX capacity exceeded number of strings}{\quad number of strings@>
@y
@:TeX capacity exceeded number of strings}{\quad number of strings@>
str_room(6); {Room for quotes, if needed.}
{add quotes if needed}
if area_delimiter<>0 then begin
  {maybe quote |cur_area|}
  must_quote:=false;
  s:=str_start_macro(str_ptr);
  t:=str_start_macro(str_ptr)+area_delimiter;
  j:=s;
  while (not must_quote) and (j<t) do begin
    must_quote:=str_pool[j]=" "; incr(j);
    end;
  if must_quote then begin
    for j:=pool_ptr-1 downto t do str_pool[j+2]:=str_pool[j];
    str_pool[t+1]:="""";
    for j:=t-1 downto s do str_pool[j+1]:=str_pool[j];
    str_pool[s]:="""";
    if ext_delimiter<>0 then ext_delimiter:=ext_delimiter+2;
    area_delimiter:=area_delimiter+2;
    pool_ptr:=pool_ptr+2;
    end;
  end;
{maybe quote |cur_name|}
s:=str_start_macro(str_ptr)+area_delimiter;
if ext_delimiter=0 then t:=pool_ptr else t:=str_start_macro(str_ptr)+ext_delimiter-1;
must_quote:=false;
j:=s;
while (not must_quote) and (j<t) do begin
  must_quote:=str_pool[j]=" "; incr(j);
  end;
if must_quote then begin
  for j:=pool_ptr-1 downto t do str_pool[j+2]:=str_pool[j];
  str_pool[t+1]:="""";
  for j:=t-1 downto s do str_pool[j+1]:=str_pool[j];
  str_pool[s]:="""";
  if ext_delimiter<>0 then ext_delimiter:=ext_delimiter+2;
  pool_ptr:=pool_ptr+2;
  end;
if ext_delimiter<>0 then begin
  {maybe quote |cur_ext|}
  s:=str_start_macro(str_ptr)+ext_delimiter-1;
  t:=pool_ptr;
  must_quote:=false;
  j:=s;
  while (not must_quote) and (j<t) do begin
    must_quote:=str_pool[j]=" "; incr(j);
    end;
  if must_quote then begin
    str_pool[t+1]:="""";
    for j:=t-1 downto s do str_pool[j+1]:=str_pool[j];
    str_pool[s]:="""";
    pool_ptr:=pool_ptr+2;
    end;
  end;
@z

@x
  str_start_macro(str_ptr+1):=str_start_macro(str_ptr)+area_delimiter; incr(str_ptr);
  end;
if ext_delimiter=0 then
  begin cur_ext:=""; cur_name:=make_string;
@y
  str_start_macro(str_ptr+1):=str_start_macro(str_ptr)+area_delimiter; incr(str_ptr);
  temp_str:=search_string(cur_area);
  if temp_str>0 then
    begin cur_area:=temp_str;
    decr(str_ptr);  {no |flush_string|, |pool_ptr| will be wrong!}
    for j:=str_start_macro(str_ptr+1) to pool_ptr-1 do
      begin str_pool[j-area_delimiter]:=str_pool[j];
      end;
    pool_ptr:=pool_ptr-area_delimiter; {update |pool_ptr|}
    end;
  end;
if ext_delimiter=0 then
  begin cur_ext:=""; cur_name:=slow_make_string;
@z

@x
else  begin cur_name:=str_ptr;
  str_start_macro(str_ptr+1):=str_start_macro(str_ptr)+ext_delimiter-area_delimiter-1;
  incr(str_ptr); cur_ext:=make_string;
@y
else  begin cur_name:=str_ptr;
  str_start_macro(str_ptr+1):=str_start_macro(str_ptr)+ext_delimiter-area_delimiter-1;
  incr(str_ptr); cur_ext:=make_string;
  decr(str_ptr); {undo extension string to look at name part}
  temp_str:=search_string(cur_name);
  if temp_str>0 then
    begin cur_name:=temp_str;
    decr(str_ptr);  {no |flush_string|, |pool_ptr| will be wrong!}
    for j:=str_start_macro(str_ptr+1) to pool_ptr-1 do
      begin str_pool[j-ext_delimiter+area_delimiter+1]:=str_pool[j];
      end;
    pool_ptr:=pool_ptr-ext_delimiter+area_delimiter+1;  {update |pool_ptr|}
    end;
  cur_ext:=slow_make_string;  {remake extension string}
@z

@x
begin slow_print(a); slow_print(n); slow_print(e);
@y
var must_quote: boolean; {whether to quote the filename}
@!j:pool_pointer; {index into |str_pool|}
begin
must_quote:=false;
if a<>0 then begin
  j:=str_start_macro(a);
  while (not must_quote) and (j<str_start_macro(a+1)) do begin
    must_quote:=str_pool[j]=" "; incr(j);
  end;
end;
if n<>0 then begin
  j:=str_start_macro(n);
  while (not must_quote) and (j<str_start_macro(n+1)) do begin
    must_quote:=str_pool[j]=" "; incr(j);
  end;
end;
if e<>0 then begin
  j:=str_start_macro(e);
  while (not must_quote) and (j<str_start_macro(e+1)) do begin
    must_quote:=str_pool[j]=" "; incr(j);
  end;
end;
{FIXME: Alternative is to assume that any filename that has to be quoted has
 at least one quoted component...if we pick this, a number of insertions
 of |print_file_name| should go away.
|must_quote|:=((|a|<>0)and(|str_pool|[|str_start|[|a|]]=""""))or
              ((|n|<>0)and(|str_pool|[|str_start|[|n|]]=""""))or
              ((|e|<>0)and(|str_pool|[|str_start|[|e|]]=""""));}
if must_quote then print_char("""");
if a<>0 then
  for j:=str_start_macro(a) to str_start_macro(a+1)-1 do
    if so(str_pool[j])<>"""" then
      print(so(str_pool[j]));
if n<>0 then
  for j:=str_start_macro(n) to str_start_macro(n+1)-1 do
    if so(str_pool[j])<>"""" then
      print(so(str_pool[j]));
if e<>0 then
  for j:=str_start_macro(e) to str_start_macro(e+1)-1 do
    if so(str_pool[j])<>"""" then
      print(so(str_pool[j]));
if must_quote then print_char("""");
@z

@x
@d append_to_name(#)==begin c:=#; incr(k);
  if k<=file_name_size then nameoffile[k]:=xchr[c];
  end
@y
@d append_to_name(#)==begin c:=#; if not (c="""") then begin incr(k);
  if k<=file_name_size then nameoffile[k]:=xchr[c];
  end end
@z

@x
for j:=str_start_macro(a) to str_start_macro(a+1)-1 do append_to_name(so(str_pool[j]));
@y
if nameoffile then libcfree (nameoffile);
nameoffile:= xmallocarray (packed_ASCII_code, length(a)+length(n)+length(e)+1);
for j:=str_start_macro(a) to str_start_macro(a+1)-1 do append_to_name(so(str_pool[j]));
@z

@x
for k:=namelength+1 to file_name_size do nameoffile[k]:=' ';
@y
nameoffile[namelength+1]:=0;
@z

@x
@d format_default_length=20 {length of the |TEX_format_default| string}
@d format_area_length=11 {length of its area part}
@d format_ext_length=4 {length of its `\.{.fmt}' part}
@y
Under {\mc UNIX} we don't give the area part, instead depending
on the path searching that will happen during file opening.  Also, the
length will be set in the main program.

@d format_area_length=0 {length of its area part}
@d format_ext_length=4 {length of its `\.{.fmt}' part}
@z

@x
@!TEX_format_default:packed array[1..format_default_length] of char;

@ @<Set init...@>=
TEX_format_default:='TeXformats:plain.fmt';
@y
@!format_default_length: integer;
@!TEX_format_default: ^char;

@ We set the name of the default format file and the length of that name
in C, instead of Pascal, since we want them to depend on the name of the
program.
@z

@x
for j:=1 to n do append_to_name(TEX_format_default[j]);
@y
if nameoffile then libcfree (nameoffile);
nameoffile := xmallocarray (packed_ASCII_code, n+(b-a+1)+format_ext_length+1);
for j:=1 to n do append_to_name(TEX_format_default[j]);
@z

@x
for k:=namelength+1 to file_name_size do nameoffile[k]:=' ';
@y
nameoffile[namelength+1]:=0;
@z

@x
  pack_buffered_name(0,loc,j-1); {try first without the system file area}
  if w_open_in(fmt_file) then goto found;
  pack_buffered_name(format_area_length,loc,j-1);
    {now try the system format file area}
  if w_open_in(fmt_file) then goto found;
@y
  pack_buffered_name(0,loc,j-1); {Kpathsea does everything}
  if w_open_in(fmt_file) then goto found;
@z

@x
  wterm_ln('Sorry, I can''t find that format;',' will try PLAIN.');
@y
  wterm ('Sorry, I can''t find the format `');
  fputs (stringcast(nameoffile + 1), stdout);
  wterm ('''; will try `');
  fputs (TEX_format_default + 1, stdout);
  wterm_ln ('''.');
@z

@x
  wterm_ln('I can''t find the PLAIN format file!');
@.I can't find PLAIN...@>
@y
  wterm ('I can''t find the format file `');
  fputs (TEX_format_default + 1, stdout);
  wterm_ln ('''!');
@.I can't find the format...@>
@z

@x
  make_name_string:=make_string;
  end;
@y
  make_name_string:=make_string;
  end;
  {At this point we also set |cur_name|, |cur_ext|, and |cur_area| to
   match the contents of |nameoffile|.}
  k:=1;
  name_in_progress:=true;
  begin_name;
  stop_at_space:=false;
  while (k<=namelength)and(more_name(nameoffile[k])) do
    incr(k);
  stop_at_space:=true;
  end_name;
  name_in_progress:=false;
@z

@x
  if not more_name(cur_chr) then goto done;
@y
  {If |cur_chr| is a space and we're not scanning a token list, check
   whether we're at the end of the buffer. Otherwise we end up adding
   spurious spaces to file names in some cases.}
  if (cur_chr=" ") and (state<>token_list) and (loc>limit) then goto done;
  if not more_name(cur_chr) then goto done;
@z

@x
var k:0..buf_size; {index into |buffer|}
@y
var k:0..buf_size; {index into |buffer|}
@!saved_cur_name:str_number; {to catch empty terminal input}
@z

@x
if e=".tex" then show_context;
@y
if (e=".tex") or (e="") then show_context;
@z

@x
clear_terminal; prompt_input(": "); @<Scan file name in the buffer@>;
if cur_ext="" then cur_ext:=e;
@y
saved_cur_name:=cur_name;
clear_terminal; prompt_input(": "); @<Scan file name in the buffer@>;
if cur_ext="" then cur_ext:=e;
if length(cur_name)=0 then cur_name:=saved_cur_name;
@z

@x
@d ensure_dvi_open==if output_file_name=0 then
@y
@d log_name == texmf_log_name
@d ensure_dvi_open==if output_file_name=0 then
@z

@x
@!months:packed array [1..36] of char; {abbreviations of month names}
@y
@!months:^char;
@z

@x
if job_name=0 then job_name:="texput";
@.texput@>
@y
if job_name=0 then job_name:=getjobname("texput");
@.texput@>
pack_job_name(".fls");
recorder_change_filename(stringcast(nameoffile+1));
@z

@x
begin wlog(banner);
@y
begin if srcspecialsp or filelineerrorstylep or parsefirstlinep
then
  wlog(banner_k)
else
  wlog(banner);
@z

@x
slow_print(format_ident); print("  ");
@y
wlog(versionstring);
slow_print(format_ident); print("  ");
@z

@x
months:='JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@y
months := ' JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@z

@x
end
@y
if shellenabledp then begin
  wlog_cr;
  wlog('\write18 enabled.')
  end;
if srcspecialsp then begin
  wlog_cr;
  wlog(' Source specials enabled.')
  end;
if filelineerrorstylep then begin
  wlog_cr;
  wlog(' file:line:error style messages enabled.')
  end;
if parsefirstlinep then begin
  wlog_cr;
  wlog(' %&-line parsing enabled.');
  end;
end
@z

@x
  prompt_file_name("input file name",".tex");
@y
  prompt_file_name("input file name","");
@z

@x
name:=a_make_name_string(cur_file);
@y
name:=a_make_name_string(cur_file);
source_filename_stack[in_open]:=name;
full_source_filename_stack[in_open]:=makefullnamestring;
if name=str_ptr-1 then {we can try to conserve string pool space now}
  begin temp_str:=search_string(name);
  if temp_str>0 then
    begin name:=temp_str; flush_string;
    end;
  end;
@z

@x
  begin job_name:=cur_name; open_log_file;
@y
  begin job_name:=getjobname(cur_name); open_log_file;
@z

@x
if name=str_ptr-1 then {we can conserve string pool space now}
  begin flush_string; name:=cur_name;
  end;
@y
@z

@x
@!ocp_list_info:array[ocp_list_index] of memory_word;
  {the big collection of ocp list data}
@!ocp_listmem_ptr:ocp_list_index; {first unused word of |ocp_list_info|}
@!ocp_listmem_run_ptr:ocp_list_index; {temp unused word of |ocp_list_info|}
@!ocp_lstack_info:array[ocp_lstack_index] of memory_word;
  {the big collection of ocp lstack data}
@!ocp_lstackmem_ptr:ocp_lstack_index; {first unused word of |ocp_lstack_info|}
@!ocp_lstackmem_run_ptr:ocp_lstack_index; {temp unused word of |ocp_lstack_info|}
@!ocp_list_ptr:internal_ocp_list_number; {largest internal ocp list number in use}
@!ocp_list_list:array[internal_ocp_list_number] of ocp_list_index;
@y
@!ocp_list_info:^memory_word;
  {the big collection of ocp list data}
@!ocp_listmem_ptr:ocp_list_index; {first unused word of |ocp_list_info|}
@!ocp_listmem_run_ptr:ocp_list_index; {temp unused word of |ocp_list_info|}
@!ocp_lstack_info:^memory_word;
  {the big collection of ocp lstack data}
@!ocp_lstackmem_ptr:ocp_lstack_index; {first unused word of |ocp_lstack_info|}
@!ocp_lstackmem_run_ptr:ocp_lstack_index; {temp unused word of |ocp_lstack_info|}
@!ocp_list_ptr:internal_ocp_list_number; {largest internal ocp list number in use}
@!ocp_list_list:^ocp_list_index;
@z

@x
@!dvi_buf:array[dvi_index] of real_eight_bits; {buffer for \.{DVI} output}
@!half_buf:dvi_index; {half of |dvi_buf_size|}
@!dvi_limit:dvi_index; {end of the current half buffer}
@!dvi_ptr:dvi_index; {the next available buffer address}
@y
@!dvi_buf:^real_eight_bits; {buffer for \.{DVI} output}
@!half_buf:integer; {half of |dvi_buf_size|}
@!dvi_limit:integer; {end of the current half buffer}
@!dvi_ptr:integer; {the next available buffer address}
@z

@x
@p procedure write_dvi(@!a,@!b:dvi_index);
var k:dvi_index;
begin for k:=a to b do write(dvi_file,dvi_buf[k]);
end;
@y
In C, we use a macro to call |fwrite| or |write| directly, writing all
the bytes in one shot.  Much better even than writing four
bytes at a time.
@z

@x
  old_setting:=selector; selector:=new_string;
@y
if output_comment then
  begin l:=strlen(output_comment); dvi_out(l);
  for s:=0 to l-1 do dvi_out(output_comment[s]);
  end
else begin {the default code is unchanged}
  old_setting:=selector; selector:=new_string;
@z

@x
  pool_ptr:=str_start_macro(str_ptr); {flush the current string}
@y
  pool_ptr:=str_start_macro(str_ptr); {flush the current string}
end;
@z

@x
dvi_out(eop); incr(total_pages); cur_s:=-1;
@y
dvi_out(eop); incr(total_pages); cur_s:=-1;
ifdef ('IPC')
if ipcon>0 then
  begin if dvi_limit=half_buf then
    begin write_dvi(half_buf, dvi_buf_size-1);
    flush_dvi;
    dvi_gone:=dvi_gone+half_buf;
    end;
  if dvi_ptr>0 then
    begin write_dvi(0, dvi_ptr-1);
    flush_dvi;
    dvi_offset:=dvi_offset+dvi_ptr; dvi_gone:=dvi_gone+dvi_ptr;
    end;
  dvi_ptr:=0; dvi_limit:=dvi_buf_size;
  ipcpage(dvi_gone);
  end;
endif ('IPC');
@z

@x
  k:=4+((dvi_buf_size-dvi_ptr) mod 4); {the number of 223's}
@y
ifdef ('IPC')
  k:=7-((3+dvi_offset+dvi_ptr) mod 4); {the number of 223's}
endif ('IPC')
ifndef ('IPC')
  k:=4+((dvi_buf_size-dvi_ptr) mod 4); {the number of 223's}
endifn ('IPC')
@z

@x
    print_nl("Output written on "); slow_print(output_file_name);
@y
    print_nl("Output written on "); print_file_name(0, output_file_name, 0);
@z

@x
    print(" ("); print_int(total_pages); print(" page");
    if total_pages<>1 then print_char("s");
@y
    print(" ("); print_int(total_pages);
    if total_pages<>1 then print(" pages")
    else print(" page");
@z

@x
if abs(fit_class-fitness(r))>1 then d:=d+adj_demerits;
@y
if abs(intcast(fit_class)-intcast(fitness(r)))>1 then d:=d+adj_demerits;
@z

@x
  begin line_diff:=line_number(r)-best_line;
@y
  begin line_diff:=intcast(line_number(r))-intcast(best_line);
@z

@x
Comparatively few different number sequences $n_0\ldots n_k$ actually occur,
since most of the |n|'s are generally zero. Therefore the number sequences
are encoded in such a way that |trie_op|$(z_k)$ is only one byte long.
If |trie_op(@t$z_k$@>)<>min_quarterword|, when $p_1\ldots p_k$ has matched
the letters in |hc[(l-k+1)..l@,]| of language |t|,
we perform all of the required operations
for this pattern by carrying out the following little program: Set
|v:=trie_op(@t$z_k$@>)|. Then set |v:=v+op_start[t]|,
|hyf[l-hyf_distance[v]]:=@tmax@>(hyf[l-hyf_distance[v]], hyf_num[v])|,
and |v:=hyf_next[v]|; repeat, if necessary, until |v=min_quarterword|.
@y
The theory that comparatively few different number sequences $n_0\ldots n_k$
actually occur, since most of the |n|'s are generally zero, seems to fail
at least for the large German hyphenation patterns.
Therefore the number sequences cannot any longer be encoded in such a way
that |trie_op|$(z_k)$ is only one byte long.
We have introduced a new constant |max_trie_op| for the maximum allowable
hyphenation operation code value; |max_trie_op| might be different for
\TeX\ and \.{INITEX} and must not exceed |max_halfword|.
An opcode will occupy a halfword if |max_trie_op| exceeds |max_quarterword|
or a quarterword otherwise.
@^system dependencies@>
If |trie_op(@t$z_k$@>)<>min_trie_op|, when $p_1\ldots p_k$ has matched
the letters in |hc[(l-k+1)..l@,]| of language |t|,
we perform all of the required operations
for this pattern by carrying out the following little program: Set
|v:=trie_op(@t$z_k$@>)|. Then set |v:=v+op_start[t]|,
|hyf[l-hyf_distance[v]]:=@tmax@>(hyf[l-hyf_distance[v]], hyf_num[v])|,
and |v:=hyf_next[v]|; repeat, if necessary, until |v=min_trie_op|.
@z

@x
@!trie_pointer=integer; {an index into |trie|}
@y
@!trie_pointer=integer; {an index into |trie|}
@!trie_opcode=integer;  {a trie opcode}
@z

@x
@ @d trie_link(#)==trie[#].rh {``downward'' link in a trie}
@d trie_char(#)==trie[#].b1 {character matched at this trie location}
@d trie_op(#)==trie[#].b0 {program for hyphenation at this trie location}
@y
@ For more than 255 trie op codes, the three fields |trie_link|, |trie_char|,
and |trie_op| will no longer fit into one memory word; thus using web2c
we define |trie| as three array instead of an array of records.
The variant will be implented by reusing the opcode field later on with
another macro.

@d trie_link(#)==trie_trl[#] {``downward'' link in a trie}
@d trie_char(#)==trie_trc[#] {character matched at this trie location}
@d trie_op(#)==trie_tro[#] {program for hyphenation at this trie location}
@z

@x
@!trie:array[trie_pointer] of two_halves; {|trie_link|, |trie_char|, |trie_op|}
@y
{We will dynamically allocate these arrays.}
@!trie_trl:^trie_pointer; {|trie_link|}
@!trie_tro:^trie_pointer; {|trie_op|}
@!trie_trc:^quarterword; {|trie_char|}
@z

@x
@!hyf_next:array[1..trie_op_size] of quarterword; {continuation code}
@y
@!hyf_next:array[1..trie_op_size] of trie_opcode; {continuation code}
@z

@x
    begin if trie_op(z)<>min_quarterword then
@y
    begin if trie_op(z)<>min_trie_op then
@z

@x
until v=min_quarterword;
@y
until v=min_trie_op;
@z

@x
different from $\alpha$, we can conclude that $\alpha$ is not in the table.
@y
different from $\alpha$, we can conclude that $\alpha$ is not in the table.
This is a clever scheme which saves the need for a hash link array.
However, it is difficult to increase the size of the hyphen exception
arrays. To make this easier, the ordered hash has been replaced by
a simple hash, using an additional array |hyph_link|. The value
|0| in |hyph_link[k]| means that there are no more entries corresponding
to the specific hash chain. When |hyph_link[k]>0|, the next entry in
the hash chain is |hyph_link[k]-1|. This value is used because the
arrays start at |0|.
@z

@x
@!hyph_pointer=0..hyph_size; {an index into the ordered hash table}
@y
@!hyph_pointer=0..ssup_hyph_size; {index into hyphen exceptions hash table;
                     enlarging this requires changing (un)dump code}
@z

@x
@!hyph_word:array[hyph_pointer] of str_number; {exception words}
@!hyph_list:array[hyph_pointer] of pointer; {list of hyphen positions}
@!hyph_count:hyph_pointer; {the number of words in the exception dictionary}
@y
@!hyph_word: ^str_number; {exception words}
@!hyph_list: ^pointer; {list of hyphen positions}
@!hyph_link: ^hyph_pointer; {link array for hyphen exceptions hash table}
@!hyph_count:integer; {the number of words in the exception dictionary}
@!hyph_next:integer; {next free slot in hyphen exceptions hash table}
@z

@x
for z:=0 to hyph_size do
  begin hyph_word[z]:=0; hyph_list[z]:=null;
  end;
hyph_count:=0;
@y
for z:=0 to hyph_size do
  begin hyph_word[z]:=0; hyph_list[z]:=null; hyph_link[z]:=0;
  end;
hyph_count:=0;
hyph_next:=hyph_prime+1; if hyph_next>hyph_size then hyph_next:=hyph_prime;
@z

@x
h:=hc[1]; incr(hn); hc[hn]:=cur_lang;
for j:=2 to hn do h:=(h+h+hc[j]) mod hyph_size;
loop@+  begin @<If the string |hyph_word[h]| is less than \(hc)|hc[1..hn]|,
    |goto not_found|; but if the two strings are equal,
    set |hyf| to the hyphen positions and |goto found|@>;
  if h>0 then decr(h)@+else h:=hyph_size;
  end;
not_found: decr(hn)
@y
h:=hc[1]; incr(hn); hc[hn]:=cur_lang;
for j:=2 to hn do h:=(h+h+hc[j]) mod hyph_prime;
loop@+  begin @<If the string |hyph_word[h]| is less than \(hc)|hc[1..hn]|,
    |goto not_found|; but if the two strings are equal,
    set |hyf| to the hyphen positions and |goto found|@>;
  h:=hyph_link[h]; if h=0 then goto not_found;
  decr(h);
  end;
not_found: decr(hn)
@z

@x
@ @<If the string |hyph_word[h]| is less than \(hc)...@>=
k:=hyph_word[h]; if k=0 then goto not_found;
if length(k)<hn then goto not_found;
if length(k)=hn then
  begin j:=1; u:=str_start_macro(k);
  repeat if so(str_pool[u])<hc[j] then goto not_found;
  if so(str_pool[u])>hc[j] then goto done;
  incr(j); incr(u);
  until j>hn;
  @<Insert hyphens as specified in |hyph_list[h]|@>;
  decr(hn); goto found;
  end;
done:
@y
@ @<If the string |hyph_word[h]| is less than \(hc)...@>=
{This is now a simple hash list, not an ordered one, so
the module title is no longer descriptive.}
k:=hyph_word[h]; if k=0 then goto not_found;
if length(k)=hn then
  begin j:=1; u:=str_start_macro(k);
  repeat
  if so(str_pool[u])<>hc[j] then goto done;
  incr(j); incr(u);
  until j>hn;
  @<Insert hyphens as specified in |hyph_list[h]|@>;
  decr(hn); goto found;
  end;
done:
@z

@x
@!s,@!t:str_number; {strings being compared or stored}
@y
@!s:str_number; {strings being compared or stored}
@z

@x
  begin h:=(h+h+hc[j]) mod hyph_size;
@y
  begin h:=(h+h+hc[j]) mod hyph_prime;
@z

@x
@ @<Insert the \(p)pair |(s,p)|...@>=
if hyph_count=hyph_size then overflow("exception dictionary",hyph_size);
@:TeX capacity exceeded exception dictionary}{\quad exception dictionary@>
incr(hyph_count);
while hyph_word[h]<>0 do
  begin @<If the string |hyph_word[h]| is less than \(or)or equal to
  |s|, interchange |(hyph_word[h],hyph_list[h])| with |(s,p)|@>;
  if h>0 then decr(h)@+else h:=hyph_size;
  end;
hyph_word[h]:=s; hyph_list[h]:=p
@y
@ @<Insert the \(p)pair |(s,p)|...@>=
  if hyph_next <= hyph_prime then
     while (hyph_next>0) and (hyph_word[hyph_next-1]>0) do decr(hyph_next);
if (hyph_count=hyph_size)or(hyph_next=0) then
   overflow("exception dictionary",hyph_size);
@:TeX capacity exceeded exception dictionary}{\quad exception dictionary@>
incr(hyph_count);
while hyph_word[h]<>0 do
  begin @<If the string |hyph_word[h]| is less than \(or)or equal to
  |s|, interchange |(hyph_word[h],hyph_list[h])| with |(s,p)|@>;
  if hyph_link[h]=0 then
  begin
    hyph_link[h]:=hyph_next;
    if hyph_next >= hyph_size then hyph_next:=hyph_prime;
    if hyph_next > hyph_prime then incr(hyph_next);
  end;
  h:=hyph_link[h]-1;
  end;

found: hyph_word[h]:=s; hyph_list[h]:=p
@z

@x
@ @<If the string |hyph_word[h]| is less than \(or)...@>=
k:=hyph_word[h];
if length(k)<length(s) then goto found;
if length(k)>length(s) then goto not_found;
u:=str_start_macro(k); v:=str_start_macro(s);
repeat if str_pool[u]<str_pool[v] then goto found;
if str_pool[u]>str_pool[v] then goto not_found;
incr(u); incr(v);
until u=str_start_macro(k+1);
found:q:=hyph_list[h]; hyph_list[h]:=p; p:=q;@/
t:=hyph_word[h]; hyph_word[h]:=s; s:=t;
not_found:
@y
@ @<If the string |hyph_word[h]| is less than \(or)...@>=
{This is now a simple hash list, not an ordered one, so
the module title is no longer descriptive.}
k:=hyph_word[h];
if length(k)<>length(s) then goto not_found;
u:=str_start_macro(k); v:=str_start_macro(s);
repeat if str_pool[u]<>str_pool[v] then goto not_found;
incr(u); incr(v);
until u=str_start_macro(k+1);
{repeat hyphenation exception; flushing old data}
flush_string; s:=hyph_word[h]; {avoid |slow_make_string|!}
decr(hyph_count);
{ We could also |flush_list(hyph_list[h]);|, but it interferes
  with \.{trip.log}. }
goto found;
not_found:
@z

@x
|hyf_next[@t$v^\prime$@>]=min_quarterword|.
@y
|hyf_next[@t$v^\prime$@>]=min_trie_op|.
@z

@x
$$\hbox{|@t$v^\prime$@>:=new_trie_op(0,1,min_quarterword)|,\qquad
@y
$$\hbox{|@t$v^\prime$@>:=new_trie_op(0,1,min_trie_op)|,\qquad
@z

@x
@!init@! trie_op_hash:array[-trie_op_size..trie_op_size] of 0..trie_op_size;
@y
@!init@! trie_op_hash:array[neg_trie_op_size..trie_op_size] of 0..trie_op_size;
@z

@x
@!trie_used:array[BMP_code] of quarterword;
@y
@!trie_used:array[BMP_code] of trie_opcode;
@z

@x
@!trie_op_val:array[1..trie_op_size] of quarterword;
@y
@!trie_op_val:array[1..trie_op_size] of trie_opcode;
@z

@x
tini
@y
tini@;
@!max_op_used:trie_opcode; {largest opcode used for any language}
@!small_op:boolean; {flag used while dumping or undumping}
@z

@x
|new_trie_op| could return |min_quarterword| (thereby simply ignoring
@y
|new_trie_op| could return |min_trie_op| (thereby simply ignoring
@z

@x
function new_trie_op(@!d,@!n:small_number;@!v:quarterword):quarterword;
label exit;
var h:-trie_op_size..trie_op_size; {trial hash location}
@!u:quarterword; {trial op code}
@y
function new_trie_op(@!d,@!n:small_number;@!v:trie_opcode):trie_opcode;
label exit;
var h:neg_trie_op_size..trie_op_size; {trial hash location}
@!u:trie_opcode; {trial op code}
@z

@x
begin h:=abs(n+313*d+361*v+1009*cur_lang) mod (trie_op_size+trie_op_size)
  - trie_op_size;
@y
begin h:=abs(intcast(n)+313*intcast(d)+361*intcast(v)+1009*intcast(cur_lang))
  mod (trie_op_size - neg_trie_op_size)
  + neg_trie_op_size;
@z

@x
    if u=max_quarterword then
      overflow("pattern memory ops per language",
        max_quarterword-min_quarterword);
    incr(trie_op_ptr); incr(u); trie_used[cur_lang]:=u;
@y
    if u=max_trie_op then
      overflow("pattern memory ops per language",
      max_trie_op-min_trie_op);
    incr(trie_op_ptr); incr(u); trie_used[cur_lang]:=u;
    if u>max_op_used then max_op_used:=u;
@z

@x
op_start[0]:=-min_quarterword;
@y
op_start[0]:=-min_trie_op;
@z

@x
for k:=0 to 255 do trie_used[k]:=min_quarterword;
@y
for k:=0 to 255 do trie_used[k]:=min_trie_op;
@z

@x
trie_op_ptr:=0;
@y
max_op_used:=min_trie_op;
trie_op_ptr:=0;
@z

@x
@!init @!trie_c:packed array[trie_pointer] of BMP_code;
  {characters to match}
@t\hskip10pt@>@!trie_o:packed array[trie_pointer] of quarterword;
  {operations to perform}
@t\hskip10pt@>@!trie_l:packed array[trie_pointer] of trie_pointer;
  {left subtrie links}
@t\hskip10pt@>@!trie_r:packed array[trie_pointer] of trie_pointer;
  {right subtrie links}
@t\hskip10pt@>@!trie_ptr:trie_pointer; {the number of nodes in the trie}
@t\hskip10pt@>@!trie_hash:packed array[trie_pointer] of trie_pointer;
  {used to identify equivalent subtries}
tini
@y
@!init @!trie_c:^BMP_code;
  {characters to match}
@t\hskip10pt@>@!trie_o:^trie_opcode;
  {operations to perform}
@t\hskip10pt@>@!trie_l:^trie_pointer;
  {left subtrie links}
@t\hskip10pt@>@!trie_r:^trie_pointer;
  {right subtrie links}
@t\hskip10pt@>@!trie_ptr:trie_pointer; {the number of nodes in the trie}
@t\hskip10pt@>@!trie_hash:^trie_pointer;
  {used to identify equivalent subtries}
tini
@z

@x
begin h:=abs(trie_c[p]+1009*trie_o[p]+@|
    2718*trie_l[p]+3142*trie_r[p]) mod trie_size;
@y
begin h:=abs(intcast(trie_c[p])+1009*intcast(trie_o[p])+@|
    2718*intcast(trie_l[p])+3142*intcast(trie_r[p])) mod trie_size;
@z

@x
@d trie_back(#)==trie[#].lh {backward links in |trie| holes}
@y
@d trie_back(#)==trie_tro[#] {use the opcode field now for backward links}
@z

@x
@!init@!trie_taken:packed array[1..trie_size] of boolean;
  {does a family start here?}
@t\hskip10pt@>@!trie_min:array[BMP_code] of trie_pointer;
  {the first possible slot for each character}
@t\hskip10pt@>@!trie_max:trie_pointer; {largest location used in |trie|}
@t\hskip10pt@>@!trie_not_ready:boolean; {is the trie still in linked form?}
tini
@y
@!init@!trie_taken: ^boolean;
  {does a family start here?}
@t\hskip10pt@>@!trie_min:array[BMP_code] of trie_pointer;
  {the first possible slot for each character}
@t\hskip10pt@>@!trie_max:trie_pointer; {largest location used in |trie|}
@t\hskip10pt@>@!trie_not_ready:boolean; {is the trie still in linked form?}
tini
@z

@x
trie_not_ready:=true; trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;
@y
trie_not_ready:=true;
@z

@x
@<Move the data into |trie|@>=
h.rh:=0; h.b0:=min_quarterword; h.b1:=min_quarterword; {|trie_link:=0|,
  |trie_op:=min_quarterword|, |trie_char:=qi(0)|}
@y
@d clear_trie == {clear |trie[r]|}
  begin trie_link(r):=0;
  trie_op(r):=min_trie_op;
  trie_char(r):=min_quarterword; {|trie_char:=qi(0)|}
  end

@<Move the data into |trie|@>=
@z

@x
  begin for r:=0 to 65536 do trie[r]:=h;
@y
  begin for r:=0 to 65536 do clear_trie;
@z

@x
  repeat s:=trie_link(r); trie[r]:=h; r:=s;
@y
  repeat s:=trie_link(r); clear_trie; r:=s;
@z

@x
@!v:quarterword; {trie op code}
@y
@!v:trie_opcode; {trie op code}
@z

@x
if trie_o[q]<>min_quarterword then
@y
if trie_o[q]<>min_trie_op then
@z

@x
trie_c[p]:=si(c); trie_o[p]:=min_quarterword;
@y
trie_c[p]:=si(c); trie_o[p]:=min_trie_op;
@z

@x
l:=k; v:=min_quarterword;
@y
l:=k; v:=min_trie_op;
@z

@x
@!h:two_halves; {template used to zero out |trie|'s holes}
@y
@z

@x
@<Append character |cur_chr|...@>=
adjust_space_factor;@/
@y
@<Append character |cur_chr|...@>=
if ((head=tail) and (mode>0)) then begin
  if (insertsrcspecialauto) then append_src_special;
end;
adjust_space_factor;@/
@z

@x
print("' in "); print_mode(mode);
@y
print_in_mode(mode);
@z

@x
if indented then begin
  p:=new_null_box; box_dir(p):=par_direction;
  width(p):=par_indent;@+
  tail_append(p);
@y
if indented then begin
  p:=new_null_box; box_dir(p):=par_direction;
  width(p):=par_indent;@+
  tail_append(p);
  if (insertsrcspecialeverypar) then insert_src_special;@+
@z

@x
begin print_err("Extra "); print_esc("endcsname");
@.Extra \\endcsname@>
help1("I'm ignoring this, since I wasn't doing a \csname.");
@y
begin
if cur_chr = 10 then
begin
  print_err("Extra "); print_esc("endmubyte");
@.Extra \\endmubyte@>
  help1("I'm ignoring this, since I wasn't doing a \mubyte.");
end else begin
  print_err("Extra "); print_esc("endcsname");
@.Extra \\endcsname@>
  help1("I'm ignoring this, since I wasn't doing a \csname.");
end;
@z

@x
if every_math<>null then begin_token_list(every_math,every_math_text);
@y
if (insertsrcspecialeverymath) then insert_src_special;
if every_math<>null then begin_token_list(every_math,every_math_text);
@z

@x
  if every_vbox<>null then begin_token_list(every_vbox,every_vbox_text);
@y
  if (insertsrcspecialeveryvbox) then insert_src_special;
  if every_vbox<>null then begin_token_list(every_vbox,every_vbox_text);
@z

@x
if (cur_cs=0)or(cur_cs>frozen_control_sequence) then
@y
if (cur_cs=0)or(cur_cs>eqtb_top)or
  ((cur_cs>frozen_control_sequence)and(cur_cs<=eqtb_size)) then
@z

@x
    begin @!init new_patterns; goto done;@;@+tini@/
@y
    begin @!Init new_patterns; goto done;@;@+Tini@/
@z

@x
@!flushable_string:str_number; {string not yet referenced}
@y
@z

@x
interaction:=cur_chr;
@y
interaction:=cur_chr;
if interaction = batch_mode
then kpsemaketexdiscarderrors := 1
else kpsemaketexdiscarderrors := 0;
@z

@x
slow_print(s); update_terminal;
@y
print(s); update_terminal;
@z

@x
begin print_err(""); slow_print(s);
@y
begin print_err(""); print(s);
@z

@x
format_ident:=" (INITEX)";
@y
if ini_version then format_ident:=" (INITEX)";
@z

@x
@!w: four_quarters; {four ASCII codes}
@y
@!format_engine: ^packed_ASCII_code;
@z

@x
@!w: four_quarters; {four ASCII codes}
@y
@!format_engine: ^packed_ASCII_code;
@z

@x
@d dump_wd(#)==begin fmt_file^:=#; put(fmt_file);@+end
@d dump_int(#)==begin fmt_file^.int:=#; put(fmt_file);@+end
@d dump_hh(#)==begin fmt_file^.hh:=#; put(fmt_file);@+end
@d dump_qqqq(#)==begin fmt_file^.qqqq:=#; put(fmt_file);@+end
@y
@z

@x
@d undump_wd(#)==begin get(fmt_file); #:=fmt_file^;@+end
@d undump_int(#)==begin get(fmt_file); #:=fmt_file^.int;@+end
@d undump_hh(#)==begin get(fmt_file); #:=fmt_file^.hh;@+end
@d undump_qqqq(#)==begin get(fmt_file); #:=fmt_file^.qqqq;@+end
@y
@z

@x
@d undump_size_end_end(#)==too_small(#)@+else undump_end_end
@y
@d format_debug_end(#)==
    writeln (stderr, ' = ', #);
  end;
@d format_debug(#)==
  if debug_format_file then begin
    write (stderr, 'fmtdebug:', #);
    format_debug_end
@d undump_size_end_end(#)==
  too_small(#)@+else format_debug (#)(x); undump_end_end
@z

@x
dump_int(@$);@/
@y
dump_int(@"57325458);  {Web2C \TeX's magic constant: "W2TX"}
{Align engine to 4 bytes with one or more trailing NUL}
x:=strlen(engine_name);
format_engine:=xmallocarray(packed_ASCII_code,x+4);
strcpy(stringcast(format_engine), stringcast(engine_name));
for k:=x to x+3 do format_engine[k]:=0;
x:=x+4-(x mod 4);
dump_int(x);dump_things(format_engine[0], x);
libcfree(format_engine);@/
dump_int(@$);@/
dump_int(max_halfword);@/
dump_int(hash_high);
@z

@x
dump_int(hyph_size)
@y
dump_int(hyph_prime)
@z

@x
x:=fmt_file^.int;
if x<>@$ then goto bad_fmt; {check that strings are the same}
@y
@+Init
libcfree(str_pool); libcfree(str_start);
libcfree(yhash); libcfree(zeqtb); libcfree(fixmem); libcfree(varmem);
@+Tini
undump_int(x);
format_debug('format magic number')(x);
if x<>@"57325458 then goto bad_fmt; {not a format file}
undump_int(x);
format_debug('engine name size')(x);
if (x<0) or (x>256) then goto bad_fmt; {corrupted format file}
format_engine:=xmallocarray(packed_ASCII_code, x);
undump_things(format_engine[0], x);
format_engine[x-1]:=0; {force string termination, just in case}
if strcmp(stringcast(engine_name), stringcast(format_engine)) then
  begin wake_up_terminal;
  wterm_ln('---! ', stringcast(nameoffile+1), ' was written by ', format_engine);
  libcfree(format_engine);
  goto bad_fmt;
end;
libcfree(format_engine);
undump_int(x);
format_debug('string pool checksum')(x);
if x<>@$ then begin {check that strings are the same}
  wake_up_terminal;
  wterm_ln('---! ', stringcast(nameoffile+1), ' was written by an older version');
  goto bad_fmt;
end;
undump_int(x);
if x<>max_halfword then goto bad_fmt; {check |max_halfword|}
undump_int(hash_high);
  if (hash_high<0)or(hash_high>sup_hash_extra) then goto bad_fmt;
  if hash_extra<hash_high then hash_extra:=hash_high;
  eqtb_top:=eqtb_size+hash_extra;
  if hash_extra=0 then hash_top:=undefined_control_sequence else
        hash_top:=eqtb_top;
  yhash:=xmallocarray(two_halves,1+hash_top-hash_offset);
  hash:=yhash - hash_offset;
  next(hash_base):=0; text(hash_base):=0;
  for x:=hash_base+1 to hash_top do hash[x]:=hash[hash_base];
  zeqtb:=xmallocarray (memory_word,eqtb_top+1);
  eqtb:=zeqtb;

  eq_type(undefined_control_sequence):=undefined_cs;
  equiv(undefined_control_sequence):=null;
  eq_level(undefined_control_sequence):=level_zero;
  for x:=eqtb_size+1 to eqtb_top do
    eqtb[x]:=eqtb[undefined_control_sequence];
@z

@x
if x<>hyph_size then goto bad_fmt
@y
if x<>hyph_prime then goto bad_fmt
@z

@x
dump_int(str_ptr);
for k:=string_offset to str_ptr do dump_int(str_start_macro(k));
k:=0;
while k+4<pool_ptr do
  begin dump_four_ASCII; k:=k+4;
  end;
k:=pool_ptr-4; dump_four_ASCII;
@y
dump_int((str_ptr-string_offset));
dump_things(str_start[0], (str_ptr-string_offset)+1);
dump_things(str_pool[0], pool_ptr);
@z

@x
undump_size(0)(pool_size)('string pool size')(pool_ptr);
undump_size(0)(max_strings)('max strings')(str_ptr);
for k:=string_offset to str_ptr do undump(0)(pool_ptr)(str_start_macro(k));
k:=0;
while k+4<pool_ptr do
  begin undump_four_ASCII; k:=k+4;
  end;
k:=pool_ptr-4; undump_four_ASCII;
@y
undump_size(0)(sup_pool_size-pool_free)('string pool size')(pool_ptr);
if pool_size<pool_ptr+pool_free then
  pool_size:=pool_ptr+pool_free;
undump_size(0)(sup_max_strings-strings_free)('sup strings')(str_ptr);@/
if max_strings<str_ptr+strings_free then
  max_strings:=str_ptr+strings_free;
str_start:=xmallocarray(pool_pointer, max_strings);
str_ptr:=str_ptr + string_offset;
undump_checked_things(0, pool_ptr, str_start[0], (str_ptr-string_offset)+1);@/
str_pool:=xmallocarray(packed_ASCII_code, pool_size);
undump_things(str_pool[0], pool_ptr);
@z

@x
for k:=fix_mem_min to fix_mem_end do dump_wd(mem(k));
@y
dump_things(mem(fix_mem_min), fix_mem_end-fix_mem_min+1);
@z

@x
for k:=fix_mem_min to fix_mem_end do undump_wd(mem(k));
@y
undump_things (mem(fix_mem_min), fix_mem_end-fix_mem_min+1);
@z

@x
undump(hash_base)(frozen_control_sequence)(par_loc);
par_token:=cs_token_flag+par_loc;@/
undump(hash_base)(frozen_control_sequence)(write_loc);@/
@y
undump(hash_base)(hash_top)(par_loc);
par_token:=cs_token_flag+par_loc;@/
undump(hash_base)(hash_top)(write_loc);@/
@z

@x
while k<l do
  begin dump_wd(eqtb[k]); incr(k);
  end;
@y
dump_things(eqtb[k], l-k);
@z

@x
while k<l do
  begin dump_wd(eqtb[k]); incr(k);
  end;
@y
dump_things(eqtb[k], l-k);
@z

@x
k:=j+1; dump_int(k-l);
until k>eqtb_size
@y
k:=j+1; dump_int(k-l);
until k>eqtb_size;
if hash_high>0 then dump_things(eqtb[eqtb_size+1],hash_high);
  {dump |hash_extra| part}
@z

@x
for j:=k to k+x-1 do undump_wd(eqtb[j]);
@y
undump_things(eqtb[k], x);
@z

@x
until k>eqtb_size
@y
until k>eqtb_size;
if hash_high>0 then undump_things(eqtb[eqtb_size+1],hash_high);
  {undump |hash_extra| part}
@z

@x
dump_int(hash_used); cs_count:=frozen_control_sequence-1-hash_used;
@y
dump_int(hash_used); cs_count:=frozen_control_sequence-1-hash_used+hash_high;
@z

@x
for p:=hash_used+1 to undefined_control_sequence-1 do dump_hh(hash[p]);
@y
dump_things(hash[hash_used+1], undefined_control_sequence-1-hash_used);
if hash_high>0 then dump_things(hash[eqtb_size+1], hash_high);
@z

@x
for p:=hash_used+1 to undefined_control_sequence-1 do undump_hh(hash[p]);
@y
undump_things (hash[hash_used+1], undefined_control_sequence-1-hash_used);
if debug_format_file then begin
  print_csnames (hash_base, undefined_control_sequence - 1);
end;
if hash_high > 0 then begin
  undump_things (hash[eqtb_size+1], hash_high);
  if debug_format_file then begin
    print_csnames (eqtb_size + 1, hash_high - (eqtb_size + 1));
  end;
end;
@z

@x
@ @<Undump the array info for internal font number |k|@>=
begin undump_font(k);@/
end
@y
@ This module should now be named `Undump all the font arrays'.

@d font_max==number_fonts

@<Undump the array info for internal font number |k|@>=
begin undump_font(k);@/
end;
{Allocate the font arrays}
pdf_font_size:=xmallocarray(scaled, font_max);
pdf_font_num:=xmallocarray(integer, font_max);
pdf_font_attr:=xmallocarray(str_number, font_max);
pdf_font_blink:=xmallocarray(internal_font_number, font_max);
pdf_font_elink:=xmallocarray(internal_font_number, font_max);
pdf_font_stretch:=xmallocarray(integer, font_max);
pdf_font_shrink:=xmallocarray(integer, font_max);
pdf_font_step:=xmallocarray(integer, font_max);
pdf_font_expand_ratio:=xmallocarray(integer, font_max);
pdf_font_auto_expand:=xmallocarray(boolean, font_max);
pdf_font_lp_base:=xmallocarray(integer, font_max);
pdf_font_rp_base:=xmallocarray(integer, font_max);
pdf_font_ef_base:=xmallocarray(integer, font_max);
pdf_font_kn_bs_base:=xmallocarray(integer, font_max);
pdf_font_st_bs_base:=xmallocarray(integer, font_max);
pdf_font_sh_bs_base:=xmallocarray(integer, font_max);
pdf_font_kn_bc_base:=xmallocarray(integer, font_max);
pdf_font_kn_ac_base:=xmallocarray(integer, font_max);

for font_k := font_base to font_max do begin
    pdf_font_size[font_k] := 0;
    pdf_font_num[font_k] := 0;
    pdf_font_attr[font_k] := "";
    pdf_font_blink[font_k] := null_font;
    pdf_font_elink[font_k] := null_font;
    pdf_font_stretch[font_k] := null_font;
    pdf_font_shrink[font_k] := null_font;
    pdf_font_step[font_k] := 0;
    pdf_font_expand_ratio[font_k] := 0;
    pdf_font_auto_expand[font_k] := false;
    pdf_font_lp_base[font_k] := 0;
    pdf_font_rp_base[font_k] := 0;
    pdf_font_ef_base[font_k] := 0;
    pdf_font_kn_bs_base[font_k] := 0;
    pdf_font_st_bs_base[font_k] := 0;
    pdf_font_sh_bs_base[font_k] := 0;
    pdf_font_kn_bc_base[font_k] := 0;
    pdf_font_kn_ac_base[font_k] := 0;
    end;
make_pdftex_banner
@z

@x
for k:=0 to active_max_ptr-1 do dump_wd(active_info[k]);
print_ln; print_int(active_max_ptr); print(" words of active ocps");

@ @<Undump the active ocp information@>=
undump_size(0)(active_mem_size)('active start point')(active_min_ptr);
undump_size(0)(active_mem_size)('active mem size')(active_max_ptr);
for k:=0 to active_max_ptr-1 do undump_wd(active_info[k]);
@y
if active_max_ptr>0 then
  dump_things(active_info[0], active_max_ptr);
print_ln; print_int(active_max_ptr); print(" words of active ocps");

@ @<Undump the active ocp information@>=
undump_size(0)(active_mem_size)('active start point')(active_min_ptr);
undump_size(0)(active_mem_size)('active mem size')(active_max_ptr);
if active_max_ptr>0 then
  undump_things(active_info[0], active_max_ptr);
@z

@x
@ @<Dump the ocp list information@>=
dump_int(ocp_listmem_ptr);
for k:=0 to ocp_listmem_ptr-1 do dump_wd(ocp_list_info[k]);
dump_int(ocp_list_ptr);
for k:=null_ocp_list to ocp_list_ptr do begin
  dump_int(ocp_list_list[k]);
  print_nl("\ocplist");
  print_esc(ocp_list_id_text(k));
  print_char("=");
  print_ocp_list(ocp_list_list[k]);
  end;
dump_int(ocp_lstackmem_ptr);
for k:=0 to ocp_lstackmem_ptr-1 do dump_wd(ocp_lstack_info[k])
@y
@ @<Dump the ocp list information@>=
dump_int(ocp_listmem_ptr);
dump_things(ocp_list_info[0], ocp_listmem_ptr);
dump_int(ocp_list_ptr);
dump_things(ocp_list_list[null_ocp_list], ocp_list_ptr+1-null_ocp_list);
for k:=null_ocp_list to ocp_list_ptr do begin
  print_nl("\ocplist");
  print_esc(ocp_list_id_text(k));
  print_char("=");
  print_ocp_list(ocp_list_list[k]);
  end;
dump_int(ocp_lstackmem_ptr);
dump_things(ocp_lstack_info[0], ocp_lstackmem_ptr)
@z

@x
@ @<Undump the ocp list information@>=
undump_size(1)(1000000)('ocp list mem size')(ocp_listmem_ptr);
for k:=0 to ocp_listmem_ptr-1 do undump_wd(ocp_list_info[k]);
undump_size(ocp_list_base)(ocp_list_biggest)('ocp list max')(ocp_list_ptr);
for k:=null_ocp_list to ocp_list_ptr do
  undump_int(ocp_list_list[k]);
undump_size(1)(1000000)('ocp lstack mem size')(ocp_lstackmem_ptr);
for k:=0 to ocp_lstackmem_ptr-1 do undump_wd(ocp_lstack_info[k])
@y
@ @<Undump the ocp list information@>=
undump_size(1)(1000000)('ocp list mem size')(ocp_listmem_ptr);
undump_things(ocp_list_info[0], ocp_listmem_ptr);
undump_size(0)(1000000)('ocp list max')(ocp_list_ptr);
undump_things(ocp_list_list[null_ocp_list], ocp_list_ptr+1-null_ocp_list);
undump_size(0)(1000000)('ocp lstack mem size')(ocp_lstackmem_ptr);
undump_things(ocp_lstack_info[0], ocp_lstackmem_ptr)
@z

@x
dump_int(hyph_count);
for k:=0 to hyph_size do if hyph_word[k]<>0 then
  begin dump_int(k); dump_int(hyph_word[k]); dump_int(hyph_list[k]);
  end;
@y
dump_int(hyph_count);
if hyph_next <= hyph_prime then hyph_next:=hyph_size;
dump_int(hyph_next);{minumum value of |hyphen_size| needed}
for k:=0 to hyph_size do if hyph_word[k]<>0 then
  begin dump_int(k+65536*hyph_link[k]);
        {assumes number of hyphen exceptions does not exceed 65535}
   dump_int(hyph_word[k]); dump_int(hyph_list[k]);
  end;
@z

@x
print_ln; print_int(hyph_count); print(" hyphenation exception");
if hyph_count<>1 then print_char("s");
@y
print_ln; print_int(hyph_count);
if hyph_count<>1 then print(" hyphenation exceptions")
else print(" hyphenation exception");
@z

@x
for k:=0 to trie_max do dump_hh(trie[k]);
dump_int(trie_op_ptr);
for k:=1 to trie_op_ptr do
  begin dump_int(hyf_distance[k]);
  dump_int(hyf_num[k]);
  dump_int(hyf_next[k]);
  end;
@y
dump_things(trie_trl[0], trie_max+1);
dump_things(trie_tro[0], trie_max+1);
dump_things(trie_trc[0], trie_max+1);
dump_int(trie_op_ptr);
if trie_op_ptr>0 then begin
  dump_things(hyf_distance[1], trie_op_ptr);
  dump_things(hyf_num[1], trie_op_ptr);
  dump_things(hyf_next[1], trie_op_ptr);
  end;
@z

@x
print(" has "); print_int(trie_op_ptr); print(" op");
if trie_op_ptr<>1 then print_char("s");
@y
print(" has "); print_int(trie_op_ptr);
if trie_op_ptr<>1 then print(" ops")
else print(" op");
@z

@x
undump(0)(hyph_size)(hyph_count);
for k:=1 to hyph_count do
  begin undump(0)(hyph_size)(j);
  undump(0)(str_ptr)(hyph_word[j]);
  undump(min_halfword)(max_halfword)(hyph_list[j]);
  end;
@y
undump_size(0)(hyph_size)('hyph_size')(hyph_count);
undump_size(hyph_prime)(hyph_size)('hyph_size')(hyph_next);
j:=0;
for k:=1 to hyph_count do
  begin undump_int(j); if j<0 then goto bad_fmt;
   if j>65535 then
   begin hyph_next:= j div 65536; j:=j - hyph_next * 65536; end
       else hyph_next:=0;
   if (j>=hyph_size)or(hyph_next>hyph_size) then goto bad_fmt;
   hyph_link[j]:=hyph_next;
  undump(0)(str_ptr)(hyph_word[j]);
  undump(min_halfword)(max_halfword)(hyph_list[j]);
  end;
  {|j| is now the largest occupied location in |hyph_word|}
  incr(j);
  if j<hyph_prime then j:=hyph_prime;
  hyph_next:=j;
  if hyph_next >= hyph_size then hyph_next:=hyph_prime else
  if hyph_next >= hyph_prime then incr(hyph_next);
@z

@x
for k:=0 to j do undump_hh(trie[k]);
undump_size(0)(trie_op_size)('trie op size')(j); @+init trie_op_ptr:=j;@+tini
for k:=1 to j do
  begin undump(0)(63)(hyf_distance[k]); {a |small_number|}
  undump(0)(63)(hyf_num[k]);
  undump(min_quarterword)(max_quarterword)(hyf_next[k]);
  end;
@y
{These first three haven't been allocated yet unless we're \.{INITEX};
 we do that precisely so we don't allocate more space than necessary.}
if not trie_trl then trie_trl:=xmallocarray(trie_pointer,j+1);
undump_things(trie_trl[0], j+1);
if not trie_tro then trie_tro:=xmallocarray(trie_pointer,j+1);
undump_things(trie_tro[0], j+1);
if not trie_trc then trie_trc:=xmallocarray(quarterword, j+1);
undump_things(trie_trc[0], j+1);
undump_size(0)(trie_op_size)('trie op size')(j); @+init trie_op_ptr:=j;@+tini
{I'm not sure we have such a strict limitation (64) on these values, so
 let's leave them unchecked.}
undump_things(hyf_distance[1], j);
undump_things(hyf_num[1], j);
undump_upper_check_things(max_trie_op, hyf_next[1], j);
@z

@x
undump(batch_mode)(error_stop_mode)(interaction);
@y
undump(batch_mode)(error_stop_mode)(interaction);
if interactionoption<>unspecified_mode then interaction:=interactionoption;
@z

@x
if (x<>69069)or eof(fmt_file) then goto bad_fmt
@y
if x<>69069 then goto bad_fmt
@z

@x
print(" (preloaded format="); print(job_name); print_char(" ");
@y
print(" (format="); print(job_name); print_char(" ");
@z

@x
@p begin @!{|start_here|}
@y
@d const_chk(#)==begin if # < inf_@&# then # := inf_@&# else
                         if # > sup_@&# then # := sup_@&# end

{|setup_bound_var| stuff duplicated in \.{mf.ch}.}
@d setup_bound_var(#)==bound_default:=#; setup_bound_var_end
@d setup_bound_var_end(#)==bound_name:=#; setup_bound_var_end_end
@d setup_bound_var_end_end(#)==if luainit>0 then begin
	get_lua_number('texconfig',bound_name,addressof(#));
	if #=0 then #:=bound_default;
    end
  else
    setupboundvariable(addressof(#), bound_name, bound_default);

@p procedure main_body;
begin @!{|start_here|}

{Bounds that may be set from the configuration file. We want the user to
 be able to specify the names with underscores, but \.{TANGLE} removes
 underscores, so we're stuck giving the names twice, once as a string,
 once as the identifier. How ugly.}

  setup_bound_var (100000)('pool_size')(pool_size);
  setup_bound_var (75000)('string_vacancies')(string_vacancies);
  setup_bound_var (5000)('pool_free')(pool_free); {min pool avail after fmt}
  setup_bound_var (15000)('max_strings')(max_strings);
  setup_bound_var (100)('strings_free')(strings_free);
  setup_bound_var (20000)('trie_size')(trie_size);
    {if |ssup_trie_size| increases, recompile}
  setup_bound_var (659)('hyph_size')(hyph_size);
  setup_bound_var (3000)('buf_size')(buf_size);
  setup_bound_var (50)('nest_size')(nest_size);
  setup_bound_var (15)('max_in_open')(max_in_open);
  setup_bound_var (60)('param_size')(param_size);
  setup_bound_var (4000)('save_size')(save_size);
  setup_bound_var (300)('stack_size')(stack_size);
  setup_bound_var (16384)('dvi_buf_size')(dvi_buf_size);
  setup_bound_var (79)('error_line')(error_line);
  setup_bound_var (50)('half_error_line')(half_error_line);
  setup_bound_var (79)('max_print_line')(max_print_line);
  setup_bound_var(1000)('ocp_list_size')(ocp_list_size);
  setup_bound_var(1000)('ocp_buf_size')(ocp_buf_size);
  setup_bound_var(1000)('ocp_stack_size')(ocp_stack_size);
  setup_bound_var (0)('hash_extra')(hash_extra);
  setup_bound_var (72)('pk_dpi')(pk_dpi);

  {Check other constants against their sup and inf.}
  const_chk (trie_size);
  const_chk (hyph_size);
  const_chk (buf_size);
  const_chk (nest_size);
  const_chk (max_in_open);
  const_chk (param_size);
  const_chk (save_size);
  const_chk (stack_size);
  const_chk (dvi_buf_size);
  const_chk (pool_size);
  const_chk (string_vacancies);
  const_chk (pool_free);
  const_chk (max_strings);
  const_chk (strings_free);
  const_chk (hash_extra);
  const_chk (obj_tab_size);
  const_chk (pdf_mem_size);
  const_chk (dest_names_size);
  const_chk (pk_dpi);
  if error_line > ssup_error_line then error_line := ssup_error_line;

  {array memory allocation}
  buffer:=xmallocarray (packed_ASCII_code, buf_size);
  nest:=xmallocarray (list_state_record, nest_size);
  save_stack:=xmallocarray (memory_word, save_size);
  input_stack:=xmallocarray (in_state_record, stack_size);
  input_file:=xmallocarray (alpha_file, max_in_open);
  input_file_callback_id:=xmallocarray (integer, max_in_open);
  line_stack:=xmallocarray (integer, max_in_open);
  eof_seen:=xmallocarray (boolean, max_in_open);
  grp_stack:=xmallocarray (save_pointer, max_in_open);
  if_stack:=xmallocarray (pointer, max_in_open);
  source_filename_stack:=xmallocarray (str_number, max_in_open);
  full_source_filename_stack:=xmallocarray (str_number, max_in_open);
  param_stack:=xmallocarray (halfword, param_size);
  dvi_buf:=xmallocarray (real_eight_bits, dvi_buf_size);
  hyph_word :=xmallocarray (str_number, hyph_size);
  hyph_list :=xmallocarray (halfword, hyph_size);
  hyph_link :=xmallocarray (hyph_pointer, hyph_size);
  ocp_list_info:=xmallocarray (memory_word, ocp_list_size);
  memset(ocp_list_info,0,sizeof(memory_word)* ocp_list_size);
  ocp_lstack_info:=xmallocarray (memory_word, ocp_list_size);
  memset(ocp_lstack_info,0,sizeof(memory_word)* ocp_list_size);
  ocp_list_list:=xmallocarray (ocp_list_index, ocp_list_size);
  otp_init_input_buf:=xmallocarray (quarterword, ocp_buf_size);
  otp_input_buf:=xmallocarray (quarterword, ocp_buf_size);
  otp_output_buf:=xmallocarray (quarterword, ocp_buf_size);
  otp_stack_buf:=xmallocarray (quarterword, ocp_stack_size);
  otp_calcs:=xmallocarray (halfword, ocp_stack_size);
  otp_states:=xmallocarray (halfword, ocp_stack_size);
  obj_tab:=xmallocarray (obj_entry, inf_obj_tab_size); {will grow dynamically}
  obj_offset(0):=0;
  pdf_mem:=xmallocarray (integer, inf_pdf_mem_size); {will grow dynamically}
  dest_names:=xmallocarray (dest_name_entry, inf_dest_names_size); {will grow dynamically}
  pdf_op_buf:=xmallocarray (real_eight_bits, pdf_op_buf_size);
  pdf_os_buf:=xmallocarray (real_eight_bits, inf_pdf_os_buf_size); {will grow dynamically}
  pdf_os_objnum:=xmallocarray (integer, pdf_os_max_objs);
  pdf_os_objoff:=xmallocarray (integer, pdf_os_max_objs);
@+Init
  fixmem:=xmallocarray (smemory_word, fix_mem_init+1);
  memset (voidcast(fixmem), 0, (fix_mem_init+1)*sizeof(smemory_word));
  fix_mem_min:=0;
  fix_mem_max:=fix_mem_init;

  eqtb_top := eqtb_size+hash_extra;
  if hash_extra=0 then hash_top:=undefined_control_sequence else
        hash_top:=eqtb_top;
  yhash:=xmallocarray (two_halves,1+hash_top-hash_offset);
  hash:=yhash - hash_offset;   {Some compilers require |hash_offset=0|}
  next(hash_base):=0; text(hash_base):=0;
  for hash_used:=hash_base+1 to hash_top do hash[hash_used]:=hash[hash_base];
  zeqtb:=xmallocarray (memory_word, eqtb_top);
  eqtb:=zeqtb;

  str_start:=xmallocarray (pool_pointer, max_strings);
  str_pool:=xmallocarray (packed_ASCII_code, pool_size);
@+Tini
@z

@x
@!init if not get_strings_started then goto final_end;
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr; fix_date_and_time;
tini@/
@y
@!Init if not get_strings_started then goto final_end;
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr; fix_date_and_time;
Tini@/
@z

@x
end_of_TEX: close_files_and_terminate;
final_end: ready_already:=0;
end.
@y
close_files_and_terminate;
final_end: do_final_end;
end {|main_body|};
@z

@x
  wterm(banner);
@y
  if srcspecialsp or filelineerrorstylep or parsefirstlinep then
    wterm(banner_k)
  else
    wterm(banner);
@z

@x
  if format_ident=0 then wterm_ln(' (no format preloaded)')
  else  begin slow_print(format_ident); print_ln;
    end;
@y
  wterm(versionstring);
  if format_ident>0 then slow_print(format_ident);
  print_ln;
  if shellenabledp then begin
    wterm_ln(' \write18 enabled.')
  end;
  if srcspecialsp then begin
    wterm_ln(' Source specials enabled.')
  end;
  if filelineerrorstylep then begin
    wterm_ln(' file:line:error style messages enabled.')
  end;
  if parsefirstlinep then begin
   wterm_ln(' %&-line parsing enabled.')
  end;
@z



@x
     slow_print(log_name); print_char(".");
@y
     print_file_name(0, log_name, 0); print_char(".");
@z

@x
  end;
@y
  end;
print_ln;
if (edit_name_start<>0) and (interaction>batch_mode) then
  calledit(str_pool,edit_name_start,edit_name_length,edit_line);
@z

@x
  wlog_ln(' ',cs_count:1,' multiletter control sequences out of ',
    hash_size:1);@/
@y
  wlog_ln(' ',cs_count:1,' multiletter control sequences out of ',
    hash_size:1, '+', hash_extra:1);@/
@z

@x
  begin @!init for i:=0 to biggest_used_mark do begin
@y
  begin @!Init for i:=0 to biggest_used_mark do begin
@z

@x
  store_fmt_file; return;@+tini@/
@y
  store_fmt_file; return;@+Tini@/
@z

@x
if (format_ident=0)or(buffer[loc]="&") then
@y
if (format_ident=0)or(buffer[loc]="&")or dump_line then
@z

@x
  w_close(fmt_file);
@y
  w_close(fmt_file);
  eqtb:=zeqtb;
@z

@x
fix_date_and_time;@/
@y
fix_date_and_time;@/

@!init
if trie_not_ready then begin {initex without format loaded}
  trie_trl:=xmallocarray (trie_pointer, trie_size);
  trie_tro:=xmallocarray (trie_pointer, trie_size);
  trie_trc:=xmallocarray (quarterword, trie_size);

  trie_c:=xmallocarray (BMP_code, trie_size);
  trie_o:=xmallocarray (trie_opcode, trie_size);
  trie_l:=xmallocarray (trie_pointer, trie_size);
  trie_r:=xmallocarray (trie_pointer, trie_size);
  trie_hash:=xmallocarray (trie_pointer, trie_size);
  trie_taken:=xmallocarray (boolean, trie_size);

  trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;
  hyph_root:=0; hyph_start:=0;

  {Allocate and initialize font arrays}
pdf_font_size:=xmallocarray(scaled,font_max);
pdf_font_num:=xmallocarray(integer,font_max);
pdf_font_attr:=xmallocarray(str_number,font_max);
pdf_font_blink:=xmallocarray(internal_font_number,font_max);
pdf_font_elink:=xmallocarray(internal_font_number,font_max);
pdf_font_stretch:=xmallocarray(integer,font_max);
pdf_font_shrink:=xmallocarray(integer,font_max);
pdf_font_step:=xmallocarray(integer,font_max);
pdf_font_expand_ratio:=xmallocarray(integer,font_max);
pdf_font_auto_expand:=xmallocarray(boolean,font_max);
pdf_font_lp_base:=xmallocarray(integer,font_max);
pdf_font_rp_base:=xmallocarray(integer,font_max);
pdf_font_ef_base:=xmallocarray(integer,font_max);
pdf_font_kn_bs_base:=xmallocarray(integer, font_max);
pdf_font_st_bs_base:=xmallocarray(integer, font_max);
pdf_font_sh_bs_base:=xmallocarray(integer, font_max);
pdf_font_kn_bc_base:=xmallocarray(integer, font_max);
pdf_font_kn_ac_base:=xmallocarray(integer, font_max);

for font_k := font_base to font_max do begin
    pdf_font_size[font_k] := 0;
    pdf_font_num[font_k] := 0;
    pdf_font_attr[font_k] := "";
    pdf_font_blink[font_k] := null_font;
    pdf_font_elink[font_k] := null_font;
    pdf_font_stretch[font_k] := null_font;
    pdf_font_shrink[font_k] := null_font;
    pdf_font_step[font_k] := 0;
    pdf_font_expand_ratio[font_k] := 0;
    pdf_font_auto_expand[font_k] := false;
    pdf_font_lp_base[font_k] := 0;
    pdf_font_rp_base[font_k] := 0;
    pdf_font_ef_base[font_k] := 0;
    pdf_font_kn_bs_base[font_k] := 0;
    pdf_font_st_bs_base[font_k] := 0;
    pdf_font_sh_bs_base[font_k] := 0;
    pdf_font_kn_bc_base[font_k] := 0;
    pdf_font_kn_ac_base[font_k] := 0;
end;
make_pdftex_banner;
end;
tini@/
@z

@x
    begin goto breakpoint;@\ {go to every label at least once}
    breakpoint: m:=0; @{'BREAKPOINT'@}@\
    end
@y
    abort {do something to cause a core dump}
@z

@x
primitive("special",extension,special_node);@/
@y
primitive("special",extension,special_node);@/
text(frozen_special):="special"; eqtb[frozen_special]:=eqtb[cur_val];@/
@z

@x
  else if cur_val>15 then cur_val:=16;
@y
  else if (cur_val>15) and (cur_val <> 18) then cur_val:=16;
@z

@x
begin @<Expand macros in the token list
@y
@!d:integer; {number of characters in incomplete current string}
@!clobbered:boolean; {system string is ok?}
begin @<Expand macros in the token list
@z

@x
if write_open[j] then selector:=j
@y
if j=18 then selector := new_string
else if write_open[j] then selector:=j
@z

@x
flush_list(def_ref); selector:=old_setting;
@y
flush_list(def_ref);
if j=18 then
  begin if (tracing_online<=0) then
    selector:=log_only  {Show what we're doing in the log file.}
  else selector:=term_and_log;  {Show what we're doing.}
  {If the log file isn't open yet, we can only send output to the terminal.
   Calling |open_log_file| from here seems to result in bad data in the log.}
  if not log_opened then selector:=term_only;
  print_nl("system(");
  for d:=0 to cur_length-1 do
    begin {|print| gives up if passed |str_ptr|, so do it by hand.}
    print(so(str_pool[str_start_macro(str_ptr)+d])); {N.B.: not |print_char|}
    end;
  print(")...");
  if shellenabledp then
    begin str_room(1); append_char(0); {Append a null byte to the expansion.}
    clobbered:=false;
    for d:=0 to cur_length-1 do {Convert to external character set.}
      begin str_pool[str_start_macro(str_ptr)+d]:=xchr[str_pool[str_start_macro(str_ptr)+d]];
      if (str_pool[str_start_macro(str_ptr)+d]=null_code)
         and (d<cur_length-1) then clobbered:=true;
        {minimal checking: NUL not allowed in argument string of |system|()}
      end;
    if clobbered then print("clobbered")
    else begin {We have the string; run system(3). We don't have anything
            reasonable to do with the return status, unfortunately discard it.}
      system(stringcast(addressof(str_pool[str_start_macro(str_ptr)])));
      print("executed");
      end;
    end
  else begin print("disabled");
  end;
  print_char("."); print_nl(""); print_ln;
  pool_ptr:=str_start_macro(str_ptr);  {erase the string}
end;
selector:=old_setting;
@z

@x
@!eTeX_mode: 0..1; {identifies compatibility and extended mode}
@y
@!eTeX_mode: 0..1; {identifies compatibility and extended mode}
@!etex_p: boolean; {was the -etex option specified}
@z

@x
@!eof_seen : array[1..max_in_open] of boolean; {has eof been seen?}
@y
@!eof_seen : ^boolean; {has eof been seen?}
@z

@x
@!grp_stack : array[0..max_in_open] of save_pointer; {initial |cur_boundary|}
@!if_stack : array[0..max_in_open] of pointer; {initial |cond_ptr|}
@y
@!grp_stack : ^save_pointer; {initial |cur_boundary|}
@!if_stack : ^pointer; {initial |cond_ptr|}
@z

@x
hyph_root:=0; hyph_start:=0;
@y
@z

@x
@!otp_init_input_buf:array[0..20000] of quarterword;

@!otp_input_start:halfword;
@!otp_input_last:halfword;
@!otp_input_end:halfword;
@!otp_input_buf:array[0..20000] of quarterword;

@!otp_output_end:halfword;
@!otp_output_buf:array[0..20000] of quarterword;

@!otp_stack_used:halfword;
@!otp_stack_last:halfword;
@!otp_stack_new:halfword;
@!otp_stack_buf:array[0..1000] of quarterword;

@!otp_pc:halfword;

@!otp_calc_ptr:halfword;
@!otp_calcs:array[0..1000] of halfword;
@!otp_state_ptr:halfword;
@!otp_states:array[0..1000] of halfword;
@y
@!otp_init_input_buf:^quarterword;

@!otp_input_start:halfword;
@!otp_input_last:halfword;
@!otp_input_end:halfword;
@!otp_input_buf:^quarterword;

@!otp_output_end:halfword;
@!otp_output_buf:^quarterword;

@!otp_stack_used:halfword;
@!otp_stack_last:halfword;
@!otp_stack_new:halfword;
@!otp_stack_buf:^quarterword;

@!otp_pc:halfword;

@!otp_calc_ptr:halfword;
@!otp_calcs:^halfword;
@!otp_state_ptr:halfword;
@!otp_states:^halfword;
@z

@x
@* \[54] System-dependent changes.
@y
@* \[54/web2c] System-dependent changes for Web2c.
Here are extra variables for Web2c.  (This numbering of the
system-dependent section allows easy integration of Web2c and e-\TeX, etc.)
@^<system dependencies@>

@<Glob...@>=
@!edit_name_start: pool_pointer; {where the filename to switch to starts}
@!edit_name_length,@!edit_line: integer; {what line to start editing at}
@!ipcon: cinttype; {level of IPC action, 0 for none [default]}
@!stop_at_space: boolean; {whether |more_name| returns false for space}

@ The |edit_name_start| will be set to point into |str_pool| somewhere after
its beginning if \TeX\ is supposed to switch to an editor on exit.

@<Set init...@>=
edit_name_start:=0;
stop_at_space:=true;

@ These are used when we regenerate the representation of the first 256
strings.

@<Global...@> =
@!save_str_ptr: str_number;
@!save_pool_ptr: pool_pointer;
@!shellenabledp: cinttype;
@!output_comment: ^char;
@!k,l: 0..255; {used by `Make the first 256 strings', etc.}

@ When debugging a macro package, it can be useful to see the exact
control sequence names in the format file.  For example, if ten new
csnames appear, it's nice to know what they are, to help pinpoint where
they came from.  (This isn't a truly ``basic'' printing procedure, but
that's a convenient module in which to put it.)

@<Basic printing procedures@> =
procedure print_csnames (hstart:integer; hfinish:integer);
var c,h:integer;
begin
  writeln(stderr, 'fmtdebug:csnames from ', hstart, ' to ', hfinish, ':');
  for h := hstart to hfinish do begin
    if text(h) > 0 then begin {if have anything at this position}
      for c := str_start_macro(text(h)) to str_start_macro(text(h) + 1) - 1
      do begin
        putbyte(str_pool[c], stderr); {print the characters}
      end;
      writeln(stderr, '|');
    end;
  end;
end;

@ Are we printing extra info as we read the format file?

@<Glob...@> =
@!debug_format_file: boolean;


@ A helper for printing file:line:error style messages.  Look for a
filename in |full_source_filename_stack|, and if we fail to find
one fall back on the non-file:line:error style.

@<Basic print...@>=
procedure print_file_line;
var level: 0..max_in_open;
begin
  level:=in_open;
  while (level>0) and (full_source_filename_stack[level]=0) do
    decr(level);
  if level=0 then
    print_nl("! ")
  else begin
    print_nl (""); print (full_source_filename_stack[level]); print (":");
    if level=in_open then print_int (line)
    else print_int (line_stack[index+1-(in_open-level)]);
    print (": ");
  end;
end;

@ To be able to determine whether \.{\\write18} is enabled from within
\TeX\ we also implement \.{\\eof18}.  We sort of cheat by having an
additional route |scan_four_bit_int_or_18| which is the same as
|scan_four_bit_int| except it also accepts the value 18.

@<Declare procedures that scan restricted classes of integers@>=
procedure scan_four_bit_int_or_18;
begin scan_int;
if (cur_val<0)or((cur_val>15)and(cur_val<>18)) then
  begin print_err("Bad number");
@.Bad number@>
  help2("Since I expected to read a number between 0 and 15,")@/
    ("I changed this one to zero."); int_error(cur_val); cur_val:=0;
  end;
end;

@* \[54/web2c-string] The string recycling routines.  \TeX{} uses 2
upto 4 {\it new\/} strings when scanning a filename in an \.{\\input},
\.{\\openin}, or \.{\\openout} operation.  These strings are normally
lost because the reference to them are not saved after finishing the
operation.  |search_string| searches through the string pool for the
given string and returns either 0 or the found string number.

@<Declare additional routines for string recycling@>=
function search_string(@!search:str_number):str_number;
label found;
var result: str_number;
@!s: str_number; {running index}
@!len: integer; {length of searched string}
begin result:=0; len:=length(search);
if len=0 then  {trivial case}
  begin result:=""; goto found;
  end
else  begin s:=search-1;  {start search with newest string below |s|; |search>1|!}
  while s>=string_offset do  {first |string_offset| strings depend on implementation!!}
    begin if length(s)=len then
      if str_eq_str(s,search) then
        begin result:=s; goto found;
        end;
    decr(s);
    end;
  end;
found:search_string:=result;
end;

@ The following routine is a variant of |make_string|.  It searches
the whole string pool for a string equal to the string currently built
and returns a found string.  Otherwise a new string is created and
returned.  Be cautious, you can not apply |flush_string| to a replaced
string!

@<Declare additional routines for string recycling@>=
function slow_make_string : str_number;
label exit;
var s: str_number; {result of |search_string|}
@!t: str_number; {new string}
begin t:=make_string; s:=search_string(t);
if s>0 then
  begin flush_string; slow_make_string:=s; return;
  end;
slow_make_string:=t;
exit:end;


@* \[54] System-dependent changes.
@z


@x
@* \[55] Index.
@y

@ @<Declare action procedures for use by |main_control|@>=

procedure insert_src_special;
var toklist, p, q,r : pointer;
begin
  if (source_filename_stack[in_open] > 0 and isnewsource (source_filename_stack[in_open]
, line)) then begin
    toklist := get_avail;
    p := toklist;
    info(p) := cs_token_flag+frozen_special;
    r := get_avail; link(p):=r; p := link(p);
    info(p) := left_brace_token+"{";
    q := str_toks (makesrcspecial (source_filename_stack[in_open], line));
    link(p) := link(temp_token_head);
    p := q;
    r := get_avail; link(p):=r; p := link(p);
    info(p) := right_brace_token+"}";
    ins_list (toklist);
    remembersourceinfo (source_filename_stack[in_open], line);
  end;
end;

procedure append_src_special;
var q : pointer;
begin
  if (source_filename_stack[in_open] > 0 and isnewsource (source_filename_stack[in_open]
, line)) then begin
    new_whatsit (special_node, write_node_size);
    write_stream(tail) := 0;
    q:=get_avail; def_ref := q;
    token_ref_count(def_ref) := null;
    q := str_toks (makesrcspecial (source_filename_stack[in_open], line));
    link(def_ref) := link(temp_head);
    write_tokens(tail) := def_ref;
    remembersourceinfo (source_filename_stack[in_open], line);
  end;
end;

@ This function used to be in pdftex, but is useful in tex too.

@p function get_nullstr: str_number;
begin
    get_nullstr := "";
end;

@* \[55] Index.
@z

