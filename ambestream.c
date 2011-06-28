/* ambestream */

// ambestream: stream out ambe-files via udp streaming to a local repeater, 
// a remote repeater or a XRF reflector
// Usage: ambestream [-t broadcasttext] [-v] [-4] [-6] [-x] [-my CALLSIGN] [-i|-d destination ] [-p port] [ -bi break-interval ] [ -bl break-length ] [ -br break-repeat ]  name-of-repeater module infile.dvtool [ infile.ambe ....]

// The goal is to use this program to produce audio announcements on
// DSTAR repeaters 

// copyright (C) 2011 Kristoff Bonne ON1ARF
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
// 27 jan. 2011: version 0.1.3. Initial release (internal release, not published)
// 27 mar. 2011: version 0.2.1. dextra support, auto-break support, code-cleanup, dns-resolution, ipv6 support

#define VERSION "0.2.1"


// define used to differenciate code in s_udpsend of ambestream and wavstream
#define AMBESTREAM 1

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

// term io: serial port
#include <termios.h>

FILE * infile;

// posix threads + inter-process control
#include <signal.h>

// posix interupt timers
#include <time.h>



// change this to 1 for debugging
#define DEBUG 0



// local include-files

// readambefile.h: contains function with that name
#include "readambefile.h"


// ambestream.h: data structures + global data
// As wav2ambe is multithreaded, global data-structures are used
// by to threads to communicate with each other
#include "ambestream.h"

// udpsend: contains the function with that name
// Is started every 20 ms to send UDP-packets to repeater
// This headerfile is shared with wavstream
#include "s_udpsend.h"

// Usage and Help functions
#include "usageandhelp.h"


// dextra linking and heartbeat functions
#include "dextra.h"

// defines for timed interrupts
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define SIG2 SIGRTMIN+1




//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Main program // Main program // Main program // Main program //////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


int main(int argc,char** argv) {



// data structures:

// global data: other integers

// vars for timed interrupts
struct sigaction sa,sa2;
struct sigevent sev,sev2;
timer_t timerid, timerid2;
struct itimerspec its, its2;

// some miscellaneous vars:
int stop;
int framecount;
int filecount;
int numinputfile;


char * modulein;

// fileformat (textual description)
char * fileformatdescr[3]={"","dvtool","ambe"};

// we can encode up to "MAXINFILE" files at one
char * infilelist[MAXINFILE];


// var used for all kind of tempory data
int ret;
int repnamesize;

unsigned char dvtool_framein[29];
// loops
int paramloop;
int fileloop;

char destportstr[10];
char * destportstrin=NULL;
char * destinationin=NULL;
char * mycallin=NULL;

char * destination=NULL;

int ipv4only;
int ipv6only;

int linktodextra;

char * breakintervalstrin=NULL; char breakintervalstr[10];
char * breaklengthstrin=NULL; char breaklengthstr[10];
char * breakrepeatstrin=NULL;	char breakrepeatstr[10];

// vars for getaddrinfo
struct addrinfo * hint;
struct addrinfo * info;


// ////// data definition done ///


// main program starts here ///
// part 1: initialise vars and check cli-arguments
// Usage: ambestream [-t broadcasttext] [-v] [-4] [-6] [-x] [-my CALLSIGN] [-i|-d destination ] [-p port] [ -bi break-interval ] [ -bl break-length ] [ -br break-repeat ]  name-of-repeater module infile.dvtool [ infile.ambe ....]

global.verboselevel=0;
global.bcmsg=NULL;
numinputfile=0;
destinationin=NULL;
global.destport=0;
mycallin=NULL;
linktodextra=0;
memset(global.mycall,' ',6);

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
		fprintf(stderr,"ambestream version %s\n",VERSION);
		exit(0);
	} else if (strcmp(thisarg,"-h") == 0) {
	// -V = Version
		help(argv[0]);
		exit(0);
	} else if (strcmp(thisarg,"-v") == 0) {
	// -v = verbose
		global.verboselevel++;
	} else if (strcmp(thisarg,"-dxl") == 0) {
	// -x = dextra protocol
		linktodextra=1;
	} else if ((strcmp(thisarg,"-i") == 0) || (strcmp(thisarg,"-d") == 0)) {
	// -d = destination , keep "-i" (ipaddr) for compatibility
		if (paramloop+1 < argc) {
			paramloop++;
			destinationin=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-4") == 0) {
	// -4 = force ipv4 only
		ipv4only=1;
	} else if (strcmp(thisarg,"-6") == 0) {
	// -6 = force ipv4 only
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

// copy repeatername to global var, maximum 6 characters
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

// check destination port. Has been extraced using a "atoi" (convert string
// to number) function, but was if a valid string?
// convert destport back to string and see if we get the same thing
// we started with
// only do this check if a UDP port-numberport has been entered
if (destportstrin != NULL) {
	snprintf(destportstr,10,"%d",global.destport);

	if (strcmp(destportstr,destportstrin) != 0) {
		fprintf(stderr,"Error: invalid input for port\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	if ((global.destport <= 0) || (global.destport > 65535)) {
		fprintf(stderr,"Error: port must be between 1 and 65535\n");
		usage(argv[0]);
		exit(-1);
	}; // end if
} else {
	// no default UDP-port set: make it 40000 (no -x option set) or 30001 (-x option set)
	if (linktodextra) {
		global.destport=30001;
	} else {
		global.destport=40000;
	}; // end if
}; // end if

if (destinationin == NULL) {
	// no destination set: default value is "127.0.0.1", but must be manually set when
	// the -x option is set

	if (linktodextra) {
		fprintf(stderr,"Error: Destination must be set when -x option is used!\n");
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
	fprintf(stderr,"Error: resolving hostname: (%s)\n",gai_strerror(ret));
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
		// for some reason, we neem to shift the address-information with 2 positions
		// to get the correct string returned
		inet_ntop(AF_INET,&info->ai_addr->sa_data[2],ipaddrtxt,INET6_ADDRSTRLEN);
	} else {
		// ipv6
		// for some reason, we neem to shift the address-information with 6 positions
		// to get the correct string returned
		inet_ntop(AF_INET6,&info->ai_addr->sa_data[6],ipaddrtxt,INET6_ADDRSTRLEN);
	}; // end else - if
		
	if (info->ai_next != NULL) {
		fprintf(stderr,"Warning. getaddrinfo returned multiple entries. Using %s\n",ipaddrtxt);
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
		fprintf(stderr,"Error: breakinterval must be zero or greater then 10\n");
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


	memcpy(global.mycall,mycallin,calllen);

} else {
	// mycall is mandatory when the -x option is set
	if (linktodextra) {
		fprintf(stderr,"Error: mycall is mandatory if the -x option is set\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	// no mycall set. Call will be the same as repeatername
	memcpy(global.mycall,global.repeatername,6);
}; // end if



// Main program

// create socket: will be used by both "dextralink" and "s_udpsend"
// we will open is as IPv6, so it can be used for both ipv4 and ipv6


// open a UDP socket and store information in the global data structure
global.udpsd=socket(AF_INET6,SOCK_DGRAM,0);

if (global.udpsd < 0) {
	fprintf(stderr,"Error: could not create udp socket! Exiting!\n");
	exit(-1);
}; // end if


if (linktodextra) {
	ret=dextralink();

	if (ret == -1) {
		fprintf(stderr,"Error: could not link to dextra gateway/reflector.\n");
		usage(argv[0]);
		exit(-1);
	}; // end if

	// wait 3 seconds, to give the system some time to stabilise
	sleep (3);
}; // end if

// First, start the "udpsend" thread. This thread takes ambe-packets from
// the ambebuffer and sends it to udp/40000 on the local host in the
// dstar format
// Before the first packet is send, this thread must also send a
// start-frame containing dstar routing-information

// init vars for serialsend function
global.datatosend=0;
global.datasendready=1;

// establing handler for signal udpsend
sa.sa_flags = 0;
sa.sa_handler = funct_udpsend;
sigemptyset(&sa.sa_mask);

ret=sigaction(SIG, &sa, NULL);
if (ret < 0) {
	fprintf(stderr,"error in sigaction udpsend!\n");
	exit(-1);
}; // end if


/* Create the timer for udpsend */
sev.sigev_notify = SIGEV_SIGNAL;
sev.sigev_signo = SIG;
sev.sigev_value.sival_ptr = &timerid;

ret=timer_create(CLOCKID, &sev, &timerid);
if (ret < 0) {
	fprintf(stderr,"error in timer_create timer udpsend!\n");
	exit(-1);
}; // end if


// start timed function timer udpsend, every 20 ms, offset = 10 ms
its.it_value.tv_sec = 0;
its.it_value.tv_nsec = 1;
its.it_interval.tv_sec = its.it_value.tv_sec;
its.it_interval.tv_nsec = 20000000; // 20 ms = 20 million nanoseconds

ret=timer_settime(timerid, 0, &its, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime timer udpsend!\n");
	exit(-1);
}; // end if


// next, start dextraheartbeat thread every 6 seconds
// only if dextralink is set

// establing handler for function dextraheartbeat
sa2.sa_flags = 0;
sa2.sa_handler = funct_dextraheartbeat;
sigemptyset(&sa2.sa_mask);

ret=sigaction(SIG2, &sa2, NULL);
if (ret < 0) {
	fprintf(stderr,"error in sigaction dextraheartbeat!\n");
	exit(-1);
}; // end if


/* Create the timer for dextraheartbeat */
sev2.sigev_notify = SIGEV_SIGNAL;
sev2.sigev_signo = SIG2;
sev2.sigev_value.sival_ptr = &timerid2;

ret=timer_create(CLOCKID, &sev2, &timerid2);
if (ret < 0) {
	fprintf(stderr,"error in timer_create timer dextraheartbeat!\n");
	exit(-1);
}; // end if


if (linktodextra) {
	// start timed function timer dextraheartbeat, every 6 sec
	// only if "link-to-dextra" enabled
	its2.it_value.tv_sec = 0;
	its2.it_value.tv_nsec = 1;
	its2.it_interval.tv_sec = 6; // 6 seconds
	its2.it_interval.tv_nsec = 0; 

	ret=timer_settime(timerid2, 0, &its2, NULL);
	if (ret < 0) {
		fprintf(stderr,"error in timer_settime timer dextraheartbeat!\n");
		exit(-1);
	}; // end if
}; // end if



framecount=0;
filecount=0;

// loop for file

for (fileloop=0;fileloop<numinputfile;fileloop++) {
	const int truefileloop=fileloop+2;

	int fileformat=0;

	// we reserve 15 bytes for dvtool header to read the ambe file.
	// a .ambe-file should start with "#C Version 1.0" which is 14 character + 1 for the \n
	char dvtool_header[15];

	// we start by opening the file in binary mode (for files in the .dvtool file
	// format).
	// If we notice the file in in .ambe (text) format, the file will be reopened
	// in text-mode
	if (! (infile = fopen(infilelist[truefileloop],"rb"))) {
		fprintf(stderr,"Error: Unable to open input file! %s\n",infilelist[truefileloop]);
		continue;
	}; // end if

	// read 10 bytes: header of .dvtool file
	// it should begin with 6 bytes containing "DVTOOL" in ascii
	ret=fread(dvtool_header,1,6,infile);

	// did we receive 10 bytes?
	if (ret < 6) {
		fprintf(stderr,"Error: Could not read header of input file while probing .dvtool format! %s\n",infilelist[truefileloop]);
		fclose(infile);
		continue;
	}; // end if

	if (strncmp(dvtool_header,"DVTOOL",6) == 0) {
		fileformat=1;

		// skip to position 68 of file: 10 octets for header + 58 octets for
		// configuration frame
		// We are at position 6
		fseek(infile,62,SEEK_CUR);

	} else {
		// OK, it's not a .dvtool file, perhaps it's a .ambe file. That should
		// start with a line containing '#C Version 1.0"

		// return to beginning of file
		fseek(infile,0,SEEK_SET);

		// read next of line, up to \n, maximum 14 characters
		ret=fread(dvtool_header,1,14,infile);

		if (ret < 14) {
			fprintf(stderr,"Error: Could not read header of input file while probing .ambe format! %s\n",infilelist[truefileloop]);
			fclose(infile);
			continue;
		}; // end if

		if (strncmp(dvtool_header,"#C Version 1.0",14) == 0) {
			fileformat=2;
			// close file, re-open in "text" mode.
			if (! (infile = freopen(infilelist[truefileloop],"r",infile))) {
				fprintf(stderr,"Error: Unable to open input file! %s\n",infilelist[truefileloop]);
				continue;
			}; // end if
		}; // end if
	}; // end else - if

	if (fileformat == 0) {
		fprintf(stderr,"Error: unknown fileformat for file %s.\n",infilelist[truefileloop]);
		fclose(infile);
		continue;
	}; // end if

                
	if (global.verboselevel >= 1) {
		fprintf(stderr,"Streaming file %s (fileformat %s) ...\n",infilelist[truefileloop],fileformatdescr[fileformat]);
	}; // end if

	// file is OK
	filecount++;

	// read data from wav-file
	stop=0;

	while (stop == 0) {
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
							memcpy(global.ambebuffer,&dvtool_framein[17],9);
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
				// .ambe file format

				ret=readambefile(infile,global.ambebuffer);

				if (ret == 9) {
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
			// we cannot send data -> sleep for 2 ms
			usleep(2000);
		}; // end else - if (data send ready?)

	}; // end while (stop, file is read)

	// close ambe-file
	fclose(infile);

}; // end for (all input files)


// no data to send anymore. End-of-file reached of wav-file
// we set the value directly to 3 to make it compatible with the udpsend function of
// wavstream
global.allpacketsend=3;

// wait for 60 ms (3 frames) to give the last frame of the file to go through all the buffer stages
usleep(60000);

while (global.welive_udpsend) {
	// not yet done, wait 5 ms
	usleep(5000);
}; // end while

// now stop interupt timer udpsend
its.it_value.tv_sec = 0;
its.it_value.tv_nsec = 0; // timer = 0 -> stop timer
its.it_interval.tv_sec = its.it_value.tv_sec;
its.it_interval.tv_nsec = its.it_value.tv_nsec;

ret=timer_settime(timerid, 0, &its, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime-stop timer udpsend!\n");
	exit(-1);
}; // end if

// stop interupt dextraheartbeat
its2.it_value.tv_sec = 0;
its2.it_value.tv_nsec = 0; // timer = 0 -> stop timer
its2.it_interval.tv_sec = its2.it_value.tv_sec;
its2.it_interval.tv_nsec = its2.it_value.tv_nsec;

ret=timer_settime(timerid2, 0, &its2, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime-stop timer dextraheartbeat!\n");
	exit(-1);
}; // end if


// OK, we are finished
fprintf(stderr,"done: %d frames encoded\n", framecount);

// done
exit(0);
}; // end main


