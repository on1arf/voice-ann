// //////////////////////////////////////
// //////////////////////////////////////
// function serialframereceive         //
// //////////////////////////////////////
// Reads one from from serial port     //
// if requested, error-correction is   //
// done                                //
// //////////////////////////////////////


int serialframereceive (int in_donglefd, unsigned char * in_buffer, uint16_t * in_p_ptype, uint16_t * in_p_plen, int in_correct, int in_delay, int in_maxmissed) {

int stop;
uint16_t plen_i;
uint16_t ptype;
char plen_c[2];

int totalreceived;

int missedreads;


int ret;
	

missedreads=0;
stop=0;

// This function needs to read one single frame from the dvdongle
// serial port. However, data is read in pieces:
// the first 2 bytes contain the length and the frame-type
// If necessairy, error-checks are done on these two fields
// then the rest of frame is read, also in multiple reads

while (stop == 0) {

	// read first 2 bytes (length + type)
	ret=read(in_donglefd,plen_c,2);

	// error-handling, no data received -> make it -1
	if (ret ==0) {
		ret=-1;
	}; // end if

	// data received?, we need at least 2 bytes
	if (ret >= 0) {
		int retrycount=0;

		// did we receive both bytes?
		if (ret ==1) {
		// nope, read 2nd byte

			ret=-1;
			// try maximum for 2 seconds (4000 * 0.5 ms)
			while ((ret < 1) && (retrycount < 4000)) {

				ret=read(in_donglefd,&plen_c[1],1);

				if (ret < 1) {
				// still not received? sleep of 0.5 ms
					retrycount++;
					usleep(500);
				}; // end if
			};// end while
		}; // end if

		if (ret < 1) {
			// we did not recieve a valid responds after 2 seconds
			// -> get out
			fprintf(stderr,"breaking out reading frame length!\n");
			ret=-1;
			break;
		}; // end if

		// reset the "missed reads" counter
		missedreads=0;


		// parse header
		plen_i=((uint8_t) (plen_c[1] & 0x1f) *256+ (uint8_t) plen_c[0]);
		
		// maximum size is 8192
		if (plen_i > 8191) {
			plen_i = 8192;
		}; // end if

		// subtract 2: (as header is included in plen)
		if (plen_i >= 2) {
			plen_i -= 2;
		} else {
			// something wrong, we should have a packet-length of at least 2
			ret=0;
			break;
		}; // end if

		ptype = (uint8_t) (plen_c[1] & 0xe0) >> 5;


#if DEBUG >= 1
		// error correction mode:

		// If the dongle is started and encoding and decoing of ambe-frames, 
		// we should only receive ambe-frame or pcm-frames
		// Sometimes the dongle returns a wrong frame-length value
		// this happens in case of a overrun or underrun of the
		// DVdongle, which is also shown by the red LED of the DVdongle
		// flashing

		// So do note that the "corrected" mode should only be used when it is
		// encoding/decoding ambe-frames, hence the "in_correct" value in
		// the function-call


		if ((in_correct == 1) && 
				(((ptype == 5) && (plen_i != 48)) || 
				((ptype==4) && (plen_i != 320)))) {
			// packet-length is not correct. Let's try to correct it


			// some local vars
			int reallength;
			int stop; 

			// the next part is only run if "stop" still at 0 (i.e. unknown length/type)
			// As we do not know this kind of frame, we will scan the stream until we
			// receive a plen/ptype combination that is valid

			// debug vars: realllength
			reallength=0;

			stop=0;

			while (stop == 0) {
				// shift bytes in plen_c
				plen_c[0]=plen_c[1];

				// read from device until 1 octet received
				ret=0;
				while (ret <1) {
					// ret can be 0 (no data) or 1 (1 octet received)
					ret=read(in_donglefd,&plen_c[1],1);

					if (ret == 0) {
						usleep(in_delay);
					};
				}; // end while

				// debug code
				reallength++;

				// recalculate plen_i and ptype
				plen_i=((uint8_t) (plen_c[1] & 0x1f) *256+ (uint8_t) plen_c[0]);
				if (plen_i >= 2) {
					plen_i -= 2;
				}; // end if

				ptype = (uint8_t) (plen_c[1] & 0xe0) >> 5;

				// If we have a new valid frame, set "stop" marker
				// next freame is ambe-frame (plen=48, ptype=5)
				if ((plen_i == 48) && (ptype == 5)) {
				// ambe frame
					stop=1;
				}; // end if

				// next freame is pcm-frame (plen=320, ptype=4)
				if ((plen_i == 320) && (ptype == 4)) {
				// pcm frame
					stop=1;
				}; // end if

			}; // end while

		}; // end if (error-correction)

#endif

                

		// read rest of frame, store it in character buffer pointed to by
		// in_buffer
		totalreceived=0;

		// "missed reads" counter: drop out after 2 seconds
		missedreads=0;
		while ((totalreceived < plen_i) && (missedreads < 2000)){

			ret=read(in_donglefd,&in_buffer[totalreceived],(plen_i-totalreceived));

			if (ret >= 1) {
				// reset missed reads
				missedreads=0;
				
				totalreceived += ret;
			} else {
				// sleep 1 ms
				missedreads++;
				usleep(1000);
			}; // end else - if

		}; // end while


		// return on timeout
		if (missedreads >= 2000) {
			fprintf(stderr,"TIMEOUT reading frame!!! \n");
			return(-1);
		}; // end if

		// frame is completely read
		stop=1;

	} else {
		// nothing received, wait for "delay" µs
		usleep(in_delay);

		// increase numher of times a read did not return any value
		missedreads++;

		if (missedreads > in_maxmissed) {
		// we have to many times received any data. Return with -1 (error);
			return(-1);
		}; // end if

	}; // end else - if (data received)

}; // end  while (stop)


// frame is completely read, return to main program

*in_p_ptype=ptype;
*in_p_plen=plen_i;
return(plen_i);
	
}; // end function serialframereceive
