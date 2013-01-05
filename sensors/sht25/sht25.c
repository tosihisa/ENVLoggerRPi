/*
 * sht25.c - Digital Humidity Sensor (RH&T) を読み取るモジュール．
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
 * gcc -Wall -Wextra -o sht25 sht25.c
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

int sht25_writeChar(int fd,int dat)
{
	unsigned char buf[1];

	buf[0] = (unsigned char)dat;
	if(write(fd,buf,1) != 1){
		return -1;
	}
	return 0;
}

int sht25_wait(int msec)
{
	struct timespec req;
	req.tv_sec = msec / 1000;
	req.tv_nsec = (msec % 1000) * 1000 * 1000;
	return nanosleep(&req,NULL);
}

int main(void)
{
	int sht25_fd;
	char *i2cFN = "/dev/i2c-1";
	int sht25_addr = 0x80 >> 1;
	int timer_fd;
	FILE *fp;
	FILE *LCDfp;
 
	printf("***** SHT25 READ *****\n");
 
	if ((sht25_fd = open(i2cFN, O_RDWR)) < 0){
		perror(i2cFN);
		return 1;
	}
 
	if (ioctl(sht25_fd, I2C_SLAVE, sht25_addr) < 0){
		perror("ioctl");
		return 2;
	}

	if((timer_fd = timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC)) < 0){
		perror("timerfd_create");
		return 10;
	}

	if((fp = fopen("./sht25.log","a+b")) == NULL){
		perror("fopen");
		return 40;
	}

	LCDfp = fopen("/tmp/LCDserver.fifo","r+b");

	if(1){
		if(daemon(0,0) != 0){
			perror("daemon");
			return 30;
		}
	}

	if(1){
		sht25_writeChar(sht25_fd,0xFE);	// soft reset
		sht25_wait(50);					// wait 50msec
	}


	if(1){
		float temperature;
		float humidity;

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
			humidity = 0;
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
				char buf[3];
				unsigned short val;
				sht25_writeChar(sht25_fd,0xF3);	// Trigger T measurement no hold master
				sht25_wait(100);	// IMPORTANT now Measurement
				if(read(sht25_fd,buf,sizeof(buf)) != sizeof(buf)){
					perror("read()");
					return 35;
				}
				val = buf[0];
				val = (val << 8) | buf[1];
				val &= ~0x0003;	/* clear bits [1..0] (status bits) */
				temperature = -46.85 + 175.72/65536 *((float)val); //T= -46.85 + 175.72 * ST/2^16

				sht25_writeChar(sht25_fd,0xF5);	// Trigger T measurement no hold master
				sht25_wait(100);	// IMPORTANT now Measurement
				if(read(sht25_fd,buf,sizeof(buf)) != sizeof(buf)){
					perror("read()");
					return 35;
				}
				val = buf[0];
				val = (val << 8) | buf[1];
				val &= ~0x0003;	/* clear bits [1..0] (status bits) */
				humidity = -6.0 + 125.0/65536 * ((float)val); // RH= -6 + 125 * SRH/2^16
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
			fprintf(fp,"T,%f,RH,%f\n",temperature,humidity);
			fflush(fp);
			if(LCDfp){
				fprintf(LCDfp,"S 0 16 1 T :%7.2f\n",temperature);
				fprintf(LCDfp,"S 0 32 1 RH:%7.2f\n",humidity);
				fflush(LCDfp);
			}
		}
	}
	
	return 0;
}


