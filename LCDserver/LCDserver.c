/*
 * LCD 表示サーバ．
 * FSTN液晶モジュール（128×64/SPI）[AD-12864-SPI] を制御します．
 * ライセンスは GPLv2 です．
 * GPLv2 である由来は，この LCD 表示サーバのフォントは Linux カーネルの表示フォントを使用しており，
 * Linux カーネルが(例外条項付き)GPLv2 であるため，このソースコードも GPLv2 です．
 * なお，GPLv2 であるのはこのディレクトリ以下のソースコードを指し，他のディレクトリは，
 * 他のディレクトリのライセンスです．
 *
 * Copyright (c) Toshihisa Tanaka <tosihisa@netfort.gr.jp>
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

static const int _SPILCD_A0_GPIO = 7;	// GPIO 4番を使用する．7 と書いているが，これは wiringPi の使用による．
static const int _SPILCD_REG  = 0;
static const int _SPILCD_DATA = 1;

void spilcdWrite(int rd,unsigned char data)
{
	digitalWrite(_SPILCD_A0_GPIO,rd & 1);
	wiringPiSPIDataRW(0, &data, 1);
}

// set position (x, 8*y)
void spilcdLocate(int x, int y){
    spilcdWrite(_SPILCD_REG,0xb0 | (y & 0x0f)); // Page Address Set (see 2.4.3)
    spilcdWrite(_SPILCD_REG,0x10 | (x >> 4 & 0x0f)); // Column Address Set (see 2.4.4)
    spilcdWrite(_SPILCD_REG,x & 0x0f);
}
 
void spilcdClear(void){
    int x, y;
    for(y = 0; y < 8; y++){
        spilcdLocate(0, y);
        for(x = 0; x < 128; x++) spilcdWrite(_SPILCD_DATA,0x00);
    }
}
 
void spilcdPlot(int x, int y){
    spilcdLocate(x, y >> 3);
    spilcdWrite(_SPILCD_DATA,1 << (y & 7));
}

int spilcdInit(void)
{
	pinMode(_SPILCD_A0_GPIO, OUTPUT) ;

	spilcdWrite(_SPILCD_REG,0xa2);    // (11) LCD Bias Set ... 1/9 bias (see 2.4.11)
	spilcdWrite(_SPILCD_REG,0xa0);    // (8)  ADC Select ... normal (see 2.4.8)
	spilcdWrite(_SPILCD_REG,0xc8);    // (15) Common Output Mode Select ... Reverse (see 2.4.15)
	spilcdWrite(_SPILCD_REG,0x24);    // (17) V5 Volatge Regulator Internal Resister Ratio Set (see 2.4.17)
	spilcdWrite(_SPILCD_REG,0x81);    // (18) set electronic volume mode (see 2.4.18)
	spilcdWrite(_SPILCD_REG,0x1e);    //      electronic volume data 00-3f ... control your own value
	spilcdWrite(_SPILCD_REG,0x2f);    // (16) power control set (see 2.4.16)
	spilcdWrite(_SPILCD_REG,0x40);    // (2)  Display Start Line Set ... 0 (see 2.4.2)
	spilcdWrite(_SPILCD_REG,0xe0);    // (6)  Write Mode Set
	spilcdWrite(_SPILCD_REG,0xaf);    // (1)  display on (see 2.4.1)

	return 0;
}

void displayLogo(void)
{
	int x,y;
	unsigned char v = 0;
	for(y = 0;y < 8;y++){
		spilcdLocate(0, y);
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
			spilcdWrite(_SPILCD_DATA,v);
		}
	}
}

int main(int argc, char **argv)
{
	int opt;
	int logo = 0;
	int run_daemon = 0;

	while ((opt = getopt(argc, argv, "ld")) != -1){
		switch(opt){
			case 'l':
				logo = 1;
				break;
			case 'd':
				run_daemon = 1;
			default:
				break;
		}
	}

	if(wiringPiSetup () < 0)
	{
		fprintf(stderr,"wiringPiSetup failed\n");
		return 1;
	}
 
	if(wiringPiSPISetup(0, 1000000) < 0)
	{
		fprintf(stderr,"wiringPiSPISetup failed\n");
		return 2;
 	}

	if(run_daemon){
		if(daemon(0,0) != 0){
			perror("daemon()");
			return 3;
		}
		if(1){
			FILE *fp;
			if((fp = fopen("/var/run/LCDserver.pid","w+b")) == NULL){
				perror("fopen()");
				return 4;
			}
			fprintf(fp,"%d\n",getpid());
			fclose(fp);
		}
	}

	spilcdInit();
	spilcdClear();

	sleep(1);

	if(logo){
		displayLogo();
	}

	return 0;
}

