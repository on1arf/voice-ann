#! /usr/bin/perl


## dvtooltext: modify or delete 20-bytes text-message inside a
## a .dvtool encoded AMBE-file
## Usage: dvtooltext {-t text , -d } -o outfile.dvtool infile.dvtool
## Usage: dvtooltext -V | --version

## This program is part of the "audio announce" package
## The goal is to use this packet to produce audio announcements on
## DSTAR repeaters 

## copyright (C) 2010 Kristoff Bonne ON1ARF
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published by
##   the Free Software Foundation; either version 2 of the License, or
##   (at your option) any later version.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##

##  release info:
## Version < 0.1.2 does not exist
## 3  jan. 2010: version 0.1.2. Initial release


use constant VERSION => "0.1.2";



# slow-speed data superframe: 63 octets (21 DV-frames with 3 octets per frame)
# The first frame contain the syncronisation packet: 0x55 0x2d 0x16
# After that, it either contains a 20-bytes text or filler
# Before sending, the D-STAR scrambler is applied to all data, except for the
# syncronisation frame
# see the "formats" text in the documentation-folder for more info

# slowspeeddatasuperframe_text: superframe containing a 20 bytes text
# This text is displayed on the screen of the handheld:
# 4 times 2 frames contain the broadcasting-text:
# 1 octet "sequence-number" (0x40 up to 0x43) + 5 bytes of text
# If the text is less then 20 characters, it is filled up with spaces
# the remainer of the superframe is fulled up with 0x66 (which becomes
# 0x16 0x29, 0xf5) after applying the scrambler

@ssdsf_text=(0x55,0x2d,0x16,0x40,0x20,0x20,0x20,0x20,0x20,0x41,0x20,0x20,0x20,0x20,0x20,0x42,0x20,0x20,0x20,0x20,0x20,0x43,0x20,0x20,0x20,0x20,0x20,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5);

# slowspeeddatasuperframe_filler: superframe containing all filler (0x66)
# which becomes {0x16 0x29 0xf5} after applying the scrambler
@ssdsf_filler=(0x55,0x2d,0x16,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5);



# parse parameters
$numparam=0;
$fout="";

$txtmsg="";
$txtpresent=0;
$txtdelete=0;

# framecounttotal starts at 1 as there always is one frame (the configuration frame)
$framecounttotal=1;


$tmp=shift;
while ($tmp ne "") {
	if ($tmp eq "-o") {
		# output filename
		$fout=shift;
	} elsif (($tmp eq "-V") || ($tmp eq "--version")) {
		# version
		print "dvtooltext version ",VERSION,"\n";
		exit;
	} elsif ($tmp eq "-d") {
		# option "-d" -> remove text-message
		$txtdelete=1;
	} elsif ($tmp eq "-t") {
		# option "-t" -> replace text-message
		$txtpresent=1;
		$txtmsg=shift;
	} else {
		# input filename
		$inparam[$numparam]=$tmp;
		$numparam++
	}; # end if

	$tmp=shift;
}; # end while


# case, text is empty, process this as a text-delete
if (($txtpresent == 1) && ($txtmsg eq "")) {
	$txtdelete=1;
	$txtpresent=0;
}; # end if


if ($fout eq "") {
	print STDERR "Error: missing output finename.\n";
	print STDERR "Usage: dvtooltext {-t text , -d } -o outfile.dvtool infile.dvtool \n";
	print STDERR "Usage: dvtooltext -V | --version \n";
	exit;
}; # end if

if ($numparam < 1) {
	print STDERR "Error: missing input finename.\n";
	print STDERR "Usage: dvtooltext {-t text , -d } -o outfile.dvtool infile.dvtool \n";
	print STDERR "Usage: dvtooltext -V | --version \n";
	exit;
}; # end if

if ($numparam > 1) {
	print STDERR "Warning: to many parameters given. Parameters after input-filename are ignored.\n";
}; # end if

if (($txtpresent == 1) && ($txtdelete == 1)) {
	print STDERR "Error: text-delete and text-replace are mutualy exclusive.\n";
	print STDERR "syntax: dvtooltext {-t text , -d } -o outfile.dvtool infile.dvtool \n";
	exit;
}; # end if

if (($txtpresent == 0) && ($txtdelete == 0)) {
	print STDERR "Error: no commands given\n";
	print STDERR "syntax: dvtooltext {-t text , -d } -o outfile.dvtool infile.dvtool \n";
	exit;
}; # end if


# create the superframe if necessairy
if ($txtpresent == 1) {
	$txtframecounter=0;

	$l=length($txtmsg);

	if ($l > 20) {
	# maximum length is 20 bytes
		$txtmsg=substr($txtmsg,0,20);
	} elsif (length($txtmsg) < 20)  {
	# add spaces is length is less then 20 bytes
		$spaces20="                    ";
		$txtmsg .= substr($spaces20,0,(20-$l));
	}; # end elsif - if

	@char=unpack("c20",$txtmsg);

	# copy text to slowspeeddata_superframe
	for ($loop1=0;$loop1<=3;$loop1++) {
		for ($loop2=0;$loop2<=4;$loop2++) {
			$ssdsf_text[4+$loop1*6+$loop2]=$char[$loop1*5+$loop2];
		}; # end for
	}; # end for loop1

	# apply scramber to octets 3 up to 26
	# this can be achieved by applying a exor on every set of 3 bytes
	# this taken from the programming-code from Scott KI4KLF and Jonathan G4KLX
	# I don't know why this works, but it does and that good enough for me. :-)
	# no scramber for octets 0 to 2 (syn-frames)
	for ($loop1=0;$loop1<8;$loop1++) {
		$ssdsf_text[3+$loop1*3] ^= 0x70;
		$ssdsf_text[4+$loop1*3] ^= 0x4f;
		$ssdsf_text[5+$loop1*3] ^= 0x93;
	}; # end for
}; # end if


# read imput file
# we only check two things before accepting the input-file:
# 1/ Does it exist
# 2/ Does it start with "DVTOOL"


# open files
# input filename is first parameter that is not an "option"

open (FILEIN,"<$inparam[0]");
binmode (FILEIN);

read (FILEIN,$frame,10);

if ((length($frame) < 10) || ((substr($frame,0,6)) ne "DVTOOL")) {
	print STDERR "Error: not a valid input file: $inparam[0].\n";
	exit;
}; # end if


# OK, we have a valid input file, copy it

# create output file
open (FILEOUT,">$fout");
binmode FILEOUT;

# write fileheader
print FILEOUT $frame;

# read and configuration frame
read (FILEIN,$frame,58);
print FILEOUT $frame;


# continue reading while length is 29 octets
read (FILEIN,$frame,29);

$txtlinecounter=0;

while (length($frame) == 29) {
	$framecounttotal++;

	# convert line in array of characters
	@c=unpack("C29",$frame);

	# OK, we have a dvtool frame, the slowspeed data text is in the last 3 bytes

	# the data in the slowspeed data frame is organised in superframes of 63 octets,
	# i.e. 21 times 3 octets

	# If there is a text to be displayed, it is placed in the first superframe of the
	# stream, followed by 9 superframes containing all "filler" data

	# If no text is to be displayed, the stream contains all "filler" superframes

	# case: text-delete or text-replace superframes 1 to 9 -> filler data
	if (($txtdelete == 1) || ($txtframecounter != 0)) {
		$c[26]=$ssdsf_filler[$txtlinecounter*3];
		$c[27]=$ssdsf_filler[$txtlinecounter*3+1];
		$c[28]=$ssdsf_filler[$txtlinecounter*3+2];
	} else {
	# case: text-replace superframe 0 -> text
		$c[26]=$ssdsf_text[$txtlinecounter*3];
		$c[27]=$ssdsf_text[$txtlinecounter*3+1];
		$c[28]=$ssdsf_text[$txtlinecounter*3+2];
	}; # end else - if

	# create new frame and write it to output file
	$newframe=pack("C29",@c);
	print FILEOUT $newframe;

	# increase $txtlinecounter and $txtframecounter if necessairy
	$txtlinecounter++;

	# wrap at 21
	if ($txtlinecounter >= 21) {
		$txtlinecounter=0;

		# increase $txtframecounter, wrap at 10
		$txtframecounter++;

		if ($txtframecounter >= 10) {
			$txtframecounter=0;
		}; # end if
	}; # end if

	# read next frame
read (FILEIN,$frame,29);
}; # end while


close (FILEIN);
close (FILEOUT);

print "done: $framecounttotal frames written. This includes 1 header-frame\n";

