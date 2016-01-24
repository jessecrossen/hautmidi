build: joy2midi.c parser.c
	gcc -std=gnu99 -Wall -fPIC -DPIC joy2midi.c -o joy2midi -lm -ljack

parser: parser.c
	bison --locations parser.c
	
install: joy2midi
	cp joy2midi /usr/local/bin

uninstall:
	rm /usr/local/bin/joy2midi

clean:
	rm joy2midi
