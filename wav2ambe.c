/* wav2ambe.c */

// wav2ambe: convert wav-files to .ambe-files
// Usage: wav2ambe [-t broadcasttext] [-d device] [-v ] -f {a,d} -o outfile.ambe infile.wav [ infile.wav ... ]
// filename "-" means read from standard input
// Usage: wav2ambe -V
// this program requires a DVdongle to do AMBE encoding, connected to
// a USB-port
// this program requires the sndfile-libraries to import wav-files
// see http://www.mega-nerd.com/libsndfile/

// The goal is to use this program to produce audio announcements on
// DSTAR repeaters by stitching pre-encoded AMBE files together

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

//  release info:
// 17 okt. 2010: version 0.0.1. Initial release
// 7  nov. 2010: version 0.0.4. Multithreaded release
// 22 nov. 2010: version 0.1.1. added support for text-messages
// 3  jan. 2011: version 0.1.2. CLI-options via "-", donglelocation, verboselevel, multiple input files
// 27 Mar. 2011: version 0.2.1. CLI-options in line with other applications, read from stdin, code-cleanup

#define VERSION "0.2.1"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

// sndfile: read wav-files
#include <sndfile.h>

// term io: serial port
#include <termios.h>

// posix threads + inter-process control
#include <pthread.h>
#include <signal.h>

// posix interupt timers
#include <time.h>

// for function htonl
#include <arpa/inet.h>

// for flock
#include <sys/file.h>

// for errno
#include <errno.h>

// change this to 1 for debugging
#define DEBUG 0

// AMBE2WAV and WAV2AMBE: used to desinguage code in serialsend.h and
// serialreceive.h (which are shared by wav2ambe and ambe2wav)
#define WAV2AMBE 1


// local include-files
// wav2ambe.h: data structures + global data
// As wav2ambe is multithreaded, global data-structures are used
// by to threads to communicate with each other
// wav2ambe.h is shared by wav2ambe and ambe2wav
#include "wav2ambe.h"

// serialsend.h: contains the function with that name
// Is started every 20 ms to send data to dongle
// serialsend.h is shared by wav2ambe and ambe2wav
#include "serialsend.h"

// serialreceive.h: contains the function with that name
// Is started as a seperate thread by the main function
// serialreceive is shared by wav2ambe and ambe2wav
#include "serialreceive.h"

// usageandhelp.h: Usage and help
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

// SNDFILE: vars for library to read wave-files
SNDFILE * infile;
SF_INFO sfinfo;


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

// loops
int paramloop;
int fileloop;


char * formatin;

// we can encode up to "MAXINFILE" files at one
char * infilelist[MAXINFILE];

// ////// data definition done ///


// main program starts here ///
// part 1: initialise vars and check cli-arguments

// Usage: wav2ambe [-t broadcasttext] [-dngl device] [-v ] -f {a,d} -o outfile.ambe infile.wav [ infile.wav ... ]

// init global data

global.fileformat=-1;
global.verboselevel=0;
global.bcmsg=NULL;
global.fname_out=NULL;

// init local data
numinputfile=0;
dongledevice=defaultdongledevice;


// start of program //
for (paramloop=1;paramloop<argc;paramloop++) {
	char * thisarg=argv[paramloop];

	if (strcmp(thisarg,"-t") == 0) {
	// -t = broadcasttext
		if (paramloop+1 < argc) {
			paramloop++;
			global.bcmsg=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-V") == 0) {
	// -V = Version
		fprintf(stderr,"wav2ambe version %s\n",VERSION);
		exit(0);
	} else if (strcmp(thisarg,"-h") == 0) {
	// -h = help
		help(argv[0]);
		exit(0);
	} else if (strcmp(thisarg,"-dngl") == 0) {
	// -dngl = dongledevice
		if (paramloop+1 < argc) {
			paramloop++;
			dongledevice=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-v") == 0) {
	// -v = verbose
		global.verboselevel++;
	} else if (strcmp(thisarg,"-f") == 0) {
	// -f = file format
	// may be "a" for .ambe files, or "d" for .dvtool files

		// set outfilefileformat to 0 to start with and correct this value later if a valid
		// argument is found
		global.fileformat=0;

		if (paramloop+1 < argc) {
			paramloop++;
			formatin=argv[paramloop];

			// check first character, if 'a' or 'A' -> format is 1
			if ((formatin[0] == 'a') || (formatin[0] == 'A')) {
				global.fileformat=1;
			}; // end if

			// check first character, if 'd' or 'D' -> format is 2
			if ((formatin[0] == 'd') || (formatin[0] == 'D')) {
				global.fileformat=2;
			}; // end if
		}; // end if
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

// output fileformat not given (-1) or incorrect (0)
if (global.fileformat < 1) {
	fprintf(stderr,"Error: Output format (option \"-f\") not provided or incorrect!\n");
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

// initialise empty_buffer: same as pcmbuffer but will never change
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

// First part of the program: initialise the dongle
// This runs pure sequencial

// main loop:
// senario:
// status 0: send "give-name" command to dongle
// status 1: wait for name -> send "start" command
// status 2: wait for confirmation of "start" -> send ambe-packet (configures encoder) + dummy pcm-frame

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
		// status is 2 (after 2 seconds of waiting, send "request name")
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
			// also send a dummy pcm-frame
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
// - the main process reads pcm-frames of the .wav-file, and
// makes them available for the "send data" thread
// - the "send data" function is started via a interrupt-time every
// 20 ms, reads the data prepared by the main thread and sends it
// out to the dvdongle
// - we start the "serialreceive" thread that receives the frames from
// the dvdongle and saves the ambe-frames it to a file
// in addition to this, it also send a dummy ambe-frame when receiving
// a ambe-frame from the dongle

// init and start thread serialreceive
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
	// if filename is "-", read from stdin
	if (strcmp(infilelist[fileloop],"-") == 0) {
		if (! (infile = sf_open_fd(0,SFM_READ,&sfinfo,0))) {
			fprintf(stderr,"Unable to open from standard in!\n");
			continue;
		}; // end if
	} else {
		// read external file
		if (! (infile = sf_open (infilelist[fileloop],SFM_READ,&sfinfo))) {
			fprintf(stderr,"Unable to open input file! %s\n",infilelist[fileloop]);
			continue;
		}; // end if
	}; // end else - if 


	// file must be wave-file, 8000 Khz sampling, mono, 16 bit little-endian

	if (sfinfo.samplerate != 8000) {
		fprintf(stderr,"Error: wav-file must be 8 Khz sampling.\n");
		sf_close(infile);
		continue;
	}; // end if

	if (sfinfo.channels != 1) {
		fprintf(stderr,"Error: wav-file must mono.\n");
		sf_close(infile);
		continue;
	}; // end if

	if (sfinfo.format != 0x10002) {
		fprintf(stderr,"Error: wav-file must MS-WAV, 16 bit Little-endian.\n");
		sf_close(infile);
		continue;
	}; // end if

	if (global.verboselevel >= 1) {
		if (strcmp(infilelist[fileloop],"-") == 0) {
			fprintf(stderr,"Encoding from standard in ... \n");
		} else {
			fprintf(stderr,"Encoding file %s.\n",infilelist[fileloop]);
		}; // end else - if
	}; // end if

	// file is OK
	filecount++;

	// read data from wav-file
	stop=0;

	while (stop == 0) {

		// can we send the next frame?
		if (global.datasendready == 1) {
		// yes, data is send by serialsend, so let's prepare the next packet
			sf_count_t nsample;

			nsample=sf_read_short(infile,(short *) &pcmbuffer.pcm,160);

			// is there still data in the wav-file
			if (nsample >0) {
				// yes

				// if less then 160 samples received, all rest as all-0 and
				// set the "lastframe" flag
				if (nsample < 160) {
					// fill rest with all 0
					memset(&pcmbuffer.pcm[nsample*2],0,(160-nsample)*2);

					// set data-to-send marker to trigger serialsend function
					global.datatosend=1;
					global.datasendready=0;
					framecount++;

				} else {
					// this is strictly not necessairy, just to be sure
					global.allpacketsend=0;

					// set data-to-send marker to trigger serialsend function
					global.datatosend=1;
					global.datasendready=0;
					framecount++;
				}; // end else - if (complete frame?)
			} else {
				// set "stop" as we have finished reading the wav-file
				stop=1;
			}; // end else - if (still data in wav-file?)

		} else {
			// no data to send -> sleep for 2 ms
			usleep(2000);
		}; // end else - if (data send ready?)

	}; // end while (stop, file is read)

	// close wav-file
	sf_close(infile);

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
	// not yet done, wait 100 ms
	usleep(100000);
}; // end while


// OK, we are finished
fprintf(stderr,"done: %d frames encoded in %d files\n", framecount,filecount);
close(global.donglefd);

exit(0);
}; // end main

