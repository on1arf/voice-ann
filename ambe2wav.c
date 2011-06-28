/* ambe2wave.c */

// ambe2wav: convert ambe-files to .wav-files
// Usage: ambe2wav [-dngl device] [-v ] [-g gain] -o outfile.wav infile.dvtool [ infile.dvtool ... ]

// this program requires a DVdongle to do AMBE decoding, connected to
// a USB-port
// this program requires the sndfile-libraries to create wav-files
// see http://www.mega-nerd.com/libsndfile/

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

// release info:
// release < 0.1.2 does not exist
// 3  jan. 2011: version 0.1.2. Initial release
// 27 Mar. 2011: version 0.2.1: CLI options in line with other applications and code-cleanup

#define VERSION "0.2.1"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

// sndfile: read and write wav-files
#include <sndfile.h>

// term io: serial port
#include <termios.h>

// posix threads + inter-process control
#include <pthread.h>
#include <signal.h>

// posix interupt timers
#include <time.h>


// AMBE2WAV and WAV2AMBE: used to do selective programming in serialsend.h and
// serialreceive.h (which are shared by wav2ambe and ambe2wav)
#define AMBE2WAV 1


// for flock
#include <sys/file.h>

// errno
#include <errno.h>

// change this to 1 for debugging
#define DEBUG 0


// local include-files
// readambefile.h: contains function with that name
#include "readambefile.h"

// wav2ambe.h: data structures + global data
// As wav2ambe is multithreaded, global data-structures are used
// by the threads to communicate with each other
// wav2ambe is shared by wav2ambe and ambe2wav
#include "wav2ambe.h"

// serialsend.h: contains the function with that name
// Is started every 20 ms to send data to dongle
// serialsend.h is shared by wav2ambe and ambe2wav
#include "serialsend.h"

// serialreceive.h: contains the function with that name
// Is started as a seperate thread by the main function
// serialreceive.h is shared by wav2ambe and ambe2wav
#include "serialreceive.h"

// usageandhelp.h: as its name implies: usage and help
#include "usageandhelp.h"


// defines for timed interrupts
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN


//////////////////////////////////////////////////////////////////////////
// configuration part // // configuration part // // configuration part //
//////////////////////////////////////////////////////////////////////////
char * defaultdongledevice="/dev/ttyUSB0";
char * dongledevice;

// end configuration part. Do not change below unless you
// know what you are doing


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Main program // Main program // Main program // Main program //////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


int main(int argc,char** argv) {

// data structures:

// termios: structure to configure serial port
struct termios tio;

// in-buffer for serial port: packets can be up to 8192 bytes
unsigned char serialinbuff[8192];


FILE * infile;

int ambegain;

			
// posix threads
pthread_t thr_serialreceive;


// vars for timed interrupts
struct sigaction sa;
struct sigevent sev;
timer_t timerid;
struct itimerspec its;

// some miscellaneous vars:
int status;
int stop;
int ret;
int framecount;
int filecount;

int numinputfile;

unsigned char dvtool_framein[29];

// loops
int paramloop;
int fileloop;


// fileformat (textual description)
char * fileformatdescr[3]={"","dvtool","ambe"};

// we can encode up to "MAXINFILE" files at one
char * infilelist[MAXINFILE];

// ////// data definition done ///


// main program starts here ///
// part 1: initialise vars and check cli-arguments

// Usage: ambe2wav [-dngl device] [-v ] [-g gain] -o outfile.wav infile.dvtool [ infile.ambe ... ]
// Usage: ambe2wav -V
// Usage: ambe2wav -h

global.verboselevel=0;
dongledevice=defaultdongledevice;
global.fname_out=NULL;
numinputfile=0;
ambegain=192;

// start of program //
for (paramloop=1;paramloop<argc;paramloop++) {
	char * thisarg=argv[paramloop];

	if (strcmp(thisarg,"-dngl") == 0) {
		// -dngl = dongledevice
		if (paramloop+1 < argc) {
			paramloop++;
			dongledevice=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-V") == 0) {
		// -V = version
			fprintf(stderr,"ambe2wav version %s\n",VERSION);
			exit(0);
	} else if (strcmp(thisarg,"-h") == 0) {
		// -h = help
			help(argv[0]);
			exit(0);
	} else if (strcmp(thisarg,"-g") == 0) {
	// -g = ambe gain
		if (paramloop+1 < argc) {
			paramloop++;
			ambegain=atoi(argv[paramloop]);
		}; // end if
	} else if (strcmp(thisarg,"-v") == 0) {
		// -v = verbose
		global.verboselevel++;
	} else if (strcmp(thisarg,"-o") == 0) {
		// -o = output file
		if (paramloop+1 < argc) {
			paramloop++;
			global.fname_out=argv[paramloop];
		}; // end if
	} else {
		// argument without option is input filename
		infilelist[numinputfile]=thisarg;

		if (numinputfile < MAXINFILE - 1) {
			numinputfile++;
		}; // end if
	}; // end else - elsif - elsif - elsif - elsif - if

}; // end for


// error conditions:
// input filename not given
if (numinputfile ==  0) {
	fprintf(stderr,"Error: Missing input file name!\n");
	usage(argv[0]);
	exit(-1);
}; // end if

// output filename not given
if (global.fname_out ==  NULL) {
	fprintf(stderr,"Error: Missing output file name!\n");
	usage(argv[0]);
	exit(-1);
}; // end if


// output AMBE gain must be between 1 and 255
if ((ambegain  <= 1)  || (ambegain > 255)) {
	fprintf(stderr,"Error: AMBEgain must be between 1 and 255 (default = 192)!\n");
	usage(argv[0]);
	exit(-1);
}; // end if



// initialise pcmbuffer: always 320 bytes (20 ms, 8000 sps, 16 bit/sample)
// buffsize (0x8142) actualy contains 2 data-items:
// - first 5 bits: "ptype" (here: 10000)
// - remaining 13 bites: length: 0x142 = 322 (320 databytes + 2 header bytes)
pcmbuffer.buffsize1=0x42;
pcmbuffer.buffsize2=0x81;
memset(&pcmbuffer.pcm,0,320);

// empty pcm-buffer: the same as above. But will never change
empty_pcmbuffer.buffsize1=0x42;
empty_pcmbuffer.buffsize2=0x81;
memset(&empty_pcmbuffer.pcm,0,320);




// open serial port to dongle
global.donglefd=open(dongledevice, O_RDWR | O_NONBLOCK);

if (global.donglefd == 0) {
	fprintf(stderr,"open failed! \n");
	exit(-1);
}; // end if

// make exclusive lock
ret=flock(global.donglefd,LOCK_EX | LOCK_NB);

if (ret != 0) {
        int thiserror=errno;

        fprintf(stderr,"Exclusive lock to DVdongle failed. (%s) \n",strerror(thiserror));
        fprintf(stderr,"Dongle device not found or already in use\n");
        exit(-1);
}; // end if


// now configure serial port
// Serial port programming for Linux, see termios.h for more info
memset(&tio,0,sizeof(tio));
tio.c_iflag=0; tio.c_oflag=0; tio.c_cflag=CS8|CREAD|CLOCAL; // 8n1
tio.c_lflag=0; tio.c_cc[VMIN]=1; tio.c_cc[VTIME]=0;

cfsetospeed(&tio,B230400); // 230 Kbps
cfsetispeed(&tio,B230400); // 230 Kbps
tcsetattr(global.donglefd,TCSANOW,&tio);


// overwrite AMBE gain in ambe frame
ambe_configframe[25]=(unsigned char) ambegain;
// copy empty "configuration frame" onto ambe frame
memcpy(&ambe_dataframe,&ambe_configframe,50);


// First part of the program: initialise the dongle
// This runs pure sequencial

// main loop:
// senario:
// status 0: send "give-name" command to dongle
// status 1: wait for name -> send "start" command
// status 2: wait for confirmation of "start" -> send ambe-packet (configures decoder) + dummy pcm-frame

// init some vars
status=0;

if (global.verboselevel >= 1) {
	fprintf(stderr,"Initialising DVdongle\n");
};


while (status < 3) {
// some local vars

	uint16_t plen_i; 
	uint16_t ptype;

	if (global.verboselevel >= 1) {
		fprintf(stderr,"DVdongle status = %d\n",status);
	}

	// read next frame, no error-correction, 2 ms of delay if no data
	// timeout is 4 seconds (2 ms * 2000) in status 3 (after sending
	// first command to dongle) or 1 (2 ms * 500) seconds otherwize
	if (status == 1) {
		ret=serialframereceive(global.donglefd,serialinbuff,&ptype,&plen_i,0,2000,2000);
	} else {
		ret=serialframereceive(global.donglefd,serialinbuff,&ptype,&plen_i,0,2000,500);
	}; // end else - if


	// part 1: what if no data received ???	
	if (ret < 0) {
	// nothing received.

		// check for status
		if (status == 1) {
			// 4 second timeout after first command to dongle.
			// are you sure there is a dongle present?
			fprintf(stderr,"Error: timeout receiving data from dongle.! Exiting\n");
			exit(-1);
		}; // end if


		if (status == 0) {
		// status is 0 (after 1 second of waiting, send "request name")
			ret=write (global.donglefd,request_name,4);
			// move to status 1
			status=1;
		}; // end if

	} else {
		// part2: data is received

		// parse received packet:
		// case: status 2 in senario
		// reply to start received -> send initial ambe-packet
		if ((status == 2) && (plen_i == 3) && (ptype == 0)) {
			// also send a dummy pcm-frame to keep de encoder busy
			ret=write (global.donglefd,(void *) &pcmbuffer,322);

			ret=write (global.donglefd,ambe_configframe,50);

			// move to status 3
			status=3;
		}; // end if

		// case: status 1 in senario:
		// dongle name received -> send "start" command
		if ((status == 1) && (plen_i == 12) && (ptype == 0) && (serialinbuff[0] == 1) && (serialinbuff[1] == 0)) {
			ret=write (global.donglefd,command_start,5);
			// move to status 2
			status=2;
		}; // end if


	}; // end else - if (data received or not?)

}; // end while (status < 3)

if (global.verboselevel >= 1) {
	fprintf(stderr,"DVdongle initialising done. Starting encoding files.\n");
};

// Part 2 of main program

// now we are at status level 3. We have just received the acknowledge
// to the "start" and have send the 1st ambe-packet to configure
// the encoder

// now we do three things:
// - the main process reads ambe-frames of the .dvtool or .ambe-file, and
// makes them available for the "send data" thread
// - the "send data" function is started via a interrupt-time every
// 20 ms, reads the data prepared by the main thread and sends it
// out to the dvdongle
// - we start the "serialreceive" thread that receives the frames from
// the dvdongle and saves the pcm-frames it to a PCM-file
// in addition to this, it also send a dummy PCM-frame when receiving
// a ambe-frame from the dongle

// init and start thread serialreceive
// copy fileformat parameter from global configuration
pthread_create(&thr_serialreceive,NULL,funct_serialreceive, (void *) &global);

// init vars for serialsend function
global.datatosend=0;
global.datasendready=1;

// establing handler for signal 
sa.sa_flags = 0;
sa.sa_handler = funct_serialsend;
sigemptyset(&sa.sa_mask);

ret=sigaction(SIG, &sa, NULL);
if (ret <0) {
	fprintf(stderr,"error in sigaction!\n");
	exit(-1);
}; // end if


/* Create the timer */
sev.sigev_notify = SIGEV_SIGNAL;
sev.sigev_signo = SIG;
sev.sigev_value.sival_ptr = &timerid;

ret=timer_create(CLOCKID, &sev, &timerid);
if (ret < 0) {
	fprintf(stderr,"error in timer_create!\n");
	exit(-1);
}; // end if


// start timed function, every 20 ms
its.it_value.tv_sec = 0;
its.it_value.tv_nsec = 5000000; // 5 ms: start after 1/4 of first frame
its.it_interval.tv_sec = its.it_value.tv_sec;
its.it_interval.tv_nsec = 20000000; // 20 ms = 20 million nanoseconds

ret=timer_settime(timerid, 0, &its, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime!\n");
	exit(-1);
}; // end if


framecount=0;
filecount=0;

// loop per file

for (fileloop=0;fileloop<numinputfile;fileloop++) {
	// open wave file
	int fileformat=0;
	// we reserve 15 bytes for dvtool header to read the ambe file.
	// a .ambe-file should start with "#C Version 1.0" which is 14 character + 1 for the \n
	char dvtool_header[15];

	// we start by opening the file in binary mode (for files in the .dvtool file
	// format).
	// If we notice the file in in .ambe (text) format, the file will be reopened
	// in text-mode
	if (! (infile = fopen(infilelist[fileloop],"rb"))) {
		fprintf(stderr,"Error: Unable to open input file! %s\n",infilelist[fileloop]);
		continue;
	}; // end if

	// read 10 bytes: header of .dvtool file
	// it should begin with 6 bytes containing "DVTOOL" in ascii
	ret=fread(dvtool_header,1,6,infile);

	// did we receive 10 bytes?
	if (ret < 6) {
		fprintf(stderr,"Error: Could not read header of input file while probing .dvtool format! %s\n",infilelist[fileloop]);
		fclose(infile);
		continue;
	}; // end if

	if (strncmp(dvtool_header,"DVTOOL",6) == 0) {
		fileformat=1;

		// skip to position 68 of file: 10 octets for header + 58 octets for
		// configuration frame
		// We are at position 6 so move up 62 octets
		fseek(infile,62,SEEK_CUR);

	} else {
		// OK, it's not a .dvtool file, perhaps it's a .ambe file. That should
		// start with a line containing '#C Version 1.0"

		// return to beginning of file
		fseek(infile,0,SEEK_SET);

		// read next of line, up to \n, maximum 14 characters
		ret=fread(dvtool_header,1,14,infile);

		if (ret < 14) {
			fprintf(stderr,"Error: Could not read header of input file while probing .ambe format! %s\n",infilelist[fileloop]);
			fclose(infile);
			continue;
		}; // end if

		if (strncmp(dvtool_header,"#C Version 1.0",14) == 0) {
			fileformat=2;

			// close file, re-open in "text" mode.
			if (! (infile = freopen(infilelist[fileloop],"r",infile))) {
				fprintf(stderr,"Error: Unable to open input file! %s\n",infilelist[fileloop]);
				continue;
			}; // end if
		};
	}; // end else - if

	
	if (fileformat == 0) {
		fprintf(stderr,"Error: unknown fileformat for file %s.\n",infilelist[fileloop]);
		fclose(infile);
		continue;
	}; // end if

		
	if (global.verboselevel >= 1) {
		printf("Decoding file %s (fileformat %s).\n",infilelist[fileloop],fileformatdescr[fileformat]);
	}; // end if

	// file is OK
	filecount++;

	// read data from wav-file
	stop=0;

	while (stop == 0) {
		unsigned char ambebuffer[9];

		// can we send the next frame?
		if (global.datasendready == 1) {
		// yes, data is read to be send by serialsend, so let's prepare the next packet

			if (fileformat == 1) {
				int frameok=0;

				while ((frameok == 0) && (stop == 0)) {
					// .dvtool file-format
					ret=fread(dvtool_framein,1,29,infile);

					if (ret < 29) {
						// could not read 29 octets, set "stop" to 1
						stop=1;
					} else {
						// check frame, should begin with length (0x1b00) and "DSVT"
						if ((dvtool_framein[0] == 0x1b) && (dvtool_framein[1] == 0) && 
								(memcmp(&dvtool_framein[2],"DSVT",4)==0)) {
							// valid frame: copy the 9 bytes of AMBE data to first 9 positions of
							// data-part of ambe_dataframe structure
							memcpy(&ambe_dataframe.data,&dvtool_framein[17],9);
							frameok=1;

							// ok, we have a valid frame
							// set data-to-send marker to trigger serialsend function
							global.datatosend=1;
							global.datasendready=0;
							framecount++;
						}; // end if
					}; // end else - if
				}; // end while
			} else if (fileformat == 2) {

				ret=readambefile(infile,ambebuffer);

				if (ret == 9) {
					// valid frame
					memcpy(&ambe_dataframe.data,ambebuffer,9);

					// ok, we have a valid frame
					// set data-to-send marker to trigger serialsend function
					global.datatosend=1;
					global.datasendready=0;
					framecount++;
				} else {
					// no frames left to read in AMBE-file
					stop=1;
				}; // end else - if
				
			}; // end elseif - if

		} else {
			// no data to send -> sleep for 2 ms
			usleep(2000);
		}; // end else - if (data send ready?)

	}; // end while (stop, file is read)

	// close wav-file
	fclose(infile);

}; // end for (all input files)

// wait for 20 ms to give the last frame of the file to go through all the buffer stages
usleep(20000);

// no data to send anymore. End-of-file reached of wav-file
global.allpacketsend=1;

// OK, we have read all of the file, now wait for the "serialsend" function to terminate

while (global.welive_serialsend) {
	// not yet done, wait 100 ms
	usleep(100000);
}; // end while

// no stop interupt timer
its.it_value.tv_sec = 0;
its.it_value.tv_nsec = 0; // timer = 0 -> stop timer
its.it_interval.tv_sec = its.it_value.tv_sec;
its.it_interval.tv_nsec = its.it_value.tv_nsec;

ret=timer_settime(timerid, 0, &its, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime!\n");
	exit(-1);
}; // end if


// now wait for the serialreceive program to terminate
while (global.welive_serialreceive) {
	// not yet done, wait 5 ms
	usleep(100000);
}; // end while


// OK, we are finished
fprintf(stderr,"done: %d frames encoded in %d files\n", framecount,filecount);
close(global.donglefd);

exit(0);
}; // end main

