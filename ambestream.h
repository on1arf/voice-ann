
// ambestream: stream out ambe-files to UDP port 20000

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
// 27 jan. 2011: version 1.1.2 Initial release
// 27 mar. 2011: version 1.2.1 code cleanup, auto-break, dextra linking


// maximum number of input files that can be streamed at once
#define MAXINFILE 100

// maximum repeat is 4 seconds (200 frames)
#define MAXBREAKREPEAT 200


//////// start global data 

// global data: used to communicate between threads

// global data, used for communication between the "timer-
// interrupt" threads: udpsend

typedef struct {

// datatosend and datasendready, used for syncronisation between the main
// program and "serialsend"
int datatosend;
int datasendready;

// allpacketsend 
int allpacketsend;
int welive_udpsend;

// broadcast-message: passed from main program to udpsend
char * bcmsg;

// ambebuffer, used to transport information from main program to udpsend
// contains ambe-data (3600 bps (2400 voice + 1200 fec) = 9 bytes
// per 20 ms frame
unsigned char ambebuffer[9];

// information extracted by main thread, used by udpsend
char repeatername[6];
int module;
int destport;
struct sockaddr * ai_addr;

int verboselevel;

int breakinterval;
int breaklength;
int breakrepeat;

char mycall[7];

char * sourceip;
struct sockaddr_in * sourceip_addr;

// socket
int udpsd;

} globaldatastr; // define global data structure

globaldatastr global; // the global data itself



///////// start data structure configuration

