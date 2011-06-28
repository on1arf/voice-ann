// //////////////////////////////////
// USAGE and HELP Functions       ///
// //////////////////////////////////

// Release-info:
// 27 Mar. 2011: version 0.2.1. Initial release

#ifdef WAV2AMBE
void usage (char * progname) {
	fprintf(stderr,"Usage: %s [-t broadcasttext] [-dngl device] [-v ] -f {a,d} -o outfile.ambe infile.wav [ infile.wav ... ] \n",progname);
	fprintf(stderr,"Usage: %s -V \n",progname);
	fprintf(stderr,"Usage: %s -h \n",progname);
}; // end function usage

void help (char * progname) {
	fprintf(stderr,"wav2ambe: convert any number of PCM-files (also known as .wav files) into a\n");
	fprintf(stderr,"  AMBE-encoded file\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage: %s [-t broadcasttext] [-dngl device] [-v ] -f {a,d} -o outfile.ambe infile.wav [ infile.wav ... ] \n",progname);
	fprintf(stderr,"\n");
	fprintf(stderr,"CLI Options:\n");
	fprintf(stderr,"-dngl: DVdongle serial device (Default: /dev/ttyUSB0).\n");
	fprintf(stderr,"-t: text-message included in AMBE-file. (only for .dvtool files)\n");
	fprintf(stderr,"-f: outputfile format: text-based .ambe files or binary .dvtool files\n");
	fprintf(stderr,"-v: verbose\n");
	fprintf(stderr,"-o: outputfile\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Wav2ambe supports up to %d input files that are read and encoded in sequence.\n",MAXINFILE);
	fprintf(stderr,"To read from standard in, use filename \"-\"\n");
	fprintf(stderr,"\n");
}; // end if

#endif


#ifdef AMBE2WAV
void usage (char * progname) {
	fprintf(stderr,"Usage: %s [-dngl device] [-v] [-g gain] -o outfile.wav infile.dvtool [ infile.ambe ... ]\n",progname);
	fprintf(stderr,"Usage: %s -V\n",progname);
	fprintf(stderr,"Usage: %s -h\n",progname);
}; // end function usage

void help (char * progname) {
	fprintf(stderr,"wav2ambe: convert any number of AMBE encoded audio-files into a PCM file (also\n");
	fprintf(stderr,"   known as .wav file)\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage: %s [-dngl device] [-v] [-g gain] -o outfile.wav infile.dvtool [ infile.ambe ... ]\n",progname);
	fprintf(stderr,"\n");
	fprintf(stderr,"CLI Options:\n");
	fprintf(stderr,"-dngl: DVdongle serial device (Default: /dev/ttyUSB0)\n");
	fprintf(stderr,"-v: verbose\n");
	fprintf(stderr,"-g: audio gain (0-255) (Default: 192)\n");
	fprintf(stderr,"-o: output file\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Wav2ambe supports up to %d input files that are read and decoded in sequence.\n",MAXINFILE);
	fprintf(stderr,"\n");
	fprintf(stderr,"Both the .ambe and .dvtool file-format is supported.\n");
	fprintf(stderr,"\n");
}; // end if

#endif



#ifdef AMBESTREAM
void usage (char * progname) {
fprintf(stderr,"Usage: %s [-t broadcasttext] [-v] [-4|-6] [-dxl] [-my CALLSIGN] [-d destination ] [-p port] [ -bi break-interval ] [ -bl break-length ] [ -br break-repeat ] name-of-repeater module infile.dvtool [ infile.ambe ....] \n",progname);
fprintf(stderr,"Usage: %s -h \n",progname);
fprintf(stderr,"Usage: %s -V \n",progname);

}; // end function usage

void help (char * progname) {
	fprintf(stderr,"ambestrea: stream out AMBE digitalvoice stream to a D-STAR repeater or\n");
	fprintf(stderr,"   reflector.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage: %s [-t broadcasttext] [-v] [-4|-6] [-dxl] [-my CALLSIGN] [-d destination ] [-p port] [ -bi break-interval ] [ -bl break-length ] [ -br break-repeat ] name-of-repeater module infile.dvtool [ infile.ambe ....] \n",progname);
	fprintf(stderr,"\n");
	fprintf(stderr,"CLI Options:\n");
	fprintf(stderr,"-t: text-message included in DV-stream\n");
	fprintf(stderr,"-v: verbose\n");
	fprintf(stderr,"-d: destination ip-address or host (default value is 127.0.0.1)\n");
	fprintf(stderr,"-p: udp port (default: 40000 or 30001 when -dxl option is set)\n");
	fprintf(stderr,"-my: set \"mycall\" (mandatory when -dxl option is set)\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-4 : force IPv4-only\n");
	fprintf(stderr,"-6 : force IPv6-only\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-dxl : dextra linking: initiate dextra linking\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-bi: break-interval (default: 0 (no break))\n");
	fprintf(stderr,"-bl: break-length (default: 200 frames (4 seconds)\n");
	fprintf(stderr,"-br: break-repeat (default: 200 frames (4 seconds))\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"When streaming to a X-reflector or repeater using the dextra-protocol, the\n");
	fprintf(stderr,"-dxl option should ONLY be used if the originating server is NOT link to\n");
	fprintf(stderr,"the destination reflector or repeater. Using the -dxl option on a repeater\n");
	fprintf(stderr,"or host that IS already linked to the remote repeater or reflector will\n");
	fprintf(stderr,"cause the first link to disconnect!\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"The auto break-setting is designed to deal with i-com repeaters that interrupt\n");
	fprintf(stderr,"a stream after (by default) 3 minutes. Enabling this feature will will cause a\n");
	fprintf(stderr,"stream to be interrupted automatically after 'bi' frames, pause of 'bl' frames,\n");
	fprintf(stderr,"repeat the last 'br' frames and then continue\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Ambestream supports up to %d input files that are processed in sequence.\n",MAXINFILE);
	fprintf(stderr,"Both the .ambe and .dvtool file-format is supported\n");
	fprintf(stderr,"\n");

}; // end function usage


#endif



#ifdef WAVSTREAM
void usage (char * progname) {

	fprintf(stderr,"Usage: %s [-t broadcasttext] [-dngl device] [-v] [-4] [-6] [-d ipaddress ] [-p port] [-my CALLSIGN] [-bi break-interval] [-bl break-length] [-br break-repeat] name-of-repeater module infile.wav [ infile.wav ....]\n",progname);
	fprintf(stderr,"Usage: %s -h \n",progname);
	fprintf(stderr,"Usage: %s -V \n",progname);

}; // end function usage

void help (char * progname) {
	fprintf(stderr,"wavstream: stream out PCM-streams (.wav files) to a D-STAR repeater or\n");
	fprintf(stderr,"	reflector\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage: %s [-t broadcasttext] [-dngl device] [-v] [-4] [-6] [-d ipaddress ] [-p port] [-my CALLSIGN] [-bi break-interval] [-bl break-length] [-br break-repeat] name-of-repeater module infile.wav [ infile.wav ....]\n",progname);
	fprintf(stderr,"\n");
	fprintf(stderr,"CLI Options:\n");
	fprintf(stderr,"-dngl: DVdongle serial device (Default: /dev/ttyUSB0).\n");
	fprintf(stderr,"-t: text to be included in the \"slow-speed data\" part of the DV streamfile.\n");
	fprintf(stderr,"-v : verbose\n");
	fprintf(stderr,"-d : destination ip-address or host (default value is 127.0.0.1)\n");
	fprintf(stderr,"-p : udp port (default value is 40000 or 30001 when the -x option is set)\n");
	fprintf(stderr,"-my: set \"mycall\" (mandatory when -x option is used)\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-4 : force IPv4-only\n");
	fprintf(stderr,"-6 : force IPv6-only\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-dxl : dextra linking: initiate a dextra linking process in parallel\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-bi: break-interval (default: 0 (no break))\n");
	fprintf(stderr,"-bl: break-length (default: 200 frames (4 seconds)\n");
	fprintf(stderr,"-br: break-repeat (default: 200 frames (4 seconds))\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"When streaming to a X-reflector or repeater using the dextra-protocol, the\n");
	fprintf(stderr,"-dxl option should ONLY be used if the originating server is NOT link to\n");
	fprintf(stderr,"the destination reflector or repeater. Using the -dxl option on a repeater\n");
	fprintf(stderr,"or host that IS already linked to the remote repeater or reflector will\n");
	fprintf(stderr,"cause the first link to disconnect!\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"The auto break-setting is designed to deal with i-com repeaters that interrupt\n");
	fprintf(stderr,"a stream after (by default) 3 minutes. Enabling this feature will will cause a\n");
	fprintf(stderr,"stream to be interrupted automatically after 'bi' frames, pause of 'bl' frames,\n");
	fprintf(stderr,"repeat the last 'br' frames and then continue\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Ambestream supports up to %d input files that are processed in sequence.\n",MAXINFILE);
	fprintf(stderr,"Both the .ambe and .dvtool file-format is supported\n");
	fprintf(stderr,"\n");

}; // end function usage


#endif



