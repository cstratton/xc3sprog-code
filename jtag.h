/* JTAG routines

Copyright (C) 2004 Andrew Rogers

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
*/



#ifndef JTAG_H
#define JTAG_H

#include <stdio.h>
#include <vector>

#include "iobase.h"
#include "bitrev.h"

#ifdef __WIN32__
#include <windows.h>
#endif

typedef unsigned char byte;

class Jtag
{
 public:
  enum tapState_t{
    TEST_LOGIC_RESET=0,
    RUN_TEST_IDLE=1,
    SELECT_DR_SCAN=2,
    CAPTURE_DR=3,
    SHIFT_DR=4,
    EXIT1_DR=5,
    PAUSE_DR=6,
    EXIT2_DR=7,
    UPDATE_DR=8,
    SELECT_IR_SCAN=9,
    CAPTURE_IR=10,
    SHIFT_IR=11,
    EXIT1_IR=12,
    PAUSE_IR=13,
    EXIT2_IR=14,
    UPDATE_IR=15,
    UNKNOWN=999
  };
 
 private:
  bool	      verbose;
  tapState_t  current_state;
  static const int MAXNUMDEVICES=1000;
 protected:
  struct chainParam_t
  {
    unsigned long idcode; // Store IDCODE
    //byte bypass[4]; // The bypass instruction. Most instruction register lengths are a lot less than 32 bits.
    int irlen; // instruction register length.
  };
  std::vector<chainParam_t> devices;
  IOBase *io;
  int numDevices;
  tapState_t postDRState;
  tapState_t postIRState;
  int deviceIndex;
  FILE *fp_svf;
  bool shiftDRincomplete;
  FILE *fp_dbg;
  const char* getStateName(tapState_t s);
 public:
  Jtag(IOBase *iob);
  ~Jtag();
  void setVerbose(bool v) { verbose = v; }
  bool getVerbose(void) { return verbose; }
  int getChain(bool detect = false); // Shift IDCODEs from devices
  inline void setPostDRState(tapState_t s){postDRState=s;}
  inline void setPostIRState(tapState_t s){postIRState=s;}
  void setTapState(tapState_t state, int pre=0);
  void tapTestLogicReset(void);
  void nextTapState(bool tms);
  void cycleTCK(int n, bool tdi=1);
  tapState_t getTapState(void);
  int setDeviceIRLength(int dev, int len);
  unsigned long getDeviceID(unsigned int dev){
    if(dev>=devices.size())return 0;
    return devices[dev].idcode;
  }
  void Usleep(unsigned int usec) {io->Usleep(usec);}
  int selectDevice(int dev);
  void shiftDR(const byte *tdi, byte *tdo, int length, int align=0, bool exit=true);// Some devices use TCK for aligning data, for example, Xilinx FPGAs for configuration data.
  void shiftIR(const byte *tdi, byte *tdo=0); // No length argumant required as IR length specified in chainParam_t 
  inline void longToByteArray(unsigned long l, byte *b){
    b[0]=(byte)(l&0xff);
    b[1]=(byte)((l>>8)&0xff);
    b[2]=(byte)((l>>16)&0xff);
    b[3]=(byte)((l>>24)&0xff);
  }
  inline void longToByteArrayRev(unsigned long l, byte *b){
    b[0]=bitRevTable[ l      & 0xff];
    b[1]=bitRevTable[(l>> 8) & 0xff];
    b[2]=bitRevTable[(l>>16) & 0xff];
    b[3]=bitRevTable[(l>>24) & 0xff];
  }
  inline void shortToByteArray(const unsigned short l, byte *b){
    b[0]=(byte)(l&0xff);
    b[1]=(byte)((l>>8)&0xff);
  }
  inline unsigned long byteArrayToLong(const byte *b){
    return ((unsigned long)b[3]<<24)+((unsigned long)b[2]<<16)+
      ((unsigned long)b[1]<<8)+(unsigned long)b[0];
  }
};

#endif //JTAG_H
