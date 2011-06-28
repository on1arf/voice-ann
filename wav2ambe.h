
// wav2ambe: convert wav-files to ambe-files
// ambe2wav: convert ambe-files to wav-files

// this program requires a DVdongle to do AMBE encoding, connected to
// a USB-port
// this program requires the sndfile-libraries to import wav-files
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

//  release info:
// 17 okt. 2010: version 0.0.1. Initial release
// 28 dec. 2010: version 0.1.2. merge wav2ambe and ambe2wav

// global DEFINEs
#define PCMBUFFERSIZE 320

// maximum number of input files that can be en/decoded in one go
#define MAXINFILE 100

// global data-structure. This is used to communicate between threads
// The global data is initialy filled in by the main part of the process
// and then send to the child-processes as parameter during startup

typedef struct {
int donglefd;
int allpacketsend;

int welive_serialreceive;
int welive_serialsend;

int datatosend;
int datasendready;

int fileformat;

char * bcmsg;

int verboselevel;

char * fname_out;

} globaldatastr; // define global data structure

// global data itself
globaldatastr global;





// packets sent to dongle:
// starts with two bytes containing length of packet and packet type

// command to dongle: request name
const unsigned char request_name[] = {0x04, 0x20, 0x01, 0x00};
// command to dongle: start
const unsigned char command_start[] = {0x05, 0x00, 0x18, 0x00, 0x01};
// command to dongle: stop
const unsigned char command_stop[] = {0x05, 0x00, 0x18, 0x00, 0x02};

// ambe-packet: does not contain any actual ambe-data (last 24 bytes) but
// contains configuration-setting for ambe-encoder
// see http://www.moetronix.com/dvdongle and http://www.dvsinc.com/products/a2020.htm for more info
unsigned char ambe_configframe[] = {0x32, 0xA0,0xEC, 0x13, 0x00, 0x00, 0x30, 0x10, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#ifdef AMBE2WAV
struct {
	unsigned char length[2];
	unsigned char config[24];
	unsigned char data[24];
} ambe_dataframe;
#endif

// End of global data



// start data structure configuration

// As these structures as defined here, they are also global so can be used
// to communicate between the main thread and serialsend

// structure of PCM-frame send and received from/to dongle
struct {
	char buffsize1;
	char buffsize2;
	char pcm[PCMBUFFERSIZE];
} pcmbuffer;


// empty pcm-buffer
struct {
	char buffsize1;
	char buffsize2;
	char pcm[PCMBUFFERSIZE];
} empty_pcmbuffer;


// end data structure configuration
