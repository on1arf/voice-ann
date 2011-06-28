// dextra linking and heartbeats

// release-info:
// 25 Mar. 2011: version 0.2.1: Initial release

// dextralink: init link to dextra gateway or reflector
int dextralink() {

static struct sockaddr_in6 * serverAddress;

// linkmessage is "MYCALL  D"
unsigned char linkmsg[] = "        D";

int missed, stop;
int ret;

// 1500 octets: MTU of ethernet, so we should not receive anything longer
unsigned char recbuffer[1500];

// ai_addr was created in main thread
serverAddress = (struct sockaddr_in6 *) global.ai_addr;

if (serverAddress == NULL) {
	fprintf(stderr,"Error: main thread does not give valid a_addr \n");
	exit(-1);
}; // end if

// destport was created in main thread
serverAddress->sin6_port = htons((unsigned short int) global.destport);  

// linkmessage is "MYCALL  D"
memcpy(linkmsg,global.mycall,6);

ret=sendto(global.udpsd, linkmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));

if (ret== -1 ) {
	fprintf(stderr,"Error: error during sending udp-packet for dextra linking!\n");
	exit(-1);
}; // end if
ret=sendto(global.udpsd, linkmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
ret=sendto(global.udpsd, linkmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
ret=sendto(global.udpsd, linkmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
ret=sendto(global.udpsd, linkmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));

missed=0;
stop=0;

// wait for reply: a UDP message of 9 octets: gateway/reflector callsign + \0

while (stop==0) {
		  ret=recv(global.udpsd, recbuffer, 1500, MSG_DONTWAIT);

	if (ret == -1) {
	// nothing received
		missed++;

		if (missed > 5) {
			stop=1;
		} else {
			sleep(1);
		}; // end if
	} else {
		// data received, is it 9 characters, otherwize ignore the package
		// also, last character should be a 0
		if ((ret == 9) && (recbuffer[8] == 0)) {
			// found it
				  if (global.verboselevel >= 1) {
				fprintf(stderr,"Link established to %s\n",recbuffer);
			}; // end if
			stop=1;		
		}; // end if
	}; // end if

}; // end if

if (missed <= 5) {
	return(0);
} else {
	return(-1);
}; // end if

}; // end function

/// end function ///

// function dextra heartbeat
// is started by kernel interrupt every 6 seconds.

static void funct_dextraheartbeat () {

static struct sockaddr_in6 * serverAddress;

// heartbeat message is "MYCALL  \0"
unsigned char heartbeatmsg[] = "        \0";

int ret;

// ai_addr was created in main thread
serverAddress = (struct sockaddr_in6 *) global.ai_addr;

if (serverAddress == NULL) {
	fprintf(stderr,"Error: main thread does not give valid a_addr \n");
	exit(-1);
}; // end if

// destport was created in main thread
serverAddress->sin6_port = htons((unsigned short int) global.destport);  

// linkmessage is "MYCALL  \0"
memcpy(heartbeatmsg,global.mycall,6);

ret=sendto(global.udpsd, heartbeatmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
if (ret == -1) {
	fprintf(stderr,"Error: error during sending udp-packet for dextra linking!\n");
	exit(-1);
}; // end if

ret=sendto(global.udpsd, heartbeatmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
ret=sendto(global.udpsd, heartbeatmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
ret=sendto(global.udpsd, heartbeatmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));
ret=sendto(global.udpsd, heartbeatmsg, 9,0,(struct sockaddr *) serverAddress, sizeof(struct sockaddr_in6));

}; // end function
