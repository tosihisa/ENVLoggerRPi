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
#include <string.h>
#include <limits.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "linuxfont/linuxfont.h"

extern char *debian_xpm[];

#define	LCD_MAX_X	(128)
#define	LCD_MAX_Y	(64)

// SPI LCD 用の VRAM．ここに一旦描いてコピーする．
// イメージは，基本的に SPI LCD と同じ．
static unsigned char VRAM[(LCD_MAX_X * LCD_MAX_Y)/8];

static const int _SPILCD_A0_GPIO = 7;	// GPIO 4番を使用する．7 と書いているが，これは wiringPi の使用による．
static const int _SPILCD_REG  = 0;
static const int _SPILCD_DATA = 1;

static const struct font_desc *fontTbl[] = {
	&font_vga_8x8,		// 0
	&font_vga_8x16,		// 1
	&font_pearl_8x8,	// 2
	&font_vga_6x11,		// 3
	&font_7x14,			// 4
	&font_10x18,		// 5
	&font_sun_8x16,		// 6
	&font_sun_12x22,	// 7
	&font_acorn_8x8,	// 8
	&font_mini_4x6,		// 9
	NULL
};

void spilcdWrite(int rd,unsigned char data)
{
	digitalWrite(_SPILCD_A0_GPIO,rd & 1);
	wiringPiSPIDataRW(0, &data, 1);
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

// set position (x, 8*y)
void spilcdLocate(int x, int y){
    spilcdWrite(_SPILCD_REG,0xb0 | (y & 0x0f)); // Page Address Set (see 2.4.3)
    spilcdWrite(_SPILCD_REG,0x10 | (x >> 4 & 0x0f)); // Column Address Set (see 2.4.4)
    spilcdWrite(_SPILCD_REG,x & 0x0f);
}
 
void spilcdUpdate(void)
{
    int x, y;
    for(y = 0; y < 8; y++){
        spilcdLocate(0, y);
        for(x = 0; x < 128; x++){
			spilcdWrite(_SPILCD_DATA,VRAM[(LCD_MAX_X*y)+x]);
		}
    }
}

void spilcdPlot(int x, int y,int fb)
{
	int ly;
	int my;
	unsigned char *pVRAM;
	unsigned char w;

	ly = y / 8;
	my = y % 8;

	pVRAM = &VRAM[(LCD_MAX_X*ly)+x];
	w = 1 << my;
	if(fb){
		*pVRAM |= w;
	} else {
		*pVRAM &= ~w;
	}
}

void spilcdClear(void)
{
	memset(VRAM,0,sizeof(VRAM));
}

void displayLogo(void)
{
	int x,y;
	unsigned char v = 0;
	for(y = 0;y < 64;y++){
		for(x = 0; x < 128; x++){
			v = (debian_xpm[y+3][x] == ' ') ? 1 : 0;
			spilcdPlot(x,y,v);
		}
	}
}

int main(int argc, char **argv)
{
	int opt;
	int logo = 0;
	int run_daemon = 0;
	char fifoName[PATH_MAX];
	FILE *fifofp;
	FILE *debugfp = stdout;


	strcpy(fifoName,"/tmp/LCDserver.fifo");

	if((debugfp = fopen("/dev/null","r+b")) == NULL){
		perror("/dev/null");
		return 6;
	}

	while ((opt = getopt(argc, argv, "Dldf:")) != -1){
		switch(opt){
			case 'D':
				fclose(debugfp);
				debugfp = stdout;
				break;
			case 'l':
				logo = 1;
				break;
			case 'd':
				run_daemon = 1;
			case 'f':
				strncpy(fifoName,optarg,sizeof(fifoName)-1);
				fifoName[sizeof(fifoName)-1] = (char)('\0');
				break;
			default:
				break;
		}
	}

	fprintf(debugfp,"%s:%d\n",__FILE__,__LINE__);

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

	if(1){
		mode_t o_u;
		o_u = umask(0011);
		unlink(fifoName);
		if(mkfifo(fifoName,0666) != 0){
			perror("mkfifo()");
			return 5;
		}
		(void)umask(o_u);
		if((fifofp = fopen(fifoName,"r+b")) == NULL){
			perror("fopen()");
			return 6;
		}
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

	if(logo){
		displayLogo();
	}

	while(1){
		char linebuf[80+1];

		fprintf(debugfp,"%s:%d\n",__FILE__,__LINE__);
		spilcdUpdate();

		memset(linebuf,0,sizeof(linebuf));
		if(fgets(linebuf,sizeof(linebuf)-1,fifofp) == NULL){
			continue;
		}
		if(linebuf[0] == (char)('C')){
			spilcdClear();
		} if(linebuf[0] == (char)('L')){
			displayLogo();
		}
	}

	return 0;
}

