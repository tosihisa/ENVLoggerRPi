/*
 * bmp085.c - 気圧センサ BMP085 の値を読み取るモジュール．
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
 * gcc -Wall -Wextra -o bmp085 bmp085.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <sys/timerfd.h>
#include <sys/time.h>

int bmp085_readShort(int fd,unsigned short *v,int addr)
{
	unsigned char buf[2];

	buf[0] = (unsigned char)addr;
	if(write(fd,buf,1) != 1){
		return -1;
	}
	if(read(fd,buf,2) != 2){
		return -2;
	}

	*v = 0;
	*v = buf[0];
	*v = (*v << 8) | buf[1];

	return 0;
}

int bmp085_readLong(int fd,unsigned long *v,int addr)
{
	unsigned char buf[3];

	buf[0] = (unsigned char)addr;
	if(write(fd,buf,1) != 1){
		return -1;
	}
	if(read(fd,buf,3) != 3){
		return -2;
	}

	*v = 0;
	*v = buf[0];
	*v = (*v << 8) | buf[1];
	*v = (*v << 8) | buf[2];

	return 0;
}

int bmp085_writeChar(int fd,int addr,int dat)
{
	unsigned char buf[2];

	buf[0] = (unsigned char)addr;
	buf[1] = (unsigned char)dat;
	if(write(fd,buf,2) != 2){
		return -1;
	}
	return 0;
}

int bmp085_wait(int msec)
{
	struct timespec req;
	req.tv_sec = msec / 1000;
	req.tv_nsec = (msec % 1000) * 1000 * 1000;
	return nanosleep(&req,NULL);
}

int main(void)
{
	int bmp085_fd;
	char *i2cFN = "/dev/i2c-1";
	int bmp085_addr = 0xEE >> 1;
	int timer_fd;
	FILE *fp;
	FILE *LCDfp;
 
	printf("***** BMP085 READ *****\n");
 
	if ((bmp085_fd = open(i2cFN, O_RDWR)) < 0){
		perror(i2cFN);
		return 1;
	}
 
	if (ioctl(bmp085_fd, I2C_SLAVE, bmp085_addr) < 0){
		perror("ioctl");
		return 2;
	}

	if((timer_fd = timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC)) < 0){
		perror("timerfd_create");
		return 10;
	}

	if((fp = fopen("./bmp085.log","a+b")) == NULL){
		perror("fopen");
		return 40;
	}

	LCDfp = fopen("/tmp/LCDserver.fifo","r+b");

	if(daemon(0,0) != 0){
		perror("daemon");
		return 30;
	}

	if(1){
		short ac1=0, ac2=0, ac3=0, b1=0, b2=0, mb=0, mc=0, md=0, oss=0;
		unsigned short ac4=0, ac5=0, ac6=0;
		float temperature;
		float pressure;

		if(bmp085_readShort(bmp085_fd,&ac1 , 0xaa) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&ac2 , 0xac) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&ac3 , 0xae) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&ac4 , 0xb0) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&ac5 , 0xb2) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&ac6 , 0xb4) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&b1  , 0xb6) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&b2  , 0xb8) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&mb  , 0xba) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&mc  , 0xbc) != 0){perror("bmp085_readShort"); return 3; }
		if(bmp085_readShort(bmp085_fd,&md  , 0xbe) != 0){perror("bmp085_readShort"); return 3; }

		if(1){
			struct itimerspec itm;
			itm.it_value.tv_sec = 1;
			itm.it_value.tv_nsec = 0;
			itm.it_interval = itm.it_value;
			if(timerfd_settime(timer_fd,0,&itm,NULL) != 0){
				perror("timerfd_settime");
				return 11;
			}
		}

		while(1){
			temperature = 0;
			pressure = 0;
			struct timeval s_t;

			if(1){
				char buf[8];
				if(read(timer_fd,buf,sizeof(buf)) != sizeof(buf)){
					perror("read(timer_fd");
					return 12;
				}
			}

			if(gettimeofday(&s_t,NULL) != 0){
				perror("gettimeofday");
				return 20;
			}

			if(1){
				long t=0, p=0, ut=0, up=0, x1=0, x2=0, x3=0, b3=0, b5=0, b6=0;
				unsigned long b4=0, b7=0;
				oss = 2;

				if(bmp085_writeChar(bmp085_fd, 0xf4, 0x2e) != 0){ perror("bmp085_writeChar"); return 4; }
				bmp085_wait(10);
				ut = 0;
				if(bmp085_readShort(bmp085_fd,&ut, 0xf6) != 0){perror("bmp085_readShort"); return 4; }

				if(bmp085_writeChar(bmp085_fd, 0xf4, 0x34 | (oss << 6)) != 0){ perror("bmp085_writeChar"); return 4; }
				bmp085_wait(50);
				if(bmp085_readLong(bmp085_fd,&up,0xf6) != 0){perror("bmp085_readLong"); return 4; }
				up = up >> (8 - oss);

				x1 = ((ut - ac6) * ac5) >> 15;
				x2 = ((long)mc << 11) / (x1 + md);
				b5 = x1 + x2;
				t = (b5 + 8) >> 4; 
				temperature = (float)t / 10.0;

#if 0
				b6 = b5 - 4000;
				x1 = (b2 * (b6 * b6 / xpow(2, 12))) / xpow(2, 11);
				x2 = ac2 * b6 / xpow(2, 11);
				x3 = x1 + x2;
				b3 = ((((unsigned long)ac1 * 4 + x3) << oss) + 2) / 4;
				x1 = ac3 * b6 / xpow(2, 13);
				x2 = (b1 * (b6 * b6 / xpow(2, 12))) / xpow(2, 16);
				x3 = ((x1 + x2) + 2) / xpow(2, 2);
				b4 = ac4 * (unsigned long)(x3 + 32768) / xpow(2, 15);
				b7 = ((unsigned long)up - b3) * (50000 >> oss);
				if (b7 < (unsigned long)0x80000000) {
					p = (b7 * 2) / b4;
				} else {
					p = (b7 / b4) * 2;
				}
				x1 = (p / xpow(2, 8)) * (p / xpow(2, 8));
				x1 = (x1 * 3038) / xpow(2, 16);
				x2 = (-7357 * p) / xpow(2, 16);
				p = p + (x1 + x2 + 3791) / xpow(2, 4);
#else
				b6 = b5 - 4000;
				// Calculate B3
				x1 = (b2 * (b6 * b6)>>12)>>11;
				x2 = (ac2 * b6)>>11;
				x3 = x1 + x2;
				b3 = (((((long)ac1)*4 + x3)<<oss) + 2)>>2;
  
				// Calculate B4
				x1 = (ac3 * b6)>>13;
				x2 = (b1 * ((b6 * b6)>>12))>>16;
				x3 = ((x1 + x2) + 2)>>2;
				b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
 
				b7 = ((unsigned long)(up - b3) * (50000>>oss));
				if (b7 < 0x80000000)
					p = (b7<<1)/b4;
				else
					p = (b7/b4)<<1;
 
				x1 = (p>>8) * (p>>8);
				x1 = (x1 * 3038)>>16;
				x2 = (-7357 * p)>>16;
				p += (x1 + x2 + 3791)>>4;
#endif
				pressure = (float)p / 100.0;
			}
			if(1){
				struct tm nowtm;
				char tmstr[64];
				gmtime_r(&(s_t.tv_sec),&nowtm);
				strftime(tmstr,sizeof(tmstr)-1,"%Y-%m-%dT%H:%M:%S",&nowtm);
				//fprintf(fp,"%s.%ldZ,",tmstr,s_t.tv_usec);
				fprintf(fp,"%sZ,",tmstr);
				if(LCDfp){
					fprintf(LCDfp,"S 0 0 3 %sZ\n",tmstr);
				}
			}
			fprintf(fp,"T,%f,P,%f\n",temperature,pressure);
			fflush(fp);
			if(LCDfp){
				fprintf(LCDfp,"S 0 16 1 T:%7.2f\n",temperature);
				fprintf(LCDfp,"S 0 32 1 P:%7.2f\n",pressure);
				fflush(LCDfp);
			}
		}
	}
	
	return 0;
}


