// cpdpl: copies a .dvtool-file to /dstar/tmp/play-<module>.dvtool

// this application is designed to help secure the D-STAR voice-
// announcement software package
// One of the ways a .dvtool-file can be played-out by a repeater
// is by using the Dplus package of Robin AA4RC
// This can be done by copying the .dvtool-file to 
// /dstar/tmp/play-<module>.dvtool

// However, that directory is owned by root and the file-priviledges
// is set up as such that non-root users cannot write in that
// directory
// Using this application and "sudo" with the no-pass option, it
// is still possible to allow the voice-announcement application to
// run as non-root and still be able to copy .dvtool files to
// that directory
// This is much more secure then -either- run the voice-announcement
// application as root, or to put "/bin/cp" in the /etc/sudoers file

// copyright (C) 2010 Kristoff Bonne ON1ARF
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// 22 nov. 2010: version 0.1.1. initial release
// 4  jan. 2011: version 0.1.2. add version number

#define VERSION "0.1.2"


// standard integers
#include <stdint.h>

// reading and writing files
#include <stdio.h>

// "exit" function
#include <stdlib.h>

// string functions
#include <string.h>

// buffersize = 128K
#define BUFFERSIZE 131072

int main (int argc, char ** argv) {

char module=0;
char buffer[BUFFERSIZE];
char * modulein;
int ret;

FILE * filein;
FILE * fileout;

char * outfile_name;

outfile_name=strdup("/dstar/tmp/play-X.dvtool");

if (argc < 3) {
	fprintf(stderr,"Error: at least 2 arguments needed.\n");
	fprintf(stderr,"Usage: %s module infile.wav\n",argv[0]);
	fprintf(stderr,"Info: version = %s\n",VERSION);
	exit(-1);
}; // end if

modulein=argv[1];

// check first character, if 'a' or 'A' -> module is 1
if ((modulein[0] == 'a') || (modulein[0] == 'A')) {
	module=0x61; // 'a' in ascii
}; // end if

// check first character, if 'b' or 'B' -> module is 2
if ((modulein[0] == 'b') || (modulein[0] == 'B')) {
	module=0x62; // 'b' in ascii
}; // end if

// check first character, if 'c' or 'C' -> module is 3
if ((modulein[0] == 'c') || (modulein[0] == 'C')) {
	module=0x63; // 'c' in ascii
}; // end if

if (module == 0) {
	fprintf(stderr,"Error: module must be 'a', 'b' or 'c'\n");
	fprintf(stderr,"usage: %s infile.dvtool repeater-name module\n",argv[0]);
	exit(-1);
}; // end if



// try to open input file
filein=fopen(argv[2],"rb");

if (filein == NULL) {
	fprintf(stderr,"Error: cannot open input file\n");
	exit(-1);
}; // end if


// try to open output file

// replace "X" in output file name with repeater
outfile_name[16]=module;

fileout=fopen(outfile_name,"wb");

if (fileout == NULL) {
	fprintf(stderr,"Error: cannot open output file %s\n",outfile_name);
	exit(-1);
}; // end if

ret=fread(buffer,1,BUFFERSIZE,filein);

while (ret > 0) {
	fwrite(buffer,1,ret,fileout);

	ret=fread(buffer,1,BUFFERSIZE,filein);
}; // end while

fclose(filein);
fclose(fileout);

exit(0);
}; // end if
