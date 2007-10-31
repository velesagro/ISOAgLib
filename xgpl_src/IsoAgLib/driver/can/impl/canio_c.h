/***************************************************************************
                          canio_c.h  -  master object for CAN communication;
                                       owns array of FilterBox_c and
                                       MsgObj_c instances
                             -------------------
    begin                : Fri Apr 07 2000
    copyright            : (C) 2000 - 2004 by Dipl.-Inform. Achim Spangler
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
 * Copyright (C) 2000 - 2004 Dipl.-Inform. Achim Spangler                  *
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

 /**************************************************************************
 *                                                                         *
 *     ###    !!!    ---    ===    IMPORTANT    ===    ---    !!!    ###   *
 * Each software module, which accesses directly elements of this file,    *
 * is considered to be an extension of IsoAgLib and is thus covered by the *
 * GPL license. Applications must use only the interface definition out-   *
 * side :impl: subdirectories. Never access direct elements of __IsoAgLib  *
 * and __HAL namespaces from applications which shouldnt be affected by    *
 * the license. Only access their interface counterparts in the IsoAgLib   *
 * and HAL namespaces. Contact a.spangler@osb-ag:de in case your applicat- *
 * ion really needs access to a part of an internal namespace, so that the *
 * interface might be extended if your request is accepted.                *
 *                                                                         *
 * Definition of direct access:                                            *
 * - Instantiation of a variable with a datatype from internal namespace   *
 * - Call of a (member-) function                                          *
 * Allowed is:                                                             *
 * - Instatiation of a variable with a datatype from interface namespace,  *
 *   even if this is derived from a base class inside an internal namespace*
 * - Call of member functions which are defined in the interface class     *
 *   definition ( header )                                                 *
 *                                                                         *
 * Pairing of internal and interface classes:                              *
 * - Internal implementation in an :impl: subdirectory                     *
 * - Interface in the parent directory of the corresponding internal class *
 * - Interface class name IsoAgLib::iFoo_c maps to the internal class      *
 *   __IsoAgLib::Foo_c                                                     *
 *                                                                         *
 * AS A RULE: Use only classes with names beginning with small letter :i:  *
 ***************************************************************************/
#ifndef CAN_IO_H
#define CAN_IO_H

/* *************************************** */
/* ********** include headers ************ */
/* *************************************** */
#include <IsoAgLib/util/iliberr_c.h>
#include <IsoAgLib/util/impl/singleton.h>
#include <IsoAgLib/hal/can.h>
#include <IsoAgLib/hal/system.h>



// include base standard headers and define some constants
// (f.e. for hardware dependent settings)

// include declarations of objects for dynamic array
#include "ident_c.h"
#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  #include "msgobj_c.h"
#endif
#include "filterbox_c.h"

#ifdef DO_USE_SLIST
  #if defined(SYSTEM_PC) && !defined(SYSTEM_PC_VC) && !defined(SYSTEM_A1) && __GNUC__ >= 3
    #include <ext/slist>
    namespace std { using __gnu_cxx::slist;}
  #else
    #include <slist>
  #endif
#else
  #include <list>
#endif



// Begin Namespace __IsoAgLib
namespace __IsoAgLib {

/* *************************************** */
/* ********** special defines ************ */
/* *************************************** */
#define MIN_MSG_LIMIT 0
#define MAX_MSG_LIMIT 1

/* *************************************** */
/* ********* class definition ************ */
/* *************************************** */

class CanIo_c;
typedef CAN_SINGLETON(CanIo_c) SingletonCanIo_c;
/**
  Encapsulation of CAN IO with MsgObj_c instances in dynamic array.
  Initialization of CAN hardware, with default setting of one send object.

  An __IsoAgLib::CanCustomer_c  can create a FilterBox_c object with its own Filter/Mask
  combination, and with automatic direct processMsg() call on received CAN
  telegram. Direct access via pointer to __IsoAgLib::CanCustomer_c  instances on FilterBox
  insertion. Uses dynamic array of MsgObj_c, FilterBox_c and for receive
  notification dyn. array of pointer to FilterBox_c.

  @short Encapsulation of CAN IO with managing received data by dynamic arrays
  of FilterBox_c and MsgObj
  @author Dipl.-Inform. Achim Spangler
  @see FilterBox_c MsgObj
*/
class CanIo_c : public SingletonCanIo_c {
 public: // changed from protected to work with the access from MsgObj_c
  /** define dynamic array of MsgObj_c instances for each hardware
    MsgObj_c one object instances in array
  */
  #ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
    #ifdef OPTIMIZE_HEAPSIZE_IN_FAVOR_OF_SPEED
    typedef STL_NAMESPACE::USABLE_SLIST<MsgObj_c,STL_NAMESPACE::__malloc_alloc_template<0> > ArrMsgObj;
    #else
    typedef STL_NAMESPACE::USABLE_SLIST<MsgObj_c> ArrMsgObj;
    #endif
    /** define dynamic array of FilterBox_c instances;
      if a __IsoAgLib::CanCustomer_c  creates one FilterBox_c definitions,
      one object instance is inserted in array
    */
  #endif //SYSTEM_WITH_ENHANCED_CAN_HAL



   typedef STL_NAMESPACE::vector<FilterBox_c> ArrFilterBox;


 public:



   /** states if this Application should be prioritized over other applications.
    * This is being set simply for this "can_server"-client.
    * NOTE: This feature only works for applications running utilizing the "can_server"
    */
   static bool sb_sendPrioritized;



  /** Initialize the CAN hardware, and instantiate one msg object for
      sending of messages. Do configuration for BUS number, sending bitrate,
      CAN ident length, minx/max hardware/BIOS Msg Obj numbers by parameters;
      called by specified constructor or external functions;
      wrong BUS and msg obj numbers are rejected and cause set of Err_c:range

      If even the aui8_busNumber parameter has the default value 0xFF,
      then the configuration settings of a previous init call are not
      changed. In this case, the CAN BUS is only reset with the old settings.
      This is enabled by the default value 0xFF for aui8_busNumber, which is
      changed to CONFIG_CAN_DEFAULT_BUS_NUMBER for the first call of init() after the constructor.
      In all other cases, the special value 0xFF is indicator for empty parameter list.

      possible errors:
          * Err_c::range on undefined BUS,  msgOb_nr or sendPufferSize,
          * Err_c::hwConfig on uninitialized BUS, undef. msgType or CAN-BIOS mem-err,
          * Err_c::busy on already used sending Msg-Obj

      @param aui8_busNumber       optional number of the CAN bus
      @param aui16_bitrate        optional bitrate (default by define in isoaglib_config.h)
      @param Ident_c::identType_t optional length of the ident (S (11bit), E (29bit), B
                                  (send both standard and extended ident msg)
                                  (default by define in isoaglib_config.h)
      @param aui8_minObjNr        optional minimum number for hardware CAN
                                  message object (important for sharing CAN controller with
                                  other tasks) (default by define in isoaglib_config.h)
      @param aui8_maxObjNr        optional maximum number for hardware CAN
                                  message object (default by define in isoaglib_config.h)
      @return                     true -> correct initialisation without errors
      @see    HAL::init_can
      @see    HAL::tCanObjConfig
      @see    HAL::configCanObj
      @see    Ident_c::t_identType
    */
  bool init(uint8_t aui8_busNumber, uint16_t aui16_bitrate, Ident_c::identType_t ren_identType, uint8_t aui8_minObjNr, uint8_t aui8_maxObjNr);

  /** check if this CanIo_c instance is configured so that it can be used to send */
  bool isReady2Send() const { return ( ui8_busNumber != 0xFF )?true:false;}


/** is the fist add after a reconfiguration */
 bool isFirstAddFilterBox() const
{
    if(i32_minChangedFilterBox == -1) return true; //it is the fist addFilterBox after the reconfiguration or the initCAN

    return false;
}

/** fast or full reconfiguration */
  bool isFullReconfigNecessary()const {return ((i32_minChangedFilterBox < 0 )?  true:  false);}

/** set full reconfiguration */
 void setFullReconfigNecessary(){i32_minChangedFilterBox = -2 ;}

/** set value */
 void setMinChangedFilterBox(int32_t ai32_value){i32_minChangedFilterBox = ai32_value;}

/** get a filterbox index */
  int32_t getMinChangedFilterBox() const { return (i32_minChangedFilterBox >= 0 )? i32_minChangedFilterBox : 0;}

/** init value */
  void initMinChangedFilterBox(){i32_minChangedFilterBox = -1 ;}


  /** every subsystem of IsoAgLib has explicit function for controlled shutdown */
  void close( void );

  /** destructor of CanIo_c calls BIOS function for closing hardware CAN controller
    @see HAL::closeCan
  */
  ~CanIo_c(){close();}

  /** periodically called function which does
    periodically needed actions; f.e. trigger watch
    dog and start processing of received messages
    @return true -> time events was performed
  */
  bool timeEvent( void );

  /** provide BUS number */
  uint8_t getBusNumber( void ) const { return ui8_busNumber;}

  #ifdef USE_CAN_MEASURE_BUSLOAD
  /** deliver actual BUS load in baud
    @return baudrate in [kbaud] on used CAN BUS
  */
  uint16_t getBusLoad() const;
  #endif

  /** wait until specified timeout or until next CAN message receive
   *  @return true -> there are CAN messages waiting for process. else: return due to timeout
   */
  static bool waitUntilCanReceiveOrTimeout( uint16_t aui16_timeoutInterval )
  { return HAL::can_waitUntilCanReceiveOrTimeout( aui16_timeoutInterval );}

  /** set the minimum delay in msec. between two sent CAN messages
    @param aui16_minDelay minimum time between two CAN messages [msec.]
  */
  void setSendpause(uint16_t aui16_minDelay) const;

  /** deliver the numbers which can be placed at the moment in the send puffer
    @param ren_identType type of searched ident: standard 11bit or extended 29bit
      (default DEFAULT_IDENT_TYPE set in isoaglib_config.h)
    @return number of msgs which fit into send buffer
  */
  uint8_t sendCanFreecnt(Ident_c::identType_t ren_identType = DEFAULT_IDENT_TYPE);

  /** clear the send buffer
    @param ren_identType type of searched ident: standard 11bit or extended 29bit
      (default DEFAULT_IDENT_TYPE set in isoaglib_config.h)
  */
  void sendCanClearbuf(Ident_c::identType_t ren_identType = DEFAULT_IDENT_TYPE);

  /** test if a FilterBox_c definition already exist
    (version expecial for standard ident, chosen at compile time)
    @param ar_customer reference to the processing class ( the same filter setting can be registered by different consuming classes )
    @param aui32_mask individual mask for this filter box
    @param aui32_filter individual filter
    @param ren_identType type of searched ident: standard 11bit or extended 29bit
      (default DEFAULT_IDENT_TYPE set in isoaglib_config.h)
    @param apc_iter optional pointer Iterator to result FilterBox
    @return true -> same FilterBox_c already exist
  */
  bool existFilter(const __IsoAgLib::CanCustomer_c& ar_customer,
    uint16_t aui16_mask, uint16_t aui16_filter,
    Ident_c::identType_t ren_identType = DEFAULT_IDENT_TYPE,
    ArrFilterBox::iterator* apc_iter = NULL);

  /** test if a FilterBox_c definition already exist
    (version expecial for extended ident, chosen at compile time)
    @param ar_customer reference to the processing class ( the same filter setting can be registered by different consuming classes )
    @param aui32_mask individual mask for this filter box
    @param aui32_filter individual filter
    @param ren_identType type of searched ident: standard 11bit or extended 29bit
      (default DEFAULT_IDENT_TYPE set in isoaglib_config.h)
    @param apc_iter optional pointer Iterator to result FilterBox
    @return true -> same FilterBox_c already exist
  */
  bool existFilter(const __IsoAgLib::CanCustomer_c& ar_customer,
      uint32_t aui32_mask, uint32_t aui32_filter,
      Ident_c::identType_t ren_identType = DEFAULT_IDENT_TYPE,
      ArrFilterBox::iterator* apc_iter = NULL);

  /** test if a FilterBox_c definition already exist
    (version with comper items as Ident_c class instances, chosen by compiler)
    @param ar_customer reference to the processing class ( the same filter setting can be registered by different consuming classes )
    @param ac_compMask individual mask for this filter box
    @param ac_compFilter individual filter
    @param apc_iter optional pointer Iterator to result FilterBox
    @return true -> same FilterBox_c already exist
  */

  bool existFilter(const __IsoAgLib::CanCustomer_c& ar_customer,
      const Ident_c& ac_compMask, const Ident_c& ac_compFilter,
      ArrFilterBox::iterator* apc_iter = NULL);


  /** create a Filter Box with specified aui32_mask/aui32_filter
    on ui8_busNr of object; reconfig HW CAN MsgObj_c only if
    ab_reconfigImmediate == true -> useful for
    avoiding unneeded reconfiguration during
    sequence of FilterBox_c insertions;
    by ar_customer CanIo_c (FilterBox_c) can start direct processing
    of received data in dedicated customer object (no search);
    uses BIOS functions

    possible errors:
        * Err_c::badAlloc on not enough memory for new FilterBox
        instance or for new configured MsgObj_c's
    @see __IsoAgLib::CANCustomer
    @param ar_customer reference to __IsoAgLib::CanCustomer_c  which needs
         filtered messages (-> on received msg call
       ar_customer.processMsg())
    @param aui32_mask individual mask for this filter box
    @param aui32_filter individual filter
    @param ab_reconfigImmediate true -> all Filter objects are reconfigured
         to according CAN hardware MsgObj after creating this filter
    @param at_identType ident type of the created ident: standard 11bit or extended 29bit
      (default DEFAULT_IDENT_TYPE set in isoaglib_config.h)
    @return != NULL -> if inserting and wanted reconfiguration are performed without errors,
      a reference to the created FilterBox is returned
   @exception badAlloc
  */

  FilterBox_c* insertFilter(__IsoAgLib::CanCustomer_c& ar_customer, uint32_t aui32_mask,
                            uint32_t aui32_filter, bool ab_reconfigImmediate = true,
                            const Ident_c::identType_t at_identType = DEFAULT_IDENT_TYPE,
                            int8_t ai8_dlcForce = -1,
                            FilterBox_c* apc_connectedFilterBox = NULL);

   FilterBox_c* insertFilter(__IsoAgLib::CanCustomer_c& ar_customer, uint32_t aui32_mask,
                            uint32_t aui32_filter, bool ab_reconfigImmediate,
                            const Ident_c::identType_t at_identType,
                            uint32_t at_connectedMask, uint32_t at_connectedFilter, const Ident_c::identType_t at_connectedIdentType,
                            int8_t ai8_dlcForce = -1);

  /** reconfigure the MsgObj after insert/delete of FilterBox */
  bool reconfigureMsgObj();

  /** verify given BUS number and MsgObj number, if they are correct
    (mostly used by MsgObj_c to verify itself)
    @param aui8_busNr number of the BUS
    @param aui8_msgobjNr number of the MsgObj
    @return true -> values are correct
  */
  bool verifyBusMsgobjNr(uint8_t aui8_busNr, uint8_t aui8_msgobjNr);

/** helper function to search all FilterBoxes for matching
    instance which maps to received CAN messages
    ( needed if the coordinated FilterBoxes don't match,
      especially important to process messages from global
      CAN MsgObj which doesn't have explicit connected FilterBox
      instances )
    @param aui32_ident Ident of received CAN message
    @return is another filterbox found?
  */
  bool canMsg2FilterBox( uint32_t aui32_ident, Ident_c::identType_t at_type, ArrFilterBox::iterator& ar_arrFilterBoxIter, bool ab_start );

  /** delete a FilerBox definition
    @param ar_customer reference to the processing class ( the same filter setting can be registered by different consuming classes )
    @param aui32_mask individual mask for this filter box
    @param aui32_filter individual filter
    @param at_identType ident type of the deleted ident: standard 11bit or extended 29bit
        (defualt DEFAULT_IDENT_TYPE defined in isoaglib_config.h)
    @return true -> FilterBox_c found and deleted
  */
  bool deleteFilter(const __IsoAgLib::CanCustomer_c& ar_customer,
      MASK_TYPE aui32_mask, MASK_TYPE aui32_filter,
      const Ident_c::identType_t at_identType = DEFAULT_IDENT_TYPE);

  bool deleteAllFiltersForCustomer (const __IsoAgLib::CanCustomer_c& ar_customer);


/**
    initiate processing of all received msg
    check all active MsgObj_c for received CAN msg and
    initiate their processing
    @return <0 --> not enough time to process all messages.
           ==0 --> no messages were received.
           >0  --> all messages are processed, number of messages
  */
  int16_t processMsg();

  /** deliver count of CAN messages which were processed during last timeEvent() / processMsg() call
    * this helps Scheduler_c to decide about needed double retrigger of CanIo_c::processMsg()
    * at high CAN BUS load (important to avoid overflow of CAN buffers in HAL
    */
  uint8_t getProcessedMsgCnt( void ) const { return ui8_processedMsgCnt;}

  /** function for sending data out of CanPkg_c (uses BIOS function)
      if send puffer is full a local loop waits till puffer has enough space
      (every 100ms the watchdog is triggered, to avoid watchdog reset)

      possible errors:
          * Err_c::hwConfig on wrong configured CAN obj,
            not init BUS or no configured send obj
          * Err_c::range on undef BUS or BIOS send obj nr
          * Err_c::can_warn on physical CAN-BUS problems
          * Err_c::can_off on physical CAN-BUS off state
      @see CANPkg
      @see HAL::sendCanMsg
      @param arc_src CanPkg_c which holds the to be sent data
      @return reference to this CanIo_c instance ==>
            needed by commands like "c_can_io << pkg_1 << pkg_2 ... << pkg_n;"
    */
  CanIo_c& operator<<(CanPkg_c& arc_src);

  /** function for sending data out of CanPkgExt_c (uses BIOS function)
      if send puffer is full a local loop waits till puffer has enough space
      (every 100ms the watchdog is triggered, to avoid watchdog reset)

      possible errors:
          * Err_c::hwConfig on wrong configured CAN obj, not init BUS or no configured send obj
          * Err_c::range on undef BUS or BIOS send obj nr
          * Err_c::can_warn on physical CAN-BUS problems
          * Err_c::can_off on physical CAN-BUS off state
      @param arc_src CanPkgExt_c which holds the to be sent data
      @return reference to this CANIOExt_c instance ==> needed by commands like "c_can_io << pkg_1 << pkg_2 ... << pkg_n;"
    */
  CanIo_c& operator<<(CanPkgExt_c& rc_src);

  /** check for can send conflict error and stop send retry on error
      (thus avoid BUS OFF)
      @return true -> there was send error and send retry stopped
    */
  bool stopSendRetryOnErr();

  /** set the new maximum send delay
      @param ai32_maxSendDelay new maximum send delay in milli-seconds
   */
  void setMaxSendDelay (int32_t ai32_maxSendDelay);

  /** set this client to have send-priority
      @param ab_sendPrioritized enable (true) or disable (false) sending in Prioritized Mode
   */
  void setSendPriority(bool ab_sendPrioritized);

  FilterBox_c& getFilterBoxInstance(int32_t ai32_fbIdx) {return arrFilterBox[ai32_fbIdx];};


  #if defined(DEBUG_CAN_FILTERBOX_MSGOBJ_RELATION) && !defined(SYSTEM_WITH_ENHANCED_CAN_HAL)
  void printMsgObjInfo();
  #endif

  /** for precise time checks e.g. within isVtActive() the last timestamp of the last processed can package is accessible.
    @return time stamp of the last can package that has been received and processed successfully
  */
  int32_t getLastProcessedCanPkgTime() const { return i32_lastProcessedCanPkgTime; }

 protected: // Protected methods
#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  /** evaluate common bits of all defined filterBox
     instances and set it in mask -> build up global mask
   */
  void getCommonFilterMask();

  /** correct global masks after merge based on merged IDs of all
      resulting MsgObj_c instances
    */
  void getCommonFilterMaskAfterMerge();

  /** build unique MsgObj_c's based on given
      global mask (different FilterBox_c def's can result in one
      MsgObj_c if differences are hidden by global mask), return count of obj

      possible errors:
          * Err_c::badAlloc on not enough memory for created MsgObj_c instances
      @see MsgObj
      @see FilterBox
    */
  int16_t FilterBox2MsgObj();

  /** checks if count of preconfigured msgObj
      is allowed; shrink by merging the objects
      with smallest Bit distance
      -> all existing MsgObj_c instances can be
        configured in hardware after execution
      @see MsgObj
    */
  void CheckSetCntMsgObj();

#endif //SYSTEM_WITH_ENHANCED_CAN_HAL

 private: // private methods
  friend class CAN_SINGLETON( CanIo_c );
  /** private constructor which prevents direct instantiation in user application
    * NEVER define instance of CanIo_c within application
    * (set ui8_busNumber to 0xFF so that init() detects first call after constructor)
    */
  CanIo_c( void );

  /** initialize directly after the singleton instance is created.
      this is called from singleton.h and should NOT be called from the user again.
      users please use init(...) instead.
    */
  void singletonInit();

  /** deliver min msg obj nr
      @return min msg obj nr
    */
  uint8_t minHALMsgObjNr()const{return ui8_min_msgObjLimit;}

  /** get offset for received messages
      @return offset for received messages
    */
 uint8_t minReceiveObjNr() const {return ui8_minReceiveObjNr;}

  /** set min msg obj nr
      @param ab_limit wanted min msg obj nr
    */
  void setMinHALMsgObjNr(uint8_t ab_limit){ui8_min_msgObjLimit = ab_limit;}

  /** search for a FilterBox with given mask and filter
      @param  at_mask    filterBox mask
      @param  at_filter  filterBox filter
      @return            located FilterBox or NULL if not exist
    */
  FilterBox_c* getFilterBox(Ident_c& at_mask, Ident_c& at_filter) const;

  void setCntFilter(size_t at_newSize ) { tm_filterBoxCnt = at_newSize;}
  size_t cntFilter() const { return tm_filterBoxCnt;}

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  void setCntMsgObj(size_t at_newSize ) { tm_MsgObjCnt = at_newSize;}
  size_t cntMsgObj() const { return tm_MsgObjCnt;}
  /** deliver max msg obj nr
      @return max msg obj nr
    */
  uint8_t maxHALMsgObjNr()const{return ui8_maxMsgObjLimit;}

  /** set max msg obj nr
      @param ab_limit wanted max msg obj nr
    */
  void setMaxHALMsgObjNr(uint8_t ab_limit){ui8_maxMsgObjLimit = ab_limit;}

  /** call the needed HAL function for setting the new global masks,
      without invalidating already open send and last msg obj.
    */
  bool registerChangedGlobalMasks(void);
#endif

  /** get offset for received messages
      @return offset for received messages
    */
  uint8_t updateMinReceiveObjNr();


  /** switch CAN bitrate (possible during runtime
      with automatic reconfiguring of CAN MsgObj)
      @param aui16_newSpeed
    */
  bool setBitrate(uint16_t aui16_newSpeed, bool ab_force = false);

  /** perform base init for CAN with set of speed and init of send object(s)
      @param aui16_bitrate wanted CAN bitrate
    */
  bool baseCanInit(uint16_t aui16_bitrate);

#ifdef DEBUG
  void doDebug(uint8_t ui8_busNr, uint8_t ui8_sendObjNr);
#endif

  void setEndLastReconfigTime(){ mi32_endLastReconfigTime = HAL::getTime(); }

// Private attributes
#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  /** dynamic array of MsgObj_c which manages each one a
      hardware CAN object
      @see MsgObj
    */
  ArrMsgObj arrMsgObj;

  /** temp MsgObj_c for inserting of new msgObj instances
      @see MsgObj
    */
  MsgObj_c c_tempObj;

  /** MsgObj_c for last hardware CAN message object where all CAN messages are
      stored which don't match t one of the active CAN MsgObj_c ( typical number 15 )
      @see MsgObj
    */
  MsgObj_c c_lastMsgObj;
  size_t tm_MsgObjCnt;
#endif

/** for the fast reconfiguration */
    int32_t i32_minChangedFilterBox;

  /** dynamic array of FilterBox_c instances which
      represents the demanded filter boxes
      @see FilterBox
    */
  ArrFilterBox arrFilterBox;
  size_t tm_filterBoxCnt;

/** return a reference of a FilterBox instance */


 #if defined( DEBUG_CAN_FILTERBOX_MSGOBJ_RELATION )
 void printMyFilterBox();
 #endif
  /** temp filer box to avoid new/delete for each insert of a filterBox
      @see FilterBox
    */
  FilterBox_c c_tempFilterBox;

  /** maximum send delay - value of < 0 indicates that no send-delay check is requested*/
  int32_t i32_maxSendDelay;

  /** begin of an CAN error period */
  int32_t i32_canErrStart;

  /** time of last detected CAN error */
  int32_t i32_canErrEnd;

  /**  timestamp of last CAN BUS integrity check  */
  int32_t i32_lastCanCheck;

  /**  timestamp of last received and and processed CAN package  */
  int32_t i32_lastProcessedCanPkgTime;

  /** CAN ui16_bitrate (in kBit/s) */
  uint16_t ui16_bitrate;


  int32_t mi32_endLastReconfigTime;

  /** global mask with standard 11bit type */
  Ident_c c_maskStd;

  /** global mask with extended 29bit type */
  Ident_c c_maskExt;

  /** global last msg mask */
  Ident_c c_maskLastmsg;



  /** identifier type  CanIo_c::S, CanIo_c::extendendIdent
      or CanIo_c::B (CanIo_c should send both standard and extended ident msg
      --> two send obj are created)
    */
  Ident_c::identType_t en_identType;

  /** BUS Number for systems with more than one BUS
      (for each BUS one  CanIo_c instance)
    */
  uint8_t ui8_busNumber;

  /** min limit of allowed msgObj numbers for this CanIo_c instance
      (important for multithreading environments where different
      Processes must share one BUS)
    */
  uint8_t ui8_min_msgObjLimit;

  /** minimal obj nr that can be used for a receive msg obj which is dependent of identType_t */
  uint8_t ui8_minReceiveObjNr;

#ifndef SYSTEM_WITH_ENHANCED_CAN_HAL
  /** max limit of allowed msgObj numbers for this CanIo_c instance
      (important for multithreading environments where different
      Processes must share one BUS)
    */
  uint8_t ui8_maxMsgObjLimit;
#endif

  /** count of CAN messages which were processed during last timeEvent() / processMsg() call
    * this helps Scheduler_c to decide about needed double retrigger of CanIo_c::processMsg()
    * at high CAN BUS load (important to avoid overflow of CAN buffers in HAL
    */
  uint8_t ui8_processedMsgCnt;

  /** flag to avoid loop of CAN message processing, when timeEvent() is called during previous
   *  timeEvent triggered CAN processing -> when this flag is TRUE, no further processing is performed
   */
  bool b_runningCanProcess;

#if ((defined( USE_ISO_11783)) && ((CAN_INSTANCE_CNT > PRT_INSTANCE_CNT) || defined (ALLOW_PROPRIETARY_MESSAGES_ON_STANDARD_PROTOCOL_CHANNEL)))
  /** we have either compiled for ISO, OR there is at least one internal / proprietary CAN channel */
  bool b_canChannelCouldSendIso;
  #endif
};

#if defined( CAN_INSTANCE_CNT ) && ( CAN_INSTANCE_CNT > 1 )
  /** C-style function, to get access to the unique CanIo_c singleton instance
    * if more than one CAN BUS is used for IsoAgLib, an index must be given to select the wanted BUS
    */
  CanIo_c& getCanInstance( uint8_t aui8_instance = 0 );
#else
  /** C-style function, to get access to the unique CanIo_c singleton instance */
  CanIo_c& getCanInstance( void );
#endif

/** this typedef is only for some time to provide backward compatibility at API level */
typedef CanIo_c CANIO_c;
};

#endif
