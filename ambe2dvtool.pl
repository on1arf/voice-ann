#! /usr/bin/perl



## ambe2dvtool: converts ambe-files to DSTAR .dvtool files
## Usage: ambe2dvtool \[-t txtmessage \] -o outfile.dvtool infile.ambe \[infile.ambe ... \]
## Usage: ambe2dvtool -V | --version

## The goal is to use this program to produce audio announcements on
## DSTAR repeaters by stitching pre-encoded AMBE files together

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
## 17 okt. 2010: version 0.0.1. Initial release in C
## 22 okt. 2010: version 0.0.2. Initial release in perl
## 3  jan. 2011: version 0.1.2. parameters via "-" options, add support for text, add "version" option

use constant VERSION => "0.1.2";

# datastructures:
# header file of .dvtool file: consists of file-header (10 bytes: "DVTOOL"
# + 4 bytes length) + 1 configuration-frame (58 bytes)
# note that large parts of the .dvtool file is overwritten when being
# played out, so much of these bytes are irrelevant
# see "readme_formats.txt" in docs-directory for more information on the exact
# format of the header

@header = (0x44,0x56,0x54,0x4f,0x4f,0x4c,0x00,0x00,0x00,0x00,0x38,0x00,0x44,0x53,0x56,0x54,0x10,0x00,0x81,0x00,0x20,0x00,0x01,0x02,0xe8,0x4f,0x80,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xff,0xff);

# dataline of .dvtool file.
# important fields:
# octets 0 to 1: length (LSB)
# octets 2 to 5: "DVT "
# octet 17: line counter (goes from 0 to 20)
# octets 18 to 26: ambe voice data
# octets 27 to 29: slow-speed data

@line = (0x1b,0x00,0x44,0x53,0x56,0x54,0x20,0x00,0x81,0x00,0x20,0x00,0x01,0x02,0xe8,0x4f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x2d,0x16);


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


# 1st up to last-1 parameters: input filename (.ambe files)
# last parameters: output file (.dvtool)
# option -t: include a text-message in the .dvtool file

$txtmsg="";
$txtmsgincluded=0;

# read parameters and store them in array
$numparam=0;
$fout="";
$tmp=shift;
while ($tmp ne "") {

	if ($tmp eq '-t') {
		# ("text" option)

		$txtmsgincluded=1;
		# text-message is next parameter
		$txtmsg=shift;
		
	} elsif (($tmp eq "-V") || ($tmp eq "--version") ) {
		# version
		print "ambe2dvtool version ",VERSION,"\n";
		exit;


	} elsif ($tmp eq "-o") {
		# output filename

		$fout=shift;

	} else {
		# normal option: filename

		$inparam[$numparam]=$tmp;
		$numparam++;
	}; # end else - if

	$tmp=shift;
}; # end while


if ($fout eq "") {
	print STDERR "Error: missing output filename.\n";
	print STDERR "Usage: ambe2dvtool \[-t txtmessage \] -o outfile.dvtool infile.ambe \[infile.ambe ... \] \n";
	print STDERR "Usage: ambe2dvtool -V | --version \n";
	exit;
}; # end if

if ($numparam < 1) {
	print STDERR "Error: missing input filename.\n";
	print STDERR "Usage: ambe2dvtool \[-t txtmessage \] -o outfile.dvtool infile.ambe \[infile.ambe ... \] \n";
	print STDERR "Usage: ambe2dvtool -V | --version \n";
	exit;
}; # end if


# create the superframe if necessairy
if ($txtmsgincluded == 1) {
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


# open files
open (FILEOUT,">$fout");

binmode FILEOUT;

# write header to outfile
# header = 68 "char"
$headerpacket=pack("C68",@header);
print FILEOUT $headerpacket;

# init some vars
$packetcount=0;
$packetcounttotal=1;
$filecount=0;

$txtlinecounter=0;
	

for ($fileloop=0;$fileloop<$numparam;$fileloop++) {
# $fin is the name of input filename
	$fin=$inparam[$fileloop];

	# open input file
	open (FILEIN,"<$fin");
	binmode FILEIN;

	$filecount++;

	# read file, line per line
	inline: while (<FILEIN>) {

		# remove comment-lines (everything beginning with '#')
		if ($_ =~ /^#/) {
			next inline;
		} # end if;

		# split line in words: ambe-data in in hexstring (3th word)
		($part1,$part2,$hexstring,$dummy)=split(" ",$_,4);

		# convert string of hex-characters to characters:
		# shamelessly stolen from the internet: see
		# http://icfun.blogspot.com/2009/05/perl-convert-hex-string-into-character.html
		$hexstring =~ s/([a-fA-F0-9][a-fA-F0-9])/chr(hex($1))/eg;

		# insert 9 bytes of hexstring into line
		for ($loop=0;$loop<9;$loop++) {
			$line[$loop+17]=ord(substr($hexstring,$loop,1));
		};  # end for


		# 16th byte of line is line-counter
		$line[16]=$packetcount;


		# slow speed data: octet 26 to 28 of line (counting from 0)
		# only when text included and for every 10th superframe
		if (($txtmsgincluded == 1) && ($txtframecounter == 0)) {
			# a frame with slow-speed data
			$line[26]=$ssdsf_text[$txtlinecounter*3];
			$line[27]=$ssdsf_text[$txtlinecounter*3+1];
			$line[28]=$ssdsf_text[$txtlinecounter*3+2];

		} else {
			# a frame with filler data
			$line[26]=$ssdsf_filler[$txtlinecounter*3];
			$line[27]=$ssdsf_filler[$txtlinecounter*3+1];
			$line[28]=$ssdsf_filler[$txtlinecounter*3+2];
		}; # end if

		# increase txtlinecounter and txtframecounter if necessairy
		$txtlinecounter++;

		# wrap at 21
		if ($txtlinecounter >= 21) {
			$txtlinecounter=0;

			# increase txtframecounter, wrap at 10
			$txtframecounter++;

			if ($txtframecounter >= 10) {
				$txtframecounter=0;
			}; # end if
		}; # end if


		# save packet to file
		$linepacket=pack("C29",@line);
		print FILEOUT $linepacket;


		# before reading next frame, increase packetcount
		$packetcount++;
		$packetcounttotal++;

		if ($packetcount > 20) {
			$packetcount=0;
		}; # end if
	}; # end while

	# close input file
	close FILEIN;

}; # end for (filename input file)


# all input files read: set "end of stream" marker by adding 0x40 to
# frame-counter (octet 16)

# rewind 13 octets to packet-counter of last frame
$ret=seek(FILEOUT,-13,1);

# correct framecounter: add 0x40 - 1 (as one has already been added before)
$packetcount += 0x3f;

if ($packetcount == 0x3f) {
	# correct special case where there has just been a "wrap" of the frame-counter
	# value used to be 0x14, so it must become 0x54 now
	$packetcount=0x54;
}; # end end

# save packetcount to file
print FILEOUT chr($packetcount);



# now rewind to beginning of file
# write number-of-frames counter (octets 6 to 9)
seek(FILEOUT,6,SEEK_SET);

# correct packetcounttotal
$packetcounttotal--;

$linepacket=pack("N",($packetcounttotal));
print FILEOUT $linepacket;

close (FILEOUT);

print "done: $packetcounttotal frames writen from $filecount input file(s). This includes 1 header-frame\n";

