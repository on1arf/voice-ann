////////////////////////////////////
// Function readambefile ///////////
////////////////////////////////////

int readambefile (FILE * fp, unsigned char * ambebuffer) {

char linein[80];
char retbuffer[9];

char * word3;

//scanfline: tempory line used to do "scanf" parsing of linein
char scanfline[27];
int parsedok;

int retcode=0,stop=0;


while (stop == 0) {
// get line, maximum 80 characters

	if (! fgets(linein,80,fp)) {
	// fails, end of line
		retcode=0;
		stop=1;
		continue;
	}; // end if

	if (linein[0] == '#') {
		/// comment-line begin with '#' -> skip it
		continue;
	}; // end if

	// OK, we have a line, parse it

	// ambe-data in in 3th word
	// find first space
	word3=index(linein,' ');

	// find next space
	if (word3 != NULL) {
		word3=index(word3+1,' ');
	};

	// if we have at least 3 words and it starts at maximum at the 62th character of line
	// (ambe-data is 9 bytes, so 18 hex-characters)
	if ((word3 != NULL) && ((word3 - linein) < 62)) {
		// set word3 to next character (beginning of 3th word]
		word3++;
                
		// reinit scanfline
		memset(scanfline,' ',26);
		scanfline[26]='\n';
		parsedok=0;

		memcpy(&scanfline[0],&word3[0],2);memcpy(&scanfline[3],&word3[2],2);
		memcpy(&scanfline[6],&word3[4],2); memcpy(&scanfline[9],&word3[6],2);
		memcpy(&scanfline[12],&word3[8],2); memcpy(&scanfline[15],&word3[10],2);
		memcpy(&scanfline[18],&word3[12],2); memcpy(&scanfline[21],&word3[14],2);
		memcpy(&scanfline[24],&word3[16],2);

		sscanf(scanfline,"%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx%n",&retbuffer[0],&retbuffer[1],&retbuffer[2],&retbuffer[3],&retbuffer[4],&retbuffer[5],&retbuffer[6],&retbuffer[7],&retbuffer[8],&parsedok);

		// if parsedok is 26 (26 characters parsed: 7 * "xx " + 1 * "xx"), we have
		// a valid line
		if (parsedok == 26) {
			memcpy(ambebuffer,retbuffer,9);
			retcode=9;
			stop=1;
		}; // endif
                
	};  // end if (valid line)
     
}; // end while (stop==0)


return(retcode);
}; // end function readambeline

