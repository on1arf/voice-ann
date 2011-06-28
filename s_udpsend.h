
/////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////
// function udpsend: called by timer every 20 ms

// This function is shared by wavstream and udpstream
// Differences in code are controlled by the AMBESTREAM or
// WAVSTREAM defines


#include "crc.h"

static void funct_udpsend (int sig) {

// static variables
// As this function is restarted every 20 ms, the information in the
// local variables is lost, except when the variable is defined as
// static

// socket descriptor of udp socket + serverAddress structure
// created by main program, used by udpsend
// udpsd is now a global var as it is also used by dextra linking and heartbeat functions
// the socket is created by the main thread
static struct sockaddr_in6 * serverAddress;

// some other vars
static int init=0;
static int networkinit=0;
static int firstframe=1;
static int afterfirstframe=0;
static int framesmissed=0;
static int done=0;
static int slowdataframeid;

// counters for frames.
static int voiceframecounter=0;

// status
// 0: normal situation during streaming
// 1; break: breakinterval timer has run out, streaming is paused
// 2: break: stream has been restarted, sending repeat-buffer
static int status=0;

// break repeat buffer
static int breakframecounter=0;
static int breakrepeatcounter=0;
static unsigned char breakbuffer [9 * MAXBREAKREPEAT];
static int breakbufferlow; static int breakbufferhigh;


// slow-speed data:
// The slow speed data is stored in the 3 last octets of every
// DV-frame, just behind the 9 AMBE octets of voice-data
// The basic "shogen" DSTAR documentation only specifies two things:
// 1/ The 1ste frame and every consequative 21 frame contains syncronisation:
// - twice the "7 bit-sequence maximum-lenght sequence" 1101000 (read from LEFT
// to RIGHT, so 0001011)
// - followed by the 10 bit syncronisation-patern "1010101010" (read from LEFT
// to RIGHT, so 0101010101)
// In addition to that, the order of the 3 octets is revered.
// 2/ Before sending, the default D-STAR scrambler is applied to the slow-speed
// data. This can be emulated by doing a exor operation on the 3 bytes of
// the slow-speed data in every DV-frame with the values 0x70, 0x4f and 0x93
// (for octets 1 to 3 of the slow-speed data-frame)
// The D-STAR scrambler is NOT applied to the syncronisation-byte

// ICOM has implemented an extension to the slow-speed data protocol, which has
// been reversed by Jonathan G4KLX and Denis Bederov DL3OCK
// See this URL for more info: 
// http://www.qsl.net/kb9mwr/projects/voip/dstar/Slow%20Data.pdf


// In this program , only the "20 bytes free-text" extension is implemented
// The 20 bytes of data are send in 4 "packets" of 5 octets each
// Each packet is distributed over two 3-bytes slow-speed frames in a DV-frame
// Each packet start with 1 octet containing (0x40 + sequence number) 
// followed by the 5 bytes of text itself. Unused space is filled up with spaces
// The remaining part of the slow-data superframe is filled up with "0x66"

// slowspeedata superframe
// Contains 63 octets, or 21 time 3 octets per DV-frame. 
// The first 3 of them (1 DV-frame) is fixed: the syncronisation-packet
// The last 36 (12 DV-frames) is fixed and all contain 0x66 as "filler"
// In the definition of this array, the D-STAR scrambler is already applied
// for that data
static unsigned char slowdata_superframe[]= {0x55,0x2d,0x16,0x40,0x20,0x20,0x20,0x20,0x20,0x41,0x20,0x20,0x20,0x20,0x20,0x42,0x20,0x20,0x20,0x20,0x20,0x43,0x20,0x20,0x20,0x20,0x20,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5};

// slowspeedata superframe "nodata"
// contains only the syncronisation-packet and filled up with 0x66 as
// fillers (after scrambling, this becomes 0x16, 0x29, 0xf5)
static const unsigned char slowdata_superframe_nodata[]= {0x55,0x2d,0x16,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5};


// voice-frame
// we start with a default packet-skeleton and then fill in the fields
// The description of the voice-frames can be found 
static unsigned char voiceframe[]={0x44,0x53,0x56,0x54,0x20,0x00,0x00,0x00,0x20,0x00,0x01,0x01,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// streamid is a 2 byte random-number
// streamid_i and streamid_c point to the same data
static int streamid_i; 
static unsigned char * streamid_c;






// function begins here

if (done == 0) {
	global.welive_udpsend=1;

	// initialisation: is started at the beginning of the program or during just after a break

	if (init == 0) {
		// this part is started the first time this function gets called

		struct timeval now;
		struct timezone tz;

		int bcmsglen;

		init=1;

		if (networkinit == 0) {
		// network part and text-message data structures are only executed at the very first time
		// this function is called
			networkinit = 1;

			// UDP socket has been created by main thread
			if (global.udpsd == 0) {
				fprintf(stderr,"Error: main thread does not give valid UDP socket id \n");
				exit(-1);
			}; // end if

//			// ai_addr was created in main thread
			serverAddress = (struct sockaddr_in6 *) global.ai_addr;

			if (serverAddress == NULL) {
				fprintf(stderr,"Error: main thread does not give valid ai_addr \n");
				exit(-1);
			}; // end if

			serverAddress->sin6_port = htons((unsigned short int) global.destport);  


			// create data-structures for text message

			if (global.bcmsg == NULL) {
				// no text to send
				// so we will only send "filler" frames
				slowdataframeid=-1;

			} else {
				int bytesleft, loop; // needed below

				bcmsglen=strlen(global.bcmsg);

				if (bcmsglen > 20) {
				// length is maximum 20 bytes
					bcmsglen=20;
				}; // end if

				slowdataframeid=0;

				bytesleft=bcmsglen;

				// store broadcast message in slowspeed_superframe structure
				for (loop=0;loop<4;loop++) {

					// Store up to 5 bytes
					int bytesthispackage;

					if (bytesleft > 5) {
						bytesthispackage=5;
					} else {
						bytesthispackage=bytesleft;
					}; // end else - if

					// store data (if there is data left)
					if (bytesleft > 0) {
						memcpy(&slowdata_superframe[4+loop*6],&global.bcmsg[loop*5],bytesthispackage);
						bytesleft -= bytesthispackage;
					}; // end if

				}; // end while


				// apply standard D-STAR scrambler
				// This is done by executing a exor on every data-set of 3 bytes
				// This is based on code from Scott KI4KLF and Jonathan G4KLX
				// I don't know why or how it works, but it seams to do its job and
				// that's fine for me :-)

				// We only need to apply this on bytes 3 up to 27 as for the other
				// parts the D-STAR scrambler has already been applied before when
				// initialising the structure 

				for (loop=0;loop<8;loop++) {
					slowdata_superframe[3+loop*3] ^= 0x70;
					slowdata_superframe[4+loop*3] ^= 0x4f;
					slowdata_superframe[5+loop*3] ^= 0x93;
				}; // end for

			}; // else - if  (text message)

		}; // end if (networkinit)

		// streamid is (re)created at the beginning of the program AND after a restart
		// of a fragment

		// streamid: a random 16-bit counter
		// get current time, usec are used as seed for random
		gettimeofday(&now,&tz);
		srand(now.tv_usec);
		streamid_i=rand() & 0xffff;
		// make char-pointer point to integer;
		streamid_c=(unsigned char*)&streamid_i;

	}; // end if (init)




	// is there something to send?
#ifdef WAVSTREAM
	if (global.ringbufferlow != global.ringbufferhigh)
#endif
#ifdef AMBESTREAM
	if (global.datatosend == 1)
#endif
	{
		// yes

		// framesmissed: error-handling: terminate program when 100 frames (2 seconds) no data to be send
		// reset counter when data can be send
		framesmissed=0;

		// first (init) frame or later (voice) frame ???
		// is send the very first time of a stream and at the beginning of a new "break-fragment"
		if (((status == 0) || (status == 2)) && (firstframe == 1)) {
			// first frame: contains D-star addressng info
			int loop;
			int ret;

			// start-packet (56 bytes). This contains the addressing data for DSTAR
			// frame format:
			// octets  0- 3: "DSVT"
			// octet   4:    0x10 (= configuration)
			// octets  5- 7: 0x00, 0x00, 0x00 (don't know why) 
			// octet   8:    0x20 (= voice)
			// octets  9-11: 0x00, 0x01, 0x01 (don't know why)
			// octets 12-13: streamid (random, 16 bits)
			// octet  14   : 0x80
			// octet  15-17: 0x00, 0x00, 0x00 (flag1, flag2, flag3)
			// octets 18-25: destination = repeatername + destination module (rpt1)
			// octets 26-33: departure = repeatername + 'G' (rpt2)
			// octets 34-41: companion = "CQCQCQ  " (your)
			// octets 42-49: own1 = repeatername (my)
			// octets 50-53: own2 = "MSG " (my)
			// octets 54-55: p_fcs (packet frame checksum)

			// voice-frame packet-skeleton has been configured above
			// now fill in the remaining parts.

			// we start with a default packet-skeleton and then fill in the fields
			unsigned char startframe[]={0x44,0x53,0x56,0x54,0x10,0x00,0x00,0x00,0x20,0x00,0x01,0x01,0xff,0xff,0x80,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x43,0x51,0x43,0x51,0x43,0x51,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x4D,0x53,0x47,0x20,0xff,0xff};

			// framecounter, reset first
			voiceframecounter=0;
			breakframecounter=0;

			// streamid
			memcpy(&startframe[12],streamid_c,2);

			// also copy streamid to voice-frame structure (to be used for the 
			// normal DV-packets
			memcpy(&voiceframe[12],streamid_c,2);

			// repeatername: used in rpt2, rpt1 and my1
			for (loop=0;loop<strlen(global.repeatername);loop++) {
				char c;

				c=toupper(global.repeatername[loop]);
				startframe[18+loop]=c; // rpt2
				startframe[26+loop]=c; // rpt1
			}; // end for

			// my
			for (loop=0;loop<strlen(global.mycall);loop++) {
				char c;

				c=toupper(global.mycall[loop]);
				startframe[42+loop]=c; // my1
			}; // end for


			// rpt2: 8th character is destination module
			startframe[25]=(unsigned char)global.module;

			// rpt1: 8th character is 'G'
			startframe[33]='G';

			// all of the start-frame is now filled-in
			// generate checksum
			dvframe_crc(startframe);

			// now send frame
			// send if 5 times (as that is how i-com does it)
			ret=sendto(global.udpsd, startframe, 56, 0, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
			ret=sendto(global.udpsd, startframe, 56, 0, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
			ret=sendto(global.udpsd, startframe, 56, 0, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
			ret=sendto(global.udpsd, startframe, 56, 0, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
			ret=sendto(global.udpsd, startframe, 56, 0, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));

			firstframe=0;
			afterfirstframe=0;

		} else if ((status == 0) || (status == 2)) {
			// status 0 is normal operations. Based on earlier "if" above, we know
			// this is a normal voice traffic -> send it

			// status = 2, the repeater is re-transmitting the last x. frames
			// Based on earlier "if" above, we know this is normal DV traffic -> send it


			int ret;
#ifdef WAVSTREAM
			int new_ringbufferlow;
#endif

			// voice-packet (27 bytes). This contains 20 ms of voice-data for DSTAR
			// frame format:
			// octets  0- 3: "DSVT"
			// octet   4:    0x20 (= voice)
			// octets  5- 7: 0x00, 0x00, 0x00 (don't know why) 
			// octet   8:    0x20 (= voice)
			// octets  9-11: 0x00, 0x01, 0x01 (don't know why)
			// octets 12-13: streamid (random, 16 bits)
			// octet  14   : framecounter (goes from 0 to 20)
			// octet  15-23: ambe-data 
			// octets 24-26: slow-speed data

			// a skeleton of the voice-frame is defined at the level of the function
			// the streamid is filled in during streaming-start (above)
			// just copy the voice-data, slow-speed data and framecounter


			// ambe-data
			if (status == 0) {

#ifdef WAVSTREAM
				memcpy(&voiceframe[15],&global.ringbuffer[global.ringbufferlow].ambedata,9);
				memcpy(&breakbuffer[9*breakbufferhigh],&global.ringbuffer[global.ringbufferlow].ambedata,9);
#endif

#ifdef AMBESTREAM
				memcpy(&voiceframe[15],global.ambebuffer,9);
				memcpy(&breakbuffer[9*breakbufferhigh],global.ambebuffer,9);
#endif
				// move up breakbufferhigh
				breakbufferhigh++;

				if (breakbufferhigh >= MAXBREAKREPEAT) {
					breakbufferhigh=0;
				}; // end if

			} else {
			// status = 2, copy from breakbuffer
				memcpy(&voiceframe[15],&breakbuffer[9*breakbufferlow],9);

				breakbufferlow++;

				if (breakbufferlow >= MAXBREAKREPEAT) {
					breakbufferlow=0;
				}; // end if

				// set status to 0 if all frames from breakbuffer send
				if (breakbufferlow == breakbufferhigh) {
					if (global.verboselevel >= 1) {
						fprintf(stderr,"streaming status normal\n");
					}; // end if
					status=0;
				}; // end if
			}; // end if 


			voiceframe[14]=(unsigned char)voiceframecounter;


			// increase framecounter by 0x40 if last frame of stream

			// possibility 1: end of file reached
			// for wavstream, do check on value of "allpacksend" in ringbuffer, as
			// stored when the frame was received by s_serialreceived
			// only do this check for status=0 (normal file reading)

			// for ambestream, input-file read is stopped during (status != 0) and
			// there is no buffering in the ringbuffer, so no additional checks are needed

#ifdef WAVSTREAM
			if ((global.ringbuffer[global.ringbufferlow].allpacketsend == 3) && (status == 0) )
#endif


#ifdef AMBESTREAM
			if (global.allpacketsend == 3) 
#endif
			{
				global.allpacketsend =4;
				voiceframe[14] += 0x40;
				done=1;
				global.welive_udpsend=0;
			} else if ((global.breakinterval) && ( breakframecounter >= global.breakinterval)) {
				// posibility 2: we have reached a break-interval
				// - > go to status 1 and terminate stream
					if (global.verboselevel >= 1) {
						fprintf(stderr,"streaming status break-wait\n");
					}; // end if 

					status=1;
					breakrepeatcounter=global.breaklength;

					voiceframe[14] += 0x40;

					// set breakbufferlow
					breakbufferlow=breakbufferhigh-global.breakrepeat;
					if (breakbufferlow < 0) {
						breakbufferlow += MAXBREAKREPEAT;
					}; // end if
			}; // end else - if


			// copy slow-speed data (fixed text)
			if (slowdataframeid == 0) {
				// send slowspeed frame with broadcast text
				memcpy(&voiceframe[24],&slowdata_superframe[voiceframecounter*3],3);
			} else {
				// send slowspeed frame with fillers
				memcpy(&voiceframe[24],&slowdata_superframe_nodata[voiceframecounter*3],3);
			};

			// now send udp-packet
			ret=sendto(global.udpsd, voiceframe, 27, 0, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));

			if (ret < 0) {
				// error
				fprintf(stderr,"udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
			}

			// mark as read only when status=0 (normal file reading)
			if (status == 0) {
#ifdef WAVSTREAM
				// calculate new lowwatermark
				new_ringbufferlow=global.ringbufferlow+1;
				if (new_ringbufferlow >= RINGBUFFERSIZE) {
					// wrapped -> restart at beginning of buffer
					new_ringbufferlow=0;
				}; // end if

				global.ringbufferlow = new_ringbufferlow;
#endif

#ifdef AMBESTREAM
				// packet is send, set markers
				global.datasendready=1;
				global.datatosend=0;
#endif
			}; // end if

			afterfirstframe=1;
	
		} else if (status == 1) {
		// status = 1, and -based on earlier tests above-, still need to wait
			breakrepeatcounter--;

			// move to status 0 (normal streaming) or 2 (repeating) when pause is over
			if (breakrepeatcounter <= 0) {

				if (global.breakrepeat >0) {
					status=2;
				} else {
					// no repeating
					status=0;
				}; // end if

				if (global.verboselevel >= 1) {
					if (status == 0) {
						fprintf(stderr,"streaming status normal \n");
					} else {
						fprintf(stderr,"streaming status break-repeat\n");
					}; // else - if
				}; // if end

				firstframe=1;

				init=0;
			}; // end if
		}; // else elseif - elseif - if

	} else {
		// no data to send

		// has the stream ended?
		if (global.allpacketsend == 3) {
		// yes, repeat last frame with "end" flag set
			int ret;

			global.allpacketsend=4;
			voiceframe[14]=(unsigned char)voiceframecounter;
			voiceframe[14] += 0x40;
			done=1;
			global.welive_udpsend=0;

			// resend previous udp-packet
			ret=sendto(global.udpsd, voiceframe, 27, 0, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));

			if (ret < 0) {
				// error
				fprintf(stderr,"udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
			}

		} else {
		// nope. We should have received a frame
		// count it as "missing".
	
			framesmissed++;

			if (framesmissed > 100) {
				// no data during 100 * 20 ms (= 2 second), stop
				if (global.verboselevel >= 1) {
					fprintf(stderr,"2 second timeout in udpsend!\n");
				}; // end if

				done=1;
				global.allpacketsend=4;
				global.welive_udpsend=0;
			}; // end if
		}; // end else - if

	}; // end else - if  (data to send)


	// counters
	breakframecounter++;

	if (afterfirstframe == 1) {
		// increase counters
		voiceframecounter++;
	}; // end if

	// voice: wrap at 20
	if (voiceframecounter > 20) {
		voiceframecounter=0;

		// wrap slowdata frameid
		// if at -1, always send slow-speed data "filler"

		// If at 0, send broadcast-text slow-speed data
		// If at >0, send filler slow-speed data
		// Wrap at 8, so the broadcast-text will be repeated every 8 superframe

		if (slowdataframeid >= 0) {
			slowdataframeid++;

			if (slowdataframeid >= 8) {
				slowdataframeid=0;
			}; // end if
		}; 
	}; // end if

}; // end if (done == 0)


}; // end function func_udpsend

