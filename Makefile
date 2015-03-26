timer.out: timer.c
	gcc -g -L/usr/local/lib -I/usr/local/include -lwiringPi -lwiringPiDev timer.c -o timer.out
