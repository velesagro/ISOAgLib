/*
  hal_can_interface.cpp: CAN interface between BIOS/OS and IsoAgLib to
    concentrate CAN handling abstraction within one module

  (C) Copyright 2009 - 2010 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/

#include <IsoAgLib/isoaglib_config.h>
#include "../commercial_BIOS/bios_ams5/ams_bios.h"
#include <IsoAgLib/driver/can/impl/ident_c.h>
#include <IsoAgLib/driver/can/impl/canpkg_c.h>
#include <IsoAgLib/hal/ams5/system/system.h>
#include <IsoAgLib/hal/generic_utils/can/write_central_fifo.h>

#include <list>
#include <stddef.h>

#define CONFIG_CAN_MIN_TIME_AFTER_BUS_PROBLEM_FOR_BUS_OK 3000
/* ************************************* */
/* **** Some Modul Global Variables **** */
/* ************************************* */

// __IsoAgLib::Ident_c c_cinterfIdent;

/** array of 100msec. timeslice conters of received and sent msg per BUS [uint8_t] */
__near static uint16_t               gw_cinterfBusLoad[2][10];

/** actual index in gwBusLoad */
__near static uint8_t                gb_cinterfBusLoadSlice[2];

/** CAN bus error state and timestamps */
__near static canErrorStateStruct_t  gt_cinterfaceBusErrorState;

/** CAN message and configuration objekts */
__near static canMsg_t               gt_cinterfMsgobjSend;
__near static canConfigObj_t         gt_cinterfMsgobjConfig;

namespace __HAL
{

#ifndef USE_CAN_MEASURE_BUSLOAD
  #define USE_CAN_MEASURE_BUSLOAD
#endif

#ifdef USE_CAN_MEASURE_BUSLOAD
  void updateCanBusLoad(uint8_t aui8_busNr, uint8_t ab_dlc);
#endif

  static const uint32_t cui32_maxCanBusCnt = (HAL_CAN_MAX_BUS_NR + 1);
  static uint8_t ui8_cinterfLastSendBufCnt[cui32_maxCanBusCnt][16];

extern "C" {
   // ---------------------------------------------------------------------------------------------
   /** \name CAN Interrupt handler  */
   /*@{*/
/** user defined CAN IRQ Function
    @param aui8_busNr bus number [0,1]
    @param aui8_ObjNr Message Object number [1...14] (send)
    @param tCanregister pointer to the CAN register
    @param aui8_bXtd
    @return tCanregister
  */

void IwriteCentralCanfifo(uint8_t aui8_busNr,uint8_t aui8_ObjNr, canSlotMBox_t *tCanregister, uint8_t aui8_bXtd)
{

   int32_t i32_fbIndex = -1; /** initialization value*/
   uint32_t i32_msgId = 0 ;

    if (aui8_bXtd)
    {
     // extended format
        i32_msgId=  ( ((uint32_t)tCanregister->ba.eidl & 0x3f)      );   // get EID5-0
        i32_msgId+= ( ((uint32_t)tCanregister->ba.eidm & 0xff) <<  6);   // get EID13-6
        i32_msgId+= ( ((uint32_t)tCanregister->ba.eidh & 0x0f) << 14);   // get EID17-14
        i32_msgId+= ( ((uint32_t)tCanregister->ba.sidl & 0x3f) << 18);   // get SID5-0
        i32_msgId+= ( ((uint32_t)tCanregister->ba.sidh & 0x1f) << 24);   // get SID10-6

      #ifdef USE_CAN_MEASURE_BUSLOAD
      updateCanBusLoad(aui8_busNr, (tCanregister->ba.dlc + 4));
      #endif
    }
    else
    {
      // standard format
      i32_msgId =  (((uint32_t)tCanregister->ba.sidl & 0x3f)     );
      i32_msgId+=  (((uint32_t)tCanregister->ba.sidh & 0x1f) << 6);

      #ifdef USE_CAN_MEASURE_BUSLOAD
      updateCanBusLoad(aui8_busNr, (tCanregister->ba.dlc + 2));
      #endif
    }
   
    /** if the irQTable is not valid, maybe there is a reconfiguration,
    * so put all the received message in the FIFO
    */
    if(true == HAL::isIrqTable(aui8_busNr, aui8_ObjNr))
    {
      /** BIOS objects starts from 1 */
      HAL::findFilterBox(aui8_busNr, aui8_ObjNr,i32_msgId,&i32_fbIndex);
      if(i32_fbIndex == -1)
      {/** message discarded, no FB interested **/
        return;
        /** exit from the switch and doesn't write in the central fifo **/
      }
    }

    //TODO !!! in the function iFifoWrite define the last parameter as optional and pass the value to
    // function getIrqData
    bool b_ret = HAL::iFifoWrite(aui8_busNr,i32_fbIndex,i32_msgId,(void*)tCanregister,aui8_bXtd);

    #if DEBUG_FIFO_WRITE
    if(!b_ret)
    {
       INTERNAL_DEBUG_DEVICE << "Fifo FULL" << INTERNAL_DEBUG_DEVICE_ENDL;
    }
    if(i32_fbIndex == -1)
    {
      INTERNAL_DEBUG_DEVICE << "message received during the reconfiguration" << INTERNAL_DEBUG_DEVICE_ENDL;
    }
    #endif
  return;
}

/** user defined function to retrieve the data from tCanMsgReg  */
void getIrqData(void* inputData, HAL::fifoData_s* destination,uint8_t aui8_bXtd)
{
  canSlotMBox_t* tCanregister = (canSlotMBox_t*)inputData;
 // destination->bDlc = C0Slot1.ba.dlc;     /** len of the data **/
  destination->bDlc = tCanregister->ba.dlc;
  destination->bXtd = aui8_bXtd ;

  for(uint8_t ui8_i=0; ui8_i < destination->bDlc; ui8_i++)
  {
     //destination->abData[ui8_i] = C0Slot1.ba.data[ui8_i];
     destination->abData[ui8_i] = tCanregister->ba.data[ui8_i];
  }
}


} //extern C
   /* ************************************************** */
   /** \name Global Status Per BUS
    *  Functions for status check of global CAN BUS      */
   /* ************************************************** */
   /*@{*/


   /**
      test if the CAN BUS is in WARN state
      @param aui8_busNr number of the BUS to check (default 0)
      @return true == CAN BUS is in WARN state, else normal operation
   */
   bool can_stateGlobalWarn(uint8_t aui8_busNr)
   {
      // update CAN bus error state and timestamps
      AMSBIOS::can_getBusErrorState(aui8_busNr, &gt_cinterfaceBusErrorState);
      // check if the CAN module is placed in an error-passive state now
      return (gt_cinterfaceBusErrorState.statusReg&0x2000);
   };

   /**
      test if the CAN BUS is in OFF state
      @param aui8_busNr number of the BUS to check (default 0)
      @return true == CAN BUS is in OFF state, else normal operation
   */
   bool can_stateGlobalOff(uint8_t aui8_busNr)
   {
      // update CAN bus error state and timestamps
      AMSBIOS::can_getBusErrorState(aui8_busNr, &gt_cinterfaceBusErrorState);
      // check if the CAN module is placed in a bus-off state now
      return (gt_cinterfaceBusErrorState.statusReg&0x4000);
    };

   /**
      check if a send try on this BUS caused an Bit1Error
      (occurs if >1 controller try to send msg with same ident)
      ==> ISO 11783 forces stop of retries in this case for
          adress claim
          @param aui8_busNr number of the BUS to check
          @return true -> Bit1Err occured
   */
   bool can_stateGlobalBit1err(uint8_t aui8_busNr)
   {
      // update CAN bus error state and timestamps
      AMSBIOS::can_getBusErrorState(aui8_busNr, &gt_cinterfaceBusErrorState);
      return !(gt_cinterfaceBusErrorState.lastSuccTx>gt_cinterfaceBusErrorState.lastBit1Err);
   };

   /**
      test if the CAN BUS is in Blocked state, which can be a sign
      for CAN controllers which are configured with different baudrates;
      this is the case if neither succesfull sent nor received msg
      is detcted AND CAN controller is in WARN or OFF state
      (the time since last succ. send/rec and the time of WARN/OFF
      can be defined with CONFIG_CAN_MIN_TIME_AFTER_BUS_PROBLEM_FOR_BUS_OK
      -> should not be to short to avoid false alarm)
      @param aui8_busNr number of the BUS to check (default 0)
      @return true == CAN BUS is in blocked state, else normal operation
   */
   bool can_stateGlobalBlocked(uint8_t aui8_busNr)
   {
      bool b_busBlocked = true;
      const int32_t i32_now = AMSBIOS::sys_getSystemTimer();

      /**< update CAN bus error state and timestamps */
      AMSBIOS::can_getBusErrorState(aui8_busNr, &gt_cinterfaceBusErrorState);

      /**< set b_busBlocked to false, if sign for correct work was detected */

      // check if successful reception within defined time
     if ((i32_now-gt_cinterfaceBusErrorState.lastSuccTx)<CONFIG_CAN_MAX_SEND_WAIT_TIME)
         b_busBlocked = false;

      // check if successful reception within defined time
      if ((i32_now-gt_cinterfaceBusErrorState.lastSuccRx)<CONFIG_CAN_MAX_SEND_WAIT_TIME)
         b_busBlocked = false;

      // check if no Error-Passive State was active for defined time
      if ((i32_now-gt_cinterfaceBusErrorState.lastErrPas)>CONFIG_CAN_MIN_TIME_AFTER_BUS_PROBLEM_FOR_BUS_OK)
         b_busBlocked = false;

      // check if no Bus-Off State was active for defined time
      if ((i32_now-gt_cinterfaceBusErrorState.lastBusOff)>CONFIG_CAN_MIN_TIME_AFTER_BUS_PROBLEM_FOR_BUS_OK)
         b_busBlocked = false;

      // check if no Bit1-Error happened for defined time
      if ((i32_now-gt_cinterfaceBusErrorState.lastBit1Err)>CONFIG_CAN_MIN_TIME_AFTER_BUS_PROBLEM_FOR_BUS_OK)
         b_busBlocked = false;

      return b_busBlocked;
   };


#ifdef USE_CAN_MEASURE_BUSLOAD
   /**
      deliver the baudrate of the CAN BUS in [kbaud]
      @param aui8_busNr number of the BUS to check (default 0)
      @return BUS load of the last second [kbaud]
   */
   int32_t can_stateGlobalBusload(uint8_t aui8_busNr)
   {
      if (aui8_busNr > 1) return 0;

      int32_t i32_baudrate = 0;

      for (uint8_t ui8_ind = 0; ui8_ind < 10; ui8_ind++)
      {
         i32_baudrate += (gw_cinterfBusLoad[aui8_busNr][ui8_ind] * 8);
      }
      return i32_baudrate;
   }


   /**
      update the CAN BUS load statistic
      @param aui8_busNr BUS number to update
      @param ab_dlc length of the detected (send or receive) message
   */
   void updateCanBusLoad(uint8_t aui8_busNr, uint8_t ab_dlc)
   {
      if (aui8_busNr > 1) return;

      int32_t i32_now = AMSBIOS::sys_getSystemTimer();
      uint8_t b_newSlice = ((i32_now / 100)%10);

      if (gb_cinterfBusLoadSlice[aui8_busNr] != b_newSlice)
      {
         gw_cinterfBusLoad[aui8_busNr][b_newSlice] = ab_dlc;
         gb_cinterfBusLoadSlice[aui8_busNr] = b_newSlice;
      }
      else
      {
         gw_cinterfBusLoad[aui8_busNr][b_newSlice] += ab_dlc;
      }
   }
#endif


   /*@}*/

   /* ************************************************** */
   /** \name Specific for one MsgObj
    *  Functions for status check of a single MsgObj     */
   /* ************************************************** */
   /*@{*/

   /**
     deliver the timestamp of last successfull CAN send action
     @param aui8_busNr number of the BUS to check  [0..1]
     @param aui8_msgobjNr number of the MsgObj to check [0..15] is ignorred
     @return timestamp of last successful send
             OR -1 if aui8_msgObjNr corresponds to no valid send obj
   */
   int32_t can_stateMsgobjTxok(uint8_t aui8_busNr, uint8_t aui8_msgobjNr)
   {
      if ( ( aui8_busNr > 1 ) || ( aui8_msgobjNr> 15 ) ) return 0;

      // check direction and open/close state of MSgObj: 0==RX, 1==TX, 0xFF==closed
      if (AMSBIOS::can_getMsgObjDirState(aui8_busNr, aui8_msgobjNr) != 1)
      {
         return -1;
      }
      else
      {
         // update CAN bus error state and timestamps
         AMSBIOS::can_getBusErrorState(aui8_busNr, &gt_cinterfaceBusErrorState);
         return gt_cinterfaceBusErrorState.lastSuccTx;

         // for debug with bios sourcecode #define DEBUG_RXTX 1 in ams_can.cpp module
         // return AMSBIOS::canLastSuccRxTx[aui8_busNr][aui8_msgobjNr];
      }
   }


   /**
     check if a send MsgObj can't send msgs from buffer to the
     BUS (detecetd by comparing the inactive time with
     CONFIG_CAN_MAX_SEND_WAIT_TIME (defined in isoaglib_config)
     @param rui8_busNr number of the BUS to check  [0..1]
     @param rui8_msgobjNr number of the MsgObj to check [0..15] is ignorred
     @return true -> longer than CONFIG_CAN_MAX_SEND_WAIT_TIME no msg sent on BUS
   */
   bool can_stateMsgobjSendproblem(uint8_t rui8_busNr, uint8_t rui8_msgobjNr)
   {
      if ( ( rui8_busNr > 1 ) || ( rui8_msgobjNr> 15 ) ) return true;
      const int32_t i32_now = AMSBIOS::sys_getSystemTimer();

      // update CAN bus error state and timestamps
      AMSBIOS::can_getBusErrorState(rui8_busNr, &gt_cinterfaceBusErrorState);

      if (  ((i32_now-gt_cinterfaceBusErrorState.lastSuccTx)>CONFIG_CAN_MAX_SEND_WAIT_TIME)
          &&(AMSBIOS::can_getRingBufferSize(rui8_busNr, rui8_msgobjNr)>0) )
           return true;
      else return false;

      // for debug with bios sourcecode #define DEBUG_RXTX 1 in ams_can.cpp module
      // and use AMSBIOS::canLastSuccRxTx[rui8_busNr][rui8_msgobjNr];
   }

   /**
      test if buffer of a MsgObj is full (e.g. no more
      msg can be put into buffer (important for TX objects))
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return true -> buffer is full -> no further can_send allowed
   */
   bool can_stateMsgobjOverflow(uint8_t aui8_busNr, uint8_t aui8_msgobjNr)
   {
      return (AMSBIOS::can_isRingBufferFull( aui8_busNr, aui8_msgobjNr ) == 1 );
   };

   /**
      deliver amount of messages in buffer
      (interesting for RX objects)
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return number of messages in buffer or negative error code:
              HAL_CONFIG_ERR == BUS not initialised
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_stateMsgobjBuffercnt(uint8_t aui8_busNr, uint8_t aui8_msgobjNr)
   {
      if ( ( aui8_busNr > 1 ) || ( aui8_msgobjNr> 15 ) ) return HAL_RANGE_ERR;
//      if (!AMSBIOS::canInitDone[aui8_busNr])             return HAL_CONFIG_ERR;
      return AMSBIOS::can_getRingBufferSize( aui8_busNr, aui8_msgobjNr );
   };

   /**
      deliver amount of messages which can be placed in buffer
      (interesting for TX objects)
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return number of messages which can be placed in buffer or negative error code:
              HAL_CONFIG_ERR == BUS not initialised
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_stateMsgobjFreecnt(uint8_t aui8_busNr, uint8_t aui8_msgobjNr)
   {
      if ( ( aui8_busNr > 1 ) || ( aui8_msgobjNr> 15 ) ) return HAL_RANGE_ERR;

      // check direction and open/close state of MSgObj: 0==RX, 1==TX, 0xFF==closed
      if (AMSBIOS::can_getMsgObjDirState(aui8_busNr, aui8_msgobjNr) == 0xFF) return HAL_CONFIG_ERR;

      return AMSBIOS::can_getMsgObjFreeCnt(aui8_busNr, aui8_msgobjNr);

   };

   /**
      check if MsgObj is currently locked
      @param rui8_busNr number of the BUS to check
      @param rui8_msgobjNr number of the MsgObj to check
      @return true -> MsgObj is currently locked
   */
   bool can_stateMsgobjLocked(uint8_t rui8_busNr, uint8_t rui8_msgobjNr)
   {
      return (AMSBIOS::can_isMsgObjLocked(rui8_busNr, rui8_msgobjNr) == 1 );
   };

   /*@}*/

   /* ************************************************** */
   /** \name Global Configuration Per BUS
    *  Functions to configure the global CAN BUS         */
   /* ************************************************** */
   /*@{*/

   /**
      init CAN BUS with given baudrate and masks
      (11bit, 29bit and last msg mask)
      @param aui8_busNr number of the BUS to init (default 0)
      @param ab_baudrate baudrate in [kbaud]
      @param aui16_maskStd 11bit global mask
      @param aui32_maskExt 29bit global mask
      @param aui32_maskLastmsg mask of last CAN msg
      @return HAL_NO_ERR == no error;
              HAL_RANGE_ERR == wrong BUS nr or wrong baudrate;
              HAL_WARN_ERR == BUS previously initialised - no problem if only masks had to be changed
   */
   int16_t can_configGlobalInit(uint8_t aui8_busNr, uint16_t ab_baudrate, uint16_t aui16_maskStd, uint32_t aui32_maskExt, uint32_t aui32_maskLastmsg)
   {
      uint16_t cinterfaceBaudrate;

      switch (ab_baudrate)                     /*! convert baudrate for BIOS: */
      {
         case 50:   cinterfaceBaudrate = 0;    /*! -  50 kBit */
                    break;
         case 100:  cinterfaceBaudrate = 1;    /*! - 100 kBit */
                    break;
         case 125:  cinterfaceBaudrate = 2;    /*! - 125 kBit */
                    break;
         case 250:  cinterfaceBaudrate = 3;    /*! - 250 kBit */
                    break;
         case 500:  cinterfaceBaudrate = 4;    /*! - 500 kBit */
                    break;
         case 1000: cinterfaceBaudrate = 5;    /*! -   1 MBit */
                    break;
         default:   return HAL_RANGE_ERR;
      }

      canMaskStruct_t cinterfaceAcceptanceMasks;

      // Global mask register (Slot 0 to 13)
      cinterfaceAcceptanceMasks.GM_IdType  = 1; // extended format
      cinterfaceAcceptanceMasks.GlobalMask = aui32_maskExt;

       // Local mask A register (Slot 14)
      cinterfaceAcceptanceMasks.LMA_IdType = 1; // extended format
      cinterfaceAcceptanceMasks.LocalMaskA = aui32_maskExt;

      // Local mask B register (Slot 15) -> mask of last CAN msg
      cinterfaceAcceptanceMasks.LMB_IdType = 1; // extended format
      cinterfaceAcceptanceMasks.LocalMaskB = aui32_maskLastmsg;

      return AMSBIOS::can_openBus(aui8_busNr, cinterfaceBaudrate, &cinterfaceAcceptanceMasks);
   };


   /**
      set the global masks
      (11bit, 29bit and last msg mask)
      @param aui8_busNr number of the BUS to config (default 0)
      @param aui16_maskStd 11bit global mask
      @param aui32_maskExt 29bit global mask
      @param aui32_maskLastmsg mask of last CAN msg
      @return HAL_NO_ERR == no error;
              HAL_RANGE_ERR == wrong BUS nr
   */
   int16_t can_configGlobalMask(uint8_t aui8_busNr, uint16_t aui16_maskStd, uint32_t aui32_maskExt, uint32_t aui32_maskLastmsg)
   {
      canMaskStruct_t cinterfaceAcceptanceMasks;

      // Global mask register (Slot 0 to 13)
      cinterfaceAcceptanceMasks.GM_IdType  = 1; // extended format
      cinterfaceAcceptanceMasks.GlobalMask = aui32_maskExt;

       // Local mask A register (Slot 14)
      cinterfaceAcceptanceMasks.LMA_IdType = 1; // extended format
      cinterfaceAcceptanceMasks.LocalMaskA = aui32_maskExt;

      // Local mask B register (Slot 15) -> mask of last CAN msg
      cinterfaceAcceptanceMasks.LMB_IdType = 1; // extended format
      cinterfaceAcceptanceMasks.LocalMaskB = aui32_maskLastmsg;

      // if the bus is already opened, only the AcceptanceMasks will be changed
      return AMSBIOS::can_openBus(aui8_busNr, 0, &cinterfaceAcceptanceMasks);
   };



   /**
      close CAN BUS
      (important to close BUS before re-init with other baudrate)
      @param aui8_busNr number of the BUS to close (default 0)
      @return HAL_NO_ERR == no error;
              HAL_RANGE_ERR == wrong BUS nr;
              HAL_CONFIG_ERR == BUS previously not initialised
   */
   int16_t can_configGlobalClose(uint8_t aui8_busNr)
   {
     return AMSBIOS::can_closeBus(aui8_busNr);
   };

#ifndef MAX_SLEEP_SLICE
  #define MAX_SLEEP_SLICE 5
#endif
/** wait until specified timeout or until next CAN message receive
 *  @return true -> there are CAN messages waiting for process. else: return due to timeout
 */
bool can_waitUntilCanReceiveOrTimeout( uint16_t aui16_timeoutInterval )
{
  const int32_t ci32_endWait = AMSBIOS::sys_getSystemTimer() + aui16_timeoutInterval;
  int32_t i32_waitSlice = aui16_timeoutInterval;

  // if greater than MAX_SLEEP_SLICE msec -> divide so that about 10 slices are realized
  if ( i32_waitSlice > MAX_SLEEP_SLICE ) i32_waitSlice /= 10;
  // if still more than 10msec slice limit to MAX_SLEEP_SLICE
  if ( i32_waitSlice > MAX_SLEEP_SLICE ) i32_waitSlice = MAX_SLEEP_SLICE;


  while (true)
  {
    // check whether any message is waiting for processing in central FIFO
  // return true, as soon as at least one message is available
    for ( unsigned int busInd = 0; busInd < cui32_maxCanBusCnt; busInd++)
    {

        /** a message is available */
        if (HAL::iFifoIsMsgAvailable(busInd)) return true;
     }

//    delay_us( i32_waitSlice * 1000 );
    if ( AMSBIOS::sys_getSystemTimer() >= ci32_endWait ) return false;
  }
}

   /*@}*/

   /* ************************************************** */
   /** \name Configuration specific for one MsgObj
    *  Functions to configure a specific MsgObj          */
   /* ************************************************** */
   /*@{*/

   /**
      config a MsgObj
      (buffer size is defined locally in implementation of this interface;
      all received msgs should get a time stamp;
      class Ident_c has ident and type 11/29bit)
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @param arc_ident filter ident of this MsgObj
      @param ab_rxtx 0==RX receive; 1==TX transmit
      @return HAL_NO_ERR == no error;
           C_BUSY == this MsgObj is already used
           HAL_CONFIG_ERR == BUS not initialised or error during buffer allocation
           HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_configMsgobjInit(uint8_t aui8_busNr, uint8_t aui8_msgobjNr, __IsoAgLib::Ident_c& arc_ident, uint8_t ab_rxtx)
   {
      if ( ( aui8_busNr > 1 ) || ( aui8_msgobjNr> 15 ) ) return HAL_RANGE_ERR;
//      if (!AMSBIOS::canInitDone[aui8_busNr])             return HAL_CONFIG_ERR;

      canConfigObj_pt pt_config = &gt_cinterfMsgobjConfig;

      pt_config->objNr = aui8_msgobjNr;
      pt_config->dwId    = arc_ident.ident();


      pt_config->idType = arc_ident.identType();

      if (ab_rxtx == 0)
      {  // receive
         pt_config->direction = 0; // receive (0) or transmit (1)
         pt_config->pfIrqFunction = (tIRQ_FUNCTION)IwriteCentralCanfifo;
         pt_config->bufferSize = 0;

      }
      else
      {  // send
         pt_config->direction = 1; // receive (0) or transmit (1)
         pt_config->pfIrqFunction = 0;
         pt_config->bufferSize = CONFIG_CAN_SEND_BUFFER_SIZE;

  #ifdef USE_CAN_SEND_DELAY_MEASUREMENT
  // clear send timestamp list
    list_sendTimeStamps.erase(list_sendTimeStamps.begin(),list_sendTimeStamps.end());
  #endif
      }

      pt_config->bufferLock = false;

      // config a CAN MsgObj with Config-Objekt
      return AMSBIOS::can_openMsgObj(aui8_busNr, pt_config);
   };



   /**
      close a MsgObj
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised, MsgObj previously not used or buffer memory not freed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_configMsgobjClose(uint8_t aui8_busNr, uint8_t aui8_msgobjNr)
   {
      return AMSBIOS::can_closeMsgObj(aui8_busNr, aui8_msgobjNr);
   };

   /*@}*/

   /* ******************************************** */
   /** \name Use of MsgObj
    *  Functions to use a specific MsgObj          */
   /* ******************************************** */
   /*@{*/

   /**
      send a message via a MsgObj;
      CanPkg_c (or derived object) must provide (virtual)
      functions:
      * Ident_c& getIdent() -> deliver ident of msg to send
      * void getData(MASK_TYPE& rt_ident, uint8_t& rui8_identType,
                     uint8_t& rb_dlcTarget, uint8_t* pb_dataTarget)
        -> put DLC in referenced r_dlc and insert data in uint8_t string pb_data
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @param apc_data pointer to CanPkg_c instance with data to send
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised, MsgObj is no send object
              HAL_NOACT_ERR == BUS OFF
              HAL_OVERFLOW_ERR == send buffer overflowed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_useMsgobjSend(uint8_t aui8_busNr, uint8_t aui8_msgobjNr, __IsoAgLib::CanPkg_c* apc_data)
   {
     // check if some msg were sent from buffer
      canBuffer_pt pt_send = &gt_cinterfMsgobjSend;

      uint8_t b_count = ui8_cinterfLastSendBufCnt[aui8_busNr][aui8_msgobjNr];

      __IsoAgLib::Ident_c at_ident (pt_send->dwId, (pt_send->idType == 1) ? __IsoAgLib::Ident_c::ExtendedIdent : __IsoAgLib::Ident_c::StandardIdent);

      // CanPkgExt_c::getData transforms flag data to ident and 8byte string
      apc_data->getData(pt_send->dwId, pt_send->idType, pt_send->dlc, pt_send->data);

      // pt_send->id = apc_data->ident();
      // if (apc_data->identType() == 1)
      // CanPkg_c::ident() and CanPkg_c::identType() changed to static
      // pt_send->id = __IsoAgLib::CanPkg_c::ident();

      if (pt_send->idType == 1)
      { // extended 29bit ident
         updateCanBusLoad(aui8_busNr, (pt_send->dlc + 4));
         // pt_send->bXtd = 1;
      }
      else
      { // standard 11bit ident
         // pt_send->bXtd = 0;
         updateCanBusLoad(aui8_busNr, (pt_send->dlc + 2));
      }

      return AMSBIOS::can_writeMsgObjTx(aui8_busNr, aui8_msgobjNr, pt_send);
   };

   /**
      clear the buffer of a MsgObj (e.g. to stop sending retries)
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_useMsgobjClear(uint8_t aui8_busNr, uint8_t aui8_msgobjNr)
   {
      AMSBIOS::can_clearMsgObj(aui8_busNr, aui8_msgobjNr);
      return HAL_NO_ERR;
   };
/**
      lock a MsgObj to avoid further placement of messages into buffer.
      @param rui8_busNr number of the BUS to config
      @param rui8_msgobjNr number of the MsgObj to config
      @param rb_doLock true==lock(default); false==unlock
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised or ident can't be changed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
     */
   int16_t can_configMsgobjLock( uint8_t rui8_busNr, uint8_t rui8_msgobjNr, bool rb_doLock )
   {
      return AMSBIOS::can_setMsgObjLockBit(rui8_busNr, rui8_msgobjNr, rb_doLock);
   };

  /**
      change the Ident_c of an already initialised MsgObj
      (class __IsoAgLib::Ident_c has ident and type 11/29bit)
      @param rui8_busNr number of the BUS to config
      @param rui8_msgobjNr number of the MsgObj to config
      @param rrefc_ident filter ident of this MsgObj
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised or ident can't be changed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_configMsgobjChgid(uint8_t rui8_busNr, uint8_t rui8_msgobjNr, __IsoAgLib::Ident_c& rrefc_ident)
   {
      return AMSBIOS::can_setMsgObjRxID(rui8_busNr, rui8_msgobjNr, rrefc_ident.ident(), rrefc_ident.identType());
   };


   /*@}*/
   /**
      get the ident of a received message to decide about the further
      processing before the whole data string is retreived
      @param rui8_busNr number of the BUS to config
      @param rui8_msgobjNr number of the MsgObj to config
      @param reflIdent reference to the var, where the ident should be inserted
      @return error code
              HAL_NO_ERR == No problem
              HAL_CONFIG_ERR == BUS not initialised, MsgObj is no RX object
              HAL_NOACT_ERR == BUS OFF
              HAL_OVERFLOW_ERR == send buffer overflowed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
              HAL_WARN_ERR == BUS WARN or no received message
   */
   int32_t can_useMsgobjReceivedIdent(uint8_t rui8_busNr, uint8_t rui8_msgobjNr, int32_t &reflIdent)
   {
      int16_t i16_retVal = HAL_NO_ERR;
      canBuffer_pt pt_receive = NULL;

      i16_retVal = AMSBIOS::can_readMsgObjRx(rui8_busNr, rui8_msgobjNr, &pt_receive);

      if ((i16_retVal == HAL_NO_ERR) || (HAL_OVERFLOW_ERR) || (HAL_WARN_ERR))
      {
         reflIdent = pt_receive->dwId;
      }
      return i16_retVal;
   };

   /**
      transfer front element in buffer into the pointed CANPkg_c;
      DON'T clear this item from buffer.
      @see can_useMsgobjPopFront for explicit clear of this front item
      functions:
      * setIdent(Ident_c& rrefc_ident)
        -> set ident rrefc_ident of received msg in CANPkg
      * uint8_t setDataFromString(uint8_t* rpb_data, uint8_t rb_dlc)
        -> set DLC in CANPkg_c from rb_dlc and insert data from uint8_t string rpb_data
      @param rui8_busNr number of the BUS to config
      @param rui8_msgobjNr number of the MsgObj to config
      @param rpc_data pointer to CANPkg_c instance with data to send
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised, MsgObj is no RX object
              HAL_NOACT_ERR == BUS OFF
              HAL_OVERFLOW_ERR == send buffer overflowed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
              HAL_WARN_ERR == BUS WARN or no received message
   */
   int16_t can_useMsgobjGet(uint8_t rui8_busNr, uint8_t rui8_msgobjNr, __IsoAgLib::CANPkg_c* rpc_data)
   {
      int16_t i16_retVal = HAL_NO_ERR;
      canBuffer_pt pt_receive = NULL;

      i16_retVal = AMSBIOS::can_readMsgObjRx(rui8_busNr, rui8_msgobjNr, &pt_receive);

      if ((i16_retVal == HAL_NO_ERR) || (HAL_OVERFLOW_ERR) || (HAL_WARN_ERR))
      {
         __IsoAgLib::CANPkg_c::setTime(pt_receive->timestamp);
         __IsoAgLib::Ident_c::identType_t IDType;

         if (pt_receive->idType == true)
         { // extended 29bit ident
            IDType = __IsoAgLib::Ident_c::ExtendedIdent;
            updateCanBusLoad(rui8_busNr, (pt_receive->dlc + 4));
         }
         else
         { // standard  11bit ident
            IDType = __IsoAgLib::Ident_c::StandardIdent;
            updateCanBusLoad(rui8_busNr, (pt_receive->dlc + 2));
         }
         // rpc_data->setIdent(pt_receive->dwId, idType);
         // CANPkg_c::setIdent changed to static member function
         __IsoAgLib::CANPkg_c::setIdent(pt_receive->dwId, IDType);
         rpc_data->setDataFromString(pt_receive->data, pt_receive->dlc);
      }
      return i16_retVal;
   };

   /**
      Either register the currenct front item of buffer as not relevant,
      or just pop the front item, as it was processed.
      This explicit pop is needed, as one CAN message shall be served to
      several CANCustomer_c instances, as long as one of them indicates a
      succesfull process of the received message.
      @param rui8_busNr number of the BUS to config
      @param rui8_msgobjNr number of the MsgObj to config
   */
   void can_useMsgobjPopFront(uint8_t rui8_busNr, uint8_t rui8_msgobjNr)
   {
      AMSBIOS::can_popDataFromRingBuffer(rui8_busNr, rui8_msgobjNr);
   };

   /*@}*/
}  // end of namespace __HAL
