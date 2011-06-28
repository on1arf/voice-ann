/////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////
// function serialsend: called by timer every 20 ms
static void funct_serialsend (int sig) {
// static variables
// As this function is restarted every 20 ms, the information in the
// local variables is lost, except if it is defined as static

static int init=0;
static int done=0;

// other variables
int ret;

if (init == 0) {
// init some variables
	global.welive_serialsend=1;
}; // end if


if (done == 0) {

	// sending ambe-packet (to keep the dvdongle busy)
	ret=write (global.donglefd,ambe,50);

	// allpacketsend can have the following values:
	// 0: normal packets during a stream
	// 1: the last frame of the stream has been send -> now send a stop
	// 2 and above not used here

	if (global.allpacketsend < 1) {
		// we should have data to send

		// but do we?
		if (global.datatosend == 1) {
		// yes!
			ret=write (global.donglefd,&pcmbuffer,322);
			global.pcmvalid=1;

			global.datasendready=0;

		} else {
			if (global.verboselevel >= 1) {
				fprintf(stderr,"Warning: serialsend did not receive any data. Sending silence\n");
			}; // end if

			ret=write (global.donglefd,&empty_pcmbuffer,322);
			global.pcmvalid=0;
					

		}; // end if (datatosend)

		
	} else if (global.allpacketsend == 1) {
	// last frame has already been send, so send a "stop" command
		ret=write(global.donglefd,command_stop,5);
		global.pcmvalid=1;
		global.allpacketsend=2;

		// done, set all flags
		global.datatosend=0;
		done=1;
		global.welive_serialsend=0;
	};

	// ok, we can receive the next frame
	global.datasendready=1;

}; // end if (done=0);

}; // end function funct_serialsend
