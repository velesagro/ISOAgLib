/***************************************************************************
             hal_can_interface.cpp - CAN interface between
                           BIOS/OS and IsoAgLib to concentrate
                           CAN handling abstraction within
                           one module
                -------------------
    begin    : 12 Jun 2006
    copyright: (C) 2006 Joel T. Morton (DICKEY-john Corp.)
    email    : jmorton@dickey-john:com
    type     : Source
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
 * Copyright (C) 1999 - 2004 Dipl.-Inform. Achim Spangler                  *
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

/* **********************************************************
 * The module halCanInterface implements a standard
 * interface for the CAN interactions of the IsoAgLib
 * This way the the adoption of the IsoAgLib can be
 * better restricted to the various BIOS specific the
 * directories xgpl_src/IsoAgLib/hal/xx/can/ .
 * ********************************************************** */

#include "../typedef.h"
#include "../config.h"
#include "../errcodes.h"
#include "hal_can_interface.h"

#include <IsoAgLib/driver/can/impl/ident_c.h>
#include <IsoAgLib/driver/can/impl/canpkg_c.h>


namespace __HAL 
{
  extern "C" 
  {
    /** include the BIOS specific header with the part for CAN into __HAL */
    #include <commercial_BIOS/bios_Dj1/DjBiosMVT.h>
  }

  /**
    Get the Elaped time since the last sent message for the given 
    message object
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return elapse_time  number of ms since last message on object
  */
  int32_t Can_TxObjElapseTime ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr )
  {
    int32_t LastTime = __HAL::DjBios_CanObjTxTimeStamp ( aui8_busNr, aui8_msgobjNr );
    int32_t Now = __HAL::DjBios_TimeGetNow();

    /* If there is nothing in the Queue waiting to be sent, the time must be 
       zero.  This is really a time that the message has been waiting in 
       the queue */
    if ( DjBios_CanObjBufCount(aui8_busNr, aui8_msgobjNr) > 0 )
    {
      if ( Now >= LastTime )
      {
        return ( Now - LastTime );    
      } /* end if() */
      else
      {
        return ( 0x7FFFFF );   /* if times are invalid, use a very large time */
      } /* end else() */
    }
    else
    {
      return ( 0 );
    }
  } /* end RxObjElapseTime() */


  /**
    deliver amount of messages in buffer
    (interesting for RX objects)
    @param aui8_busNr number of the BUS to check
    @param aui8_msgobjNr number of the MsgObj to check
    @return number of messages in buffer or negative error code:
      HAL_CONFIG_ERR == BUS not initialised
      HAL_RANGE_ERR == wrong BUS or MsgObj number
  */
  uint16_t Can_ObjBufferCount ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr )
  {
    uint16_t Count = __HAL::DjBios_CanObjBufCount ( aui8_busNr, aui8_msgobjNr );

    if ( Count == (uint16_t)BIOS_CAN_BAD_BUS || Count == (uint16_t)BIOS_CAN_BAD_OBJ )
    {
      Count = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Count == (uint16_t)BIOS_CAN_QUEUE_INIT_ERR )
    {
      Count = HAL_CONFIG_ERR;
    } /* end if() */

    return ( Count ); 
    
  } /* end Can_RxObjBufferCount() */


  /**
    deliver amount of messages which can be placed in buffer
    (interesting for TX objects)
    @param aui8_busNr number of the BUS to check
    @param aui8_msgobjNr number of the MsgObj to check
    @return number of messages which can be placed in buffer or negative error code:
      HAL_CONFIG_ERR == BUS not initialised
      HAL_RANGE_ERR == wrong BUS or MsgObj number
  */
  int16_t Can_ObjBufferSpace ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr )
  {
    uint16_t Count = __HAL::DjBios_CanObjBufSpace ( aui8_busNr, aui8_msgobjNr );

    if ( Count == (uint16_t)BIOS_CAN_BAD_BUS || Count == (uint16_t)BIOS_CAN_BAD_OBJ )
    {
      Count = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Count == (uint16_t)BIOS_CAN_QUEUE_INIT_ERR )
    {
      Count = HAL_CONFIG_ERR;
    } /* end if() */

    return ( Count ); 
  } /* end Can_ObjBufferSpace() */


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
  int16_t Can_GlobalInit  ( uint8_t aui8_busNr, uint16_t ab_baudrate, 
              uint16_t aui16_maskStd, uint32_t aui32_maskExt, uint32_t aui32_maskLastmsg )
  { 
    int16_t Code = __HAL::DjBios_CanOpen ( aui8_busNr, ab_baudrate, aui16_maskStd,
                                     aui32_maskExt, aui32_maskLastmsg );

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_RATE )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_INIT_CHANGE )
    {
      Code = HAL_WARN_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  } /* end Can_GlobalInit() */


  /**
    set the global masks
    (11bit, 29bit and last msg mask)
    @param aui8_busNr number of the BUS to config (default 0)
    @param aui16_maskStd 11bit global mask
    @param aui32_maskExt 29bit global mask
    @param aui32_maskLastmsg mask of last CAN msg
    @return HAL_NO_ERR == no error
            HAL_RANGE_ERR == wrong BUS number
  */
  int16_t Can_GlobalMask ( uint8_t aui8_busNr, uint16_t aui16_maskStd, 
                           uint32_t aui32_maskExt, uint32_t aui32_maskLastmsg )
  {
    int16_t Code = __HAL::DjBios_CanConfig ( aui8_busNr, aui16_maskStd,
                                          aui32_maskExt, aui32_maskLastmsg );

    if ( Code == BIOS_CAN_BAD_BUS )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  };


  /**
    close CAN BUS
    (important to close BUS before re-init with other baudrate)
    @param aui8_busNr number of the BUS to close (default 0)
    @return HAL_NO_ERR == no error
            HAL_RANGE_ERR == wrong BUS number
            HAL_CONFIG_ERR == BUS previously not initialised
  */
  int16_t Can_Close ( uint8_t aui8_busNr )
  {
    int16_t Code = __HAL::DjBios_CanClose ( aui8_busNr );

    if ( Code == BIOS_CAN_BAD_BUS )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  } /* end Can_Close() */


  /**
    config a MsgObj
    (buffer size is defined locally in implementation of this interface;
    all received msgs should get a time stamp;
    class Ident_c has ident and type 11/29bit)
    @param aui8_busNr number of the BUS to config
    @param aui8_msgobjNr number of the MsgObj to config
    @param arc_ident filter ident value of this MsgObj
    @param ab_rxtx 0==RX receive; 1==TX transmit
    @return HAL_NO_ERR == no error;
            HAL_BUSY_ERR == this MsgObj is already used
            HAL_CONFIG_ERR == BUS not initialised or error during buffer allocation
            HAL_RANGE_ERR == wrong BUS or MsgObj number
  */
  int16_t Can_ObjectInit ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr, 
                            __IsoAgLib::Ident_c& arc_ident, uint8_t ab_rxtx )
  {

//  if (arc_ident.identType() == __IsoAgLib::Ident_c::BothIdent)
//    pt_config->bXtd = DEFAULT_IDENT_TYPE;
//  else 
//    pt_config->bXtd = arc_ident.identType();


    uint32_t Ident = arc_ident.ident();
    uint8_t  Type  = arc_ident.identType();

    int16_t Code = __HAL::DjBios_CanObjConfig ( aui8_busNr, aui8_msgobjNr, Ident, 
                                         (Type==0)?BIOS_FALSE:BIOS_TRUE, (ab_rxtx==1)?BIOS_TRUE:BIOS_FALSE,
                                         (ab_rxtx==1)?CONFIG_CAN_SEND_BUFFER_SIZE:CONFIG_CAN_HIGH_LOAD_REC_BUF_SIZE_MIN );

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_OBJ )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_QUEUE_INIT_ERR || Code == BIOS_CAN_NO_INIT )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_OBJ_BUSY )
    {
      Code = HAL_BUSY_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  };


  /**
    change the Ident_c of an already initialised MsgObj
    (class __IsoAgLib::Ident_c has ident and type 11/29bit)
    @param aui8_busNr number of the BUS to config
    @param aui8_msgobjNr number of the MsgObj to config
    @param arc_ident filter ident value of this MsgObj
    @return HAL_NO_ERR == no error;
            HAL_CONFIG_ERR == BUS not initialised or ident can't be changed
            HAL_RANGE_ERR == wrong BUS or MsgObj number
  */
  int16_t Can_ObjectChange ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr, 
                                              __IsoAgLib::Ident_c& arc_ident )
  {
    uint32_t Ident = arc_ident.ident();
    uint8_t  Type  = arc_ident.identType();
    int16_t Code = __HAL::DjBios_CanObjChangeIdent ( aui8_busNr, aui8_msgobjNr, Ident, 
                                               (Type==0)?BIOS_FALSE:BIOS_TRUE);

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_OBJ )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );

  };


  /**
    close a MsgObj
    @param aui8_busNr number of the BUS to config
    @param aui8_msgobjNr number of the MsgObj to config
    @return HAL_NO_ERR == no error;
            HAL_CONFIG_ERR == BUS not initialised, MsgObj previously not used or buffer memory not freed
            HAL_RANGE_ERR == wrong BUS or MsgObj number
  */
  int16_t Can_ObjClose ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr )
  {
    int16_t Code = __HAL::DjBios_CanObjClose ( aui8_busNr, aui8_msgobjNr );

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_OBJ )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT || Code == BIOS_CAN_QUEUE_INIT_ERR )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  } /* end Can_ObjClose() */


  /**
    send a message via a MsgObj;
    CanPkg_c (or derived object) must provide (virtual)
    functions:
    * Ident_c& getIdent() -> deliver ident of msg to send
    * uint8_t getData(uint8_t& rb_dlc, uint8_t* pb_data)
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
  int16_t Can_ObjSend ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr, 
                                             __IsoAgLib::CanPkg_c* apc_data )
  {
    static struct_CanMsg Msg;

    apc_data->getData(Msg.ID, Msg.Ext, Msg.Length, Msg.Data);

    int16_t Code = __HAL::DjBios_CanObjSend ( aui8_busNr, aui8_msgobjNr, &Msg );

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_OBJ )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_Q_FULL )
    {
      Code = HAL_OVERFLOW_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_BUS_OFF )
    {
      Code = HAL_NOACT_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT || Code == BIOS_CAN_OBJ_MISMATCH )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  };


  /**
    get the ident of a received message to decide about the further
    processing before the whole data string is retreived
    @param aui8_busNr number of the BUS to config
    @param aui8_msgobjNr number of the MsgObj to config
    @param reflIdent reference to the var, where the ident should be inserted
    @return HAL_NO_ERR       == No problem
            HAL_CONFIG_ERR   == BUS not initialised, MsgObj is no RX object
            HAL_NOACT_ERR    == BUS OFF
            HAL_OVERFLOW_ERR == Recieve buffer overflowed
            HAL_RANGE_ERR    == wrong BUS or MsgObj number
            HAL_WARN_ERR     == BUS WARN or no received message
  */
  int32_t Can_ReadObjRxIdent ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr, 
                                                          int32_t &reflIdent )
  {
    static struct_CanMsg Msg;

    /* Get the message and leave it in the buffer */
    int16_t Code = __HAL::DjBios_CanObjRead ( aui8_busNr, aui8_msgobjNr, &Msg );

    if ( Code == BIOS_CAN_NO_ERR || 
         Code == BIOS_CAN_Q_OVERFLOW || 
         Code == BIOS_CAN_BUS_WARN )
    {
      /* return just the ID field */
      reflIdent = Msg.ID;
    } /* end if() */

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_OBJ )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_Q_OVERFLOW )
    {
      Code = HAL_OVERFLOW_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_BUS_WARN || BIOS_CAN_NO_MSG )
    {
      Code = HAL_WARN_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_BUS_OFF )
    {
      Code = HAL_NOACT_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT || Code == BIOS_CAN_OBJ_MISMATCH )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  };


  /**
    transfer front element in buffer into the pointed CanPkg_c;
    DON'T clear this item from buffer.
    @see can_useMsgobjPopFront for explicit clear of this front item
    functions:
    * setIdent(Ident_c& arc_ident)
      -> set ident arc_ident of received msg in CANPkg
    * uint8_t setData(uint8_t ab_dlc, uint8_t* apb_data)
      -> set DLC in CanPkg_c from ab_dlc and insert data from uint8_t string apb_data
    @param aui8_busNr number of the BUS to config
    @param aui8_msgobjNr number of the MsgObj to config
    @param apc_data pointer to CanPkg_c instance with data to send
    @return HAL_NO_ERR       == no error;
            HAL_CONFIG_ERR   == BUS not initialised, MsgObj is no RX object
            HAL_NOACT_ERR    == BUS OFF
            HAL_OVERFLOW_ERR == send buffer overflowed
            HAL_RANGE_ERR    == wrong BUS or MsgObj number
            HAL_WARN_ERR     == BUS WARN or no received message
  */
  int16_t Can_ReadObjRx ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr, __IsoAgLib::CanPkg_c* apc_data )
  {
    static struct_CanMsg Msg;

    /* Get the message and leave it in the buffer */
    int16_t Code = __HAL::DjBios_CanObjRead ( aui8_busNr, aui8_msgobjNr, &Msg );

    /* return Message values */

    if ( Code == BIOS_CAN_NO_ERR || 
         Code == BIOS_CAN_Q_OVERFLOW || 
         Code == BIOS_CAN_BUS_WARN )
    {
      /* load the message */
      // apc_data->setTime(Msg.Time);
      // CanPkg_c::setTime changed to static
      __IsoAgLib::CanPkg_c::setTime(Msg.Time);

      __IsoAgLib::Ident_c::identType_t idType;
      idType = (Msg.Ext == 1) ? __IsoAgLib::Ident_c::ExtendedIdent : __IsoAgLib::Ident_c::StandardIdent;

      // apc_data->setIdent(Msg.ID, idType);
      // CanPkg_c::setIdent changed to static member function
      __IsoAgLib::CanPkg_c::setIdent(Msg.ID, idType);

// IsoAgLib Update 28Sept      apc_data->setDataString(Msg.Data, Msg.Length);
      apc_data->setDataFromString(Msg.Data, Msg.Length);
    } /* end if() */

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_OBJ )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_Q_OVERFLOW )
    {
      Code = HAL_OVERFLOW_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_BUS_OFF )
    {
      Code = HAL_NOACT_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT || Code == BIOS_CAN_OBJ_MISMATCH )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  }

  /**
    clear the buffer of a MsgObj (e.g. to stop sending retries)
    @param aui8_busNr number of the BUS to config
    @param aui8_msgobjNr number of the MsgObj to config
    @return HAL_NO_ERR == no error;
            HAL_CONFIG_ERR == BUS not initialised
            HAL_RANGE_ERR == wrong BUS or MsgObj number
  */
  int16_t Can_PurgeObj ( uint8_t aui8_busNr, uint8_t aui8_msgobjNr )
  {
    /* Clear the recieve buffer */
    int16_t Code = __HAL::DjBios_CanObjPurge ( aui8_busNr, aui8_msgobjNr );

    if ( Code == BIOS_CAN_BAD_BUS || Code == BIOS_CAN_BAD_OBJ )
    {
      Code = HAL_RANGE_ERR;
    } /* end if() */
    else if ( Code == BIOS_CAN_NO_INIT )
    {
      Code = HAL_CONFIG_ERR;
    } /* end if() */
    else
    {
      Code = HAL_NO_ERR;
    } /* end else() */

    return ( Code );
  }

  int32_t can_getMaxSendDelay ( uint8_t aui8_busNr )
  {
    /* Clear the recieve buffer */
    int32_t Code = 0;
    return ( Code );
  }


} // end of namespace __HAL

