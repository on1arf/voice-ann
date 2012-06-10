/* wavstream.h */

// wavstream: convert wav-files to .ambe-files and stream it out
// via a udp-stream to the local repeater, a remote repeater or a
// X-reflector


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
// 27 mar. 2011: version 0.2.1. Code cleanup, dextra linking, auto-break

// global DEFINEs
#define PCMBUFFERSIZE 320
// ringbuffersize: see remarks below
// THis value indicates the number of frames:
// 6000 = 50 frames per second * 120 seconds (2 minutes)
#define RINGBUFFERSIZE 6000

// maximum number of input files that can be streamed at once
#define MAXINFILE 100

// maximum repeat is 4 seconds (200 frames)
#define MAXBREAKREPEAT 200

//////// start global data 
// global data: used to communicate between threads
// The global data is initialy filled in by the main part of the process
// and then send to the child-processes as parameter during startup

// But first we define the ringbuffercell-structure which is needed by
// the ringbuffer used in the global data-structure

// ringbuffer, used to transport information from serialreceive to udpsend
// 10 bytes per 20 ms frame
// contains ambe-data (3600 bps (2400 voice + 1200 fec) = 9 bytes 
// + 1 integer containing the allbytesend variable at time of arrival in
// s_serialreceive
typedef struct {
	unsigned char ambedata[9];
	int allpacketsend;
} ringbuffercell;


typedef struct {
// file-descriptor for the serial-port to the dvdongle
int donglefd;
int udpsd;

struct sockaddr * ai_addr;

// allpacketsend: serves as "status-indication" between the different threads
int allpacketsend;
// flag that indicate if a posix-thread is still running or not
int welive_serialsend;
int welive_serialreceive;
int welive_udpsend;

int datatosend;
int datasendready;

// broadcast-message: passed from main program to udpsend
char * bcmsg;

// ringbuffercell structure defined above
ringbuffercell ringbuffer[RINGBUFFERSIZE];
int ringbufferlow;
int ringbufferhigh;

// information extracted by main thread, used by udpsend
char repeatername[6];
int module;
char mycall[6];

int destport;
int destport2;

int verboselevel;

int pcmvalid;

// break values
int breakinterval;
int breaklength;
int breakrepeat;

// dextra
int linktodextra;

// source ip address
char * sourceip;

} globalstr; // posix thread global data structure

// we just created the structure, now use it
globalstr global;


// packets sent to dongle: used by main thread and serialreceive
// starts with two bytes containing length of packet and packet type

// command to dongle: request name
const unsigned char request_name[] = {0x04, 0x20, 0x01, 0x00};
// command to dongle: start
const unsigned char command_start[] = {0x05, 0x00, 0x18, 0x00, 0x01};
// command to dongle: stop
const unsigned char command_stop[] = {0x05, 0x00, 0x18, 0x00, 0x02};

// ambe config-packet: does not contain any actual ambe-data (last
// 24 bytes) but is used to configure the ambe-encoder
// see http://www.moetronix.com/dvdongle and
// http://www.dvsinc.com/products/a2020.htm for more info
const unsigned char ambe[] = {0x32, 0xA0,0xEC, 0x13, 0x00, 0x00, 0x30, 0x10, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

///////// End of global data



///////// start data structure configuration

// structure of PCM-frame send to / received from dongle
struct {
	char buffsize1;
	char buffsize2;
	char pcm[PCMBUFFERSIZE];
} pcmbuffer;

// empty pcm buffer
struct {
	char buffsize1;
	char buffsize2;
	char pcm[PCMBUFFERSIZE];
} empty_pcmbuffer;


///////// end data structure configuration
