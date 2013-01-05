/*
 * co2_k30.c - CO2 Sensor K30 Read program.
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
 * gcc -Wall -Wextra -o co2_k30 co2_k30.c
 */
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

int CO2_Read(int ttyFd,unsigned short *val)
{
    unsigned char sbuf[] = { 0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5 };
    unsigned char rbuf[7];
    unsigned int i;
    unsigned short crc;
    extern unsigned short modbus_CRC(unsigned char *DataPtr, unsigned short len);

	if(write(ttyFd,sbuf,sizeof(sbuf)) != sizeof(sbuf)){
		return 1;
	}
	tcdrain(ttyFd);

    for(i=0;i < sizeof(rbuf);i++){
		if((read(ttyFd,&rbuf[i],1)) != 1){
			return 2;
		}
    }
    if(rbuf[0] != 0xFE){
        return -4;
    }
    if(rbuf[1] != 0x04){
        return -1;
    }
    if(rbuf[2] != 0x02){
        return -2;
    }
    crc = rbuf[6];
    crc = (crc << 8) | rbuf[5];
    if(crc != modbus_CRC(rbuf,5)){
        return -3;
    }
    *val = rbuf[3];
    *val = (*val << 8) | rbuf[4];
    return 0;
}

int main(int argc,char *argv[])
{
	struct termios o_term;
	struct termios s_term;
	char *ttyName = "/dev/ttyUSB1";
	int ttyFd = -1;

	if(argc >= 2){
		if(argv[1] != NULL){
			ttyName = argv[1];
		}
	}

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

	cfsetospeed(&s_term,B9600);

	s_term.c_cc[VMIN] = 0;
	s_term.c_cc[VTIME] = 10;

	if(tcsetattr(ttyFd,TCSANOW,&s_term) != 0){
		perror("tcsetattr()");
		return 2;
	}

	tcflush(ttyFd,TCIOFLUSH);
	while(1){
		unsigned char ign;
		if(read(ttyFd,&ign,sizeof(ign)) <= 0){
			break;
		}
	}

	while(1){
		unsigned short CO2_val;
		if(CO2_Read(ttyFd,&CO2_val) != 0){
			perror("CO2_Read()");
			break;
		}
		printf("CO2:%5u.%1uppm\n",CO2_val/10,CO2_val%10);
		sleep(1);
	}

	return 0;
}

