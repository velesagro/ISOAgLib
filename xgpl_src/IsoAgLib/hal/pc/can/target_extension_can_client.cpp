/***************************************************************************
              target_extension_can_A1_binary.cpp - source for the
                     PSEUDO BIOS for development and test on
                     the Opus A1 with CAN communication using
                     the Binary can drivers for /dev/wecan0
                     and /dev/wecan1

                             -------------------
    begin                : Tue Oct 2 2001
    copyright            : (C) 1999 - 2004 Dipl.-Inform. Achim Spangler
    email                : a.spangler@osb-ag:de
 ***************************************************************************/


/***************************************************************************
 *                                                                         *
 * This file is part of the "IsoAgLib", an object oriented program library *
 * to serve as a software layer between application specific program and   *
 * communication protocol details. By providing simple function calls for  *
 * jobs like starting a measuring program for a process data value on a    *
 * remote ECU, the main program has not to deal with single CAN telegram   *
 * formatting. This way communication problems between ECU's which use     *
 * this library should be prevented.                                       *
 * Everybody and every company is invited to use this library to make a    *
 * working plug and play standard out of the printed protocol standard.    *
 *                                                                         *
 * Copyright (C) 1999 - 2004 Dipl.-Inform. Achim Spangler                 *
 *                                                                         *
 * The IsoAgLib is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published          *
 * by the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This library is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with IsoAgLib; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
 *                                                                         *
 * As a special exception, if other files instantiate templates or use     *
 * macros or inline functions from this file, or you compile this file and *
 * link it with other works to produce a work based on this file, this file*
 * does not by itself cause the resulting work to be covered by the GNU    *
 * General Public License. However the source code for this file must still*
 * be made available in accordance with section (3) of the                 *
 * GNU General Public License.                                             *
 *                                                                         *
 * This exception does not invalidate any other reasons why a work based on*
 * this file might be covered by the GNU General Public License.           *
 *                                                                         *
 * Alternative licenses for IsoAgLib may be arranged by contacting         *
 * the main author Achim Spangler by a.spangler@osb-ag:de                  *
 ***************************************************************************/
#include "can_target_extensions.h"
#include <IsoAgLib/hal/pc/system/system.h>
#include <IsoAgLib/hal/pc/system/system_target_extensions.h>
#include <IsoAgLib/driver/can/impl/canio_c.h>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <list>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>


#include "can_msq.h"

namespace __HAL {


// globals
static msqData_s msqDataClient = {-1,-1,-1,-1,-1,-1};
static int32_t i32_lastReceiveTime = 0;


/////////////////////////////////////////////////////////////////////////
int16_t can_startDriver()
{

  msqCommand_s msqCommandBuf;

  DEBUG_PRINT("can_startDriver called\n");
#ifdef SYSTEM_WITH_ENHANCED_CAN_HAL
  printf("SYSTEM_WITH_ENHANCED_CAN_HAL is defined !\n");
#else
  printf("SYSTEM_WITH_ENHANCED_CAN_HAL is NOT defined !\n");
#endif

  int16_t i16_rc = ca_createMsqs(msqDataClient);

  if (i16_rc)
    // queue connect/creation failed
    return i16_rc;

  // client registration

  // use process id for own client id
  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_REGISTER;
  // call getTime just to be sure that start up time is set
  getTime();

  msqCommandBuf.s_startTimeClock.t_clock = getStartUpTime();

  i16_rc = send_command(&msqCommandBuf, &msqDataClient);

  if (i16_rc == HAL_NO_ERR) {

    if (msqCommandBuf.s_acknowledge.i32_dataContent != ACKNOWLEDGE_DATA_CONTENT_PIPE_ID)
    {
      if (msqCommandBuf.s_acknowledge.i32_dataContent == ACKNOWLEDGE_DATA_CONTENT_ERROR_VALUE)
        return msqCommandBuf.s_acknowledge.i32_data; // this is the error-code!
      else
        return HAL_UNKNOWN_ERR;
    }
    // else: PIPE_ID
    char pipe_name[255];
    sprintf(pipe_name, "%s%d", PIPE_PATH, msqCommandBuf.s_acknowledge.i32_data); // == i32_pipeId

    // pipe is already created by can server!

    // O_NONBLOCK because open in read only mode blocks!
    if ((msqDataClient.i32_pipeHandle = open(pipe_name, O_NONBLOCK | O_RDONLY)) == -1)
      return HAL_UNKNOWN_ERR;
  }

  return i16_rc;

}

int16_t can_stopDriver()
{
  msqCommand_s msqCommandBuf;

  DEBUG_PRINT("can_stopDriver called\n");

  if (msqDataClient.i32_pipeHandle)
    close(msqDataClient.i32_pipeHandle);

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_DEREGISTER;

  return send_command(&msqCommandBuf, &msqDataClient);

}


int16_t init_can ( uint8_t bBusNumber,uint16_t wGlobMask,uint32_t dwGlobMask,uint32_t dwGlobMaskLastmsg,uint16_t wBitrate )
{
  msqCommand_s msqCommandBuf;

  DEBUG_PRINT2("init_can, bus %d, bitrate %d\n", bBusNumber, wBitrate);

  if ( bBusNumber > HAL_CAN_MAX_BUS_NR ) return HAL_RANGE_ERR;

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_INIT;
  msqCommandBuf.s_init.ui8_bus = bBusNumber;
  msqCommandBuf.s_init.ui16_wGlobMask = wGlobMask;
  msqCommandBuf.s_init.ui32_dwGlobMask = dwGlobMask;
  msqCommandBuf.s_init.ui32_dwGlobMaskLastmsg = dwGlobMaskLastmsg;
  msqCommandBuf.s_init.ui16_wBitrate = wBitrate;

  return send_command(&msqCommandBuf, &msqDataClient);

};

int16_t changeGlobalMask( uint8_t bBusNumber,uint16_t wGlobMask,uint32_t dwGlobMask,uint32_t dwGlobMaskLastmsg )
{
  msqCommand_s msqCommandBuf;

  DEBUG_PRINT1("changeGlobalMask, bus %d\n", bBusNumber);

  if ( bBusNumber > HAL_CAN_MAX_BUS_NR ) return HAL_RANGE_ERR;

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_CHG_GLOBAL_MASK;
  msqCommandBuf.s_init.ui8_bus = bBusNumber;
  msqCommandBuf.s_init.ui16_wGlobMask = wGlobMask;
  msqCommandBuf.s_init.ui32_dwGlobMask = dwGlobMask;
  msqCommandBuf.s_init.ui32_dwGlobMaskLastmsg = dwGlobMaskLastmsg;

  return send_command(&msqCommandBuf, &msqDataClient);

};

int16_t closeCan ( uint8_t bBusNumber )
{
  msqCommand_s msqCommandBuf;

  DEBUG_PRINT1("closeCan, bus %d\n", bBusNumber);

  if ( bBusNumber > HAL_CAN_MAX_BUS_NR ) return HAL_RANGE_ERR;

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_CLOSE;
  msqCommandBuf.s_init.ui8_bus = bBusNumber;

  // read/write queue is cleared by server!
  return send_command(&msqCommandBuf, &msqDataClient);

};

int16_t chgCanObjPause ( uint8_t bBusNumber, uint8_t bMsgObj, uint16_t wPause)
{
  // just to remove compiler warnings
  bBusNumber = bBusNumber; bMsgObj = bMsgObj; wPause = wPause;
  DEBUG_PRINT2("chgCanObjPause, bus %d, obj %d\n", bBusNumber, bMsgObj);
/*
  fprintf(stderr,"sende Pause auf BUS %d fuer CAN Objekt %d auf %d eingestellt\n",
  (int32_t)bBusNumber, (int32_t)bMsgObj, wPause);
  */
  return HAL_NO_ERR;
}

int16_t getCanBusStatus(uint8_t /*bBusNumber*/, tCanBusStatus* /*ptStatus*/)
{
  //DEBUG_PRINT1("getCanBusStatus, bus %d\n", bBusNumber);

  return HAL_NO_ERR;
}


int16_t configCanObj ( uint8_t bBusNumber, uint8_t bMsgObj, tCanObjConfig* ptConfig )
{
  DEBUG_PRINT2("configCanObj, bus %d, obj %d\n", bBusNumber, bMsgObj);

  msqCommand_s msqCommandBuf;

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) || ( bMsgObj > cui8_maxCanObj-1 ) ) return HAL_RANGE_ERR;
#else
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) ) return HAL_RANGE_ERR;
#endif

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_CONFIG;
  msqCommandBuf.s_config.ui8_bus = bBusNumber;
  msqCommandBuf.s_config.ui8_obj = bMsgObj;
  msqCommandBuf.s_config.ui8_bXtd = ptConfig->bXtd;
  msqCommandBuf.s_config.ui8_bMsgType = ptConfig->bMsgType;
  msqCommandBuf.s_config.ui32_dwId = ptConfig->dwId;

#ifdef SYSTEM_WITH_ENHANCED_CAN_HAL
  msqCommandBuf.s_config.ui32_mask = ptConfig->mask;
#endif

  msqCommandBuf.s_config.ui16_wNumberMsgs = ptConfig->wNumberMsgs;

  // read/write queue is cleared by server!
  return send_command(&msqCommandBuf, &msqDataClient);

};



int16_t closeCanObj ( uint8_t bBusNumber,uint8_t bMsgObj )
{
  msqCommand_s msqCommandBuf;

  DEBUG_PRINT2("closeCanObj, bus %d, obj %d\n", bBusNumber, bMsgObj);

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) || ( bMsgObj > cui8_maxCanObj-1 ) ) return HAL_RANGE_ERR;
#else
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) ) return HAL_RANGE_ERR;
#endif

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_CLOSEOBJ;
  msqCommandBuf.s_config.ui8_bus = bBusNumber;
  msqCommandBuf.s_config.ui8_obj = bMsgObj;

  // read / write queue is cleared by server!
  return send_command(&msqCommandBuf, &msqDataClient);

};

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
int16_t chgCanObjId ( uint8_t bBusNumber, uint8_t bMsgObj, uint32_t dwId, uint8_t bXtd )
#else
int16_t chgCanObjId ( uint8_t bBusNumber, uint8_t bMsgObj, uint32_t dwId, uint32_t mask, uint8_t bXtd )
#endif
{

  DEBUG_PRINT2("chgCanObjId, bus %d, obj %d\n", bBusNumber, bMsgObj);
#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) || ( bMsgObj > cui8_maxCanObj-1 ) ) return HAL_RANGE_ERR;
#else
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) ) return HAL_RANGE_ERR;
#endif

  msqCommand_s msqCommandBuf;

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_CHG_CONFIG;
  msqCommandBuf.s_config.ui8_bus = bBusNumber;
  msqCommandBuf.s_config.ui8_obj = bMsgObj;
  msqCommandBuf.s_config.ui8_bXtd = bXtd;
  msqCommandBuf.s_config.ui32_dwId = dwId;

#ifdef SYSTEM_WITH_ENHANCED_CAN_HAL
  msqCommandBuf.s_config.ui32_mask = mask;
#endif

  return send_command(&msqCommandBuf, &msqDataClient);

}
/**
       lock a MsgObj to avoid further placement of messages into buffer.
  @param rui8_busNr number of the BUS to config
  @param rui8_msgobjNr number of the MsgObj to config
       @param rb_doLock true==lock(default); false==unlock
  @return HAL_NO_ERR == no error;
          HAL_CONFIG_ERR == BUS not initialised or ident can't be changed
          HAL_RANGE_ERR == wrong BUS or MsgObj number
       */
int16_t lockCanObj( uint8_t rui8_busNr, uint8_t rui8_msgobjNr, bool rb_doLock )
{

  msqCommand_s msqCommandBuf;

  DEBUG_PRINT3("lockCanObj, bus %d, obj %d, lock %d\n", rui8_busNr, rui8_msgobjNr, rb_doLock);

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  if ( ( rui8_busNr > HAL_CAN_MAX_BUS_NR ) || ( rui8_msgobjNr > cui8_maxCanObj-1 ) ) return HAL_RANGE_ERR;
#else
  if ( ( rui8_busNr > HAL_CAN_MAX_BUS_NR ) ) return HAL_RANGE_ERR;
#endif

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;

  if (rb_doLock)
    msqCommandBuf.i16_command = COMMAND_LOCK;
  else
    msqCommandBuf.i16_command = COMMAND_UNLOCK;

  msqCommandBuf.s_config.ui8_bus = rui8_busNr;
  msqCommandBuf.s_config.ui8_obj = rui8_msgobjNr;

  return send_command(&msqCommandBuf, &msqDataClient);

}

/**
       check if MsgObj is currently locked
  @param rui8_busNr number of the BUS to check
  @param rui8_msgobjNr number of the MsgObj to check
       @return true -> MsgObj is currently locked
*/
bool getCanMsgObjLocked( uint8_t rui8_busNr, uint8_t rui8_msgobjNr )
{

  int16_t i16_rc;
  msqCommand_s msqCommandBuf;

  DEBUG_PRINT2("getCanMsgObjLocked, bus %d, obj %d\n", rui8_busNr, rui8_msgobjNr);

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  if ( ( rui8_busNr > HAL_CAN_MAX_BUS_NR ) || ( rui8_msgobjNr > cui8_maxCanObj-1 ) ) return true;
#else
  if ( ( rui8_busNr > HAL_CAN_MAX_BUS_NR ) ) return true;
#endif
  else {

    msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
    msqCommandBuf.i16_command = COMMAND_QUERYLOCK;
    msqCommandBuf.s_config.ui8_bus = rui8_busNr;
    msqCommandBuf.s_config.ui8_obj = rui8_msgobjNr;

    i16_rc = send_command(&msqCommandBuf, &msqDataClient);

    if (!i16_rc)
    {
      if (msqCommandBuf.s_acknowledge.i32_dataContent != ACKNOWLEDGE_DATA_CONTENT_QUERY_LOCK)
        return false;
      else
        return msqCommandBuf.s_acknowledge.i32_data;
    }
    else
      return false;
  }

}


int16_t clearCanObjBuf(uint8_t bBusNumber, uint8_t bMsgObj)
{

  DEBUG_PRINT2("clearCanObjBuf, bus %d, obj %d\n", bBusNumber, bMsgObj);

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) || ( bMsgObj > cui8_maxCanObj-1 ) ) return HAL_RANGE_ERR;
#else
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) ) return HAL_RANGE_ERR;
#endif

  clearReadQueue(bBusNumber, bMsgObj, msqDataClient.i32_rdHandle, msqDataClient.i32_pid);
//clearWriteQueue(bBusNumber, bMsgObj, msqDataClient.i32_wrHandle, msqDataClient.i32_pid);

  return HAL_NO_ERR;
}


/** get last timestamp of CAN receive */
int32_t can_lastReceiveTime()
{
  DEBUG_PRINT("can_lastReceiveTime\n");

  return i32_lastReceiveTime;
}


int16_t getCanMsgBufCount(uint8_t bBusNumber,uint8_t bMsgObj)
{
  int16_t i16_rc;
  // in this direction (server->client), the sendPrio-flag is NEVER set, so we need only to check for FALSE assembled mtypes!
  if ((i16_rc = msgrcv(msqDataClient.i32_rdHandle, NULL, 0, assembleRead_mtype(msqDataClient.i32_pid, bBusNumber, bMsgObj), IPC_NOWAIT)) == -1)
    if (errno == E2BIG) {
      //DEBUG_PRINT("messages 1\n");
      return 1;
    }

  return 0;

};


bool waitUntilCanReceiveOrTimeout( uint16_t rui16_timeoutInterval )
{
  // for debug only
  //int32_t i32_endWait = getTime() + rui16_timeoutInterval;

  int16_t i16_rc;
  fd_set rfds;
  struct timeval s_timeout;
  static uint8_t ui8_buf[16];

  FD_ZERO(&rfds);
  FD_SET(msqDataClient.i32_pipeHandle, &rfds);

  s_timeout.tv_sec = 0;
  s_timeout.tv_usec = rui16_timeoutInterval * 1000;

  i16_rc = select(msqDataClient.i32_pipeHandle+1, &rfds, NULL, NULL, &s_timeout);

  // return true, when the timeout was NOT the trigger for coming back from select
  const bool cb_result = ( i16_rc > 0 )?true:false;

  if(i16_rc > 0 && FD_ISSET(msqDataClient.i32_pipeHandle, &rfds) > 0)
  {  // clear pipe (is done also in can server before next write)
    for (i16_rc = read(msqDataClient.i32_pipeHandle, &ui8_buf, 16); i16_rc == 16 ;
         i16_rc = read(msqDataClient.i32_pipeHandle, &ui8_buf, 16) );
  }

  return cb_result;
  /*
  if ( getTime() < i32_endWait ) std::cout << "#";
  else std::cout << ".";
  static int dbgCnt = 0;
  static int32_t i32_lastDebugLinefeed = 0;
  dbgCnt++;
  if ( ( dbgCnt % 50 ) == 0 )
  {
    std::cout << (getTime() - i32_lastDebugLinefeed)/50 << std::endl;
    i32_lastDebugLinefeed = getTime();
  }
  */
};

// MDS NOTE: This is code is highly inefficient and needs to be optimized!
//  The receive structure should be changed so that it does not need to be copied from
//  one structure to another 3 or 4 times before sending it to the higher levels.
//  This should be done at most once!
//  There are several different CAN msg receive structures defined, and that should be
//  changed to only one through this file, and the one defined externally.
int16_t getCanMsg ( uint8_t bBusNumber,uint8_t bMsgObj, tReceive * ptReceive )
{
  int16_t i16_rc;
  msqRead_s msqReadBuf;

  //DEBUG_PRINT2("getCanMsg, bus %d, obj %d\n", bBusNumber, bMsgObj);

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) || ( bMsgObj > cui8_maxCanObj-1 ) ) return HAL_RANGE_ERR;
#else
  if ( ( bBusNumber > HAL_CAN_MAX_BUS_NR ) ) return HAL_RANGE_ERR;
#endif

  const uint8_t cui8_useMsgObj = bMsgObj;

  // we assemble the mtype with FALSE here, as we do NOT have sendPriorities in this direction!
  if ((i16_rc = msgrcv(msqDataClient.i32_rdHandle, &msqReadBuf, sizeof(msqRead_s) - sizeof(int32_t), assembleRead_mtype(msqDataClient.i32_pid, bBusNumber, cui8_useMsgObj), IPC_NOWAIT)) == -1)
    return HAL_UNKNOWN_ERR;
  i32_lastReceiveTime = getTime();

  // data received
  // get access to data
  can_data* pc_data = &(msqReadBuf.s_canData);
  // copy data
  ptReceive->dwId = pc_data->i32_ident;
#ifdef SYSTEM_WITH_ENHANCED_CAN_HAL
  ptReceive->bMsgObj = pc_data->bMsgObj;
#endif
  ptReceive->bDlc = pc_data->b_dlc;
  // prevent timestamp which is in the future! (because of 10ms clock jitter)
  ptReceive->tReceiveTime.l1ms = (i32_lastReceiveTime > pc_data->i32_time) ? pc_data->i32_time : i32_lastReceiveTime;
  ptReceive->bXtd = pc_data->b_xtd;
  memcpy(ptReceive->abData, pc_data->pb_data, pc_data->b_dlc);

#ifdef DEBUG_CAN
  printf("Empfang: %x  %hx %hx %hx %hx %hx %hx %hx %hx\n", ptReceive->dwId,
         ptReceive->abData[0], ptReceive->abData[1], ptReceive->abData[2],
         ptReceive->abData[3], ptReceive->abData[4], ptReceive->abData[5],
         ptReceive->abData[6], ptReceive->abData[7]);
#endif

  return HAL_NO_ERR;

};



int16_t sendCanMsg ( uint8_t bBusNumber,uint8_t bMsgObj, tSend* ptSend )
{
  int16_t i16_rc;

  //DEBUG_PRINT2("sendCanMsg, bus %d, obj %d\n", bBusNumber, bMsgObj);
  msqWrite_s msqWriteBuf;
  msqCommand_s msqCommandBuf;

#ifdef DEBUG_CAN
  printf("Senden: %x  %hx %hx %hx %hx %hx %hx %hx %hx\n", ptSend->dwId,
         ptSend->abData[0], ptSend->abData[1], ptSend->abData[2],
         ptSend->abData[3], ptSend->abData[4], ptSend->abData[5],
         ptSend->abData[6], ptSend->abData[7]);
#endif

  memset(&msqWriteBuf, 0, sizeof(msqWrite_s));
  msqWriteBuf.i32_mtypePrioAnd1 = assembleWrite_mtype(__IsoAgLib::CANIO_c::sb_sendPrioritized);
  msqWriteBuf.ui16_pid = msqDataClient.i32_pid;
  msqWriteBuf.ui8_bus = bBusNumber;
  msqWriteBuf.ui8_obj = bMsgObj;
  memcpy(&(msqWriteBuf.s_sendData), ptSend, sizeof(tSend));
  msqWriteBuf.i32_sendTimeStamp = getTime();

  if (msgsnd(msqDataClient.i32_wrHandle, &msqWriteBuf, sizeof(msqWrite_s) - sizeof(int32_t), 0) == -1) {
    perror("msgsnd");
    return HAL_UNKNOWN_ERR;
  }

  // wait for ACK
  if((i16_rc = msgrcv(msqDataClient.i32_cmdAckHandle, &msqCommandBuf, sizeof(msqCommand_s) - sizeof(int32_t), msqDataClient.i32_pid, 0)) == -1)
    return HAL_UNKNOWN_ERR;

  if (msqCommandBuf.i16_command == COMMAND_ACKNOWLEDGE )
  {
    if (msqCommandBuf.s_acknowledge.i32_dataContent == ACKNOWLEDGE_DATA_CONTENT_ERROR_VALUE)
    {
#ifdef DEBUG
      if ((msqCommandBuf.s_acknowledge.i32_data != HAL_NO_ERR) && (msqCommandBuf.s_acknowledge.i32_data != HAL_NEW_SEND_DELAY)) // == i32_error
      {
        printf("acknowledge-error received in sendCanMsg\n");
      }
#endif
      return msqCommandBuf.s_acknowledge.i32_data;
    }
  }
  // other unexpected answer - shouldn't occur!
  return HAL_UNKNOWN_ERR;
};

/** @todo maybe make return code the error code and pass the ref to the value as parameter?
 * for now, simply no error code is generated! */
int32_t getMaxSendDelay(uint8_t rui8_busNr)
{
  msqCommand_s msqCommandBuf;

  DEBUG_PRINT1("getMaxSendDelay called, bus %d\n", rui8_busNr);

/*  if ( bBusNumber > HAL_CAN_MAX_BUS_NR ) return HAL_RANGE_ERR; */

  msqCommandBuf.i32_mtypePid = msqDataClient.i32_pid;
  msqCommandBuf.i16_command = COMMAND_SEND_DELAY;
  msqCommandBuf.s_config.ui8_bus = rui8_busNr;
  // the other fields of the s_config struct are NOT of interest here!

  int i16_rc = send_command(&msqCommandBuf, &msqDataClient);
  if (i16_rc == HAL_NO_ERR)
  { // we got an answer - see if it was ACKNOWLEDGE_DATA_CONTENT_SEND_DELAY
    if (msqCommandBuf.s_acknowledge.i32_dataContent == ACKNOWLEDGE_DATA_CONTENT_SEND_DELAY)
    { // yihaa, we got what we wanted!
      return msqCommandBuf.s_acknowledge.i32_data;
    }
  }

  perror("msgsnd");
  return 0; // we don't have no error code, we can just return some error-maxDelay instead
}

}; // end namespace __HAL
