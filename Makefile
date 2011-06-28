ALL: wav2ambe wavstream ambe2wav cp2dpl ambestream


wav2ambe: wav2ambe.c crc.h serialframe.h serialreceive.h serialsend.h wav2ambe.h
	 gcc -Wall -o wav2ambe -lpthread -lsndfile -lrt wav2ambe.c

ambe2wav: ambe2wav.c crc.h serialframe.h serialreceive.h serialsend.h readambefile.h wav2ambe.h
	 gcc -Wall -o ambe2wav -lpthread -lsndfile -lrt ambe2wav.c

wavstream: wavstream.c crc.h serialframe.h s_serialreceive.h s_serialsend.h s_udpsend.h wavstream.h
	gcc -Wall -o wavstream -lpthread -lsndfile -lrt wavstream.c

ambestream: ambestream.c crc.h serialframe.h s_serialreceive.h s_serialsend.h s_udpsend.h ambestream.h
	gcc -Wall -o ambestream -lrt ambestream.c

cp2dpl: cp2dpl.c
	gcc -Wall -o cp2dpl cp2dpl.c

# Installing: cp2dpl needs setuid priviledges to be able to
# write files in /dstar/tmp
install:
	install -o root -g root -m 755 ambe2dvtool.pl /usr/bin/
	install -o root -g root -m 755 dvtool2ambe.pl /usr/bin/
	install -o root -g root -m 755 dvtooltext.pl /usr/bin/
	install -o root -g root -m 4755 cp2dpl /usr/bin/
	install -o root -g root -m 755 wav2ambe /usr/bin/
	install -o root -g root -m 755 ambe2wav /usr/bin/
	install -o root -g root -m 755 wavstream /usr/bin/
	install -o root -g root -m 755 ambestream /usr/bin/


