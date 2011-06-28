#! /usr/bin/perl


## dvtool2ambe: convert .dvtool files into .ambe files
## Usage: ambe2dvtool -o outfile.ambe infile.dvtool [infile.dvtool ... ]
## Usage: ambe2dvtool -V | --version

## This program is part of the "audio announce" package
## The goal is to use this packet to produce audio announcements on
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
## Version < 0.0.2 does not exist
## 22 okt. 2010: version 0.0.2. Initial release in perl
## 3  jan. 2011: version 0.1.2. parameters via "-" options, add support for text, add "version" option



use constant VERSION => "0.1.2";


# parse parameters
$numparam=0;
$fout="";


$tmp=shift;
while ($tmp ne "") {
	if ($tmp eq "-o") {
		# output filename
		$fout=shift;
	} elsif (($tmp eq "-V") || ($tmp eq "--version")) {
		# version
		print "dvtool2ambe version ",VERSION,"\n";
		exit;
	} else {
		# input filename
		$inparam[$numparam]=$tmp;
		$numparam++
	}; # end if

	$tmp=shift;
}; # end while



if ($fout eq "") {
	print STDERR "Error: missing output finename.\n";
	print STDERR "Usage: ambe2dvtool -o outfile.ambe infile.dvtool \[infile.dvtool ... \]\n";
	print STDERR "Usage: ambe2dvtool -V | --version\n";
	exit;
}; # end if

if ($numparam < 1) {
	print STDERR "Error: missing input finename.\n";
	print STDERR "Usage: ambe2dvtool -o outfile.ambe infile.dvtool \[infile.dvtool ... \]\n";
	print STDERR "Usage: ambe2dvtool -V | --version\n";
	exit;
}; # end if

# open files
open (FILEOUT,">$fout");

# write version-number to outfile
print FILEOUT "#C Version: 1.0\n";

$packetcount=0;
$packetcounttotal=1;
$filecount=0;

# time counters: first two fields of .ambe file
# seconds (5 digits), centiseconds (2 digits)
$sec=0;
$cs=0;

# $fin is the name of input filename

for ($fileloop=0;$fileloop<$numparam;$fileloop++) {
	$fin=$inparam[$fileloop];

	# open input file
	open (FILEIN,"<$fin");
	binmode FILEIN;

	$filecount++;

	# read file header (10 bytes, no used)
	read(FILEIN,$dummy,10);

	# read 2 bytes (lenght of next frame)
	read(FILEIN,$sizetxt,2);

	# not end of file?
	while ($sizetxt ne "") {
		($size,$dummy)=unpack("v",$sizetxt);

		# read rest of frame
		read(FILEIN,$packetin,$size);

		# .dvtool file format (packetin string data-structure)
		# octets 0 to 3: "DSVT"
		# octet 14 : sequence (wrapps around at 20, last frame has
		# 						sequence + 0x40)
		# octets 15 to 23: 9 bytes ambe data
		# octets 24 to 26: unused (slow speed data?)
		# note: linepacket includes the 2 bytes length and "packetin" does not
		# so for linepacket, always add 2

		# we are only interested in data-frames:
		# 5th based is 0x20 and length is 29 bytes
		# also discard "end"-frames (sequence > 0x40)

		if (($size == 27) && (ord(substr($packetin,4,1)) == 0x20 ) && 
			(ord(substr($packetin,14,1)) < 0x40)) {

			$outtext=sprintf("%05d %02d ",$sec,$cs);

			for ($loop=0;$loop<9;$loop++) {
				$outtext .= sprintf("%02X",ord(substr($packetin,15+$loop,1)));
			}; # end for
			print FILEOUT "$outtext\n";


			# before reading next frame, increase packetcount and times
			$packetcount++;
			$packetcounttotal++;
			if ($packetcount > 20) {
				$packetcount=0;
			}; # end if

			$cs+=2;
			if ($cs >= 100) {
				$sec++;
				$cs=0;
			}; # end if

		}; # end if

		# read next packet size
		read(FILEIN,$sizetxt,2);

	}; # end while

	# close input file
	close FILEIN;

}; # end while (filename input file)


close (FILEOUT);

print "done: $packetcounttotal frames written from $filecount input file(s). This includes 1 header-frame\n";

