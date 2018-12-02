# `m68kdis`

## Prelude
_This software was not originally made by me._ It was originally written in
1994 by one Christopher G. Phillips. AFAICT, it was originally submitted
to Dr. Dobb's Journal [in 1995](http://www.drdobbs.com/cug-new-releases/184403047?pgno=4).
Other than that and some sparse Usenet postings, I have no information on the
original author.

Unlike most non-commercial assemblers, `m68kdis` can automatically infer code
and data in raw binary images. It is also extremely portable being written in
only ANSI C. If that's not your cup of tea, perhaps bindings can be extracted
for interactive use cases? I might even do it at some point :).

After thinking I lost my original copy, and not finding the source on Github,
I decided to upload a fresh copy. On Windows 7, it compiles as-is if `gcc` is
installed, and probably works fine with MSVC as well. I personally downloaded
my copy from the [Raine emulator website](https://raine.1emulation.com/download/dev.html),
and commited a copy that was last lightly modified in 2002.

The manual, converted from `mandoc` into HTML (expect formatting errors),
follows below.

## Manual
<h1 align="center">M68KDIS</h1>

<a href="#NAME">NAME</a><br>
<a href="#SYNOPSIS">SYNOPSIS</a><br>
<a href="#DESCRIPTION">DESCRIPTION</a><br>
<a href="#OPTIONS">OPTIONS</a><br>
<a href="#NOTES">NOTES</a><br>
<a href="#BUGS">BUGS</a><br>
<a href="#SEE ALSO">SEE ALSO</a><br>
<a href="#AUTHOR">AUTHOR</a><br>

<hr>


<h2>NAME
<a name="NAME"></a>
</h2>


<p style="margin-left:11%; margin-top: 1em">m68kdis &minus;
disassemble Motorola 68000 family object code</p>

<h2>SYNOPSIS
<a name="SYNOPSIS"></a>
</h2>


<p style="margin-left:11%; margin-top: 1em">m68kdis
<b>[</b>&minus;<i>ddd</i><b>]
[</b>&minus;a&nbsp;<i>file</i><b>] [</b>&minus;all[c]<b>]
[</b>&minus;b&nbsp;<i>file</i><b>] [</b>&minus;bad<b>]
[</b>&minus;f&nbsp;<i>file</i><b>]
[</b>&minus;i&nbsp;<i>file</i><b>]
[</b>&minus;j&nbsp;<i>file</i><b>] [</b>&minus;l<b>]
[</b>&minus;lft<b>] [</b>&minus;n&nbsp;<i>file</i><b>]
[</b>&minus;ns&nbsp;<i>file</i><b>]
[</b>&minus;o&nbsp;<i>file</i><b>] [</b>&minus;odd<b>]
[</b>&minus;pc&nbsp;<i>initialpc</i><b>]
[</b>&minus;s&nbsp;<i>length</i><b>]
[</b>&minus;slenp&nbsp;<i>length</i><b>]
[</b>&minus;sp<b>]</b> <i>file...</i></p>

<h2>DESCRIPTION
<a name="DESCRIPTION"></a>
</h2>


<p style="margin-left:11%; margin-top: 1em"><i>m68kdis</i>
is a disassembler for the Motorola 68000 family of cpu
chips. The disassembler attempts to discern between the
instruction and data portions of an object code file.</p>

<p style="margin-left:11%; margin-top: 1em"><i>m68kdis</i>
produces output files much like <b>cc</b>(1). When the
<b>&minus;o</b> option is not used, file arguments that end
in .o have output filenames with .s substituted for .o;
otherwise, .s is simply appended.</p>

<p style="margin-left:11%; margin-top: 1em">The output
consists of five columns:</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>program counter in hexadecimal</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>file contents with each byte displayed as two characters
in hexadecimal</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>label (if any)</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>instruction name</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>operands (if any)</p></td></tr>
</table>

<h2>OPTIONS
<a name="OPTIONS"></a>
</h2>


<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="6%">


<p style="margin-top: 1em"><i>&minus;ddd</i></p></td>
<td width="5%"></td>
<td width="78%">


<p style="margin-top: 1em">Specifies the chip and
coprocessors. Valid values for <i>ddd</i> are currently
<i>000</i>, <i>008</i>, <i>010</i>, <i>020</i>, <i>030</i>,
<i>851</i>, <i>881</i>, and <i>882</i>. This option may be
repeated as appropriate. The default is <i>000</i> with no
coprocessors.</p> </td></tr>
</table>

<p style="margin-left:11%;">&minus;a&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies that <i>file</i>
contains lines of the form
&lsquo;&lsquo;<i>aXXX&nbsp;instruction&minus;string</i>&rsquo;&rsquo;
which specify acceptable A-line opcodes. <i>XXX</i> is in
hexadecimal.</p>

<p style="margin-left:11%;">&minus;all[c]</p>

<p style="margin-left:22%;">Specifies that only one pass
should be made, outputting the instruction (if any) at each
word boundary. If the <i>c</i> is included, <b>&minus;i</b>,
<b>&minus;j</b>, <b>&minus;n</b> and <b>&minus;ns</b>
options are also processed, and another pass is made to
ensure consistency between instructions.</p>

<p style="margin-left:11%;">&minus;b&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies that <i>file</i>
contains program counter values which are taken as locations
in data to be output on a new line. The values should be one
to a line, and of a form acceptable to <b>strtoul</b>() with
base equal to 0.</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="6%">


<p>&minus;bad</p></td>
<td width="5%"></td>
<td width="78%">


<p>Specifies that lines should be printed to standard error
that specify which data caused a potential instruction to be
made invalid.</p></td></tr>
</table>

<p style="margin-left:11%;">&minus;f&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies that <i>file</i>
contains lines of the form
&lsquo;&lsquo;<i>fXXX&nbsp;instruction&minus;string</i>&rsquo;&rsquo;
which specify acceptable F-line opcodes. <i>XXX</i> is in
hexadecimal.</p>

<p style="margin-left:11%;">&minus;i&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies that <i>file</i>
contains program counter values which are, if possible, to
be taken as locations of valid instructions. The values
should be one to a line, and of a form acceptable to
<b>strtoul</b>() with base equal to 0.</p>

<p style="margin-left:11%;">&minus;j&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies that <i>file</i>
contains A-line and F-line opcodes which are unconditional
jumps and therefore do not need to be followed by a valid
instruction. The values should be one to a line, and of a
form acceptable to <b>strtoul</b>() with base equal to
0.</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="6%">


<p>&minus;l</p></td>
<td width="5%"></td>
<td width="78%">


<p>Specifies that output should be in lower-case.
(Exception: Label references retain an upper-case
&lsquo;&lsquo;L&rsquo;&rsquo;.)</p> </td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="6%">


<p>&minus;lft</p></td>
<td width="5%"></td>
<td width="78%">


<p>Specifies that instructions that &lsquo;&lsquo;fall
through&rsquo;&rsquo; to a LINK instruction should be
considered valid. By default, these instructions are
considered invalid.</p></td></tr>
</table>

<p style="margin-left:11%;">&minus;n&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies that <i>file</i>
contains program counter values which are to be taken as
locations of data. The values should be one to a line, and
of a form acceptable to <b>strtoul</b>() with base equal to
0.</p>

<p style="margin-left:11%;">&minus;ns&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies that <i>file</i>
contains program counter values which are to be taken as
locations at which instructions do not begin. The words at
these locations may, however, be extension words of
instructions. The values should be one to a line, and of a
form acceptable to <b>strtoul</b>() with base equal to
0.</p>

<p style="margin-left:11%;">&minus;o&nbsp;<i>file</i></p>

<p style="margin-left:22%;">Specifies the output file. Only
one file to be disassembled may be given when this option is
used.</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="6%">


<p>&minus;odd</p></td>
<td width="5%"></td>
<td width="78%">


<p>Specifies that instructions may begin at odd offsets.
This can be useful when code to be disassembled is not
stripped out of an object file. In particular, this option
is often needed when disassembling an intact Macintosh
resource fork.</p></td></tr>
</table>


<p style="margin-left:11%;">&minus;pc&nbsp;<i>initialpc</i></p>

<p style="margin-left:22%;">Specifies that <i>initialpc</i>
be taken as the program counter value for the start of the
object code. The default is 0.</p>


<p style="margin-left:11%;">&minus;s&nbsp;<i>length</i></p>

<p style="margin-left:22%;">Specifies that data contain at
least <i>length</i> consecutive printable characters to be
output as a string. The minimum value for <i>length</i> is
2; the default is 5.</p>


<p style="margin-left:11%;">&minus;slenp&nbsp;<i>length</i></p>

<p style="margin-left:22%;">Specifies that strings should
print out no more than <i>length</i> characters per output
line. The minimum value for <i>length</i> is 10; the default
is 30.</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="4%">


<p>&minus;sp</p></td>
<td width="7%"></td>
<td width="78%">


<p>Specifies that register A7 should be output as SP,
except in MOVEM instructions.</p></td></tr>
</table>

<h2>NOTES
<a name="NOTES"></a>
</h2>


<p style="margin-left:11%; margin-top: 1em">The output is
based on Motorola syntax.</p>

<p style="margin-left:11%; margin-top: 1em">Immediate
values are sometimes also output in hexadecimal after an
intervening !.</p>

<p style="margin-left:11%; margin-top: 1em">The following
procedure is used to filter out the data from the
instructions:</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p style="margin-top: 1em">&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p style="margin-top: 1em">An initial pass is made
determining at which file offsets potential instructions
exist and the sizes of those instructions including
operands.</p> </td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>File offsets specified by the user as being data are
processed.</p> </td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>File offsets specified by the user as not starting
instructions are processed.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>File offsets specified by the user as being instructions
are processed.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>Potential instructions which reference data as
instructions are changed to data. (This step is repeated
after each of the remaining steps.)</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>LINK instructions which are referenced by BSR and JSR
instructions are accepted as final instructions. (A final
instruction is one that is included in the final
output.)</p> </td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>Remaining LINK instructions are accepted as final
instructions.</p> </td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>Branching and jumping instructions that reference final
instructions and are not potential extension words of
floating-point instructions are accepted as final
instructions.</p> </td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>Remaining branching and jumping that are not extension
words of potential floating-point instructions and returning
instructions are accepted as final instructions.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p>&bull;</p></td>
<td width="10%"></td>
<td width="78%">


<p>Overlapping instructions are selected by minimizing the
amount of data.</p></td></tr>
</table>

<h2>BUGS
<a name="BUGS"></a>
</h2>


<p style="margin-left:11%; margin-top: 1em">You will
probably need 16-bit <i>short</i>s and 32-bit
<i>int</i>s.</p>

<p style="margin-left:11%; margin-top: 1em">Since
<i>m68kdis</i> uses the imperfect procedure given in the
<b><small>NOTES</small></b> , errors may result in the
instruction/data determination. When problems are suspected,
the <b>&minus;bad</b><i>X</i> option can be used to
determine why instructions get interpreted as data. You can
then use the <b>&minus;i</b>, <b>&minus;ns</b>, and
<b>&minus;n</b> options, as appropriate.</p>

<p style="margin-left:11%; margin-top: 1em">Two unusual
conditions checked for should be mentioned. Sometimes the
decision to designate an instruction as a final instruction
is later contradicted and the instruction is changed to
data. In general, the instruction causing the contradiction
should be regarded as data via the <b>&minus;ns</b> option.
Also, sometimes it is reported that there is an
&lsquo;&lsquo;overlap&rsquo;&rsquo; at a certain offset.
This is because <i>m68kdis</i> is unsure if the best
selection of two possible instructions which overlap each
other was made. A quick inspection of the output at this
offset should clear this up. Messages for these conditions
are printed to standard error.</p>

<h2>SEE ALSO
<a name="SEE ALSO"></a>
</h2>


<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="3%">


<p style="margin-top: 1em">1.</p></td>
<td width="8%"></td>
<td width="78%">


<p style="margin-top: 1em">Motorola: <i>M68000 8/16/32 Bit
Microprocessors: Programmer&rsquo;s Reference Manual</i>,
5th ed., Prentice-Hall, Englewood Cliffs, NJ, 1986.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="3%">


<p>2.</p></td>
<td width="8%"></td>
<td width="78%">


<p>Motorola: <i>M68030: Enhanced 32-Bit Microprocessor
User&rsquo;s Manual</i>, 2nd ed., Prentice-Hall, Englewood
Cliffs, NJ, 1989.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="3%">


<p>3.</p></td>
<td width="8%"></td>
<td width="78%">


<p>Motorola: <i>M68851: Paged Memory Management Unit
User&rsquo;s Manual</i>, 2nd ed., Prentice-Hall, Englewood
Cliffs, NJ, 1989.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="3%">


<p>4.</p></td>
<td width="8%"></td>
<td width="78%">


<p>Motorola: <i>M68881/MC68882: Floating-Point Coprocessor
User&rsquo;s Manual</i>, 2nd ed., Prentice-Hall, Englewood
Cliffs, NJ, 1989.</p></td></tr>
</table>

<h2>AUTHOR
<a name="AUTHOR"></a>
</h2>


<p style="margin-left:11%; margin-top: 1em">Christopher G.
Phillips <br>
 Christopher_Phillips@pe.utexas.edu</p>
<hr>
