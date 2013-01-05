/*
 * lea-6t.c - GPS Module LEA-6T Read Program.
 *
 * Copyright (c) 2013 @tosihisa
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *   
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *    
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * gcc -Wall -Wextra -o lea-6t lea-6t.c
 */
#define	_BSD_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "UBXPacket.h"

static const int _LEA6T_RST_GPIO = 5;	// A0 ピンアサイン．GPIO 4番を使用する．7 と書いているが，これは wiringPi の使用による．
struct UBXPacket_s UBXPacket;

int lea6t_wait(int msec)
{
	struct timespec req;
	req.tv_sec = msec / 1000;
	req.tv_nsec = (msec % 1000) * 1000 * 1000;
	return nanosleep(&req,NULL);
}

unsigned char NMEA_CalcSum(unsigned char *str,int len)
{
    unsigned char sum = 0;
    int i;
    for(i = 0;i < len;i++){
        sum = sum ^ (*(str + i));
    }
    return sum;
}

void UBX_CalcSum(unsigned char *str,int len,unsigned char *sum)
{
    int i;
    *(sum + 0) = *(sum + 1) = 0;
    for(i = 0;i < len;i++){
        *(sum + 0) = *(sum + 0) + *(str+i);
        *(sum + 1) = *(sum + 1) + *(sum + 0);
    }
}

int UBX_WaitAck(int fd/*,struct UBXPacket_s *info*/)
{
    UBXPacket.cjobst = 0;
	unsigned char buf;

    while(1){
		if(read(fd,&buf,sizeof(buf)) <= 0){
			return -1;
		}
//printf("RECV[0x%02X]\n",buf);
		if(UBXPacket_Parse(&UBXPacket,buf) == 100){
			if((UBXPacket.cls == 0x05) && (UBXPacket.id == 0x01)){
				return 1; /* ACK */
			} else if((UBXPacket.cls == 0x05) && (UBXPacket.id == 0x00)){
				return 0; /* NAK */
			} else {
				UBXPacket.cjobst = 0;
			}
		}
	}
	return -10;
}

int main(int argc,char *argv[])
{
	struct termios o_term;
	struct termios s_term;
	char *ttyName = "/dev/ttyUSB0";
	int ttyFd = -1;
	unsigned long UBXCount = 0;

	UBXPacket.cjobst = 0;

	if(argc >= 2){
		if(argv[1] != NULL){
			ttyName = argv[1];
		}
	}

	if(wiringPiSetup () < 0)
	{
		fprintf(stderr,"wiringPiSetup failed\n");
		return 1;
	}
	pinMode(_LEA6T_RST_GPIO, OUTPUT) ;
	digitalWrite(_LEA6T_RST_GPIO,1);
	lea6t_wait(500);
	digitalWrite(_LEA6T_RST_GPIO,0);
	lea6t_wait(500);

	if((ttyFd = open(ttyName,O_RDWR | O_NOCTTY)) < 0){
		perror("open()");
		return 1;
	}

	memset(&o_term,0,sizeof(o_term));
	if(tcgetattr(ttyFd,&o_term) != 0){
		perror("tcgetattr()");
		return 2;
	}

	memcpy(&s_term,&o_term,sizeof(s_term));

	/* Raw mode */
	s_term.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	s_term.c_oflag &= ~OPOST;
	s_term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	s_term.c_cflag &= ~(CSIZE | PARENB);
	s_term.c_cflag |= CS8;
	cfmakeraw(&s_term);

	cfsetospeed(&s_term,B9600);
	cfsetispeed(&s_term,B9600);

	s_term.c_cc[VMIN] = 0;
	s_term.c_cc[VTIME] = 10;

	if(tcsetattr(ttyFd,TCSANOW,&s_term) != 0){
		perror("tcsetattr()");
		return 2;
	}
	tcflush(ttyFd,TCIOFLUSH);

	while(0){
		char buf;
		if(read(ttyFd,&buf,1) == 1){
			printf("__READ[%c]\n",buf);
		}
	}

	if(1){
		//unsigned char modeStr[] = "PUBX,41,1,0007,0003,115200,0";
		unsigned char modeStr[] = "PUBX,41,1,0007,0001,115200,0";
		//unsigned char modeStr[] = "PUBX,41,1,0007,0001,38400,0";
		//unsigned char modeStr[] = "PUBX,41,1,0007,0001,9600,0";
		unsigned char sum = NMEA_CalcSum(modeStr,strlen((char *)modeStr));
		char outbuf[82];
        printf("SEND:[%s](0x%02X)\n",modeStr,sum);
        snprintf(outbuf,sizeof(outbuf)-1,"$%s*%02X%c%c",modeStr,sum,0x0d,0x0a);
		write(ttyFd,outbuf,strlen(outbuf));
		tcdrain(ttyFd);
#if 1
		//このスリープを外すと，以下の通信が正しく行えない．
		//理由は，TTYの送信バッファは空だが FIFO にあるうちに
		//ボーレート変更してしまうと，FIFOにある部分は正しく送信できないからだ．
		//つまり，Linux TTY 層やシリアルドライバは，Linux の TTY にある送信バッファは
		//面倒を見るが，H/W 側のバッファまでは意識できない．
		//当然だが，FIFO が空かどうかはソフトで十分に監視できる．
		//それが監視できていない Linux の TTY デバイス層の問題である．
		//つまり，このスリープは Linux の TTY デバイス層の都合で必要なものである．
		//ソフト都合でスリープが必要というのは情けない．
		sleep(1);

        printf("CHG BAUD:115200\n");
		cfsetspeed(&s_term,B115200);
		if(tcsetattr(ttyFd,TCSADRAIN,&s_term) != 0){
			perror("tcsetattr()");
			return 2;
		}
#endif
    }

	while(0){
		char buf;
		if(read(ttyFd,&buf,1) == 1){
			printf("__READ[%02x]\n",buf);
		}
	}

	tcflush(ttyFd,TCIOFLUSH);

    if(1){
        unsigned char chkStr[][11] = {
            { 0xB5,0x62,0x06,0x01,0x03,0x00,0x02,0x10,0x01,0xFF,0xFF }, //RXM-RAW (0x02 0x10)
            { 0xB5,0x62,0x06,0x01,0x03,0x00,0x02,0x30,0x01,0xFF,0xFF }, //RXM-ALM (0x02 0x30)
            //{ 0xB5,0x62,0x06,0x01,0x03,0x00,0x02,0x31,0x01,0xFF,0xFF }, //RXM-EPH (0x02 0x31)
            //{ 0xB5,0x62,0x06,0x01,0x03,0x00,0x02,0x41,0x02,0xFF,0xFF },
            { 0xB5,0x62,0x06,0x01,0x03,0x00,0x02,0x11,0x01,0xFF,0xFF }, //RXM-SFRB (0x02 0x11)
            { 0xB5,0x62,0x06,0x01,0x03,0x00,0x02,0x20,0x01,0xFF,0xFF }, //RXM-SVSI (0x02 0x20)
            { 0xB5,0x62,0x06,0x01,0x03,0x00,0x01,0x21,0x01,0xFF,0xFF }, //NAV-TIMEUTC (0x01 0x21)
            { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF }, //END
        };
        int j;
        int isAck;

        for(j=0;chkStr[j][0] != 0x00;j++){
            UBX_CalcSum(&chkStr[j][2],7,&chkStr[j][9]);
			write(ttyFd,chkStr[j],sizeof(chkStr[0]));
			tcdrain(ttyFd);
            isAck = UBX_WaitAck(ttyFd/*,&UBXPacket*/);
            printf("%d : SET UBX Rate : %s\n",j,(isAck == 1) ? "ACK" : "NAK");
       }
    }
    UBXPacket.cjobst = 0;

	while(1){
		char buf;
		int year=0;
		int mon=0;
		int day=0;
		int hour=0;
		int min=0;
		int sec=0;
		if(read(ttyFd,&buf,1) == 1){
            if(UBXPacket_Parse(&UBXPacket,buf) == 100){
                UBXCount++;
                if((UBXPacket.cls == 0x01) && (UBXPacket.id == 0x21)){
                    /* NAV-TIMEUTC */
                    year = (((unsigned short)UBXPacket.body[13]) << 8) | UBXPacket.body[12];
                    mon  = UBXPacket.body[14];
                    day  = UBXPacket.body[15];
                    hour = UBXPacket.body[16];
                    min  = UBXPacket.body[17];
                    sec  = UBXPacket.body[18];
					printf("%10lu,%04d-%02d-%02d %02d:%02d:%02d\n",UBXCount,year,mon,day,hour,min,sec);
                }
                printf("\t\t%ld : GET UBX Packet (Class=0x%02X,ID=0x%02X,LEN=%d)\n",
                            UBXCount,
                            UBXPacket.cls,
                            UBXPacket.id,
                            UBXPacket.len );
                UBXPacket.cjobst = 0;
            }
		}
	}
	return 0;
}
 
