/* wavstream.c */

// wavstream: convert wav-files to .ambe-files and stream it out
// via a udp-stream to the local repeater, a remote repeater or a
// X-reflector
// Usage: wavstream [-t broadcasttext] [-dngl device] [-v] [-4] [-6] [-d ipaddress ] [-p port] [-my CALLSIGN] [-bi break-interval] [-bl break-length] [-br break-repeat] name-of-repeater module infile.wav [ infile.wav ....]
// Use filename "-" for reading from standard in

// this program requires a DVdongle to do AMBE encoding, connected to
// a USB-port
// this program requires the sndfile-libraries to import wav-files
// see http://www.mega-nerd.com/libsndfile/

// The goal is to use this program to produce audio announcements on
// DSTAR repeaters 

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
// 4  jan. 2011: Version 0.1.3. CLI-options via "-", donglelocation, verboselevel, destination ip-address/port, multiple input files
// 27 Mar. 2011: version 0.2.1. CLI-options in line with other applications, dextra, read from stdin, auto-break, DNS-resolution, ipv6

#define VERSION "0.2.1"

// define used to differenciate code in s_udpsend of ambestream and wavstream
#define WAVSTREAM 1



#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>




// ip networking
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

// needed for time-related functions
#include <sys/time.h>

// sndfile: read wav-files
#include <sndfile.h>

// term io: serial port
#include <termios.h>

// posix threads + inter-process control
#include <pthread.h>
#include <signal.h>

// posix interupt timers
#include <time.h>

// for flock
#include <sys/file.h>


// change this to 1 for debugging
#define DEBUG 0


// local include-files
// wav2ambe.h: data structures + global data
// As wav2ambe is multithreaded, global data-structures are used
// by to threads to communicate with each other
#include "wavstream.h"

// Usage and Help functions
#include "usageandhelp.h"

// serialsend.h: contains the function with that name
// Is started every 20 ms to send data to dongle
#include "s_serialsend.h"

// serialreceive.h: contains the function with that name
// Is started as a seperate thread by the main function
#include "s_serialreceive.h"

// udpsend: contains the function with that name
// Is started every 20 ms to send UDP-packets to repeater
#include "s_udpsend.h"


// dextra linking and heartbeat functions
#include "dextra.h"


// defines for timed interrupts
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define SIG2 SIGRTMIN+1
#define SIG3 SIGRTMIN+2



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
struct sigaction sa1, sa2, sa3;
struct sigevent sev1, sev2, sev3;
timer_t timerid1, timerid2, timerid3;
struct itimerspec its1, its2, its3;

// some miscellaneous vars:
int status;
int stop;
int framecount;
int filecount;
int numinputfile;


char * modulein;

// we can encode up to "MAXINFILE" files at one
char * infilelist[MAXINFILE];


// var used for all kind of tempory data
int ret;
int repnamesize;
int paramloop;
int fileloop;
char * destportstrin=NULL;
char * destinationin=NULL;

char * destination=NULL; // destination is local as we copy the 
// ai_addr to the global data structure
char * mycallin=NULL;

char * breakintervalstrin=NULL; char breakintervalstr[10];
char * breaklengthstrin=NULL; char breaklengthstr[10];
char * breakrepeatstrin=NULL; char breakrepeatstr[10];

// vars for getaddrinfo
struct addrinfo *hint, *info;



int linktodextra;
int ipv4only;
int ipv6only;

// ////// data definition done ///


// main program starts here ///
// part 1: initialise vars and check cli-arguments
// Usage: wavstream [-t broadcasttext] [-dngl device] [-v] [-4] [-6] [-d ipaddress ] [-p port] [-my CALLSIGN] [-bi break-interval] [-bl break-length] [-br break-repeat] name-of-repeater module infile.wav [ infile.wav ....]

global.verboselevel=0;
dongledevice=defaultdongledevice;
global.bcmsg=NULL;
numinputfile=0;
destinationin=NULL;
global.destport=0;
linktodextra=0;

ipv4only=0;
ipv6only=0;


global.breakinterval=0;		// breakinterval is 0 (no break)
global.breaklength=200;		// break is 4 seconds. Setting this value to less then 100
							// (2 seconds) is not a good idea as some icom-repeaters
							// will have problems starting the new stream if it
							// started to quickly after the previous one has ended
global.breakrepeat=MAXBREAKREPEAT;



// start of program //

// analysing CLI parameters //
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
		fprintf(stderr,"wavstream version %s\n",VERSION);
		exit(0);
	} else if (strcmp(thisarg,"-dngl") == 0) {
	// -d = dongledevice
		if (paramloop+1 < argc) {
			paramloop++;
			dongledevice=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-v") == 0) {
	// -v = verbose
		global.verboselevel++;
	} else if (strcmp(thisarg,"-h") == 0) {
	// -h = help
		help(argv[0]);
		exit(0);
	} else if (strcmp(thisarg,"-dxl") == 0) {
	// -dxl = dextra protocol
		linktodextra=1;
	} else if ((strcmp(thisarg,"-i") == 0) || (strcmp(thisarg,"-d") == 0)) {
	// -i = ipaddr (being depreciated) -d = destination
		if (paramloop+1 < argc) {
			paramloop++;
			destinationin=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-4") == 0) {
	// -4 = force ipv4 only
		ipv4only=1;
	} else if (strcmp(thisarg,"-6") == 0) {
	// -6 = force ipv6 only
		ipv6only=1;
	} else if (strcmp(thisarg,"-p") == 0) {
	// -p = ip port
		if (paramloop+1 < argc) {
			paramloop++;
			global.destport=atoi(argv[paramloop]);
			destportstrin=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-my") == 0) {
	// -M = my call
		if (paramloop+1 < argc) {
			paramloop++;
			mycallin=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-bi") == 0) {
	// -bi = break interval
		if (paramloop+1 < argc) {
			paramloop++;
			global.breakinterval=atoi(argv[paramloop]);
			breakintervalstrin=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-bl") == 0) {
	// -bl = break length
		if (paramloop+1 < argc) {
			paramloop++;
			global.breaklength=atoi(argv[paramloop]);
			breaklengthstrin=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-br") == 0) {
	// -br = break repeat
		if (paramloop+1 < argc) {
			paramloop++;
			global.breakrepeat=atoi(argv[paramloop]);
			breakrepeatstrin=argv[paramloop];
		}; // end if
	} else {
	// argument without option is input filename
		infilelist[numinputfile]=thisarg;

		if (numinputfile < MAXINFILE - 1) {
			numinputfile++;
		}; // end if
	}; // end else - elsif - elsif - elsif - elsif - if

}; // end for


// the name of the repeater and the module-name are now in first two fields of the input files
// extract them

// but first check if we have sufficient parameters
if (numinputfile <= 2) {
	fprintf(stderr,"Error: Unsufficient number of parameters!\n");
	usage(argv[0]);
	exit(-1);
}; // end if

// now extract name-of-repeater and module

// copy epeatername to global var, maximum 6 characters
// without terminating \n
memset(global.repeatername,' ',6);

repnamesize=strlen(infilelist[0]);
if (repnamesize>6) {
	repnamesize=6;
}; // end if
memcpy(global.repeatername,infilelist[0],repnamesize);

modulein=infilelist[1];

// check first character, if 'a' or 'A' -> module is 1
if ((modulein[0] == 'a') || (modulein[0] == 'A')) {
	global.module=0x41; // ascii 'A'
}; // end if

// check first character, if 'b' or 'B' -> module is 2
if ((modulein[0] == 'b') || (modulein[0] == 'B')) {
	global.module=0x42; // ascii 'B'
}; // end if

// check first character, if 'c' or 'C' -> module is 3
if ((modulein[0] == 'c') || (modulein[0] == 'C')) {
	global.module=0x43; // ascii 'C'
}; // end if

if (global.module == 0) {
	fprintf(stderr,"Error: module must be 'a', 'b' or 'c'\n");
	usage(argv[0]);
	exit(-1);
}; // end if

// check destination port. Has been extracted using a "atoi" (convert string
// to number) function, but was it a valid string?
// convert destport back to string and see if we get the same thing
// we started with
// only do this check if a UDP port has been entered
if (destportstrin != NULL) {
	char destporttmp[10];
	snprintf(destporttmp,10,"%d",global.destport);

	if (strcmp(destporttmp,destportstrin) != 0) {
		fprintf(stderr,"Error: invalid input for UDP port\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	if ((global.destport <= 0) || (global.destport > 65535)) {
		fprintf(stderr,"Error: UDP port must be between 1 and 65535\n");
		fprintf(stderr,"Usage: %s -V \n",argv[0]);
		exit(-1);
	}; // end if
} else {
	// no default UDP-port set: make it 40000 (no -dxl option set) or 30001 (-dxl option set)
	if (linktodextra) {
		global.destport=30001;
	} else {
		global.destport=40000;
	}; // end if
}; // end if


if (destinationin == NULL) {
	// no destination set: default value is "127.0.0.1", but must be manually set when
	// the -dxl option is set

	if (linktodextra) {
		fprintf(stderr,"Error: Destination must be set when -dxl option is used!\n");
		usage(argv[0]);
		exit(-1);
	} else {
		destination="127.0.0.1";
	}; // end if

} else {
	destination=destinationin;
}; // end else - if 


// correct "infilelist" array, move up two elements;
numinputfile -= 2;



// check destination hostname

// option -4 and -6 cannot be set both
if (ipv4only & ipv6only) {
	fprintf(stderr,"Error: option -4 and -6 are mutually exclusive\n");
	usage(argv[0]);
	exit(-1);
}; // end if

hint=malloc(sizeof(struct addrinfo));

if (hint == NULL) {
	fprintf(stderr,"Internal Error: malloc failed for hint!\n");
	exit(-1);
}; // end if

// clear hint
memset(hint,0,sizeof(hint));

hint->ai_socktype = SOCK_DGRAM;

// resolve hostname, use function "getaddrinfo"
// set address family in hint if ipv4only or ipv6ony
if (ipv4only) {
	hint->ai_family = AF_INET;
} else if (ipv6only) {
	hint->ai_family = AF_INET6;
} else {
	hint->ai_family = AF_UNSPEC;
}; // end if


// do DNS-query, use getaddrinfo for both ipv4 and ipv6 support
ret=getaddrinfo(destination, NULL, hint, &info);

if (ret != 0) {
	fprintf(stderr,"Error: resolving hostname %s: (%s)\n",destination,gai_strerror(ret));
	exit(-1);
}; // end if


// getaddrinfo can return multiple results, we only use the first one

// give warning is more then one result found.
// Data is returned in info as a linked list
// If the "next" pointer is not NULL, there is more then one
// element in the chain

if ((info->ai_next != NULL) || global.verboselevel >= 1) {
	char ipaddrtxt[INET6_ADDRSTRLEN];


	// get ip-address in numeric form
	if (info->ai_family == AF_INET) {
		// ipv4
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *) info->ai_addr;
		inet_ntop(AF_INET,&sin->sin_addr,ipaddrtxt,INET6_ADDRSTRLEN);
	} else {
		// ipv6
		struct sockaddr_in6 *sin;
		sin = (struct sockaddr_in6 *) info->ai_addr;
		inet_ntop(AF_INET6,&sin->sin6_addr,ipaddrtxt,INET6_ADDRSTRLEN);
	}; // end else - if
		
	if (info->ai_next != NULL) {
		fprintf(stderr,"Warning. Multiple ip addresses found for destination %s. Using %s\n",destination,ipaddrtxt);
	} else {
		fprintf(stderr,"Sending DV-stream to ip-address %s\n",ipaddrtxt);
	}; // end if
}; // end if

// store address info for "s_udpsend" function
global.ai_addr=info->ai_addr;


// check "break" values
if (breakintervalstrin != NULL) {
	snprintf(breakintervalstr,10,"%d",global.breakinterval);

	if (strcmp(breakintervalstr,breakintervalstrin) != 0) {
		fprintf(stderr,"Error: invalid input for breakinterval\n");
		usage(argv[0]);
		exit(-1);
	}; // end if


	if ((global.breakinterval <0) || ((global.breakinterval < 10) && (global.breakinterval != 0))) {
		fprintf(stderr,"Error: breakinterval must be zero or larger then 10\n");
		usage(argv[0]);
		exit(-1);
	}; // end if
}; // end if

if (breaklengthstrin != NULL) {
	snprintf(breaklengthstr,10,"%d",global.breaklength);

	if (strcmp(breaklengthstr,breaklengthstrin) != 0) {
		fprintf(stderr,"Error: invalid input for breaklength\n");
		usage(argv[0]);
	}; // end if

	if (global.breaklength <=0) {
		fprintf(stderr,"Error: break length must be larger then zero\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	if ((global.breaklength < 100) && (global.verboselevel >= 1)) {
		fprintf(stderr,"Warning: a breaklength of less then 100 (2 seconds) can produce announcement-problems on some i-com repeaters.\n");
	}; // end if
}; // end if


if (breakrepeatstrin != NULL) {
	snprintf(breakrepeatstr,10,"%d",global.breakrepeat);

	if (strcmp(breakrepeatstr,breakrepeatstrin) != 0) {
		fprintf(stderr,"Error: invalid input for breakrepeat\n");
		usage(argv[0]);
	}; // end if

	if (global.breakrepeat <0) {
		fprintf(stderr,"Error: break length must be more or equal to zero\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	if (global.breakrepeat > MAXBREAKREPEAT) {
		fprintf(stderr,"Error: break length must be less or equal then %d\n",MAXBREAKREPEAT);
		usage(argv[0]);
		exit(-1);
	}; // end if
}; // end if


// mycall: should be between 3 and 6 characters. 
if (mycallin != NULL) {
	int calllen=strlen(mycallin);

	if ((calllen < 3) || (calllen >6)) {
		fprintf(stderr,"Error: mycall should be between 3 and 6 characters\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	// check there is no "/" in the callsign
	if ((memchr(mycallin,'/',6) != NULL)) {
		fprintf(stderr,"Error: mycall should not contain a '/'\n");
		usage(argv[0]);
		exit(-1);
	}; // end if


	
	memset(global.mycall,' ',6);
	memcpy(global.mycall,mycallin,calllen);

} else {
	// mycall is mandatory when the -dxl option is set
	if (linktodextra) {
		fprintf(stderr,"Error: mycall is mandatory if the -dxl option is set\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	// no mycall set. Call will be the same as repeatername
	memcpy(global.mycall,global.repeatername,6);
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
	fprintf(stderr,"open to dongle failed! \n");
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


// init some vars
status=0;



// open a UDP socket and store information in the global data structure
global.udpsd=socket(AF_INET6,SOCK_DGRAM,0);

if (global.udpsd < 0) {
	fprintf(stderr,"Error: could not create udp socket! Exiting!\n");
	exit(-1);
}; // end if


// We first set up the dextra link, before starting the DVdongle; as
// otherwize, the dongle can timeout due to not receiving any data during
// the dextra link process

// dextra link
if (linktodextra) {
	ret=dextralink();

	if (ret == -1) {
		fprintf(stderr,"Error: could not link to dextra gateway/reflector.\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	// sleep 1 second to let things settle down
	sleep(1);

	// start dextraheartbeat thread every 6 seconds
	// only if dextralink is set
	if (linktodextra) {

		// establing handler for function dextraheartbeat
		sa3.sa_flags = 0;
		sa3.sa_handler = funct_dextraheartbeat;
		sigemptyset(&sa3.sa_mask);

		ret=sigaction(SIG3, &sa3, NULL);
		if (ret < 0) {
			fprintf(stderr,"error in sigaction dextraheartbeat!\n");
			exit(-1);
		}; // end if


		/* Create the timer for dextraheartbeat */
		sev3.sigev_notify = SIGEV_SIGNAL;
		sev3.sigev_signo = SIG3;
		sev3.sigev_value.sival_ptr = &timerid3;
		ret=timer_create(CLOCKID, &sev3, &timerid3);
		if (ret < 0) {
			fprintf(stderr,"error in timer_create timer dextraheartbeat!\n");
			exit(-1);
		}; // end if

		// start timed function timer dextraheartbeat, every 6 sec
		its3.it_value.tv_sec = 0;
		its3.it_value.tv_nsec = 1;
		its3.it_interval.tv_sec = 6; // 6 seconds
		its3.it_interval.tv_nsec = 0; 

		ret=timer_settime(timerid3, 0, &its3, NULL);
		if (ret < 0) {
      	  	fprintf(stderr,"error in timer_settime timer dextraheartbeat!\n");
   	     	exit(-1);
		}; // end if
	}; // end if

}; // end if




// First part of the program: initialise the dongle
// This runs purely sequencial

// main loop:
// senario:
// status 0: send "give-name" command to dongle
// status 1: wait for name -> send "start" command
// status 2: wait for confirmation of "start" -> send ambe-packet (configures encoder) + dummy pcm-frame

if (global.verboselevel >= 1) {
	fprintf(stderr,"Initialising DVdongle\n");
}; // end if


while (status < 3) {
	// some local vars
	uint16_t plen_i; 
	uint16_t ptype;

	if (global.verboselevel >= 1) {
		fprintf(stderr,"DVDONGLE status = %d\n",status);
	}; 

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

			ret=write (global.donglefd,ambe,50);

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
}





// Part 2 of main program

// now we are at status level 3. We have just received the acknowledge
// to the "start" and have send the 1st ambe-packet to configure
// the encoder

// create socket: will be used by both "dextralink" and "s_udpsend"
// we will open is as IPv6, so it can be used for both ipv4 and ipv6



// now we do four things:
// - the main process reads pcm-frames of the .wav-file, and
// makes them available for the "send data" thread
// - the "send data" function is started via a interrupt-time every
// 20 ms, reads the data prepared by the main thread and sends it
// out to the dvdongle
// - start the "serialreceive" thread that receives the frames from
// the dvdongle and store it in a ringbuffer to the "udpsend" thread
// - start the "udpsend" thread. This thread takes ambe-packets from
// the ringbuffer and sends it to udp/20000 on the local host in the
// dstar format
// Before the first packet is send, this thread must also send a
// start-frame containing dstar routing-information

// init and start thread serialreceive
pthread_create(&thr_serialreceive,NULL,funct_serialreceive, (void *) &global);

// init vars for serialsend function
global.datatosend=0;
global.datasendready=1;

// establing handler for signal serialsend
sa1.sa_flags = 0;
sa1.sa_handler = funct_serialsend;
sigemptyset(&sa1.sa_mask);

ret=sigaction(SIG, &sa1, NULL);
if (ret <0) {
	fprintf(stderr,"error in sigaction serialsend!\n");
	exit(-1);
}; // end if

// establing handler for signal udpsend

sa2.sa_flags = 0;
sa2.sa_handler = funct_udpsend;
sigemptyset(&sa2.sa_mask);

ret=sigaction(SIG2, &sa2, NULL);
if (ret < 0) {
	fprintf(stderr,"error in sigaction udpsend!\n");
	exit(-1);
}; // end if


/* Create the timer for serialsend */
sev1.sigev_notify = SIGEV_SIGNAL;
sev1.sigev_signo = SIG;
sev1.sigev_value.sival_ptr = &timerid1;

ret=timer_create(CLOCKID, &sev1, &timerid1);
if (ret < 0) {
	fprintf(stderr,"error in timer_create timer serialsend!\n");
	exit(-1);
}; // end if

/* Create the timer for udpsend */
sev2.sigev_notify = SIGEV_SIGNAL;
sev2.sigev_signo = SIG2;
sev2.sigev_value.sival_ptr = &timerid2;

ret=timer_create(CLOCKID, &sev2, &timerid2);
if (ret < 0) {
	fprintf(stderr,"error in timer_create timer udpsend!\n");
	exit(-1);
}; // end if


// start timed function for serialsend, every 20 ms, offset = 5 ms
its1.it_value.tv_sec = 0;
its1.it_value.tv_nsec = 5000000; // 5 ms: start after 1/4 of first frame
its1.it_interval.tv_sec = its1.it_value.tv_sec;
its1.it_interval.tv_nsec = 20000000; // 20 ms = 20 million nanoseconds

ret=timer_settime(timerid1, 0, &its1, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime timer serialsend!\n");
	exit(-1);
}; // end if


// initialise globaldata for udpsend
global.ringbufferlow=0;
global.ringbufferhigh=0;

// start timed function timer udpsend, every 20 ms, offset = 10 ms
its2.it_value.tv_sec = 0;
its2.it_value.tv_nsec = 10000000; // 5 ms: start after 1/4 of first frame
its2.it_interval.tv_sec = its2.it_value.tv_sec;
its2.it_interval.tv_nsec = 20000000; // 20 ms = 20 million nanoseconds

ret=timer_settime(timerid2, 0, &its2, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime timer udpsend!\n");
	exit(-1);
}; // end if


framecount=0;
filecount=0;

// loop for file

for (fileloop=0;fileloop<numinputfile;fileloop++) {
	const int truefileloop=fileloop+2;


// open wave file
	// if filename is "-", read from standin
	if (strcmp(infilelist[truefileloop],"-") == 0) {	
		if (! (infile = sf_open_fd(0,SFM_READ,&sfinfo,0))) {
			fprintf(stderr,"Unable to open from standard in!\n");
			continue;
		}; // end if
	} else {
		// read external file
		if (! (infile = sf_open (infilelist[truefileloop],SFM_READ,&sfinfo))) {
			fprintf(stderr,"Unable to open input file! %s\n",infilelist[truefileloop]);
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
		if (strcmp(infilelist[truefileloop],"-") == 0) {	
			fprintf(stderr,"Encoding from standard-in ... \n");
		} else {
			fprintf(stderr,"Encoding file %s ... \n",infilelist[truefileloop]);
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
				}; // end if

				// set data-to-send marker to trigger serialsend function
				global.datatosend=1;
				global.datasendready=0;
				framecount++;

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


// wait for 6 ms to give the last frame of the file to go through all the buffer stages
usleep(6000);

// no data to send anymore. End-of-file reached of wav-file
global.allpacketsend=1;



// OK, we have read all of the file, now wait for the "serialsend"
// function to terminate.
// this is done by monitoring the welive_serialsend parameter

while (global.welive_serialsend) {
	// not yet done, wait 5 ms
	usleep(5000);
}; // end while

// now stop interupt timer
its1.it_value.tv_sec = 0;
its1.it_value.tv_nsec = 0; // timer = 0 -> stop timer
its1.it_interval.tv_sec = its1.it_value.tv_sec;
its1.it_interval.tv_nsec = its1.it_value.tv_nsec;

ret=timer_settime(timerid1, 0, &its1, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime-stop serialsend !\n");
	exit(-1);
}; // end if


// wait for the serialreceive thread to terminate
while (global.welive_serialsend) {
	// not yet done, wait .1 sec
	usleep(100000);
}; // end while

// now wait for the serialreceive thread to terminate
while (global.welive_serialreceive) {
	// not yet done, wait .1 sec
	usleep(100000);
}; // end while


while (global.welive_udpsend) {
	// not yet done, wait .1 sec
	usleep(100000);
}; // end while

// there are still some interrupt driven threads running, but they
// will terminate when the main program exits

// OK, we are finished
fprintf(stderr,"done: %d frames encoded\n", framecount);
close(global.donglefd);

// done
exit(0);
}; // end main

