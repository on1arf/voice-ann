// //////////////////////////////////////
// //////////////////////////////////////
// function serial_port receive        //
// //////////////////////////////////////
// This process receives data from the //
// serial port and save it in a file   //
// It is run as a thread               //
// //////////////////////////////////////


#ifndef WAV2AMBE
#ifndef AMBE2WAV
	fprintf(stderr,"Error: WAV2AMBE / AMBE2WAV not set in funct_serialreceive!!!\n");
	return();
#endif
#endif



#include "serialframe.h"

void * funct_serialreceive (void * globaldatain) {
globaldatastr * pglobal;

// local data
int stop;

#ifdef WAV2AMBE
// fileformat
// can be 1 for .ambe files (text)
// or 2 for .dvtool files (binary)
FILE * fileout=NULL;
#endif

#ifdef AMBE2WAV
	// libsndfile: used to create .wav files
	SNDFILE * fileout=NULL;
	SF_INFO sfinfo;
	sf_count_t nsample;
#endif

#ifdef WAV2AMBE
// bcmsg: broadcast message to be inserted into slow-speed data 
// only used in wav2ambe
int bcmsglen;
// slow-data sequencecounter
int slowdata_sequence;
int slowdataframeid;
#endif

// some other vars
// all of them only used in wav2ambe
#ifdef WAV2AMBE
int cs=0, sec=0;

int ret;
int tmp1;
#endif


// total number of frames, is at least 1 (the start frame)
// only needed in wav2ambe
#ifdef WAV2AMBE
int dvtool_framecount=1;
#endif



unsigned char serialinbuff[8192];


#ifdef WAV2AMBE
// data-structures for .dvtool files
char dvtool_header[] = {0x44,0x56,0x54,0x4f,0x4f,0x4c,0x00,0x00,0x00,0x00,0x38,0x00,0x44,0x53,0x56,0x54,0x10,0x00,0x81,0x00,0x20,0x00,0x01,0x02,0xe8,0x4f,0x80,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xff,0xff};
int dvtool_headersize=68;

char dvtool_line[] = {0x1b,0x00,0x44,0x53,0x56,0x54,0x20,0x00,0x81,0x00,0x20,0x00,0x01,0x02,0xe8,0x4f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x2d,0x16};
int dvtool_linesize=29;
#endif


// slow-speed data:
// The slow speed data  is stored in the 3 last octets of every
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



// In this program , only the "20 bytes" free-text extension is implemented
// The 20 bytes of data are send in 4 "packets" of 5 octets each
// Each packet is distributed over two 3-bytes slow-speed frames in a DV-frame
// Each packet start with 1 octet containing (0x40 + sequence number) 
// followed by the 5 bytes of text itself. Unused space is filled up with spaces
// The remaining part of the slow-data superframe is filled up with "0x66"

// slowspeedata superframe
// Contains 63 octets, or 21 time 3 octets per DV-frame. 
// The first 3 of them (1 DV-frame) is fixed: the syncronisation-packet
// The last 36 (12 DV-frames) also is fixed and all contain 0x66 as "filler"
// In the definition of this array, the D-STAR scrambler is already applied
// for that data

// slowspeed-data is only used in wav2ambe

#ifdef WAV2AMBE
unsigned char slowdata_superframe[]= {0x55,0x2d,0x16,0x40,0x20,0x20,0x20,0x20,0x20,0x41,0x20,0x20,0x20,0x20,0x20,0x42,0x20,0x20,0x20,0x20,0x20,0x43,0x20,0x20,0x20,0x20,0x20,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5};

// slowspeedata superframe "nodata"
// contains only the syncronisation-packet and filled up with 0x66 as
// fillers (after scrambling, this becomes 0x16, 0x29, 0xf5)
unsigned char slowdata_superframe_nodata[]= {0x55,0x2d,0x16,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5,0x16,0x29,0xf5};
#endif


// import data from global data
pglobal = (globaldatastr *) globaldatain;

// mark thread as started
pglobal->welive_serialreceive = 1;

#ifdef WAV2AMBE
if ((pglobal->fileformat != 1) && (pglobal->fileformat != 2)) {
	fprintf(stderr,"Error: invalid file-format parameter in serialreceive: %d\n",pglobal->fileformat);
	// mark thread as finished
	pglobal->welive_serialreceive = 0;
	exit(-1);
}; // end if
#endif


// broadcast message
// This is stored in the slow-speed data part of the DV-frame
// This is only saved in binary .dvtool files

// slowspeed data is only used in wav2ambe
#ifdef WAV2AMBE

if (pglobal->fileformat == 2) {
	// copy from global-data

	if (pglobal->bcmsg == NULL) {
		bcmsglen=0;
		// slowdataframeid indicated if we send slowdata frames with
		// real data or only filler-frames
		slowdataframeid=-1;
		// file-structure is already pre-initialised with all 0x66 above
	} else {
		int bytesleft;
		int loop;

		bcmsglen=strlen(pglobal->bcmsg);

		if (bcmsglen > 20) {
		// length is maximum 20 bytes
			bcmsglen=20;
		}; // end if

		bytesleft=bcmsglen;

		slowdataframeid=0;

		for (loop=0;loop<4;loop++) {
			int bytesthispackage;

			if (bytesleft > 5) {
				bytesthispackage=5;
			} else {
				bytesthispackage=bytesleft;
			}; // end else - if


			// store data (if there is data left)
			if (bytesleft > 0) {
				memcpy(&slowdata_superframe[4+loop*6],&pglobal->bcmsg[loop*5],bytesthispackage);
				bytesleft -= bytesthispackage;
			}; // end if

		}; // end for

		// apply standard D-STAR scrambler
		// This is done by executing a exor on every data-set of 3 bytes
		// This is based on code from Scott KI4KLF and Jonathan G4KLX
		// I don't know why or how it works, but it seams to do its job and
		// that's fine for me :-)

		// We only need to apply this on bytes 3 up to 27 as for the other parts
		// the D-STAR scrambler has already been apply when initialising the
		// structure 

		for (tmp1=0;tmp1<8;tmp1++) {
			slowdata_superframe[3+tmp1*3] ^= 0x70;
			slowdata_superframe[4+tmp1*3] ^= 0x4f;
			slowdata_superframe[5+tmp1*3] ^= 0x93;
		}; // end for

	}; // end else - if 

}; // end if (fileformat == 2)

#endif


stop=0;


if (pglobal->fname_out == NULL) {
	fprintf(stderr,"Error: output filename unknown!\n");
} else {

#ifdef WAV2AMBE
// binary or ascii-file for wav2ambe
	if (pglobal->fileformat == 1) {
	// ambe files are text-files
		fileout=fopen(pglobal->fname_out,"w");
	} else {
	// dvtool files are binary files
		fileout=fopen(pglobal->fname_out,"wb");
	}; // end else - if
#endif

// create output wav-file for ambe2wav
#ifdef AMBE2WAV
	sfinfo.samplerate=8000;
	sfinfo.channels=1;
	sfinfo.format=SF_FORMAT_WAV | SF_FORMAT_PCM_16; // MS-WAV, 16 bit, little-endian

	fileout=sf_open(global.fname_out,SFM_WRITE,&sfinfo);
#endif


}; // end else - if

if (fileout == NULL) {
	fprintf(stderr,"Unable to open output file! %s\n",pglobal->fname_out);
	pglobal->welive_serialreceive = 0;
	exit(-1);
}; // end if


// write header in output file

// only used for wav2ambe
#ifdef WAV2AMBE

if (pglobal->fileformat == 1) {
	// AMBE file-format

	// Info: lines beginning with "#" are comment and can be ignore.
	// however, for future expansion, the follow lines have be reserved:
	// #C Version: version of format: currently "1.0"
	// #C Name: name of file
	// #C Info: any additional info that can be usefull
	ret=fprintf(fileout,"#C Version 1.0\n");


} else {
	// DVTOOL file-format
	// Info: .dvtool files begin with a fixed 10 byte header (containing
	// "DVTOOL" + 4 bytes lenth) + 1 start-frame of 58 bytes
	// The length-indication (bytes 6 up to 9) are now left blank
	// and will be filled up at the end of the program
	
	// write header to outfile
	ret=fwrite(dvtool_header,dvtool_headersize,1,fileout);
}; // end while

if (ret == 0) {
	fprintf(stderr,"Error writing to output file: %s!\n",pglobal->fname_out);
	pglobal->welive_serialreceive = 0;
	exit(-1);
}; // end if

#endif


// init some vars
#ifdef WAV2AMBE
slowdata_sequence = 0;
#endif

///////////////////////////
// ///// MAIN LOOP ///// //
///////////////////////////

while (stop == 0) {
	uint16_t plen_i;
	uint16_t ptype;

	int ret;


	// get next frame, correction is enabled, 2 ms of delay if no data
	// and 1 second timeout (500 times 2 ms)
	ret=serialframereceive(pglobal->donglefd,serialinbuff,&ptype,&plen_i,1,2000,500);


// wav2ambe: look for ambe data
#ifdef WAV2AMBE
	// we are only interested in ambe data (ptype = 5, length = 48 bytes)
	if ((ptype == 5) && (plen_i == 48)) {
		// write data in output file

		if (pglobal->fileformat == 1) {
		// .ambe file
			fprintf(fileout,"%05d %02d %02x%02x%02x%02x%02x%02x%02x%02x%02x\n",sec,cs,serialinbuff[24],serialinbuff[25],serialinbuff[26],serialinbuff[27],serialinbuff[28],serialinbuff[29],serialinbuff[30],serialinbuff[31],serialinbuff[32]);

			cs += 2;

			if (cs >= 100) {
				sec++;
				cs=0;
			}; // end if
		} else {
		// .dvtool file
			dvtool_framecount++;

			// copy 9 octets ambe-data: octets 24 up to 32 of the ambe-frame 
			// go into octets 17 to 25 of the dvtool line-frame
			// note: all octets are counted started from 0
			memcpy((void *) &dvtool_line[17],&serialinbuff[24],9);


			// slow-speed data
			// copy octets of slow-speed data to octets 26 to 28 of dvtool line-frame

			// slow data frameid = 0, send slowspeed frame with real data
			if (slowdataframeid == 0) {
				memcpy((void *) &dvtool_line[26],&slowdata_superframe[slowdata_sequence*3],3);
			} else {
			// send slowspeed frame with "filler" data
				memcpy((void *) &dvtool_line[26],&slowdata_superframe_nodata[slowdata_sequence*3],3);
			}; // end else - if

			// increase slowdata-sequence-counter, wrap around at 21
			slowdata_sequence++;

			if (slowdata_sequence > 20) {
				slowdata_sequence=0;

				// wrap slowdata frameid
				// if at -1, always send slow-speed data "filler"

				// If at 0, send broadcast-text slow-speed data
				// If at >0, send filler slow-speed data
				// Wrap at 8, so the broadcast-text will be repeated every 8 superframe

				if (slowdataframeid >= 0) {
					// increase slow-data frameid, if not set to "always filler"
					if (slowdataframeid >= 0) {
						slowdataframeid++;
					}; // end if

					if (slowdataframeid >= 8) {
						slowdataframeid=0;
					}; // end if

				}; // end if

			}; // end if

			// write line to file
			if (fileout != NULL) {
				fwrite(dvtool_line,dvtool_linesize,1,fileout);
			}; // end if

			// Octet 16 of the dvtool line-frame contains a sequence-number
			// wrap around at 0x14 (=20)
			dvtool_line[16]++;

			if (dvtool_line[16] > 0x14) {
				dvtool_line[16]=0x00;
			}; // end if

		}; // end else - if (file format)
		
	}; // end if (ambe packet received)
#endif

// ambe2wav: look for PCM frame
#ifdef AMBE2WAV
	// we are only interested in pcm frames data (ptype = 4, length = 320 bytes)
	if ((ptype == 4) && (plen_i == 320)) {
		// write data in output file
		nsample=sf_write_short(fileout,(short *) serialinbuff,160);

		if (nsample < 160) {
			fprintf(stderr,"Error: could not write 160 samples in output file %s",global.fname_out);
		}; // end if

	}; // end if
#endif

	// was this the last packet and was the stop send?
	// allpacketsend is set to 2 by serialsend when the last frame has been send
	if (pglobal->allpacketsend == 2) {

		// yes, set marker to 4 and stop
		pglobal->allpacketsend=3;
		stop=1;

	}; // end if (lastpacket)

}; // end while (stop)



// Main loop is terminated, now close files

#ifdef WAV2AMBE
// same additional things to do for .dvtool files
if ((fileout != NULL) && (pglobal->fileformat == 2)) {
	uint32_t framecount_smallendian;

	// for the .dvtool file-format, we need to do two more things:
	// - send a termination-frame with all ambe-data to 0
	// and a sequence-number at normal + 0x40
	// - addapt the number-of-records counter in the beginning of the file

	// Termination-frame: 
	// clean all ambe data
	memset(&dvtool_line[17],0,9);

	// set "end-of-file" marker in sequence-number
	dvtool_line[16]+=0x40;

	// write line
	fwrite(dvtool_line,dvtool_linesize,1,fileout);
	
	// number-of-records counter
	fseek(fileout,6,SEEK_SET);

	framecount_smallendian=htonl(dvtool_framecount);
	fwrite((void *) &framecount_smallendian,4,1,fileout);

}; // end if (adapt .dvtool files)
#endif

// close fileout
if (fileout != NULL) {
#ifdef WAV2AMBE
	fclose (fileout);
#endif

#ifdef AMBE2WAV
	sf_close(fileout);
#endif
}; // end if

// mark thread as done
pglobal->welive_serialreceive = 0;

// done
return(0);
}; // end if



