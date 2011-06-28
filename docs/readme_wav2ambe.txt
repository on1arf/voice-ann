This document contains some high-level information about the "wav2ambe"
application, that is part of the D-STAR "voice-announcement" package.


There is a lot of information to be found about this program 
in the comments of the C-program; but some additional "high level"
information can be found here.


* About the program.
Wav2ambe is a C-program that -as its name implies- converts .wav-files
into ambe-encoded .ambe files.

For that purpose, it needs two things:
- a DVdongle
This device talks to the actual AMBE encoding hardware.
As AMBE is patented and only available in hardware form, it is not possible
to encode wav-files in ambe-format without this device.


- the "libsndfile" library.
This is a opensource library to read and write sound-files, among others
.wav files.
It is cross-platform and available for *nix system, mac and windows.

On Ubuntu/debian, this packet can easily be installed via apt-get:
"apt-get install libsndfile1-dev libsndfile1"
On CentOS, as found on the D-STAR repeater PCs, this package is not
available in the standard repostitory.
See the "install" document on how to install this on a D-STAR
repeater CentOS distribution.



* About the DVdongle
The DVdongle is a USB device that contains a USB serial-port and a AMBE
encoder/decoder ship.
For the computer, this device appears just like any serial device.

Information about this device can be found here:
http://www.moetronix.com/dvdongle/

The program communicates with the dongle by sending and receiving
frames to it.
Each frame is consists of a 2 byte header, which contain the "ptype"
(packet type) and "plen" packet length.

Information about the actual comments accepted by this device can
be found here: http://www.moetronix.com/files/dvdongletechref100.pdf

Including information about the actual AMBE-200 encoder/decoder
chipset can be found here: http://www.dvsinc.com/manuals/AMBE-2000_manual.pdf

The exact senario on how to convert WAV-files to ambe files can
be found in the source-code.


On linux, inserting the dongle will create a USB-serial device (/dev/ttyUSBx)
which can be used to access the device.



* About wav-files:

The input files of wav2ambe are wav-files. They are imported using the
libsndfile library.

This is a library to read and write audio-files in a a large number
possible file-formats.
This library is open-source and cross-platform and is therefor available
for unix/linux, mac en windows.
On a lot of linux distro's it is available in the standard
repostitories. (however, not the CentOS used on the D-STAR repeaters)


The DVdongle only accepts wav-files with the following specifications:
- MS-WAV, uncompressed PCM
- 8000 Khz sampling
- 1 channel (mono)
- 16 Bit little-endian

Most linux-distributions, including CentOS, include a tool called
"sox" which can be used to convert wav-files in other formats into
a file with the correct specifications.



73s
kristoff ON1ARF
