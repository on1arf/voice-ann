/////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////
// function serialsend: called by timer every 20 ms

// Release info:
// version 0.x.x: Initial release
// version 0.2.1: 27 Mar. 2011: moved "correction" to DEBUG

static void funct_serialsend (int sig) {
	int ret;



#ifndef WAV2AMBE
#ifndef AMBE2WAV
		fprintf(stderr,"Error: WAV2AMBE / AMBE2WAV not set in funct_serialsend!!!\n");
		return();
#endif
#endif

	static int init=1; // init is set only the first time the function is called

	// do this only once
	if (init == 1) {
		global.welive_serialsend=1;
		init=0;
	}; // end if

	// sending ambe-frame or empty pcm-frame (to keep the dvdongle busy)
#ifdef WAV2AMBE
		ret=write (global.donglefd,ambe_configframe,50);
#endif

#ifdef AMBE2WAV
		ret=write (global.donglefd,&empty_pcmbuffer,322);
#endif


	// allpacketsend can have the following values:
	// 0: normal packets during a stream
	// 1: the last frame of the stream has been send -> now send a stop
	// 2 and above not used here

	if (global.allpacketsend < 1) {
		// we should have data to send

		// but do we?
		if (global.datatosend == 1) {
		// yes!

#ifdef WAV2AMBE
				ret=write (global.donglefd,&pcmbuffer,322);
#endif

#ifdef AMBE2WAV
				ret=write (global.donglefd,&ambe_dataframe,50);
#endif

			global.datasendready=0;

		} else {
		// nope, send empty pcmbuffer
#ifdef WAV2AMBE
				ret=write (global.donglefd,&empty_pcmbuffer,322);
#endif

#ifdef AMBE2WAV
				// ambe_configframe contains all 0 for AMBE-part
				ret=write (global.donglefd,&ambe_configframe,50);
#endif

			if (global.verboselevel >= 1) {
				fprintf(stderr,"Warning, serialsend input buffer empty\n");
			}; // end if
		}; // end if (datatosend)

		
	} else if (global.allpacketsend == 1) {
	// last frame has already been send, so send a "stop" command
		ret=write(global.donglefd,command_stop,5);
		global.allpacketsend=2;
		global.datatosend=0;
		global.welive_serialsend=0;
	};

	// ok, we can receive the next frame
	global.datasendready=1;
}; // end function funct_serialsend
