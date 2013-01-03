/*
 * gcc -Wall -Wextra -o LCDserver LCDserver.c -lwiringPi
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "debian.xpm"

const int _SPILCD_A0_GPIO = 7;
const int _SPILCD_REG  = 0;
const int _SPILCD_DATA = 1;

void spilcd_write(int rd,unsigned char data)
{
	digitalWrite(_SPILCD_A0_GPIO,rd & 1);
	wiringPiSPIDataRW(0, &data, 1);
}

// set position (x, 8*y)
void locate(int x, int y){
    spilcd_write(_SPILCD_REG,0xb0 | (y & 0x0f)); // Page Address Set (see 2.4.3)
    spilcd_write(_SPILCD_REG,0x10 | (x >> 4 & 0x0f)); // Column Address Set (see 2.4.4)
    spilcd_write(_SPILCD_REG,x & 0x0f);
}
 
void cls(void){
    int x, y;
    for(y = 0; y < 8; y++){
        locate(0, y);
        for(x = 0; x < 128; x++) spilcd_write(_SPILCD_DATA,0x00);
    }
}
 
void plot(int x, int y){
    locate(x, y >> 3);
    spilcd_write(_SPILCD_DATA,1 << (y & 7));
}

int main(int argc, char **argv)
{
	printf("***** START SPI LCD TEST *****\n");

	if(wiringPiSetup () < 0)
	{
		printf("wiringPiSetup failed\n");
		exit(1);
	}
 
	if(wiringPiSPISetup(0, 1000000) < 0)
	{
		printf("wiringPiSPISetup failed\n");
		exit(2);
 	}

	pinMode(_SPILCD_A0_GPIO, OUTPUT) ;

#if 0
	// initialize sequence
	spilcd_write(_SPILCD_REG,0xaf);    // display on (see 2.4.1)
	spilcd_write(_SPILCD_REG,0x2f);    // power control set (see 2.4.16)
	spilcd_write(_SPILCD_REG,0x81);    // set electronic volume mode (see 2.4.18)
	spilcd_write(_SPILCD_REG,0x00);    // electronic volume data 00-3f
	spilcd_write(_SPILCD_REG,0x27);    // V5 Volatge Regulator Internal Resister Ratio Set (see 2.4.17)
	spilcd_write(_SPILCD_REG,0xa2);    // LCD Bias Set ... 1/9 bias (see 2.4.11)
	spilcd_write(_SPILCD_REG,0xc8);    // Common Output Mode Select ... Reverse (see 2.4.15)
	spilcd_write(_SPILCD_REG,0xa0);    // ADC Select ... Normal (see 2.4.8)
	spilcd_write(_SPILCD_REG,0xa4);    // Display All Points ON/OFF ... normal (see 2.4.10)
	spilcd_write(_SPILCD_REG,0xa6);    // Display Normal/Reverse ... normal (see 2.4.9)
	spilcd_write(_SPILCD_REG,0xac);    // Static Indicator ... off (see 2.4.19)
	spilcd_write(_SPILCD_REG,0x00);    // off
	spilcd_write(_SPILCD_REG,0x40);    // Display Strat Line Set ... 0 (see 2.4.2)
	spilcd_write(_SPILCD_REG,0xe0);    // Write Mode Set
#else
	spilcd_write(_SPILCD_REG,0xa2);    // (11) LCD Bias Set ... 1/9 bias (see 2.4.11)
	spilcd_write(_SPILCD_REG,0xa0);    // (8)  ADC Select ... normal (see 2.4.8)
	spilcd_write(_SPILCD_REG,0xc8);    // (15) Common Output Mode Select ... Reverse (see 2.4.15)
	spilcd_write(_SPILCD_REG,0x24);    // (17) V5 Volatge Regulator Internal Resister Ratio Set (see 2.4.17)
	spilcd_write(_SPILCD_REG,0x81);    // (18) set electronic volume mode (see 2.4.18)
	spilcd_write(_SPILCD_REG,0x1e);    //      electronic volume data 00-3f ... control your own value
	spilcd_write(_SPILCD_REG,0x2f);    // (16) power control set (see 2.4.16)
	spilcd_write(_SPILCD_REG,0x40);    // (2)  Display Start Line Set ... 0 (see 2.4.2)
	spilcd_write(_SPILCD_REG,0xe0);    // (6)  Write Mode Set
	spilcd_write(_SPILCD_REG,0xaf);    // (1)  display on (see 2.4.1)
#endif

	cls();
	sleep(1);

	if(1){
		int x,y;
		unsigned char v = 0;
		for(y = 0;y < 8;y++){
        		locate(0, y);
        		for(x = 0; x < 128; x++){
				v = 0;
				v |= (debian[(y*8)+3+7][x] == ' ') ? 1 : 0; v = v << 1;
				v |= (debian[(y*8)+3+6][x] == ' ') ? 1 : 0; v = v << 1;
				v |= (debian[(y*8)+3+5][x] == ' ') ? 1 : 0; v = v << 1;
				v |= (debian[(y*8)+3+4][x] == ' ') ? 1 : 0; v = v << 1;
				v |= (debian[(y*8)+3+3][x] == ' ') ? 1 : 0; v = v << 1;
				v |= (debian[(y*8)+3+2][x] == ' ') ? 1 : 0; v = v << 1;
				v |= (debian[(y*8)+3+1][x] == ' ') ? 1 : 0; v = v << 1;
				v |= (debian[(y*8)+3+0][x] == ' ') ? 1 : 0;
				spilcd_write(_SPILCD_DATA,v);
			}
		}
	}

	if(0){
		int y;
		for(y = 0;y < 63;y++){
			plot(y,y);
		}
	}

	return 0;
}

