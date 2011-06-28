// //////////////////////////////////////
// //////////////////////////////////////
// function serial_port receive        //
// //////////////////////////////////////
// This process receives data from the //
// serial port and save it in a file   //
// It is run as a thread               //
// //////////////////////////////////////


#include "serialframe.h"

void * funct_serialreceive (void * pglobalin) {
globalstr *pglobal = (globalstr *) pglobalin;

// copy global data
//pglobal = (globalstr *) pglobalin;

// local vars
int stop;
unsigned char serialinbuff[8192];

stop=0;



while (stop == 0) {
	uint16_t plen_i;
	uint16_t ptype;

	int ret;


	// get next frame, correction is enabled, 2 ms of delay if no data
	// and 1 second timeout (500 times 2 ms)
	ret=serialframereceive(pglobal->donglefd,serialinbuff,&ptype,&plen_i,1,2000,500);


	// we are only interested in ambe data (ptype = 5, length = 48 bytes)
	// also ignore invalid frames
	if ((ptype == 5) && (plen_i == 48) && (pglobal->pcmvalid)) {
		// write data in ringbuffer
		int newhighmarker;
		int oldhighmarker;

		// ringbuffer high-marker is "write" pointer
		oldhighmarker = pglobal->ringbufferhigh;
		newhighmarker = oldhighmarker+1;

		// wrap high-marker if end of ringbuffer reached
		if (newhighmarker >= RINGBUFFERSIZE) {
			// the ringbuffer has reached the end, we store the next frame in the
			// beginning of the buffer. now "high-marker" become 1 as it points just
			// "above" the buffer-cell that is used
			newhighmarker=1; 
			oldhighmarker=0;
		}; // end if

		// is there still place left in the ringbuffer
		if (newhighmarker !=  pglobal->ringbufferlow)
		{
		// yep, copy ambe-frame to ringbuffer
			memcpy(&pglobal->ringbuffer[oldhighmarker].ambedata,&serialinbuff[24],9);
			pglobal->ringbuffer[oldhighmarker].allpacketsend=pglobal->allpacketsend;
			

			pglobal->ringbufferhigh = newhighmarker;
		} else {
			if (pglobal->verboselevel >= 1) {
				fprintf(stderr,"Warning: serialsend could not store DVframe in ringbuffer. Dropping frame!\n");
			}; // end if
		}; // else - end if

	}; // end if (ambe packet received)


	// was this the last packet and was the stop send?
	if ((pglobal->allpacketsend == 2) && (ptype == 5) && (pglobal->pcmvalid)) {

		// yes, set marker to 3 and stop
		pglobal->allpacketsend=3;
		stop=1;

	}; // end if (lastpacket)

}; // end while (stop)


// mark thread as done
pglobal->welive_serialreceive = 0;

return(0);
}; // end if




