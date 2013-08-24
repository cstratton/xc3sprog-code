/* JTAG GNU/Linux parport device io

Copyright (C) 2004 Andrew Rogers
Additions for Byte Blaster Cable (C) 2005-2011  Uwe Bonnes 
                              bon@elektron.ikp.physik.tu-darmstadt.de

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Changes:
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
    Changes to support new IOBase interface.
    Support for byte counting and progress bar.
*/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#ifdef __linux__
// Default parport device
#ifndef PPDEV
#  define PPDEV "/dev/parport0"
#endif

#include <errno.h>
#endif
#include "ioparport.h"
#include "debug.h"

#define NO_CABLE 0
#define IS_PCIII 1
#define IS_BBLST 2

#define BIT_MASK(b)     (1<<(b))
							 
using namespace std;
int sfd;
int sendcount = 0;
unsigned char sendbuf[64];

void fatal_error(char *msg) {
  fprintf(stderr, "fatal fatal_error: %s\n", msg);
  // close(fd);
  _exit(-1);
}

void trysend(unsigned char *data, unsigned int count) {
  //  printf("sending %d\n", count);
  write(0, ".", 1);
  if (count > 64) fatal_error("send too long");
  if (write(sfd, data, count) != count) fatal_error("writefail");
  usleep(100);
}

void tryrecv(unsigned char *data) {
  int rv;
  int tries = 0;
  while ((rv=read(sfd, data, 1)) != 1) {
    //    break;
    if (rv != 0)     
      fatal_error("readfail");
    if (tries == 8) 
      fatal_error("read timeout");
    tries++;
    usleep((1 << tries) *250);
  }
}

void sendit() {
  if (sendcount) trysend(sendbuf, sendcount);
  sendcount = 0;
}

int  IOParport::detectcable(void)
{
  return 1;//IS_BBLIST; /* FIXME */
  //NO_CABLE
}

IOParport::IOParport() : IOBase(), total(0), debug(0) 
{
  printf("IOParport\n");
}

int IOParport::Init(struct cable_t *cable, const char *dev, unsigned int freq)
{
    int res;
    printf("::Init\n");
  // Try to obtain device from environment or use default if not given
  if(!dev) {
    if(!(dev = getenv("XCPORT")))  dev = PPDEV;
  }
 
  sendcount = 0;
  sfd = open (dev, O_RDWR);
  if (sfd < 0) fatal_error("cannot open tty");
  printf("opened %s = %d\n", dev, sfd);
  
  return 0;
}

bool IOParport::txrx(bool tms, bool tdi)
{
  if (sendcount) sendit();
  unsigned char ret;
  unsigned char data=0;
  if(tdi)data|=1; // D0 pin2
  if(tms)data|=4; // D2 pin4
  trysend(&data, 1);
  tryrecv(&data);

  return ((data & 1) != 0);
}
#if 0
bool IOParport::orig_txrx(bool tms, bool tdi)
{
  unsigned char ret;
  bool retval;
  unsigned char data=def_byte; // D4 pin5 TDI enable
  if(tdi)data|=tdi_value; // D0 pin2
  if(tms)data|=tms_value; // D2 pin4
  write_data(fd, data);
  data|=tck_value; // clk high D1 pin3
  write_data(fd, data);
  total++;
  read_status(fd, &ret);
  //data=data^2; // clk low
  //write_data(fd, data);
  //read_status(fd, &ret);
  retval = (ret&tdo_mask)?!tdo_inv:tdo_inv;
  if (debug & HW_FUNCTIONS)
    fprintf(stderr,"IOParport::txrx tms %s tdi %s tdo %s \n",
	    (tms)?"true ":"false", (tdi)?"true ":"false",
	    (retval)?"true ":"false");
  return retval; 
    
}

#endif

void IOParport::tx(bool tms, bool tdi)
{
  if (sendcount >= 64) {
    fatal_error("write size to big\n");
  }
  unsigned char data=0;//0x10; // D4 pin5 TDI enable
  if(tdi)data|=1; // D0 pin2
  if(tms)data|=4; // D2 pin4
  sendbuf[sendcount++] = data;
  if (sendcount == 64) sendit();
}

#if 0
void IOParport::orig_tx(bool tms, bool tdi)
{
  unsigned char data=def_byte; // D4 pin5 TDI enable
  if (debug & HW_FUNCTIONS)
    fprintf(stderr,"tx tms %s tdi %s\n",(tms)?"true ":"false",
	    (tdi)?"true ":"false");
  if(tdi)data|=tdi_value; // D0 pin2
  if(tms)data|=tms_value; // D2 pin4
  write_data(fd, data);
  //delay(2);
  data|=tck_value; // clk high 
  total++;
  write_data(fd, data);
  //delay(2);
  //data=data^2; // clk low
  //write_data(fd, data);
  //delay(2);
}
#endif 
void IOParport::tx_tdi_byte(unsigned char tdi_byte)
{
  int k;
  
  for (k = 0; k < 8; k++)
    tx(false, (tdi_byte>>k)&1);
}

void tx_no_clk(bool tms, bool tdi) {
  if (sendcount >= 64) {
    fatal_error("write size to big\n");
  }
  unsigned char data=0;//0x10; // D4 pin5 TDI enable
  if(tdi)data|=1; // D0 pin2
  if(tms)data|=4; // D2 pin4
  data |= 8;//INHIBIT CLOCK CYCLE
  sendbuf[sendcount++] = data;
  if (sendcount == 64) sendit();
}
 
void IOParport::txrx_block(const unsigned char *tdi, unsigned char *tdo,
			   int length, bool last)
{
  int i=0;
  int j=0;
  unsigned char tdo_byte=0;
  unsigned char tdi_byte;
  unsigned char data=def_byte;
  if (tdi)
      tdi_byte = tdi[j];
      
  while(i<length-1){
    //tdo_byte=tdo_byte+(txrx(false, (tdi_byte&1)==1)<<(i%8));
    if (tdo) 
      tdo_byte=tdo_byte+(txrx(false, (tdi_byte&1)==1)<<(i%8));
    else {
      tx(false, (tdi_byte&1)==1);
      tdo_byte=tdo_byte+(0<<(i%8)); 
    }     
    if (tdi)
      tdi_byte=tdi_byte>>1;
    i++;
    if((i%8)==0){ // Next byte
      if(tdo)
	tdo[j]=tdo_byte; // Save the TDO byte
      tdo_byte=0;
      j++;
      if (tdi)
	tdi_byte=tdi[j]; // Get the next TDI byte
    }
  };
  if (tdo)
    tdo_byte=tdo_byte+(txrx(last, (tdi_byte&1)==1)<<(i%8)); 
  else {
    tx(last, (tdi_byte&1)==1);
    tdo_byte=tdo_byte+(0<<(i%8)); 
  }  
  if(tdo)
    tdo[j]=tdo_byte;
  //write_data(fd, data); /* Make sure, TCK is low */
  tx_no_clk(false, false);
  return;
}
  
//void IOParport::force_tms_low(unsigned char *pat, int length, int force)

void IOParport::tx_tms(unsigned char *pat, int length, int force)
{
    int i;
    unsigned char tms;
    unsigned char data=def_byte;
    for (i = 0; i < length; i++)
    {
      if ((i & 0x7) == 0)
	tms = pat[i>>3];
      tx((tms & 0x01), true);
      tms = tms >> 1;
    }
    //    write_data(fd, data); /* Make sure, TCK is low */
    tx_no_clk(false, false);
}


IOParport::~IOParport()
{
  if (sfd) close(sfd);
  if (verbose) fprintf(stderr, "Total bytes sent: %d\n", total>>3);
}
#define XC3S_OK 0
#define XC3S_EIO 1
#define XC3S_ENIMPL 2


	
