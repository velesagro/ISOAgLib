/*
  vtclientservercommunication_c.h: class for managing the
    communication between vt client and server

  (C) Copyright 2009 - 2010 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/

//#define GET_VERSIONS
//#define USE_SIMPLE_AUX_RESPONSE


#include "vtclientservercommunication_c.h"
#include "../ivtclientservercommunication_c.h"
#include <IsoAgLib/driver/can/impl/canio_c.h>
#include <IsoAgLib/comm/Part3_DataLink/impl/multireceive_c.h>
#include <supplementary_driver/driver/datastreams/volatilememory_c.h>
#include <IsoAgLib/comm/Part5_NetworkManagement/impl/isofiltermanager_c.h>
#include "../ivtobjectpicturegraphic_c.h"
//#if not defined PRJ_ISO_TERMINAL_OBJECT_SELECTION1 || defined USE_VTOBJECT_graphicscontext
  #include "../ivtobjectgraphicscontext_c.h"
//#endif
#include "../ivtobjectlineattributes_c.h"
#include "../ivtobjectfillattributes_c.h"
#include "../ivtobjectfontattributes_c.h"
#include "../ivtobjectstring_c.h"
#include "../ivtobjectworkingset_c.h"

#include <IsoAgLib/util/iassert.h>

#if DEBUG_VTCOMM || DEBUG_HEAP_USEAGE
  #include <supplementary_driver/driver/rs232/impl/rs232io_c.h>
  #include <IsoAgLib/util/impl/util_funcs.h>
  #ifdef SYSTEM_PC
    #include <iostream>
  #else
    #include <supplementary_driver/driver/rs232/impl/rs232io_c.h>
  #endif
#endif


#if DEBUG_HEAP_USEAGE
  static uint16_t sui16_lastPrintedSendUploadQueueSize = 0;
  static uint16_t sui16_lastPrintedMaxSendUploadQueueSize = 0;
  static uint16_t sui16_sendUploadQueueSize = 0;
  static uint16_t sui16_maxSendUploadQueueSize = 0;
#endif

static const uint8_t scui8_cmdCompareTableMin = 0x92;
static const uint8_t scui8_cmdCompareTableMax = 0xBD;

/// this table is used to identify if a command can override an earlier command of same function
/// 1<<databyte to indicate which databytes to compaire to decide if command is replaced or not
static const uint8_t scpui8_cmdCompareTable[(scui8_cmdCompareTableMax-scui8_cmdCompareTableMin)+1] = {
/// (1<<0) means DO NOT OVERRIDE THESE COMMANDS AT ALL
/* 0x92 */ (1<<0) , //NEVER OVERRIDE THIS COMMAND
/* 0x93 */ 0 , //invalid command
/* 0x94 */ 0 , //invalid command
/* 0x95 */ 0 , //invalid command
/* 0x96 */ 0 , //invalid command
/* 0x97 */ 0 , //invalid command
/* 0x98 */ 0 , //invalid command
/* 0x99 */ 0 , //invalid command
/* 0x9A */ 0 , //invalid command
/* 0x9B */ 0 , //invalid command
/* 0x9C */ 0 , //invalid command
/* 0x9D */ 0 , //invalid command
/* 0x9E */ 0 , //invalid command
/* 0x9F */ 0 , //invalid command
/* 0xA0 */ (1<<1) | (1<<2) ,
/* 0xA1 */ (1<<1) | (1<<2) ,
/* 0xA2 */ (1<<1) | (1<<2) ,
/* 0xA3 */ (1<<0) , //NEVER OVERRIDE THIS COMMAND (Control Audio Device)
/* 0xA4 */ (1<<0) , //NEVER OVERRIDE THIS COMMAND (Set Audio Volume)
/* 0xA5 */ (1<<0) , //NEVER OVERRIDE THIS COMMAND (Change Child Location), as it's relative!!!
/* 0xA6 */ (1<<1) | (1<<2) ,
/* 0xA7 */ (1<<1) | (1<<2) ,
/* 0xA8 */ (1<<1) | (1<<2) , // Change Numeric Value (all has been done for THIS ONE originally ;-)
/* 0xA9 */ (1<<1) | (1<<2) ,
/* 0xAA */ (1<<1) | (1<<2) ,
/* 0xAB */ (1<<1) | (1<<2) ,
/* 0xAC */ (1<<1) | (1<<2) ,
/* 0xAD */ (1<<0) , // changed. was "(1<<1) | (1<<2) ," before, but we shouldn't change earlier to a datamask as it's probably not setup correctly at this time.  // (Change Active Mask)
/* 0xAE */ (1<<1) | (1<<2) | (1<<3) ,
/* 0xAF */ (1<<1) | (1<<2) | (1<<3) ,
/* 0xB0 */ (1<<1) | (1<<2) ,
/* 0xB1 */ (1<<1) | (1<<2) | (1<<3) ,
/* 0xB2 */ (1<<0) , //NEVER OVERRIDE THIS COMMAND (Delete Object Pool)
/* 0xB3 */ (1<<1) | (1<<2) ,
/* 0xB4 */ (1<<1) | (1<<2) | (1<<3) | (1<<4), // (Change Child Position)
/* 0xB5 */ 0 , //invalid command
/* 0xB6 */ 0 , //invalid command
/* 0xB7 */ 0 , //invalid command
/* 0xB8 */ (1<<0) ,  //NEVER OVERRIDE THIS COMMAND (Graphics Context)
/* 0xB9 */ (1<<0) , // changed. was "(1<<1) | (1<<2) | (1<<3) | (1<<4)," before, but I guess no overriding should take place in request commands // (Get Attribute Value)
/* 0xBA */ 0, //invalid command
/* 0xBB */ 0, //invalid command
/* 0xBC */ 0, //invalid command
/* 0xBD */ (1<<0) //NEVER OVERRIDE THIS COMMAND (Lock/Unlock Mask)
};


namespace __IsoAgLib {


void SendUpload_c::set (vtObjectString_c* apc_objectString)
{
  ppc_vtObjects = NULL;

  mssObjectString = apc_objectString;
}

void SendUpload_c::set (uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7, uint8_t byte8, uint8_t byte9)
{
  SendUploadBase_c::set(byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9);
  mssObjectString = NULL;
  ppc_vtObjects = NULL; /// Use BUFFER - NOT MultiSendStreamer!
  ui16_numObjects = 0;
}

void SendUpload_c::set (uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7, uint8_t byte8, IsoAgLib::iVtObject_c** rppc_vtObjects, uint16_t aui16_numObjects)
{
  SendUploadBase_c::set( byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8 );
  mssObjectString = NULL;  /// Use BUFFER - NOT MultiSendStreamer!
  ppc_vtObjects = rppc_vtObjects;
  ui16_numObjects = aui16_numObjects;
}

void SendUpload_c::set (uint16_t aui16_objId, const char* apc_string, uint16_t overrideSendLength)
{
  // if string is shorter than length, it's okay to send - if it's longer, we'll clip - as client will REJECT THE STRING (FINAL ISO 11783 SAYS: "String Too Long")
  uint16_t strLen = (CNAMESPACE::strlen(apc_string) < overrideSendLength) ? CNAMESPACE::strlen(apc_string) : overrideSendLength;

  /// Use BUFFER - NOT MultiSendStreamer!
  vec_uploadBuffer.clear();
  vec_uploadBuffer.reserve (((5+strLen) < 8) ? 8 : (5+strLen)); // DO NOT USED an UploadBuffer < 8 as ECU->VT ALWAYS has 8 BYTES!

  vec_uploadBuffer.push_back (179);
  vec_uploadBuffer.push_back (aui16_objId & 0xFF);
  vec_uploadBuffer.push_back (aui16_objId >> 8);
  vec_uploadBuffer.push_back (strLen & 0xFF);
  vec_uploadBuffer.push_back (strLen >> 8);
  int i=0;
  for (; i < strLen; i++) {
    vec_uploadBuffer.push_back (*apc_string);
    apc_string++;
  }
  for (; i < 3; i++) {
    // at least 3 bytes from the string have to be written, if not, fill with 0xFF, so the pkg-len is 8!
    vec_uploadBuffer.push_back (0xFF);
  }

#if DEBUG_HEAP_USEAGE
  if ( vec_uploadBuffer.capacity() != sui16_lastPrintedBufferCapacity )
  {
    sui16_lastPrintedBufferCapacity = vec_uploadBuffer.capacity();
    INTERNAL_DEBUG_DEVICE << "IsoTerminal_c Buffer-Capa: " << sui16_lastPrintedBufferCapacity << INTERNAL_DEBUG_DEVICE_ENDL;
  }
#endif

  mssObjectString = NULL;  /// Use BUFFER - NOT MultiSendStreamer!
  ppc_vtObjects = NULL;
}

void SendUpload_c::set (uint8_t* apui8_buffer, uint32_t bufferSize)
{
  SendUploadBase_c::set (apui8_buffer, bufferSize);
  mssObjectString = NULL;  /// Use BUFFER - NOT MultiSendStreamer!
  ppc_vtObjects = NULL;
}


/** place next data to send direct into send buffer of pointed
    stream send package - MultiSendStreamer_c will send this
    buffer afterwards
    - implementation of the abstract IsoAgLib::MultiSendStreamer_c function
 */
void
iVtObjectStreamer_c::setDataNextStreamPart (MultiSendPkg_c* mspData, uint8_t bytes)
{
  while ((m_uploadBufferFilled-m_uploadBufferPosition) < bytes)
  {
    // copy down the rest of the buffer (we have no ring buffer here!)
    int j = m_uploadBufferFilled - m_uploadBufferPosition;
    for (int i=0; i<j; i++)
      marr_uploadBuffer [i] = marr_uploadBuffer [i+m_uploadBufferPosition];
    // adjust pointers
    m_uploadBufferPosition = 0;
    m_uploadBufferFilled = j;

    // stream some more bytes into internal ISO_VT_UPLOAD_BUFFER_SIZE byte buffer...
    uint16_t bytes2Buffer;
    while ((bytes2Buffer = ((vtObject_c*)(*mpc_iterObjects))->stream (marr_uploadBuffer+m_uploadBufferFilled, ISO_VT_UPLOAD_BUFFER_SIZE-m_uploadBufferFilled, mui32_objectStreamPosition)) == 0)
    {
      mpc_iterObjects++;
      mui32_objectStreamPosition = 0;
    }
    m_uploadBufferFilled += bytes2Buffer;
    mui32_objectStreamPosition += bytes2Buffer;
  }
  mspData->setDataPart (marr_uploadBuffer, m_uploadBufferPosition, bytes);
  m_uploadBufferPosition += bytes;
}


/** set cache for data source to stream start
    - implementation of the abstract IsoAgLib::MultiSendStreamer_c function */
void
iVtObjectStreamer_c::resetDataNextStreamPart()
{
  mpc_iterObjects = mpc_objectsToUpload;
  mui32_objectStreamPosition = 0;
  m_uploadBufferPosition = 0;
  m_uploadBufferFilled = 1;
  marr_uploadBuffer [0] = 0x11; // Upload Object Pool!
}


/** save current send position in data source - neeed for resend on send problem
    - implementation of the abstract IsoAgLib::MultiSendStreamer_c function */
void
iVtObjectStreamer_c::saveDataNextStreamPart()
{
  mpc_iterObjectsStored = mpc_iterObjects;
  mui32_objectStreamPositionStored = mui32_objectStreamPosition;
  m_uploadBufferPositionStored = m_uploadBufferPosition;
  m_uploadBufferFilledStored = m_uploadBufferFilled;
  for (int i=0; i<ISO_VT_UPLOAD_BUFFER_SIZE; i++)
    marr_uploadBufferStored [i] = marr_uploadBuffer [i];
}


/** reactivate previously stored data source position - used for resend on problem
    - implementation of the abstract IsoAgLib::MultiSendStreamer_c function */
void
iVtObjectStreamer_c::restoreDataNextStreamPart()
{
  mpc_iterObjects = mpc_iterObjectsStored;
  mui32_objectStreamPosition = mui32_objectStreamPositionStored;
  m_uploadBufferPosition = m_uploadBufferPositionStored;
  m_uploadBufferFilled = m_uploadBufferFilledStored;
  for (int i=0; i<ISO_VT_UPLOAD_BUFFER_SIZE; i++)
    marr_uploadBuffer [i] = marr_uploadBufferStored [i];
}

/** *****************************************************/
/** ** Vt Client Server Communication Implementation ****/
/** *****************************************************/

/** static instance to store temporarily before push_back into list */
SendUpload_c VtClientServerCommunication_c::msc_tempSendUpload;


void
VtClientServerCommunication_c::reactOnAbort (Stream_c& /*arc_stream*/)
{
  mrc_pool.eventStringValueAbort();
}


// handle all string values between length of 9 and 259 bytes
bool
VtClientServerCommunication_c::reactOnStreamStart (const ReceiveStreamIdentifier_c& ac_ident, uint32_t aui32_totalLen)
{
  // if SA is not the address from the vt -> don't react on stream
  if ((ac_ident.getSaIsoName()) != (mpc_vtServerInstance->getIsoName())) return false;
  //handling string value >= 9 Bytes
  if (aui32_totalLen > (4 /* H.18 byte 1-4 */ + 255 /* max string length */))
    /** @todo SOON-258 Should we really ConnAbort such a stream in advance? For now don't care too much, as it shouldn't happen! */
    return false;
  return true;
}


bool
VtClientServerCommunication_c::processPartStreamDataChunk (Stream_c& arc_stream, bool ab_isFirstChunk, bool ab_isLastChunk)
{
  if (arc_stream.getStreamInvalid()) return false;

  uint8_t ui8_streamFirstByte = arc_stream.getFirstByte();
  switch ( ui8_streamFirstByte )
  {
    case 0x8:
      if (ab_isFirstChunk)  // check for command input string value H.18
      {
        mui16_inputStringId = arc_stream.getNextNotParsed() | (arc_stream.getNextNotParsed() << 8);
        mui8_inputStringLength = arc_stream.getNextNotParsed();

        const uint16_t ui16_totalstreamsize = arc_stream.getByteTotalSize();
        if (ui16_totalstreamsize < (mui8_inputStringLength + 4))
        { /** @todo SOON-258 "if (ui16_totalstreamsize > (mui8_inputStringLength + 4)) registerErronousVtMessage("VT Input String Activation CAN-Message too long.");
	      This is/was a problem of the John Deere GS2 VT and needs to be registered for any VT.
	      It will be fixed in the GS2 in 2008, but for now we have relaxed the checking and put this comment in here.
	   */
          arc_stream.setStreamInvalid();
          return false;
        }
      }
      mrc_pool.eventStringValue (mui16_inputStringId, mui8_inputStringLength, arc_stream, arc_stream.getNotParsedSize(), ab_isFirstChunk, ab_isLastChunk);
      break;
    default:
      if ( ui8_streamFirstByte >= 0x60 && ui8_streamFirstByte <= 0x7F && ab_isLastChunk)
      { // a proprietary stream message has been completely received -> process it
        mrc_pool.eventProprietaryCommand( mpc_vtServerInstance->getIsoName().toConstIisoName_c(), ui8_streamFirstByte, arc_stream );
      }
      break;
  }

  return false;
}


/** default constructor, which can optional set the pointer to the containing
  Scheduler_c object instance
 */
VtClientServerCommunication_c::VtClientServerCommunication_c(
  IdentItem_c& r_wsMasterIdentItem,
  IsoTerminal_c &r_isoTerminal,
  IsoAgLib::iIsoTerminalObjectPool_c& arc_pool,
  char* apc_versionLabel,
  uint8_t aui8_clientId,
  bool ab_isSlave SINGLETON_VEC_KEY_PARAMETER_DEF_WITH_COMMA)
  : mrc_pool (arc_pool)
  , mb_vtAliveCurrent (false) // so we detect the rising edge when the VT gets connected!
  , mb_checkSameCommand (true)
  , mrc_wsMasterIdentItem (r_wsMasterIdentItem)
  , mrc_isoTerminal (r_isoTerminal)
  , mpc_vtServerInstance (NULL)
  , mb_usingVersionLabel (false)
  //marrp7c_versionLabel [7] will be initialized below
  , men_objectPoolState (OPInitial) // dummy init value, will be set when VT (re)enters to OPInitial anyway!
  , mi8_vtLanguage (-1)
  , men_uploadType (UploadIdle) // dummy init value
  , men_uploadCommandState (UploadCommandWithAwaitingResponse) // dummy init value.
  , men_uploadPoolState (UploadPoolInit) // dummy init value, will be set when VT (re)enters to UploadInit anyway
  , men_uploadPoolType (UploadPoolTypeCompleteInitially) // dummy init value, will be set when (up)load is started
  //ms_uploadPhasesAutomatic[..] // will be corrently initialized in the body!
  , men_uploadPhaseAutomatic (UploadPhaseIVtObjectsFix)
  , ms_uploadPhaseUser() // will be corrently initialized in the body!
  , mppc_uploadPhaseUserObjects (NULL)
  , mi8_objectPoolUploadingLanguage(0) // only valid if "initially uploading" or "language updating"
  , mi8_objectPoolUploadedLanguage(0) // only valid if "ObjectPoolUploadedSuccessfully"
  , mui16_objectPoolUploadingLanguageCode (0x0000)
  , mui16_objectPoolUploadedLanguageCode (0x0000)
  , mui32_uploadTimestamp (0)
  , mui32_uploadTimeout (0) // will be set when needed
  , mui8_commandParameter (0) // this is kinda used as a cache only, because it's a four-case if-else to get the first byte!
  , mui8_uploadError (0) // only valid in OPCannotBeUploaded case
  , men_sendSuccess (__IsoAgLib::MultiSend_c::SendSuccess)
  , mui16_inputStringId (0xFFFF) // will be set when first chunk is received
  , mui8_inputStringLength (0) // will be set when first chunk is received
  , mi32_nextWsMaintenanceMsg (-1)
  , mb_receiveFilterCreated (false)
  , mui8_clientId (aui8_clientId)
  , mc_data (SINGLETON_VEC_KEY_PARAMETER_USE)
  , men_displayState (VtClientDisplayStateHidden)
  , mq_sendUpload()
  , mlist_auxAssignments()
  , mc_iVtObjectStreamer (*this)
  , mi32_timeWsAnnounceKey (-1) // no announce tries started yet...
  , mi32_fakeVtOffUntil (-1) // no faking initially
  , mb_isSlave(ab_isSlave)
{
  // the generated initAllObjectsOnce() has to ensure to be idempotent! (vt2iso-generated source does this!)
  mrc_pool.initAllObjectsOnce (SINGLETON_VEC_KEY);
  // now let all clients know which client they belong to
  if (mui8_clientId > 0) // the iVtObjects are initialised with 0 as default index
  {
    for (uint16_t ui16_objIndex = 0; ui16_objIndex < mrc_pool.getNumObjects(); ui16_objIndex++)
      mrc_pool.getIVtObjects()[0][ui16_objIndex]->setClientID (mui8_clientId);
    for (uint8_t ui8_objLangIndex = 0; ui8_objLangIndex < mrc_pool.getNumLang(); ui8_objLangIndex++)
      for (uint16_t ui16_objIndex = 0; ui16_objIndex < mrc_pool.getNumObjectsLang(); ui16_objIndex++)
        mrc_pool.getIVtObjects()[ui8_objLangIndex+1][ui16_objIndex]->setClientID (mui8_clientId);
  }

  if (apc_versionLabel)
  {
    const uint32_t cui_len = CNAMESPACE::strlen (apc_versionLabel);
    if ( ((mrc_pool.getNumLang() == 0) && (cui_len > 7))
      || ((mrc_pool.getNumLang()  > 0) && (cui_len > 5))
       )
    { // too long, fail!
      getILibErrInstance().registerError (iLibErr_c::Precondition, iLibErr_c::IsoTerminal);
      men_objectPoolState = OPCannotBeUploaded; // has to be checked after calling this constructor!
      return;
    }
    unsigned int i=0;
    for (; i<cui_len; i++) marrp7c_versionLabel [i] = apc_versionLabel [i];
    for (; i<7;       i++) marrp7c_versionLabel [i] = ' '; // ASCII: Space
    mb_usingVersionLabel = true;
  }
  // else: default set in initialization-list

  /// ms_uploadPhases
  ms_uploadPhasesAutomatic[0] = UploadPhase_s (&mc_iVtObjectStreamer, 0);
  ms_uploadPhasesAutomatic[1] = UploadPhase_s (&mc_iVtObjectStreamer, 0);
  ms_uploadPhaseUser = UploadPhase_s (&mc_iVtObjectStreamer, 0);
}


/** default destructor, which initiate sending address release for all own identities
  @see VtClientServerCommunication_c::~VtClientServerCommunication_c
 */
VtClientServerCommunication_c::~VtClientServerCommunication_c()
{
  getMultiReceiveInstance4Comm().deregisterClient (*this);
  getIsoFilterManagerInstance4Comm().removeIsoFilter (IsoFilter_s (*this, (0x3FFFF00UL), (VT_TO_ECU_PGN << 8),       &getIdentItem().isoName(), NULL, 8));
  getIsoFilterManagerInstance4Comm().removeIsoFilter (IsoFilter_s (*this, (0x3FFFF00UL), (ACKNOWLEDGEMENT_PGN << 8), &getIdentItem().isoName(), NULL, 8));
}


void
VtClientServerCommunication_c::timeEventSendLanguagePGN()
{
  // Get Local Settings (may not be reached, when terminal is switched on after ECU, as VT sends LNAGUAGE Info on startup!
  mc_data.setExtCanPkg3 (6, 0, REQUEST_PGN_MSG_PGN>>8,
                        mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                        (LANGUAGE_PGN & 0xFF), ((LANGUAGE_PGN >> 8)& 0xFF), ((LANGUAGE_PGN >> 16)& 0xFF));
  getCanInstance4Comm() << mc_data;      // Command: REQUEST_PGN_MSG_PGN
  mpc_vtServerInstance->getLocalSettings()->lastRequested = HAL::getTime();
}


void
VtClientServerCommunication_c::timeEventUploadPoolTimeoutCheck()
{
  /// Do TIME-OUT Checks ALWAYS!
  if ((men_uploadPoolState == UploadPoolWaitingForLoadVersionResponse)
   || (men_uploadPoolState == UploadPoolWaitingForMemoryResponse)
   || (men_uploadPoolState == UploadPoolWaitingForEOOResponse))
  { // waiting for initial stuff was timed out
    if (((uint32_t) HAL::getTime()) > (mui32_uploadTimeout + mui32_uploadTimestamp))
    {
      men_uploadPoolState = UploadPoolFailed;
      mui32_uploadTimestamp = HAL::getTime();
      mui32_uploadTimeout = DEF_WaitFor_Reupload; // wait X secs for possible reuploading...
    }
  }

  if (men_uploadPoolState == UploadPoolWaitingForStoreVersionResponse)
  { // store version was timed out
    if (((uint32_t) HAL::getTime()) > (mui32_uploadTimeout + mui32_uploadTimestamp))
    { // we couldn't store for some reason, but don't care, finalize anyway...
#if DEBUG_VTCOMM
      INTERNAL_DEBUG_DEVICE << "StoreVersion TimedOut!" << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
      finalizeUploading();
    }
  }

  if (men_uploadPoolState == UploadPoolUploading)
  { // Check if we expect a partial (OBJECTPOOL) UPLOAD to fail/finish?
    checkPoolPhaseRunningMultiSend();
  }
}

void
VtClientServerCommunication_c::checkPoolPhaseRunningMultiSend()
{
  switch (men_sendSuccess)
  {
    case __IsoAgLib::MultiSend_c::Running:
    { // do nothing, still wait.
    } break;
    case __IsoAgLib::MultiSend_c::SendAborted:
    { // aborted sending - re-send the current stream (partial OP)
      startCurrentUploadPhase();
    } break;
    case __IsoAgLib::MultiSend_c::SendSuccess:
    { // see what part we just finished
      indicateUploadPhaseCompletion(); // may complete the upload or switch to the next phase
    } break;
  } // switch
}


void
VtClientServerCommunication_c::indicateUploadPhaseCompletion()
{
  if (men_uploadPoolType == UploadPoolTypeUserPoolUpdate)
  { // we only have one part, so we're done!
    mc_iVtObjectStreamer.mpc_objectsToUpload = NULL; // just for proper cleanup.
    // We don't need that pointer anymore. It can be invalid after we told the client
    // that we're done with partial user objectpool upload/update.
    indicateUploadCompletion(); // Send "End of Object Pool" message
  }
  else
  { // we may have multiple parts, so check that..
    // move to next possible one.
    if (men_uploadPoolType == UploadPoolTypeLanguageUpdate)
      men_uploadPhaseAutomatic = UploadPhase_t (men_uploadPhaseAutomatic+2); // skip the GENERAL parts, move on directly to next LANGUAGE part!
    else
      men_uploadPhaseAutomatic = UploadPhase_t (men_uploadPhaseAutomatic+1);

    startCurrentUploadPhase();
  }
}


void
VtClientServerCommunication_c::startCurrentUploadPhase()
{
  IsoAgLib::iMultiSendStreamer_c* streamer = NULL;
  switch (men_uploadPoolType)
  {
    case UploadPoolTypeUserPoolUpdate:
      streamer = ms_uploadPhaseUser.pc_streamer;
      mc_iVtObjectStreamer.mpc_objectsToUpload = mppc_uploadPhaseUserObjects;
      mc_iVtObjectStreamer.setStreamSize (ms_uploadPhaseUser.ui32_size);
      break;

    case UploadPoolTypeCompleteInitially:
    case UploadPoolTypeLanguageUpdate:
      // First, check current phase.
      // while the current phase is n/a, move to next.
      while ((men_uploadPhaseAutomatic <= UploadPhaseLAST) && (ms_uploadPhasesAutomatic [men_uploadPhaseAutomatic].ui32_size == 0))
      { // prepare for the next part
        if (men_uploadPoolType == UploadPoolTypeLanguageUpdate)
          men_uploadPhaseAutomatic = UploadPhase_t (men_uploadPhaseAutomatic+2); // skip the GENERAL parts, move on directly to next LANGUAGE part!
        else
          men_uploadPhaseAutomatic = UploadPhase_t (men_uploadPhaseAutomatic+1);
      }
      if (men_uploadPhaseAutomatic > UploadPhaseLAST)
      { // done with all phases!
        indicateUploadCompletion(); // Send "End of Object Pool" message
        return;
      }
      // else: start next phase
      streamer = ms_uploadPhasesAutomatic [men_uploadPhaseAutomatic].pc_streamer;
      // first, prepare the individual upload phases.
      switch (men_uploadPhaseAutomatic)
      {
        case UploadPhaseIVtObjectsFix:
          mc_iVtObjectStreamer.mpc_objectsToUpload = mrc_pool.getIVtObjects()[0]; // main FIX (lang. indep) iVtObject part
          mc_iVtObjectStreamer.setStreamSize (ms_uploadPhasesAutomatic [men_uploadPhaseAutomatic].ui32_size);
          break;

        case UploadPhaseIVtObjectsLang:
        { // phase 0 & 1 use iVtObjectStreamer, so prepare for that!
          const int8_t ci8_realUploadingLanguage = ((mi8_objectPoolUploadingLanguage < 0) ? 0 : mi8_objectPoolUploadingLanguage) + 1;
          mc_iVtObjectStreamer.mpc_objectsToUpload = mrc_pool.getIVtObjects()[ci8_realUploadingLanguage];
          mc_iVtObjectStreamer.setStreamSize (ms_uploadPhasesAutomatic [men_uploadPhaseAutomatic].ui32_size);
        } break;

        case UploadPhaseAppSpecificFix:
          break; // nop
        case UploadPhaseAppSpecificLang:
          break; // nop

        // no default, to catch warning on unhandled enum in case an enum-value is added.
      }
      break;
    // no default, to catch warning on unhandled enum in case an enum-value is added.
  }

  // streamer set up above (with correct size, etc.)
  isoaglib_assert (streamer);

  getMultiSendInstance4Comm().sendIsoTarget(
    mrc_wsMasterIdentItem.isoName(),
    mpc_vtServerInstance->getIsoName(),
    streamer,
    ECU_TO_VT_PGN, men_sendSuccess);
}


void
VtClientServerCommunication_c::timeEventPrePoolUpload()
{
  /// first you have to get number of softkeys, text font data and hardware before you could upload
  if (!mpc_vtServerInstance->getVtCapabilities()->lastReceivedSoftkeys
       && ((mpc_vtServerInstance->getVtCapabilities()->lastRequestedSoftkeys == 0)
       || ((HAL::getTime()-mpc_vtServerInstance->getVtCapabilities()->lastRequestedSoftkeys) > 1000)))
  { // Get Number Of Soft Keys
    mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8,
                          mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                          194, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    getCanInstance4Comm() << mc_data;      // Command: Get Technical Data --- Parameter: Get Number Of Soft Keys
    mpc_vtServerInstance->getVtCapabilities()->lastRequestedSoftkeys = HAL::getTime();
#if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE << "Requested first property (C2)..." << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
  }

  if (mpc_vtServerInstance->getVtCapabilities()->lastReceivedSoftkeys
      && (!mpc_vtServerInstance->getVtCapabilities()->lastReceivedFont)
      && ((mpc_vtServerInstance->getVtCapabilities()->lastRequestedFont == 0) || ((HAL::getTime()-mpc_vtServerInstance->getVtCapabilities()->lastRequestedFont) > 1000)))
  { // Get Text Font Data
    mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8,
                          mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                          195, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    getCanInstance4Comm() << mc_data;      // Command: Get Technical Data --- Parameter: Get Text Font Data
    mpc_vtServerInstance->getVtCapabilities()->lastRequestedFont = HAL::getTime();
#if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE << "Requested fonts (C3)..." << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
  }

  if (mpc_vtServerInstance->getVtCapabilities()->lastReceivedSoftkeys
      && mpc_vtServerInstance->getVtCapabilities()->lastReceivedFont
      && (!mpc_vtServerInstance->getVtCapabilities()->lastReceivedHardware)
      && ((mpc_vtServerInstance->getVtCapabilities()->lastRequestedHardware == 0)
      || ((HAL::getTime()-mpc_vtServerInstance->getVtCapabilities()->lastRequestedHardware) > 1000)))
  { // Get Hardware
    mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8,
                          mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                          199, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    getCanInstance4Comm() << mc_data;      // Command: Get Technical Data --- Parameter: Get Hardware
    mpc_vtServerInstance->getVtCapabilities()->lastRequestedHardware = HAL::getTime();
#if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE << "Requested hardware (C7)..." << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
  }
}


bool
VtClientServerCommunication_c::timeEventPoolUpload()
{
  // Do MAIN-Phase a) at INIT and b) <timeout> seconds after FAIL
  if (((men_uploadPoolState == UploadPoolFailed) && (((uint32_t) HAL::getTime()) > (mui32_uploadTimeout + mui32_uploadTimestamp)))
    || (men_uploadPoolState == UploadPoolInit))
  {
    // Take the version that's been set up NOW and try to load/upload it.
    setObjectPoolUploadingLanguage();
    // Do we want to try to "Load Version" or go directly to uploading?
    // Added this preprocessor so loading of object pools can be prevented for development purposes
#ifdef NO_LOAD_VERSION
#else
    if (mb_usingVersionLabel)
    {
#ifdef GET_VERSIONS
      // this is for test purposes only!
      static int b_getVersionsSendTime = 0;
      if (b_getVersionsSendTime == 0)
      { // send out get versions first
        mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8,
                              mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                              223, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
        getCanInstance4Comm() << mc_data;     // Command: Non Volatile Memory --- Parameter: Load Version
        b_getVersionsSendTime = HAL::getTime();
      }
      if ((b_getVersionsSendTime+500) > HAL::getTime())
      { // wait for answer first
        return true;
      }
#endif
      char lang1, lang2;
      if (mui16_objectPoolUploadingLanguageCode != 0x0000)
      {
        lang1 = mui16_objectPoolUploadingLanguageCode >> 8;
        lang2 = mui16_objectPoolUploadingLanguageCode & 0xFF;
      }
      else
      {
        lang1 = marrp7c_versionLabel [5];
        lang2 = marrp7c_versionLabel [6];
      }

      // Try to "Non Volatile Memory - Load Version" first!
      mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8, mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                            209,
                            marrp7c_versionLabel [0], marrp7c_versionLabel [1], marrp7c_versionLabel [2], marrp7c_versionLabel [3], marrp7c_versionLabel [4], lang1, lang2);
      getCanInstance4Comm() << mc_data;     // Command: Non Volatile Memory --- Parameter: Load Version
                                            //(Command: Non Volatile Memory --- Parameter: Delete Version - just a quick hack!)
      // start uploading after reception of LoadVersion Response
      men_uploadPoolState = UploadPoolWaitingForLoadVersionResponse;
      men_uploadPoolType = UploadPoolTypeCompleteInitially; // need to set this, so that eventObjectPoolUploadedSucessfully is getting called (also after load, not only after upload)
      mui32_uploadTimeout = DEF_TimeOut_VersionLabel;
      mui32_uploadTimestamp = HAL::getTime();
#if DEBUG_VTCOMM
      INTERNAL_DEBUG_DEVICE << "Trying Load Version (D1) for Version ["<<marrp7c_versionLabel [0]<< marrp7c_versionLabel [1]<< marrp7c_versionLabel [2]<< marrp7c_versionLabel [3]<< marrp7c_versionLabel [4]<< lang1<< lang2<<"]..." << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
    }
    else
#endif
    { // NO_LOAD_VERSION
      // Start uploading right now, no "LoadVersion" first
      initObjectPoolUploadingPhases (UploadPoolTypeCompleteInitially); // no language specific upload - normal upload!
      sendGetMemory();
    }
  }
  return false;
}


/** periodically event
  @return true -> all planned activities performed in allowed time
 */
bool
VtClientServerCommunication_c::timeEvent(void)
{
#if DEBUG_HEAP_USEAGE
  if ((sui16_lastPrintedSendUploadQueueSize < sui16_sendUploadQueueSize)
  || (sui16_lastPrintedMaxSendUploadQueueSize < sui16_maxSendUploadQueueSize))
  { // MAX amount of sui16_sendUploadQueueSize or sui16_maxSendUploadQueueSize
    sui16_lastPrintedSendUploadQueueSize = sui16_sendUploadQueueSize;
    sui16_lastPrintedMaxSendUploadQueueSize = sui16_maxSendUploadQueueSize;
    INTERNAL_DEBUG_DEVICE << "Max: " << sui16_sendUploadQueueSize << " Items in FIFO, "
                            << sui16_sendUploadQueueSize << " x SendUpload_c: Mal-Alloc: "
                            << sizeSlistTWithMalloc (sizeof (SendUpload_c), sui16_sendUploadQueueSize)
                            << "/" << sizeSlistTWithMalloc (sizeof (SendUpload_c), 1)
                            << ", Chunk-Alloc: "
                            << sizeSlistTWithChunk (sizeof (SendUpload_c), sui16_sendUploadQueueSize)
                            << INTERNAL_DEBUG_DEVICE_NEWLINE << INTERNAL_DEBUG_DEVICE_ENDL;
  }
#endif

  // do further activities only if registered ident is initialised as ISO and already successfully address-claimed...
  if (!mrc_wsMasterIdentItem.isClaimedAddress()) return true;

  if (!mb_receiveFilterCreated)
  { /*** MultiReceive/IsoFilterManager Registration ***/
    getMultiReceiveInstance4Comm().registerClientIso (*this, getIdentItem().isoName(), VT_TO_ECU_PGN);
    getIsoFilterManagerInstance4Comm().insertIsoFilter (IsoFilter_s (*this, (0x3FFFF00UL), (VT_TO_ECU_PGN << 8),       &getIdentItem().isoName(), NULL, 8));
    getIsoFilterManagerInstance4Comm().insertIsoFilter (IsoFilter_s (*this, (0x3FFFF00UL), (ACKNOWLEDGEMENT_PGN << 8), &getIdentItem().isoName(), NULL, 8));

    mb_receiveFilterCreated = true;
  }

  /*** Regular start is here (the above preconditions should be satisfied if system is finally set up. ***/
  /*******************************************************************************************************/
  System_c::triggerWd();

  // VT Alive checks
  // Will trigger "doStart" / "doStop"
  // doStart will also take care for announcing the working-set
  checkAndHandleVtStateChange();

  // Do nothing if there's no VT active
  if (!isVtActive()) return true;

  if (mb_isSlave)
  {
    if (men_objectPoolState != OPUploadedSuccessfully)
    { // @todo WS SLAVE: set to OPUploadedSuccessfully only after master has successfully uploaded pool
      men_objectPoolState = OPUploadedSuccessfully;
      men_uploadType = UploadIdle;
    }
  }
  else
  {
    // Check if the working-set is completely announced
    if (!mrc_wsMasterIdentItem.getIsoItem()->isWsAnnounced (mi32_timeWsAnnounceKey)) return true;

    // Check if WS-Maintenance is needed
    if ((mi32_nextWsMaintenanceMsg <= 0) || (HAL::getTime() >= mi32_nextWsMaintenanceMsg))
    { // Do periodically WS-Maintenance sending (every second)
      mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8, mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                            0xFF, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
      getCanInstance4Comm() << mc_data;     // G.2: Function: 255 / 0xFF Working Set Maintenance Message

      mi32_nextWsMaintenanceMsg = HAL::getTime() + 1000;
    }

    // If our IsoItem has claimed address, immediately try to get the LANGUAGE_PGN from VT/anyone ;-) (regardless of pool-upload!)
    if ((!mpc_vtServerInstance->getLocalSettings()->lastReceived) && ((mpc_vtServerInstance->getLocalSettings()->lastRequested == 0) || ((HAL::getTime()-mpc_vtServerInstance->getLocalSettings()->lastRequested) > 2000)))
    { // Try every 2 seconds to get the LANGUAGE_PGN, be polite to not bombard the VT...
      /** @todo SOON-258 Give up somewhen?? Or retry really every 2 seconds? Don't care too much for now, shouldn't happen in real systems... */
      timeEventSendLanguagePGN();
    }

    // lastReceived will be set if vtserverinstance processes the language pgn
    if (!mpc_vtServerInstance->getLocalSettings()->lastReceived)
      return true; // do not proceed if LANGUAGE not yet received!

    if (men_objectPoolState == OPCannotBeUploaded)
      /** @todo SOON-258 is this correctly assumed? -> if it couldn't be uploaded, only disconnecting/connecting VT helps! Should be able to be uploaded anyway... */
      return true;
  }
  // from HERE ON potential longer command sequences might be started
  // which include sending or starting of TP sessions and the like
  // - and the most time critical ALIVE has already been sent
  // ---> stop continuation, if execution time end is reached
  if ( Scheduler_Task_c::getAvailableExecTime() == 0 ) return false;


  /// Now from here on the Pool's state is: "OPInitial" or "OPUploadedSuccessfully"
  /// UPLOADING --> OBJECT-POOL<--
  if (men_uploadType == UploadPool)
  {
    timeEventUploadPoolTimeoutCheck();

    // Handled down below are (as they're no TIME-OUTs but INITIATIONs):
    // if (UploadPoolFailed)
    // if (UploadPoolInit)

    // Do we have to request (any) vt capabilities?
    if (!(mpc_vtServerInstance->getVtCapabilities()->lastReceivedFont && mpc_vtServerInstance->getVtCapabilities()->lastReceivedHardware && mpc_vtServerInstance->getVtCapabilities()->lastReceivedSoftkeys))
    { /// Pool-Upload: PRE Phase (Get VT-Properties)
      timeEventPrePoolUpload();
    }
    else
    { /// Handle other than Response states now here: INITIALIZING-States!
      // - if (UploadPoolFailed)
      // - if (UploadPoolInit)
      /// Pool-Upload: MAIN Phase (Try2Load / Upload / Try2Save)
      if (timeEventPoolUpload()) return true;
    }
  }

  /// UPLOADING --> COMMAND <-- ///
  // Can only be done if the objectpool is successfully uploaded!
  if (men_objectPoolState != OPUploadedSuccessfully) return true;

  /// FROM HERE ON THE OBJECT-POOL >>IS<< UPLOADED SUCCESSFULLY
  /// HANDLE CASE A) AND B) FROM HERE NOW
  /// A) NOW HERE THE LANGUAGE CHANGE IS HANDLED
  if ((mi8_objectPoolUploadingLanguage == -2) // indicates no update running
       && (mi8_vtLanguage != mi8_objectPoolUploadedLanguage))
  { // update languages on the fly
    setObjectPoolUploadingLanguage();
    /// NOTIFY THE APPLICATION so it can enqueue some commands that are processed BEFORE the update is done
    /// e.g. switch to a "Wait while changing language..." datamask.
    mrc_pool.eventPrepareForLanguageChange (mi8_objectPoolUploadingLanguage, mui16_objectPoolUploadingLanguageCode);

    sendCommandUpdateLanguagePool();
    // we keep (mi8_objectPoolUploadingLanguage != -2), so a change in between doesn't care and won't happen!!
  }

  /// B) NOW HERE THE RUNTIME COMMANDS ARE BEING HANDLED
  if (men_uploadType == UploadCommand)
  { // NO Response/timeOut for (C.2.3 Object Pool Transfer Message) "UploadObjectPool" - Only for "UploadMultiPacketCommand"

    /// Handle special "command" Partial Pool Update
    switch (men_uploadCommandState)
    {
    case UploadCommandPartialPoolUpdate:
      // special "command"
      checkPoolPhaseRunningMultiSend();
      break;

    case UploadCommandWithAwaitingResponse:
      // "normal" command
      switch (men_sendSuccess)
      {
      case __IsoAgLib::MultiSend_c::SendAborted:
        /// Note: The behavior of what to do seems to be not really specified in ISO11783-3.
        // If aborted, retry regardless of any application-logic-retry, as it was a multisend problem, not a problem of the command itself!
        startUploadCommand();
        break;

      case __IsoAgLib::MultiSend_c::Running:
        // increase sent time-stamp, so it matches best the time when the multisend has finished sending, so that the timeout counts from that time on!
        mui32_uploadTimestamp = HAL::getTime();
        break;

      case __IsoAgLib::MultiSend_c::SendSuccess: // no break, handle along with default: wait for response!
        // won't reach here when "Running", as timestamp is getting get to now above!
        // Waiting for an answer now... Did it time out?
        if (((uint32_t) HAL::getTime()) > (mui32_uploadTimeout + mui32_uploadTimestamp))
        { // Time-Out! in "Upload Command"-State
          // No retries, No continue.
          // It's the safest thing to just reconnect to the VT.
          // So let's get disconnected (can't do this actively)
          fakeVtOffPeriod (6000); // fake the VT 6 seconds off!
          return true; // quit, as we're probably not anymore in the connected state!
        } // else: no time-out, wait on...
        break;
      } // switch send success
    } // switch upload state
  } // UploadCommand

  // Is a) no Upload running and b) some Upload to do?
  if ((men_uploadType == UploadIdle) && !mq_sendUpload.empty())
  { // Start Uploading
    startUploadCommand();
  }
  return true;
}


/** process received ack messages
  @return true -> message was processed; else the received CAN message will be served to other matching CanCustomer_c */
bool
VtClientServerCommunication_c::processMsgAck()
{
  // shouldn't be possible, but check anyway to get sure.
  if (!mpc_vtServerInstance) return false;

  // don't react on NACKs from other VTs than the one we're communicating with!
  if (mpc_vtServerInstance->getVtSourceAddress() != mc_data.isoSa()) return false;

  if (mc_data.getUint8Data (0) != 0x01) return true; // Only react if "NOT ACKNOWLEDGE"!

#if !defined(IGNORE_VTSERVER_NACK)  // The NACK must be ignored for the Mueller VT Server
  // check if we have Agrocom/Mller with Version < 3, so we IGNORE this NACK BEFORE the pool is finally uploaded.
  bool b_ignoreNack = false; // normally DO NOT ignore NACK
  if (getIsoMonitorInstance4Comm().existIsoMemberNr (mc_data.isoSa()))
  { // sender exists in isomonitor, so query its Manufacturer Code
    const uint16_t cui16_manufCode = getIsoMonitorInstance4Comm().isoMemberNr (mc_data.isoSa()).isoName().manufCode();
    if (((cui16_manufCode == 98) /*Mller Elektronik*/ || (cui16_manufCode == 103) /*Agrocom*/) &&
          ((mpc_vtServerInstance->getVtCapabilities()->lastReceivedVersion == 0) ||
          (mpc_vtServerInstance->getVtCapabilities()->iso11783version < 3)))
    {
      if (men_objectPoolState != OPUploadedSuccessfully)
      { // mueller/agrocom hack - ignore upload while no objectpool is displayed
        b_ignoreNack = true;
      }
    }
  }

  if (!b_ignoreNack)
  {
    // for now ignore source address which must be VT of course. (but in case a NACK comes in before the first VT Status Message
    // Check if a VT-related message was NACKed. Check embedded PGN for that
    const uint32_t cui32_pgn =  uint32_t (mc_data.getUint8Data (5)) |
                               (uint32_t (mc_data.getUint8Data (6)) << 8) |
                               (uint32_t (mc_data.getUint8Data (7)) << 16);
    switch (cui32_pgn)
    {
      case ECU_TO_VT_PGN:
      case WORKING_SET_MEMBER_PGN:
      case WORKING_SET_MASTER_PGN:
#if DEBUG_VTCOMM
        INTERNAL_DEBUG_DEVICE << "\n==========================================================================================="
                              << "\n=== VT NACKed "<<cui32_pgn<<", starting all over again -> faking VT loss in the following: ===";
#endif
        mrc_wsMasterIdentItem.getIsoItem()->sendSaClaim(); // optional, but better do this: Repeat address claim!
        /// passing "true": fake NOT-alive state of VT for now!
        fakeVtOffPeriod(1000); // arbitrary time-span > 0 so checkAndHandle..() will call doStop!
        checkAndHandleVtStateChange(); // will also notify application via "eventEnterSafeState"
        fakeVtOffStop(); // enough faking, let it get restart asap in the timeEvent!
        break;
    } // switch
  }
#endif

  return true; // (N)ACK for our SA will NOT be of interest for anyone else...
}


/** process received language pgn
  @return true -> message was processed; else the received CAN message will be served to other matching CanCustomer_c
 */
void
VtClientServerCommunication_c::notifyOnVtsLanguagePgn()
{
  mi8_vtLanguage = -1; // indicate that VT's language is not supported by this WS, so the default language should be used

  if (mpc_vtServerInstance)
  { // slave may have no WS in pool!
    if (!mb_isSlave)
    {
      const uint8_t cui8_languages = mrc_pool.getWorkingSetObject().get_vtObjectWorkingSet_a().numberOfLanguagesToFollow;
      for (int i=0; i<cui8_languages; i++)
      {
        const uint8_t* lang = mrc_pool.getWorkingSetObject().get_vtObjectWorkingSet_a().languagesToFollow[i].language;
        if (mpc_vtServerInstance->getLocalSettings()->languageCode == ((lang[0] << 8) | lang[1]))
        {
          mi8_vtLanguage = i; // yes, VT's language is directly supported by this workingset
          break;
        }
      }
    }
    mrc_pool.eventLanguagePgn (*mpc_vtServerInstance->getLocalSettings());
  }
}

void
VtClientServerCommunication_c::notifyOnVtStatusMessage()
{
  mrc_pool.eventVtStatusMsg();

  // set client display state appropriately
  setVtDisplayState (true, getVtServerInst().getVtState()->saOfActiveWorkingSetMaster);
}


void
VtClientServerCommunication_c::notifyOnAuxInputStatus()
{
  const IsoName_c& ac_inputIsoName = mc_data.getISONameForSA();
  uint8_t const cui8_inputNumber = mc_data.getUint8Data(2-1);

  // Look for all Functions that are controlled by this Input right now!
  for (STL_NAMESPACE::list<AuxAssignment_s>::iterator it = mlist_auxAssignments.begin(); it != mlist_auxAssignments.end(); it++)
  {
    if ( (it->mui8_inputNumber == cui8_inputNumber)
      && (it->mc_inputIsoName == ac_inputIsoName) )
    { // notify application on this new Input Status!
      uint16_t const cui16_inputValueAnalog = mc_data.getUint16Data (3-1);
      uint16_t const cui16_inputValueTransitions = mc_data.getUint16Data (5-1);
      uint8_t const cui8_inputValueDigital = mc_data.getUint8Data (7-1);
      mrc_pool.eventAuxFunctionValue (it->mui16_functionUid, cui16_inputValueAnalog, cui16_inputValueTransitions, cui8_inputValueDigital);
    }
  }
}


bool
VtClientServerCommunication_c::storeAuxAssignment()
{
  uint8_t const cui8_inputSaNew = mc_data.getUint8Data (2-1);
  uint8_t const cui8_inputNrNew = mc_data.getUint8Data (3-1); /// 0xFF means unassign!
  uint16_t const cui16_functionUidNew = mc_data.getUint16Data (4-1);
  if (!getIsoMonitorInstance4Comm().existIsoMemberNr (cui8_inputSaNew) && (cui8_inputNrNew != 0xFF))
    return false;

  for (STL_NAMESPACE::list<AuxAssignment_s>::iterator it = mlist_auxAssignments.begin(); it != mlist_auxAssignments.end(); )
  {
    if (it->mui16_functionUid == cui16_functionUidNew)
    { // we already have an assignment for this function
      // do we have to replace OR unassign?
      if (cui8_inputNrNew == 0xFF)
      { /// Unassign
        it = mlist_auxAssignments.erase (it);
        continue; // look through all assignments anyway, although we shouldn't have any doubles in there...
      }
      else
      { /// Reassign
        const IsoName_c& ac_inputIsoNameNew = getIsoMonitorInstance4Comm().isoMemberNr (cui8_inputSaNew).isoName();
        it->mc_inputIsoName = ac_inputIsoNameNew;
        it->mui8_inputNumber = cui8_inputNrNew;
        return true;
      }
    }
    it++;
  }

  // Function not found, so we need to add (in case it was NOT an unassignment)
  if (cui8_inputNrNew == 0xFF)
    return true; // unassignment is always okay!

  AuxAssignment_s s_newAuxAssignment;
  const IsoName_c& ac_inputIsoNameNew = getIsoMonitorInstance4Comm().isoMemberNr (cui8_inputSaNew).isoName();
  s_newAuxAssignment.mc_inputIsoName = ac_inputIsoNameNew;
  s_newAuxAssignment.mui8_inputNumber = cui8_inputNrNew;
  s_newAuxAssignment.mui16_functionUid = cui16_functionUidNew;

  mlist_auxAssignments.push_back (s_newAuxAssignment);
  return true;
}


/** process received can messages
  @return true -> message was processed; else the received CAN message will be served to other matching CanCustomer_c
 */
bool
VtClientServerCommunication_c::processMsg()
{
//  #if DEBUG_VTCOMM
//  INTERNAL_DEBUG_DEVICE << "Incoming Message: mc_data.isoPgn=" << mc_data.isoPgn() << " - HAL::getTime()=" << HAL::getTime() << " - data[0]=" << (uint16_t)mc_data.getUint8Data (0) << "...  ";
//  #endif

  if ((mc_data.isoPgn() & 0x3FF00) == ACKNOWLEDGEMENT_PGN)
  { // Pass on to ACK-Processing!
    return processMsgAck();
  }

  // for right now, if it's NOT an ACKNOWLEDGE_PGN,
  // it must be VT_TO_ECU addressed to us as defined by the IsoFilter

  uint8_t ui8_uploadCommandError; // who is interested in the errorCode anyway?
  uint8_t ui8_errByte=0; // from 1-8, or 0 for NO errorHandling, as NO user command (was intern command like C0/C2/C3/C7/etc.)

#define MACRO_setStateDependantOnError(errByte) \
  ui8_errByte = errByte;

  // If VT is not active, don't react on PKGs addressed to us, as VT's not active ;)
  if (!isVtActive()) return true;

  /// process all VT_TO_ECU addressed to us
  switch (mc_data.getUint8Data (0))
  {
    /*************************************/
    /*** ### VT Initiated Messages ### ***/
    case 0x00: // Command: "Control Element Function", parameter "Soft Key"
    case 0x01: // Command: "Control Element Function", parameter "Button"
      mrc_pool.eventKeyCode (mc_data.getUint8Data (1) /* key activation code (pressed, released, held) */,
                                         mc_data.getUint8Data (2) | (mc_data.getUint8Data (3) << 8) /* objID of key object */,
                                         mc_data.getUint8Data (4) | (mc_data.getUint8Data (5) << 8) /* objID of visible mask */,
                                         mc_data.getUint8Data (6) /* key code */,
                                         (mc_data.getUint8Data (0) != 0)/* 0 for sk, 1 for button -- matches wasButton? boolean */ );
      break;
    case 0x02: // Command: "Control Element Function", parameter "Pointing Event"
      mrc_pool.eventPointingEvent (mc_data.getUint8Data (1) | (mc_data.getUint8Data (2) << 8) /* X position in pixels */,
                                              mc_data.getUint8Data (3) | (mc_data.getUint8Data (4) << 8) /* Y position in pixels */);
      break;

    case 0x03: // Command: "VT Select Input Object"
      mrc_pool.eventVtSelectInputObject(uint16_t(mc_data.getUint8Data (1)) | (uint16_t(mc_data.getUint8Data (2)) << 8) /* objID */,
        mc_data.getUint8Data (3),
        mc_data.getUint8Data (4));
      break;

    case 0x04: // Command: "Control Element Function", parameter "VT ESC"
        /// if no error occured, that ESC is for an opened input dialog!!! Do not handle here!!!
        if (mc_data.getUint8Data (3) != 0x0)
          mrc_pool.eventVtESC(0xFFFF);
        else
          mrc_pool.eventVtESC(uint16_t(mc_data.getUint8Data (1)) | (uint16_t(mc_data.getUint8Data (2)) << 8));

        break;
    case 0x05: // Command: "Control Element Function", parameter "VT Change Numeric Value"
      mrc_pool.eventNumericValue (uint16_t(mc_data.getUint8Data (1)) | (uint16_t(mc_data.getUint8Data (2)) << 8) /* objID */,
                                              mc_data.getUint8Data (4) /* 1 byte value */,
                                              uint32_t(mc_data.getUint8Data (4)) | (uint32_t(mc_data.getUint8Data (5)) << 8) | (uint32_t(mc_data.getUint8Data (6)) << 16)| (uint32_t(mc_data.getUint8Data (7)) << 24) /* 4 byte value */);
      break;
    case 0x08:  // Command: "Control Element Function", parameter "VT Input String Value"
      if (mc_data.getUint8Data (3) <= 4) //within a 8 byte long cmd can be only a 4 char long string
      {
        VolatileMemory_c c_vmString (mc_data.getUint8DataConstPointer(4));
        mrc_pool.eventStringValue (uint16_t(mc_data.getUint8Data (1)) | (uint16_t(mc_data.getUint8Data (2)) << 8) /* objID */,
                                               mc_data.getUint8Data (3) /* total number of bytes */, c_vmString,
                                               mc_data.getUint8Data (3) /* total number of bytes */, true, true);
      }
      break;
    case 0x09:  // Command: "Command", parameter "Display Activation"

    {
      uint8_t arrui8_canData[8];
      uint8_t ui8_dataLen = mc_data.getLen();

      // cache the Data Bytes HERE
      mc_data.getDataToString(arrui8_canData);


      setVtDisplayState (false, mc_data.getUint8Data (1));


      // replace PGN, DA, SA , Data and send back as answer
      mc_data.setDataFromString(arrui8_canData,ui8_dataLen);

      mc_data.setIsoPgn (ECU_TO_VT_PGN);
      mc_data.setIsoSa (mrc_wsMasterIdentItem.getIsoItem()->nr());
      mc_data.setIsoPs (mpc_vtServerInstance->getVtSourceAddress());
      getCanInstance4Comm() << mc_data; // Command: "Command", parameter "Display Activation Response"
    }
    break;

    /***************************************************/
    /*** ### ECU Initiated Messages (=Responses) ### ***/
    case 0x12: // Command: "End of Object Pool Transfer", parameter "Object Pool Ready Response"
      if ((men_uploadType == UploadPool) && (men_uploadPoolState == UploadPoolWaitingForEOOResponse))
      { /// *** INITIAL POOL UPLOAD ***
        if (mc_data.getUint8Data (1) == 0)
        { /// NO Error with UPLOADING pool
// Added this preprocessor so storing of object pools can be prevented for development purposes
#ifndef NO_STORE_VERSION
          if (mb_usingVersionLabel)
          { // Store Version and finalize after "Store Version Response"
            char lang1, lang2;
            if (mui16_objectPoolUploadingLanguageCode != 0x0000)
            {
              lang1 = mui16_objectPoolUploadingLanguageCode >> 8;
              lang2 = mui16_objectPoolUploadingLanguageCode & 0xFF;
            }
            else
            {
              lang1 = marrp7c_versionLabel [5];
              lang2 = marrp7c_versionLabel [6];
            }
            mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8, mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                                  208 /* D0 */, marrp7c_versionLabel [0], marrp7c_versionLabel [1], marrp7c_versionLabel [2], marrp7c_versionLabel [3], marrp7c_versionLabel [4], lang1, lang2);
            getCanInstance4Comm() << mc_data;     // Command: Non Volatile Memory --- Parameter: Store Version

            // Now wait for response
            men_uploadPoolState = UploadPoolWaitingForStoreVersionResponse;
            mui32_uploadTimeout = DEF_TimeOut_VersionLabel;
            mui32_uploadTimestamp = HAL::getTime();
          }
          else
#endif // NO_STORE_VERSION
          { // Finalize now!
            finalizeUploading();
          }
        }
        else
        {
          men_uploadPoolState = UploadPoolFailed; // errorcode in mui8_uploadError;
          men_objectPoolState = OPCannotBeUploaded;
          mui8_uploadError = mc_data.getUint8Data (2);
        }
      }
      else if ((men_uploadType == UploadCommand) && (men_uploadCommandState == UploadCommandWithAwaitingResponse))
      { /// *** LANGUAGE POOL UPDATE ***
        MACRO_setStateDependantOnError(2)
        finalizeUploading(); // indicate that the language specific objects have been updated. also the user will get notified.
      }
      break;

    /***************************************/
    /*** ### AUX Assignment Messages ### ***/
    case 0x20:
    { // Command: "Auxiliary Control", parameter "Auxiliary Assignment"
      /** @todo SOON-258 If we can't assign because WE don't know this SA, should we anyway answer the assignment?
       * for now we don't answer if we can't take the assignment - VTs have to handle this anyway...
       * Update on 22.11.2007: Should be okay so far, as written, VT has to handle, and we can't NACK the assignment! */
      bool const cb_assignmentOkay = storeAuxAssignment();

      if (cb_assignmentOkay)
      { /// For now simply respond without doing anything else with this information. simply ack the assignment!
        mc_data.setIsoPgn (ECU_TO_VT_PGN);
        mc_data.setIsoSa (mrc_wsMasterIdentItem.getIsoItem()->nr());
        mc_data.setIsoPs (mpc_vtServerInstance->getVtSourceAddress());
        getCanInstance4Comm() << mc_data;
      }
    } break;

    /***************************************************/
    /*** ### ECU Initiated Messages (=Responses) ### ***/
    // ### Error field is also on byte 2 (index 1)
    case 0xA3: // Command: "Command", parameter "Control Audio Device Response"
    case 0xA4: // Command: "Command", parameter "Set Audio Volume Response"
    case 0xB2: // Command: "Command", parameter "Delete Object Pool Response"

#if DEBUG_VTCOMM
      if (0xB2 == mc_data.getUint8Data (0))
        INTERNAL_DEBUG_DEVICE << "Received response for 'Delete Object Pool' message!" << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
      MACRO_setStateDependantOnError (2)
      break;

    // ### Error field is also on byte 4 (index 3)
    case 0xA6: // Command: "Command", parameter "Change Size Response"
    case 0xA8: // Command: "Command", parameter "Change Numeric Value Response"
    case 0xA9: // Command: "Command", parameter "Change End Point Response"
    case 0xAA: // Command: "Command", parameter "Change Font Attributes Response"
    case 0xAB: // Command: "Command", parameter "Change Line Attributes Response"
    case 0xAC: // Command: "Command", parameter "Change Fill Attributes Response"
    case 0xAD: // Command: "Command", parameter "Change Active Mask Response"
    case 0x92: // Command: "Command", parameter "ESC Response"
      MACRO_setStateDependantOnError (4)
      break;

    // ### Error field is also on byte nr. 5 (index 4)
    case 0xA0: // Command: "Command", parameter "Hide/Show Object Response" (Container)
    case 0xA1: // Command: "Command", parameter "Enable/Disable Object Response" (Input Object)
    case 0xA2: // Command: "Command", parameter "Select Input Object Response"
    case 0xA7: // Command: "Command", parameter "Change Background Colour Response"
    case 0xAF: // Command: "Command", parameter "Change Attribute Response"
    case 0xB0: // Command: "Command", parameter "Change Priority Response"
    case 0xB8: // Command: "Command", parameter "Graphics Context Command"
      MACRO_setStateDependantOnError (5)
      break;

    // ### Error field is also on byte 6 (index 5)
    case 0xA5: // Command: "Command", parameter "Change Child Location Response"
    case 0xAE: // Command: "Command", parameter "Change Soft Key Mask Response"
    case 0xB3: // Command: "Command", parameter "Change String Value Response"
    case 0xB4: // Command: "Command", parameter "Change Child Position Response"
      MACRO_setStateDependantOnError (6)
      break;

    // ### Error field is on byte 7 (index 6)
    case 0xB1: // Command: "Command", parameter "Change List Item Response"
      MACRO_setStateDependantOnError (7)
      break;

    case 0xB9: // Command: "Get Technical Data", parameter "Get Attribute Value"
      MACRO_setStateDependantOnError (7)
      #ifdef USE_ISO_TERMINAL_GETATTRIBUTES
      // client requested any attribute value for an object in the pool -> create ram struct if not yet existing
      if ((mc_data.getUint8Data (1) == 0xFF) && (mc_data.getUint8Data (2) == 0xFF)) // object id is set to 0xFFFF to indicate error response
      {
        /// what to do if attribute value request returns error response???
      }
      else
      {
        // first store object ID for later use
        uint16_t ui16_objID = mc_data.getUint8Data(1) | (mc_data.getUint8Data (2) << 8);

        /// search for suitable iVtObject in all object lists of the client (pointer array to all fix and language dependent iVtObjects)

        uint8_t ui8_arrIndex = 0;
        bool b_objectFound = false;

        // first check if first item is the requested one -> working is the first item list no matter what objectID it has
        if (ui16_objID == mrc_pool.getIVtObjects()[ui8_arrIndex][0]->getID())
          mrc_pool.eventAttributeValue(mrc_pool.getIVtObjects()[ui8_arrIndex][0],
                                                  mc_data.getUint8Data(3),
                                                  mc_data.getUint8DataPointer()+4);
        else
        {
          // if last item of the list was reached or the requested object was found
          while (mrc_pool.getIVtObjects()[ui8_arrIndex] != NULL)
          {
            uint16_t ui16_arrBegin = 1;
            uint16_t ui16_arrMiddle;
            uint16_t ui16_arrEnd;

            // first item in list contains all fix objects of the pool (when language changes, these object stay the same)
            if (ui8_arrIndex == 0)
              ui16_arrEnd = mrc_pool.getNumObjects() - 1;
            else
              ui16_arrEnd = mrc_pool.getNumObjectsLang() - 1;

            // if object is among these we can leave the while-loop
            if ((ui16_objID < mrc_pool.getIVtObjects()[ui8_arrIndex][ui16_arrBegin]->getID())
             || (ui16_objID > mrc_pool.getIVtObjects()[ui8_arrIndex][ui16_arrEnd]->getID())) // range check
            {
              ui8_arrIndex++;
              continue; // try next object list, the requested object could not be found in the current list
            }

            while (ui16_arrBegin <= ui16_arrEnd)
            {
              ui16_arrMiddle = ui16_arrBegin + ((ui16_arrEnd - ui16_arrBegin) / 2);

              if (mrc_pool.getIVtObjects()[ui8_arrIndex][ui16_arrMiddle]->getID() == ui16_objID) // objID found?
              {
                b_objectFound = true;
                mrc_pool.eventAttributeValue(mrc_pool.getIVtObjects()[ui8_arrIndex][ui16_arrMiddle],
                                                        mc_data.getUint8Data(3),
                                                        mc_data.getUint8DataPointer()+4);
                break;
              }
              else
              {
                if (mrc_pool.getIVtObjects()[ui8_arrIndex][ui16_arrMiddle]->getID() > ui16_objID)
                  ui16_arrEnd = ui16_arrMiddle - 1;
                else
                  ui16_arrBegin = ui16_arrMiddle + 1;
              }
            }

            if ((ui8_arrIndex == 0) && b_objectFound) // an object is either a fix object or a language dependent object (at least once in any language dependent list)
              break;

            ui8_arrIndex++;
          }
        }
      }
      #endif
      break;
    case 0xBD: // Command: "Command", parameter "Lock/Unlock Mask Response"
      MACRO_setStateDependantOnError (3)
      break;
    case 0xC0: // Command: "Get Technical Data", parameter "Get Memory Size Response"
      mpc_vtServerInstance->setVersion();
      if ((men_uploadType == UploadPool) && (men_uploadPoolState == UploadPoolWaitingForMemoryResponse))
      {
        if (mc_data.getUint8Data (2) == 0)
        { // start uploading with all partial OPs (as init'd before Get Memory!), there MAY BE enough memory
          men_uploadPoolState = UploadPoolUploading;
        //men_uploadPhaseAutomatic [already initialized in "initObjectPoolUploadingPhases" to the correct starting phase]
          startCurrentUploadPhase();
        }
        else
          vtOutOfMemory();
      }
      break;
    case 0xC2: // Command: "Get Technical Data", parameter "Get Number Of Soft Keys Response"
      mpc_vtServerInstance->setSoftKeyData();
      break;
    case 0xC3: // Command: "Get Technical Data", parameter "Get Text Font Data Response"
      mpc_vtServerInstance->setTextFontData();
      break;
    case 0xC7: // Command: "Get Technical Data", parameter "Get Hardware Response"
      mpc_vtServerInstance->setHardwareData();
      break;
    case 0xD0: // Command: "Non Volatile Memory", parameter "Store Version Response"
      if ((men_uploadType == UploadPool) && (men_uploadPoolState == UploadPoolWaitingForStoreVersionResponse))
      {
        switch (mc_data.getUint8Data (5) & 0x0F)
        {
          case 0: // Successfully stored
          case 1: // Not used
          case 2: // Version label not known
          case 8: // General error
            break;
          case 4: // Insufficient memory available
          default: // well....
            getILibErrInstance().registerError(iLibErr_c::RemoteServiceOutOfMemory, iLibErr_c::IsoTerminal);
            break;
        }
        finalizeUploading();
      }
      else
      { // in case StoreVersion was triggered by the Application in normal operation
        MACRO_setStateDependantOnError (6)
      }
      break;
    case 0xD1: // Command: "Non Volatile Memory", parameter "Load Version Response"
      if ((men_uploadType == UploadPool) && (men_uploadPoolState == UploadPoolWaitingForLoadVersionResponse))
      {
        if ((mc_data.getUint8Data (5) & 0x0F) == 0)
        { // Successfully loaded
          finalizeUploading ();
#if DEBUG_VTCOMM
          INTERNAL_DEBUG_DEVICE << "Received Load Version Response (D1) without error..." << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
        }
        else
        {
          if (mc_data.getUint8Data (5) & (1<<2))
          { // Bit 2: // Insufficient memory available
#if DEBUG_VTCOMM
            INTERNAL_DEBUG_DEVICE << "Received Load Version Response (D1) with error OutOfMem..." << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
            vtOutOfMemory();
          }
          else
          { // Not used
            // General error
            // Version label not known
            initObjectPoolUploadingPhases (UploadPoolTypeCompleteInitially); // Send out pool! send out "Get Technical Data - Get Memory Size", etc. etc.
            sendGetMemory();
#if DEBUG_VTCOMM
            INTERNAL_DEBUG_DEVICE << "Received Load Version Response (D1) with VersionNotFound..." << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
          }
        }
      }
      break;

    default:
      // handle proprietary messages from an AGCO VT
      if (    mc_data.getUint8Data (0) >= 0x60
           && mc_data.getUint8Data (0) <= 0x7F
         )
      {
        MACRO_setStateDependantOnError( mrc_pool.eventProprietaryCommand( mpc_vtServerInstance->getIsoName().toConstIisoName_c() ) )
      }
      break;

  } // switch

  // Was it some command that requires queue-deletion & error processing?
  if (ui8_errByte != 0)
  {
    if (men_uploadType == UploadCommand)
    { /* if Waiting or Timedout (or Failed <shouldn't happen>) */
      if (men_sendSuccess == __IsoAgLib::MultiSend_c::SendSuccess)
      { /// Our command was successfully sent & responded to, so remove it from the queue
        if (mui8_commandParameter == mc_data.getUint8Data (0))
        { /* okay, right response for our current command! */
          // special treatment for Get Attribute Value command -> error byte is also being used as value byte for successful response
          if ((mui8_commandParameter == 0xB9) && (mc_data.getUint16Data (2-1) != 0xFFFF))
            ui8_uploadCommandError = 0;
          else
            ui8_uploadCommandError = mc_data.getUint8Data (ui8_errByte-1);
          /// Inform user on success/error of this command
          mrc_pool.eventCommandResponse (ui8_uploadCommandError, mc_data.getUint8DataConstPointer()); // pass "ui8_uploadCommandError" in case it's only important if it's an error or not. get Cmd and all databytes from "mc_data.name()"
#if DEBUG_VTCOMM
          if (ui8_uploadCommandError != 0)
          { /* error */
            INTERNAL_DEBUG_DEVICE << ">>> Command " << (uint32_t) mui8_commandParameter<< " failed with error " << (uint32_t) ui8_uploadCommandError << "!" << INTERNAL_DEBUG_DEVICE_ENDL;
          }
#endif
          finishUploadCommand(false); // finish command no matter if "okay" or "error"...
        }
      }
      else
      { /// Our command was Aborted or is still running, so do NOT remove from the queue
        /// let timeEvent try again / wait for stream to finish sending
      }
    }
  } // VT to this ECU
  return true;
}


uint32_t
VtClientServerCommunication_c::getUploadBufferSize()
{
  return mq_sendUpload.size();
}


uint8_t
VtClientServerCommunication_c::getUserClippedColor (uint8_t colorValue, IsoAgLib::iVtObject_c* obj, IsoAgLib::e_vtColour whichColour)
{
  if (mpc_vtServerInstance)
  {
    uint8_t colorDepth = mpc_vtServerInstance->getVtCapabilities()->hwGraphicType;
    if (((colorDepth == 0) && (colorValue > 1)) || ((colorDepth == 1) && (colorValue > 16)))
      return mrc_pool.convertColour (colorValue, colorDepth, obj, whichColour);
  }
  return colorValue;
}


void
VtClientServerCommunication_c::notifyOnNewVtServerInstance (VtServerInstance_c& r_newVtServerInst)
{
  if (mpc_vtServerInstance) return;

  mpc_vtServerInstance = &r_newVtServerInst;
}


void
VtClientServerCommunication_c::notifyOnVtServerInstanceLoss (VtServerInstance_c& r_oldVtServerInst)
{
  if (&r_oldVtServerInst == mpc_vtServerInstance)
    mpc_vtServerInstance = NULL;
}


/** special 9 byte sendCommand function for Change Child Position Command (no one else uses 9 byte commands!)
  @returns true if there was place in the SendUpload-Buffer (should always be the case now)
 */
bool
VtClientServerCommunication_c::sendCommand (uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7, uint8_t byte8, uint8_t byte9, bool b_enableReplaceOfCmd)
{
#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << "Enqueued 9-bytes: " << mq_sendUpload.size() << " -> ";
#endif

  msc_tempSendUpload.set (byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9);

  return queueOrReplace (msc_tempSendUpload, b_enableReplaceOfCmd);
}

/** @returns true if there was place in the SendUpload-Buffer (should always be the case now) */
bool
VtClientServerCommunication_c::sendCommand (uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7, uint8_t byte8, bool b_enableReplaceOfCmd, IsoAgLib::iVtObject_c** rppc_vtObjects, uint16_t aui16_numObjects)
{
#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << "Enqueued 8-bytes: " << mq_sendUpload.size() << " -> " << INTERNAL_DEBUG_DEVICE_ENDL;
#endif

  msc_tempSendUpload.set (byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, rppc_vtObjects, aui16_numObjects);

  return queueOrReplace (msc_tempSendUpload, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommand (uint8_t* apui8_buffer, uint32_t ui32_size)
{
#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << "Enqueued TP-bytes: " << mq_sendUpload.size() << " -> ";
#endif

  msc_tempSendUpload.set (apui8_buffer, ui32_size);

  return queueOrReplace (msc_tempSendUpload, false);
}

bool
VtClientServerCommunication_c::sendCommandChangeStringValue (IsoAgLib::iVtObjectString_c* apc_objectString, bool b_enableReplaceOfCmd)
{
#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << "Enqueued stringObject-mss: " << mq_sendUpload.size() << " -> ";
#endif

  msc_tempSendUpload.set (apc_objectString);

  return queueOrReplace (msc_tempSendUpload, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangePriority (IsoAgLib::iVtObject_c* apc_object, int8_t newPriority, bool b_enableReplaceOfCmd)
{
  if(newPriority < 3)
  {
    // only bother to send if priority is a legal value
    return sendCommand (176 /* Command: Command --- Parameter: Change Priority */,
                        apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                        newPriority, 0xFF, 0xFF, 0xFF, 0xFF,
                        b_enableReplaceOfCmd);
  }
  else
    return false;
}

bool
VtClientServerCommunication_c::sendCommandChangeEndPoint (IsoAgLib::iVtObject_c* apc_object,uint16_t newWidth, uint16_t newHeight, uint8_t newLineAttributes, bool b_enableReplaceOfCmd)
{
  return sendCommand (169 /* Command: Command --- Parameter: Change Size */,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      newWidth & 0xFF, newWidth >> 8,
                      newHeight & 0xFF, newHeight >> 8,
                      newLineAttributes,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandControlAudioDevice (uint8_t aui8_repetitions, uint16_t aui16_frequency, uint16_t aui16_onTime, uint16_t aui16_offTime)
{
  return sendCommand (163 /* Command: Command --- Parameter: Control Audio Device */,
                      aui8_repetitions,
                      aui16_frequency & 0xFF, aui16_frequency >> 8,
                      aui16_onTime & 0xFF, aui16_onTime >> 8,
                      aui16_offTime & 0xFF, aui16_offTime >> 8,
                      false); // don't care for enable-same command stuff
}

bool
VtClientServerCommunication_c::sendCommandSetAudioVolume (uint8_t aui8_volume)
{
  return sendCommand (164 /* Command: Command --- Parameter: Set Audio Volume */,
                      aui8_volume, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, false); // don't care for enableReplaceOfCommand parameter actually
}

bool
VtClientServerCommunication_c::sendCommandDeleteObjectPool()
{
  return sendCommand (178 /* Command: Command --- Parameter: Delete Object Pool */,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, true); // don't care for enableReplaceOfCommand parameter actually
}

bool
VtClientServerCommunication_c::sendCommandUpdateLanguagePool()
{
  /// Enqueue a fake command which will trigger the language object pool update to be multi-sent out. using 0x11 here, as this is the command then and won't be used
  return sendCommand (0x11 /* Command: Object Pool Transfer --- Parameter: Object Pool Transfer */,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, false) // replaces COULD happen if user-triggered sequences are there.
      && sendCommand (0x12 /* Command: Object Pool Transfer --- Parameter: Object Pool Ready */,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, false); // replaces COULD happen if user-triggered sequences are there.
}

bool
VtClientServerCommunication_c::sendCommandUpdateObjectPool (IsoAgLib::iVtObject_c** rppc_vtObjects, uint16_t aui16_numObjects)
{
  /// Enqueue a fake command which will trigger the language object pool update to be multi-sent out. using 0x11 here, as this is the command then and won't be used
  return sendCommand (0x11 /* Command: Object Pool Transfer --- Parameter: Object Pool Transfer */,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, false, rppc_vtObjects, aui16_numObjects) // replaces COULD happen if user-triggered sequences are there.
      && sendCommand (0x12 /* Command: Object Pool Transfer --- Parameter: Object Pool Ready */,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, false); // replaces COULD happen if user-triggered sequences are there.
}


//////////////
bool
VtClientServerCommunication_c::sendCommandChangeNumericValue (uint16_t aui16_objectUid, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, bool b_enableReplaceOfCmd)
{
  return sendCommand (168 /* Command: Command --- Parameter: Change Numeric Value */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      0xFF, byte1, byte2, byte3, byte4,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeAttribute (uint16_t aui16_objectUid, uint8_t attrId, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, bool b_enableReplaceOfCmd)
{
  return sendCommand (175 /* Command: Command --- Parameter: Change Attribute */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      attrId, byte1, byte2, byte3, byte4,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeSoftKeyMask (uint16_t aui16_objectUid, uint8_t maskType, uint16_t newSoftKeyMask, bool b_enableReplaceOfCmd)
{
  return sendCommand (174 /* Command: Command --- Parameter: Change Soft Key Mask */,
                      maskType,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      newSoftKeyMask & 0xFF, newSoftKeyMask >> 8,
                      0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeActiveMask (uint16_t aui16_objectUid, uint16_t maskId, bool b_enableReplaceOfCmd )
{
  return sendCommand (173 /* Command: Command --- Parameter: Change Active Mask */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      maskId & 0xFF, maskId >> 8,
                      0xFF, 0xFF, 0xFF, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeStringValue (uint16_t aui16_objectUid, const char* apc_newValue, uint16_t overrideSendLength, bool b_enableReplaceOfCmd)
{
#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << "Enqueued string-ref: " << mq_sendUpload.size() << " -> ";
#endif

  msc_tempSendUpload.set (aui16_objectUid, apc_newValue, overrideSendLength);

  return queueOrReplace (msc_tempSendUpload, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeChildPosition (uint16_t aui16_objectUid, uint16_t aui16_childObjectUid, int16_t x, int16_t y, bool b_enableReplaceOfCmd)
{
  return sendCommand (180 /* Command: Command --- Parameter: Change Child Position */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      aui16_childObjectUid & 0xFF, aui16_childObjectUid >> 8,
                      x & 0xFF, x >> 8,
                      y & 0xFF, y >> 8,
                      b_enableReplaceOfCmd);
}

//! should only be called with valid values ranging -127..0..128 (according to ISO!!!)
bool
VtClientServerCommunication_c::sendCommandChangeChildLocation (uint16_t aui16_objectUid, uint16_t aui16_childObjectUid, int16_t dx, int16_t dy, bool b_enableReplaceOfCmd)
{
  return sendCommand (165 /* Command: Command --- Parameter: Change Child Location */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      aui16_childObjectUid & 0xFF, aui16_childObjectUid >> 8,
                      dx+127, dy+127, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeBackgroundColour (uint16_t aui16_objectUid, uint8_t newColour, bool b_enableReplaceOfCmd)
{
  return sendCommand (167 /* Command: Command --- Parameter: Change Background Color */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      newColour, 0xFF, 0xFF, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeSize (uint16_t aui16_objectUid,uint16_t newWidth, uint16_t newHeight, bool b_enableReplaceOfCmd)
{
  return sendCommand (166 /* Command: Command --- Parameter: Change Size */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      newWidth & 0xFF, newWidth >> 8,
                      newHeight & 0xFF, newHeight >> 8,
                      0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeFillAttributes (uint16_t aui16_objectUid, uint8_t newFillType, uint8_t newFillColour, IsoAgLib::iVtObjectPictureGraphic_c* newFillPatternObject, bool b_enableReplaceOfCmd)
{
  return sendCommand (172 /* Command: Command --- Parameter: Change FillAttributes */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      newFillType, newFillColour,
                      (newFillType == 3) ? newFillPatternObject->getID() & 0xFF : 0xFF,
                      (newFillType == 3) ? newFillPatternObject->getID() >> 8 : 0xFF,
                      0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeFontAttributes (uint16_t aui16_objectUid, uint8_t newFontColour, uint8_t newFontSize, uint8_t newFontType, uint8_t newFontStyle, bool b_enableReplaceOfCmd)
{
  return sendCommand (170 /* Command: Command --- Parameter: Change FontAttributes */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      newFontColour, newFontSize, newFontType, newFontStyle, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeLineAttributes (uint16_t aui16_objectUid, uint8_t newLineColour, uint8_t newLineWidth, uint16_t newLineArt, bool b_enableReplaceOfCmd)
{
  return sendCommand (171 /* Command: Command --- Parameter: Change LineAttributes */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      newLineColour, newLineWidth, newLineArt & 0xFF, newLineArt >> 8, 0xFF,
                      b_enableReplaceOfCmd);
}
/////////////
#ifdef USE_ISO_TERMINAL_GRAPHICCONTEXT
// ########## Graphics Context ##########
//! @return Flag if successful
bool
VtClientServerCommunication_c::sendCommandSetGraphicsCursor(
  IsoAgLib::iVtObject_c* apc_object, int16_t ai16_x, int16_t ai16_y, bool b_enableReplaceOfCmd)
{
  uint16_t x=convert_n::castUI( ai16_x );
  uint16_t y=convert_n::castUI( ai16_y );
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_setGraphicsCursorCmdID,
                      x & 0xFF, x >> 8, y & 0xFF, y >> 8,
                      b_enableReplaceOfCmd );
}

bool
VtClientServerCommunication_c::sendCommandSetForegroundColour(
  IsoAgLib::iVtObject_c* apc_object, uint8_t newValue, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_setForegroundColourCmdID,
                      newValue, 0xFF, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandSetBackgroundColour(
  IsoAgLib::iVtObject_c* apc_object, uint8_t newValue, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_setBackgroundColourCmdID,
                      newValue, 0xFF, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandSetGCLineAttributes(
  IsoAgLib::iVtObject_c* apc_object, const IsoAgLib::iVtObjectLineAttributes_c* const newLineAttributes, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_setLineAttributeCmdID,
                      newLineAttributes->getID() & 0xFF, newLineAttributes->getID() >> 8, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandSetGCFillAttributes(
  IsoAgLib::iVtObject_c* apc_object, const IsoAgLib::iVtObjectFillAttributes_c* const newFillAttributes, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_setFillAttributeCmdID,
                      newFillAttributes->getID() & 0xFF, newFillAttributes->getID() >> 8, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandSetGCFontAttributes(
  IsoAgLib::iVtObject_c* apc_object, const IsoAgLib::iVtObjectFontAttributes_c* const newFontAttributes, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_setFontAttributeCmdID,
                      newFontAttributes->getID() & 0xFF, newFontAttributes->getID() >> 8, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandEraseRectangle(
  IsoAgLib::iVtObject_c* apc_object, int16_t ai16_x, int16_t ai16_y, bool b_enableReplaceOfCmd)
{
  uint16_t x=convert_n::castUI( ai16_x );
  uint16_t y=convert_n::castUI( ai16_y );
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_eraseRectangleCmdID,
                      x & 0xFF, x >> 8, y & 0xFF, y >> 8,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandDrawPoint(
  IsoAgLib::iVtObject_c* apc_object, bool  b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_drawPointCmdID,
                      0xFF, 0xFF, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandDrawLine(
  IsoAgLib::iVtObject_c* apc_object, int16_t ai16_x, int16_t ai16_y, bool b_enableReplaceOfCmd)
{
  uint16_t x=convert_n::castUI( ai16_x );
  uint16_t y=convert_n::castUI( ai16_y );
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_drawLineCmdID,
                      x & 0xFF, x >> 8, y & 0xFF, y >> 8,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandDrawRectangle(
  IsoAgLib::iVtObject_c* apc_object, int16_t ai16_x, int16_t ai16_y, bool b_enableReplaceOfCmd)
{
  uint16_t x=convert_n::castUI( ai16_x );
  uint16_t y=convert_n::castUI( ai16_y );
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_drawRectangleCmdID,
                      x & 0xFF, x >> 8, y & 0xFF, y >> 8,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandDrawClosedEllipse(
  IsoAgLib::iVtObject_c* apc_object, int16_t ai16_x, int16_t ai16_y, bool b_enableReplaceOfCmd)
{
  uint16_t x=convert_n::castUI( ai16_x );
  uint16_t y=convert_n::castUI( ai16_y );
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_drawClosedEllipseCmdID,
                      x & 0xFF, x >> 8, y & 0xFF, y >> 8,
                      b_enableReplaceOfCmd);
}

/// @todo OPTIMIZATION Revision4 Better struct for array of x/y pairs!
bool
VtClientServerCommunication_c::sendCommandDrawPolygon(
  IsoAgLib::iVtObject_c* apc_object, uint16_t ui16_numOfPoints, const int16_t* api16_x, const int16_t* api16_y, bool b_enableReplaceOfCmd)
{
  // Prevent from derefernzing NULL pointer.
  if ((0 == api16_x) || (0 == api16_y)) { ui16_numOfPoints = 0; }

  // Check if valid polgon (at least one point)
  if (0 == ui16_numOfPoints) { return false; }

  // Trivial case (like draw line) without TP.
  if (ui16_numOfPoints == 1) {
    uint16_t x = convert_n::castUI( *api16_x );
    uint16_t y = convert_n::castUI( *api16_y );
    return sendCommand( vtObjectGraphicsContext_c::e_commandID,
                        apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                        vtObjectGraphicsContext_c::e_drawPolygonCmdID,
                        x & 0xFF, x >> 8, y & 0xFF, y >> 8,
                        b_enableReplaceOfCmd );
  }

  // send a polygon with more than one point
  uint16_t ui16_bufferSize = 4+(ui16_numOfPoints*4);
  uint8_t *pui8_buffer = new uint8_t[ ui16_bufferSize ];
  pui8_buffer[0] = vtObjectGraphicsContext_c::e_commandID;
  pui8_buffer[1] = apc_object->getID() & 0xFF;
  pui8_buffer[2] = apc_object->getID() >> 8;
  pui8_buffer[3] = vtObjectGraphicsContext_c::e_drawPolygonCmdID;

  // add all points from the list to the buffer
  uint16_t ui16_index = 4;		/* first 4 bytes are set */
  for ( uint16_t ui16_currentPoint = 0;
        ui16_currentPoint < ui16_numOfPoints;
        ui16_currentPoint++ )
  {
    uint16_t x = convert_n::castUI( api16_x[ui16_currentPoint] );
    pui8_buffer[ui16_index]   = x & 0xFF;
    pui8_buffer[ui16_index+1] = x >> 8;
    uint16_t y = convert_n::castUI( api16_y[ui16_currentPoint] );
    pui8_buffer[ui16_index+2] = y & 0xFF;
    pui8_buffer[ui16_index+3] = y >> 8;
    ui16_index+=4;
  }

  // Send buffer as ISOBUS command.
  msc_tempSendUpload.set (pui8_buffer, ui16_bufferSize );
  delete[] pui8_buffer;
  return queueOrReplace (msc_tempSendUpload, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandDrawText(
  IsoAgLib::iVtObject_c* apc_object, uint8_t ui8_textType, uint8_t ui8_numOfCharacters, const char *apc_newValue, bool b_enableReplaceOfCmd)
{
  // Non TP case
  if (ui8_numOfCharacters <= 2) {
    uint8_t a = (ui8_numOfCharacters >= 1) ? apc_newValue[0] : 0xFF;
    uint8_t b = (ui8_numOfCharacters >= 2) ? apc_newValue[1] : 0xFF;
    return sendCommand( vtObjectGraphicsContext_c::e_commandID,
                        apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                        vtObjectGraphicsContext_c::e_drawTextCmdID,
                        ui8_textType, ui8_numOfCharacters, a, b,
                        b_enableReplaceOfCmd );
  }

  uint8_t *pui8_buffer = new uint8_t[6+ui8_numOfCharacters];
  pui8_buffer[0] = vtObjectGraphicsContext_c::e_commandID;
  pui8_buffer[1] = apc_object->getID() & 0xFF;
  pui8_buffer[2] = apc_object->getID() >> 8;
  pui8_buffer[3] = vtObjectGraphicsContext_c::e_drawTextCmdID;
  pui8_buffer[4] = ui8_textType;
  pui8_buffer[5] = ui8_numOfCharacters;

  for (uint8_t ui8_index = 0; ui8_index < ui8_numOfCharacters; ui8_index++)
    pui8_buffer[6+ui8_index] = apc_newValue [ui8_index];

  msc_tempSendUpload.set (pui8_buffer, (6+ui8_numOfCharacters));
  delete[] pui8_buffer;

  return queueOrReplace (msc_tempSendUpload, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandPanViewport(
  IsoAgLib::iVtObject_c* apc_object, int16_t ai16_x, int16_t ai16_y, bool b_enableReplaceOfCmd)
{
  uint16_t x = convert_n::castUI( ai16_x );
  uint16_t y = convert_n::castUI( ai16_y );
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_panViewportCmdID,
                      x & 0xFF, x >> 8, y & 0xFF, y >> 8,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandZoomViewport(
  IsoAgLib::iVtObject_c* apc_object, int8_t newValue, bool b_enableReplaceOfCmd)
{
  uint8_t zoom = convert_n::castUI( newValue );
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_zoomViewportCmdID,
                      zoom, 0xFF, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandPanAndZoomViewport(
  IsoAgLib::iVtObject_c* apc_object, int16_t ai16_x, int16_t ai16_y, int8_t newValue, bool b_enableReplaceOfCmd)
{
  uint16_t x = convert_n::castUI( ai16_x );
  uint16_t y = convert_n::castUI( ai16_y );
  uint8_t zoom = convert_n::castUI( newValue );
  uint8_t pui8_buffer[9];
  pui8_buffer[0] = vtObjectGraphicsContext_c::e_commandID;
  pui8_buffer[1] = apc_object->getID() & 0xFF;
  pui8_buffer[2] = apc_object->getID() >> 8;
  pui8_buffer[3] = vtObjectGraphicsContext_c::e_panAndZoomViewportCmdID;
  pui8_buffer[4] = x & 0xFF;
  pui8_buffer[5] = x >> 8;
  pui8_buffer[6] = y & 0xFF;
  pui8_buffer[7] = y >> 8;
  pui8_buffer[8] = zoom;

  // Send buffer as ISOBUS command.
  msc_tempSendUpload.set( pui8_buffer, sizeof(pui8_buffer)/sizeof(*pui8_buffer) );
  return queueOrReplace (msc_tempSendUpload, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandChangeViewportSize(
IsoAgLib::iVtObject_c* apc_object, uint16_t newWidth, uint16_t newHeight, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_changeViewportSizeCmdID,
                      newWidth & 0xFF, newWidth >> 8, newHeight & 0xFF, newHeight >> 8,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandDrawVtObject(
  IsoAgLib::iVtObject_c* apc_object, const IsoAgLib::iVtObject_c* const pc_VtObject, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_drawVTObjectCmdID,
                      pc_VtObject->getID() & 0xFF, pc_VtObject->getID() >> 8, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandCopyCanvas2PictureGraphic(
  IsoAgLib::iVtObject_c* apc_object, const IsoAgLib::iVtObjectPictureGraphic_c* const pc_VtObject, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_copyCanvasToPictureGraphicCmdID,
                      pc_VtObject->getID() & 0xFF, pc_VtObject->getID() >> 8, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandCopyViewport2PictureGraphic(
  IsoAgLib::iVtObject_c* apc_object, const IsoAgLib::iVtObjectPictureGraphic_c* const pc_VtObject, bool b_enableReplaceOfCmd)
{
  return sendCommand (vtObjectGraphicsContext_c::e_commandID,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      vtObjectGraphicsContext_c::e_copyViewportToPictureGraphicCmdID,
                      pc_VtObject->getID() & 0xFF, pc_VtObject->getID() >> 8, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}
// ########## END Graphics Context ##########
#endif

#ifdef USE_ISO_TERMINAL_GETATTRIBUTES
bool
VtClientServerCommunication_c::sendCommandGetAttributeValue( IsoAgLib::iVtObject_c* apc_object, const uint8_t cui8_attrID, bool b_enableReplaceOfCmd)
{
  return sendCommand (185 /* Command: Get Technical Data --- Parameter: Get Attribute Value */,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8,
                      cui8_attrID,
                      0xFF, 0xFF, 0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}
#endif

bool
VtClientServerCommunication_c::sendCommandLockUnlockMask( IsoAgLib::iVtObject_c* apc_object, bool b_lockMask, uint16_t ui16_lockTimeOut, bool b_enableReplaceOfCmd)
{
#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << "\n LOCK *** LOCK *** send lock(1)/unlock(0) message. With b_lockMask = " << b_lockMask << INTERNAL_DEBUG_DEVICE_ENDL;
  INTERNAL_DEBUG_DEVICE << "   and client sa = " << (int)dataBase().isoSa() << " and client id = " << (int)getClientId() << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
  return sendCommand (189 /* Command: Command --- Parameter: Lock/Undlock Mask */,
                      b_lockMask,
                      apc_object->getID() & 0xFF, apc_object->getID() >> 8, /* object id of the data mask to lock */
                      ui16_lockTimeOut & 0xFF, ui16_lockTimeOut >> 8, /* lock timeout on ms or zero for no timeout */
                      0xFF, 0xFF,
                      b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandHideShow (uint16_t aui16_objectUid, uint8_t b_hideOrShow, bool b_enableReplaceOfCmd)
{
  return sendCommand (160 /* Command: Command --- Parameter: Hide/Show Object */,
                      aui16_objectUid & 0xFF, aui16_objectUid >> 8,
                      b_hideOrShow,
                      0xFF, 0xFF, 0xFF, 0xFF, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::sendCommandEsc (bool b_enableReplaceOfCmd)
{
  return sendCommand (146 /* Command: Command --- Parameter: ESC */,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, b_enableReplaceOfCmd);
}

bool
VtClientServerCommunication_c::queueOrReplace (SendUpload_c& ar_sendUpload, bool b_enableReplaceOfCmd)
{
  if (men_objectPoolState != OPUploadedSuccessfully)
  {
#if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE << "--NOT ENQUEUED - POOL NO YET COMPLETELY UPLOADED!--" << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
    return false;
  }

  SendUpload_c* p_queue = NULL;
  uint8_t i = 0;
#ifdef USE_LIST_FOR_FIFO
#ifdef OPTIMIZE_HEAPSIZE_IN_FAVOR_OF_SPEED
  STL_NAMESPACE::list<SendUpload_c,MALLOC_TEMPLATE(SendUpload_c) >::iterator i_sendUpload;
#else
  STL_NAMESPACE::list<SendUpload_c>::iterator i_sendUpload;
#endif
#else
  STL_NAMESPACE::queue<SendUpload_c>::iterator i_sendUpload;
#endif
  if (mb_checkSameCommand && b_enableReplaceOfCmd && !mq_sendUpload.empty())
  { //get first equal command in queue
    i_sendUpload = mq_sendUpload.begin();
    if ( men_uploadType == UploadCommand )
    { // the first item in the queue is currently in upload process - so do NOT use this for replacement, as the next action
      // after receive of the awaited ACK is simple erase of the first command
      i_sendUpload++;
    }
    for (; (p_queue == NULL) && (i_sendUpload != mq_sendUpload.end()); i_sendUpload++)
    { //first check if multisendstreamer is used!
      /* four cases:
      1. both use buffer
      2. both use mssObjectString
      3. mss is queued and could be replaced by buffer
      4. buffer is queued and could be replaced by mssObjectString
        */
      if ((i_sendUpload->mssObjectString == NULL) && (ar_sendUpload.mssObjectString == NULL))
      {
        if (i_sendUpload->vec_uploadBuffer[0] == ar_sendUpload.vec_uploadBuffer[0])
        {
          uint8_t ui8_offset = (ar_sendUpload.vec_uploadBuffer[0]);
          if ( (ui8_offset<scui8_cmdCompareTableMin) || (ui8_offset > scui8_cmdCompareTableMax))
          { // only 0x12 is possible, but no need to override, it shouldn't occur anyway!
            if ((ui8_offset == 0x12) ||
                ((ui8_offset >= 0x60) && (ui8_offset <= 0x7F)) ) /// no checking for Proprietary commands (we don't need the replace-feature here!)
              break;

            // the rest is not possible by definition, but for being sure :-)
#if DEBUG_VTCOMM
            INTERNAL_DEBUG_DEVICE << "--INVALID COMMAND! SHOULDN'T HAPPEN!!--" << INTERNAL_DEBUG_DEVICE_ENDL;
            INTERNAL_DEBUG_DEVICE << "commandByte " << (int)ui8_offset << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
            return false;
          }
          // get bitmask for the corresponding command
          uint8_t ui8_bitmask = scpui8_cmdCompareTable [ui8_offset-scui8_cmdCompareTableMin];
          if (!(ui8_bitmask & (1<<0)))
          { // go Check for overwrite...
            for (i=1;i<=7;i++)
            {
              if (((ui8_bitmask & 1<<i) !=0) && !(i_sendUpload->vec_uploadBuffer[i] == ar_sendUpload.vec_uploadBuffer[i]))
                break;
            }
            if (!(i<=7))
            { // loop ran through, all to-compare-bytes matched!
              p_queue = &*i_sendUpload; // so overwrite this SendUpload_c with the new value one
            }
          }
        }
      }
      if ((i_sendUpload->mssObjectString != NULL) && (ar_sendUpload.mssObjectString != NULL))
      {
        if ((*i_sendUpload).mssObjectString->getStreamer()->getFirstByte() == ar_sendUpload.mssObjectString->getStreamer()->getFirstByte())
        {
          if ((*i_sendUpload).mssObjectString->getStreamer()->getID() == ar_sendUpload.mssObjectString->getStreamer()->getID())
            p_queue = &*i_sendUpload;
        }
      }
      if ((i_sendUpload->mssObjectString != NULL) && (ar_sendUpload.mssObjectString == NULL))
      {
        if ((*i_sendUpload).mssObjectString->getStreamer()->getFirstByte() == ar_sendUpload.vec_uploadBuffer[0])
        {
          if ((*i_sendUpload).mssObjectString->getStreamer()->getID() == (ar_sendUpload.vec_uploadBuffer[1] | (ar_sendUpload.vec_uploadBuffer[2]<<8)))
            p_queue = &*i_sendUpload;
        }
      }
      if ((i_sendUpload->mssObjectString == NULL) && (ar_sendUpload.mssObjectString != NULL))
      {
        if ((*i_sendUpload).vec_uploadBuffer[0] == ar_sendUpload.mssObjectString->getStreamer()->getFirstByte())
        {
          if (((*i_sendUpload).vec_uploadBuffer[1] | (*i_sendUpload).vec_uploadBuffer[2]<<8) == ar_sendUpload.mssObjectString->getStreamer()->getID())
            p_queue = &*i_sendUpload;
        }
      }
    } // for
  }
  if (p_queue == NULL)
  {
  /* The SendUpload_c constructor only takes a reference, so don't change the string in the meantime!!! */
#ifdef USE_LIST_FOR_FIFO
    // queueing with list: queue::push <-> list::push_back; queue::front<->list::front; queue::pop<->list::pop_front
    mq_sendUpload.push_back (ar_sendUpload);
#else
    mq_sendUpload.push (ar_sendUpload);
#endif
  }
  else
    *p_queue = ar_sendUpload; // overloaded "operator="
#if DEBUG_HEAP_USEAGE
  sui16_sendUploadQueueSize++;
  if (sui16_sendUploadQueueSize > sui16_maxSendUploadQueueSize)
    sui16_maxSendUploadQueueSize = sui16_sendUploadQueueSize;
#endif

#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << mq_sendUpload.size() << "." << INTERNAL_DEBUG_DEVICE_ENDL;
  //dumpQueue(); /* to see all enqueued cmds after every enqueued cmd */
#endif
  /** push(...) has no return value */
  return true;
}


void
VtClientServerCommunication_c::dumpQueue()
{
#ifdef USE_LIST_FOR_FIFO
#ifdef OPTIMIZE_HEAPSIZE_IN_FAVOR_OF_SPEED
  STL_NAMESPACE::list<SendUpload_c,MALLOC_TEMPLATE(SendUpload_c) >::iterator i_sendUpload;
#else
  STL_NAMESPACE::list<SendUpload_c>::iterator i_sendUpload;
#endif
#else
  STL_NAMESPACE::queue<SendUpload_c>::iterator i_sendUpload;
#endif

  for (i_sendUpload = mq_sendUpload.begin(); i_sendUpload != mq_sendUpload.end(); i_sendUpload++)
  {
    if (i_sendUpload->mssObjectString == NULL)
    {
      for (uint8_t i=0; i<=7; i++)
      {
#if DEBUG_VTCOMM
        INTERNAL_DEBUG_DEVICE << " " << (uint16_t)(i_sendUpload->vec_uploadBuffer[i]);
#endif
      }
    }
    else
    {
      MultiSendPkg_c msp;
      int i_strSize = i_sendUpload->mssObjectString->getStreamer()->getStreamSize();
      for (int i=0; i < i_strSize; i+=7) {
        i_sendUpload->mssObjectString->getStreamer()->setDataNextStreamPart (&msp, (unsigned char) ((i_strSize - i) > 7 ? 7 : (i_strSize-i)));
        for (uint8_t j=1; j<=7; j++)
        {
#if DEBUG_VTCOMM
          INTERNAL_DEBUG_DEVICE << " " << (uint16_t)(msp[j]);
#endif
        }
      }
    }
  }
#if DEBUG_VTCOMM
  INTERNAL_DEBUG_DEVICE << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
}


void
VtClientServerCommunication_c::doStart()
{
  /// First, trigger sending of WS-Announce
  mi32_timeWsAnnounceKey = mrc_wsMasterIdentItem.getIsoItem()->startWsAnnounce();

  mi32_nextWsMaintenanceMsg = 0; // send out ws maintenance message immediately after ws has been announced.

  men_objectPoolState = OPInitial; // try (re-)uploading, not caring if it was successfully or not on the last vt!
  men_uploadType = UploadPool;          // Start Pool Uploading sequence!!
  men_uploadPoolState = UploadPoolInit; // with "UploadInit
}


void
VtClientServerCommunication_c::doStop()
{
  // actually not needed to be reset here, because if VT not active it's not checked and if VT gets active we restart the sending.
  mi32_timeWsAnnounceKey = -1;

  // VT has left the system - clear all queues now, don't wait until next re-entering (for memory reasons)
#if DEBUG_HEAP_USEAGE
  sui16_sendUploadQueueSize -= mq_sendUpload.size();
#endif
#ifdef USE_LIST_FOR_FIFO
  // queueing with list: queue::push <-> list::push_back; queue::front<->list::front; queue::pop<->list::pop_front
  mq_sendUpload.clear();
#else
  while (!mq_sendUpload.empty()) mq_sendUpload.pop();
#endif
  /// if any upload is in progress, abort it
  if (mpc_vtServerInstance) // no Vt Alives anymore but Vt still announced
  {
    getMultiSendInstance4Comm().abortSend (mrc_wsMasterIdentItem.isoName(), mpc_vtServerInstance->getIsoName());
  }
  else
  {
    // vt's not announced
    // that case should be handles by the multisend itself
  }
  mrc_pool.eventEnterSafeState();

  // set display state of the client to a defined state
  men_displayState = VtClientDisplayStateHidden;
}


void
VtClientServerCommunication_c::checkAndHandleVtStateChange()
{
  const bool cb_fakeVtOff = ((mi32_fakeVtOffUntil >= 0) && (HAL::getTime() < mi32_fakeVtOffUntil));

  bool b_vtAliveOld = mb_vtAliveCurrent;
  mb_vtAliveCurrent = !cb_fakeVtOff && isVtActive();

  if (!b_vtAliveOld && mb_vtAliveCurrent)
  { /// OFF --> ON  ***  VT has (re-)entered the system
#if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=========================================================================="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=== VT has entered the system, trying to receive all Properties now... ==="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=== time: " << HAL::getTime() << " ==="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=========================================================================="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << INTERNAL_DEBUG_DEVICE_ENDL;

#endif
    doStart();
  }
  else if (b_vtAliveOld && !mb_vtAliveCurrent)
  { /// ON -> OFF  ***  Connection to VT lost
#if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=============================================================================="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=== VT has left the system, clearing queues --> eventEnterSafeState called ==="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << (cb_fakeVtOff ? "=== (as it was forced to)                                                  ===" : "")
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=== time: " << HAL::getTime() << " ==="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << "=============================================================================="
      << INTERNAL_DEBUG_DEVICE_NEWLINE << INTERNAL_DEBUG_DEVICE_ENDL;
#endif
    doStop();
  }
}


bool
VtClientServerCommunication_c::isVtActive()
{
  if (!mpc_vtServerInstance) return false;
  return mpc_vtServerInstance->isVtActive();
}


//! initialize the streamer. calculate streams size and (if needed) request Get Memory with the complete size.
void
VtClientServerCommunication_c::initObjectPoolUploadingPhases (uploadPoolType_t ren_uploadPoolType, IsoAgLib::iVtObject_c** rppc_listOfUserPoolUpdateObjects, uint16_t aui16_numOfUserPoolUpdateObjects)
{
  if (ren_uploadPoolType == UploadPoolTypeUserPoolUpdate)
  { // Activate User triggered Partial Pool Update
    // check params first
    if (aui16_numOfUserPoolUpdateObjects == 0)
      return;

    /// INIT FIRST
    ms_uploadPhaseUser.pc_streamer = &mc_iVtObjectStreamer;
    ms_uploadPhaseUser.ui32_size = 1; // the 0x11 command-byte is always there.
    mppc_uploadPhaseUserObjects = rppc_listOfUserPoolUpdateObjects;

    /// COUNT
    for (uint32_t curObject=0; curObject < aui16_numOfUserPoolUpdateObjects; ++curObject)
      ms_uploadPhaseUser.ui32_size += (static_cast<vtObject_c*>(mppc_uploadPhaseUserObjects[curObject]))->fitTerminal ();
  }
  else
  { // *CONDITIONALLY* Calculate GENERAL Parts sizes
    if (ren_uploadPoolType == UploadPoolTypeCompleteInitially)
    { // start with first phase
      men_uploadPhaseAutomatic = UploadPhaseFIRSTfix;

      /// Phase 0
      ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsFix].pc_streamer = &mc_iVtObjectStreamer;
      ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsFix].ui32_size = 1; // the 0x11 command-byte is always there.
      for (uint32_t curObject=0; curObject < mrc_pool.getNumObjects(); ++curObject)
        ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsFix].ui32_size += ((vtObject_c*)mrc_pool.getIVtObjects()[0][curObject])->fitTerminal ();

      /// Phase 2
      const std::pair<uint32_t, IsoAgLib::iMultiSendStreamer_c*> cpair_retval = mrc_pool.getAppSpecificFixPoolData();
      ms_uploadPhasesAutomatic [UploadPhaseAppSpecificFix].pc_streamer = cpair_retval.second;
      ms_uploadPhasesAutomatic [UploadPhaseAppSpecificFix].ui32_size = cpair_retval.first;
    }
    else
    { // start with second phase (lang. dep that is)
      men_uploadPhaseAutomatic = UploadPhaseFIRSTlang;
    }

    // *ALWAYS* Calculate LANGUAGE Part size (if objectpool has multilanguage!)
    /// Phase 1
    ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsLang].pc_streamer = &mc_iVtObjectStreamer;
    ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsLang].ui32_size = 0; // there may not always be a language part.
    if (mrc_pool.getWorkingSetObject().get_vtObjectWorkingSet_a().numberOfLanguagesToFollow > 0) // supporting multilanguage.
    {
      const int8_t ci8_realUploadingLanguage = ((mi8_objectPoolUploadingLanguage < 0) ? 0 : mi8_objectPoolUploadingLanguage) + 1;

      for (uint32_t curObject=0; curObject < mrc_pool.getNumObjectsLang(); ++curObject)
        ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsLang].ui32_size += ((vtObject_c*)mrc_pool.getIVtObjects()[ci8_realUploadingLanguage][curObject])->fitTerminal ();
      if (ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsLang].ui32_size > 0)
      { // only if there's at least one object being streamed up as user-partial-objectpool-update add the CMD byte for size calculation...
        ++ms_uploadPhasesAutomatic [UploadPhaseIVtObjectsLang].ui32_size; // add the 0x11 byte!
      }
    } // else: no LANGUAGE SPECIFIC objectpool, so keep this at 0 to indicate this!

    /// Phase 3
    const std::pair<uint32_t, IsoAgLib::iMultiSendStreamer_c*> cpair_retval = mrc_pool.getAppSpecificLangPoolData(mi8_objectPoolUploadingLanguage, mui16_objectPoolUploadingLanguageCode);
    ms_uploadPhasesAutomatic [UploadPhaseAppSpecificLang].pc_streamer = cpair_retval.second;
    ms_uploadPhasesAutomatic [UploadPhaseAppSpecificLang].ui32_size = cpair_retval.first;
  }
  men_uploadPoolType = ren_uploadPoolType;
}


void
VtClientServerCommunication_c::sendGetMemory()
{ // Issue GetMemory-Command (don't care if several 0x11s are counted from each partial object pool...)
  uint32_t ui32_size = 0;
  for (int i=0; i <= UploadPhaseLAST; ++i)
  {
    ui32_size += ms_uploadPhasesAutomatic[i].ui32_size;
  }

  mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8, mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                         192 /* 0xC0 */, 0xff,
                         (ui32_size) & 0xFF, (ui32_size >>  8) & 0xFF, (ui32_size >> 16) & 0xFF, ui32_size >> 24,
                         0xff, 0xff);
  getCanInstance4Comm() << mc_data;     // Command: Get Technical Data --- Parameter: Get Memory Size

  // Now proceed to uploading
  men_uploadPoolState = UploadPoolWaitingForMemoryResponse;
  mui32_uploadTimeout = DEF_TimeOut_GetMemory;
  mui32_uploadTimestamp = HAL::getTime();
}


/// finalizeUploading() is getting called after LoadVersion or UploadPool...
void
VtClientServerCommunication_c::finalizeUploading() //bool ab_wasLanguageUpdate)
{
  if (men_uploadPoolType == UploadPoolTypeUserPoolUpdate)
  { /// Was user-pool-update
    mrc_pool.eventPartialPoolUploadedSuccessfully();
  }
  else
  { /// Was complete initial pool or language pool update.
    /// in both cases we uploaded in one specific language!! so do the following:
    mi8_objectPoolUploadedLanguage = mi8_objectPoolUploadingLanguage;
    mui16_objectPoolUploadedLanguageCode = mui16_objectPoolUploadingLanguageCode;
    mi8_objectPoolUploadingLanguage = -2; // -2 indicated that the language-update while pool is up IS IDLE!
    mui16_objectPoolUploadingLanguageCode = 0x0000;
  #if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE << "===> finalizeUploading () with language: "<<(int)mi8_objectPoolUploadedLanguage;
    if (mi8_objectPoolUploadedLanguage >= 0) INTERNAL_DEBUG_DEVICE <<" ["<<uint8_t(mui16_objectPoolUploadedLanguageCode>>8) <<uint8_t(mui16_objectPoolUploadedLanguageCode&0xFF)<<"]";
    INTERNAL_DEBUG_DEVICE << INTERNAL_DEBUG_DEVICE_ENDL;
  #endif
    if (men_uploadPoolType == UploadPoolTypeLanguageUpdate)
    {
      // no need to set "men_objectPoolState" and "men_uploadType", this is done in "finishUploadCommand()"
    }
    else
    {
  #if DEBUG_VTCOMM
      INTERNAL_DEBUG_DEVICE << "Now men_objectPoolState = OPUploadedSuccessfully;" << INTERNAL_DEBUG_DEVICE_ENDL;
  #endif
      men_objectPoolState = OPUploadedSuccessfully;
      men_uploadType = UploadIdle;
    }
    mrc_pool.eventObjectPoolUploadedSuccessfully ((men_uploadPoolType == UploadPoolTypeLanguageUpdate), mi8_objectPoolUploadedLanguage, mui16_objectPoolUploadedLanguageCode);
  }
}


/** Send "End of Object Pool" message */
void
VtClientServerCommunication_c::indicateUploadCompletion()
{
  switch (men_uploadType)
  {
    case UploadIdle:
      isoaglib_assert (false); // shouldn't happen
      break;

    case UploadPool: // successfully sent complete initial pool, so now send out the "End of Object Pool Message" and wait for response!
      mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8, mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                             0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
      getCanInstance4Comm() << mc_data;     // Command: Object Pool Transfer --- Parameter: Object Pool Ready
      men_uploadPoolState = UploadPoolWaitingForEOOResponse; // and wait for response to set en_uploadState back to UploadIdle;
      mui32_uploadTimeout = DEF_TimeOut_EndOfObjectPool; // wait X seconds for terminal to initialize pool!
      mui32_uploadTimestamp = HAL::getTime();
      break;

    case UploadCommand: // user / language updates are being sent as "command"
      finishUploadCommand(true);
      break;
  }
}


// only being called if there IS a msq_sendUpload.front()
bool
VtClientServerCommunication_c::startUploadCommand()
{
  /** @todo SOON-258 Up to now, noone cares for the return code. implement error handling in case multisend couldn't be started? */
  // Set new state
  men_uploadType = UploadCommand;
  // along with UploadCommand ALWAYS set "men_sendSuccess", not only for Multipacket,
  // because we will generally check for timeout or not in the successfully-sent case.
  men_uploadCommandState = UploadCommandWithAwaitingResponse;

  // Get first element from queue
  SendUpload_c* actSend = &mq_sendUpload.front();

   /// Use Multi or Single CAN-Pkgs?
  //////////////////////////////////

  bool b_return = false;

  if ((actSend->mssObjectString == NULL) && (actSend->vec_uploadBuffer.size() < 9))
  { /// Fits into a single CAN-Pkg!

    if (actSend->vec_uploadBuffer[0] == 0x11)
    { /// Handle special case of LanguageUpdate / UserPoolUpdate
      men_uploadCommandState = UploadCommandPartialPoolUpdate; // There's NO response for command 0x11! And there may be multiple parts!
      if (actSend->ppc_vtObjects)
      { /// User triggered Partial Pool Update
        initObjectPoolUploadingPhases (UploadPoolTypeUserPoolUpdate, actSend->ppc_vtObjects, actSend->ui16_numObjects);
      }
      else
      { /// Language Pool Update
        initObjectPoolUploadingPhases (UploadPoolTypeLanguageUpdate);
      }
      startCurrentUploadPhase();
      b_return = true;
    }
    else
    { /// normal 8 byte package
      // Shouldn't be less than 8, else we're messin around with vec_uploadBuffer!
      mc_data.setExtCanPkg8 (7, 0, ECU_TO_VT_PGN>>8, mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(),
                            actSend->vec_uploadBuffer [0], actSend->vec_uploadBuffer [1],
                            actSend->vec_uploadBuffer [2], actSend->vec_uploadBuffer [3],
                            actSend->vec_uploadBuffer [4], actSend->vec_uploadBuffer [5],
                            actSend->vec_uploadBuffer [6], actSend->vec_uploadBuffer [7]);
      getCanInstance4Comm() << mc_data;
      // Save first byte for Response-Checking!
      mui8_commandParameter = actSend->vec_uploadBuffer [0];

      men_sendSuccess = __IsoAgLib::MultiSend_c::SendSuccess; // as it has been sent out right now.
      b_return = true;
    }
  }
  else if ((actSend->mssObjectString != NULL) && (actSend->mssObjectString->getStreamer()->getStreamSize() < 9))
  { /// Fits into a single CAN-Pkg!
    uint8_t ui8_len = actSend->mssObjectString->getStreamer()->getStreamSize();

    mc_data.setExtCanPkg (7, 0, ECU_TO_VT_PGN>>8, mpc_vtServerInstance->getVtSourceAddress(), mrc_wsMasterIdentItem.getIsoItem()->nr(), 8); // ECU->VT PGN is ALWAYS 8 Bytes!
    actSend->mssObjectString->getStreamer()->set5ByteCommandHeader (mc_data.getUint8DataPointer());
    int i=5;
    for (; i < ui8_len; i++) mc_data.setUint8Data( i, actSend->mssObjectString->getStreamer()->getStringToStream() [i-5] );
    for (; i < 8;       i++) mc_data.setUint8Data( i, 0xFF); // pad unused bytes with "0xFF", so CAN-Pkg is of size 8!
    getCanInstance4Comm() << mc_data;
    // Save first byte for Response-Checking!
    mui8_commandParameter = actSend->mssObjectString->getStreamer()->getFirstByte();

    men_sendSuccess = __IsoAgLib::MultiSend_c::SendSuccess; // as it has been sent out right now.
    b_return = true;
  }
  else if (actSend->mssObjectString == NULL)
  { /// Use multi CAN-Pkgs [(E)TP], doesn't fit into a single CAN-Pkg!
    // Save first byte for Response-Checking!
    mui8_commandParameter = actSend->vec_uploadBuffer [0]; // Save first byte for Response-Checking!

    b_return = getMultiSendInstance4Comm().sendIsoTarget (mrc_wsMasterIdentItem.isoName(),
      mpc_vtServerInstance->getIsoName(), &actSend->vec_uploadBuffer.front(),
      actSend->vec_uploadBuffer.size(), ECU_TO_VT_PGN, men_sendSuccess);
  }
  else
  {
    // Save first byte for Response-Checking!
    mui8_commandParameter = actSend->mssObjectString->getStreamer()->getFirstByte();

    b_return = getMultiSendInstance4Comm().sendIsoTarget (mrc_wsMasterIdentItem.isoName(),
      mpc_vtServerInstance->getIsoName(), (IsoAgLib::iMultiSendStreamer_c*)actSend->mssObjectString->getStreamer(),
      ECU_TO_VT_PGN, men_sendSuccess);
  }

  // Set time-out values
  mui32_uploadTimestamp = HAL::getTime();
  mui32_uploadTimeout =
    (men_uploadCommandState == UploadCommandPartialPoolUpdate)
      ? -1 /* There's no timeout (hence no valid "mui8_commandParameter"), so just reset the variable - but actually it wouldn't need to be set at all. */
      : ((mui8_commandParameter >= 0xD0) && (mui8_commandParameter <= 0xD2)) ? DEF_TimeOut_VersionLabel
                                          : (mui8_commandParameter == 0x12)  ? DEF_TimeOut_EndOfObjectPool
                                             /* default: sending Annex F. */ : DEF_TimeOut_NormalCommand;

  return b_return;
}


void
VtClientServerCommunication_c::finishUploadCommand(bool ab_TEMPORARYSOLUTION_fromTimeEvent)
{
  men_uploadType = UploadIdle;

  if ( !mq_sendUpload.empty() )
  {

    //dumpQueue(); /* to see all left queued cmds after every dequeued cmd */
    #if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE << "Dequeued (after success, timeout, whatever..): " << mq_sendUpload.size() <<" -> ";
    #endif

    #ifdef USE_LIST_FOR_FIFO
    // queueing with list: queue::push <-> list::push_back; queue::front<->list::front; queue::pop<->list::pop_front
    mq_sendUpload.pop_front();
    #else
    mq_sendUpload.pop();
    #endif
    #if DEBUG_HEAP_USEAGE
    sui16_sendUploadQueueSize--;
    #endif

    #if DEBUG_VTCOMM
    INTERNAL_DEBUG_DEVICE << mq_sendUpload.size() << "." << INTERNAL_DEBUG_DEVICE_ENDL;
    #endif

    // trigger fast reschedule if more messages are waiting
    if ( ( getUploadBufferSize() > 0 ) && ( getIsoTerminalInstance4Comm().getTimePeriod() != 4 ) )
    { // there is a command waiting
      if (ab_TEMPORARYSOLUTION_fromTimeEvent)
      { // from timeEvent we can just call
        // IsoTerminal's setTimePeriod() is protected, so we need a wrapper for it.
        // this is all just a bad hack and should really be TEMPORARY
        // Make a mechanismn where all vtCSCs are asked for the state and the
        // IsoTerminal calculates the appropriate period from
        // - timeEvent
        // - processMsg
        // This needs to be replaced by new scheduling architecture: every vtCSC should be an own SchedulerTask_c.
        getIsoTerminalInstance4Comm().TEMPORARYSOLUTION_setTimePeriod (4);
      }
      else
      { // from processMsg or elsewhere we call
        getSchedulerInstance().changeTimePeriodAndResortTask(&(getIsoTerminalInstance4Comm()), 4);
      }
    }
  }
#if DEBUG_VTCOMM
  else
  {
    INTERNAL_DEBUG_DEVICE << "Attempt to Dequeue while empty!" << INTERNAL_DEBUG_DEVICE_ENDL;
  }
#endif
}


void
VtClientServerCommunication_c::vtOutOfMemory()
{  // can't (up)load the pool.
  getILibErrInstance().registerError (iLibErr_c::RemoteServiceOutOfMemory, iLibErr_c::IsoTerminal);
  men_uploadPoolState = UploadPoolFailed; // no timeout needed
  men_objectPoolState = OPCannotBeUploaded;
}

/** set display state of vt client */
void
VtClientServerCommunication_c::setVtDisplayState (bool b_isVtStatusMsg, uint8_t ui8_saOrDisplayState)
{
  if (men_objectPoolState != OPUploadedSuccessfully) return;
  // as we don't properly seem to reset "men_objectPoolState" at doStop(), we'll for now add the extra
  // isAddress-Claimed-check here for safety:
  if (!getIdentItem().isClaimedAddress()) return;

  vtClientDisplayState_t newDisplayState;
  if (b_isVtStatusMsg) // state change triggered from VT Status Msg
  {
    if (ui8_saOrDisplayState == getIdentItem().getIsoItem()->nr())
      newDisplayState = VtClientDisplayStateActive;
    else
    {
      if (getVtDisplayState() == VtClientDisplayStateActive)
        // only cause state change if currently displayed is active
        newDisplayState = VtClientDisplayStateInactive;
      else
        newDisplayState = getVtDisplayState();
    }
  }
  else // state change triggered from Display Activation Msg
  {
    if (ui8_saOrDisplayState) // display client but no input focus
    {
      if (getVtDisplayState() == VtClientDisplayStateHidden)
        newDisplayState = VtClientDisplayStateInactive;
      else
        newDisplayState = getVtDisplayState(); // if already in state inactive or active, nothing to do
    }
    else // client is no longer displayed
    {
      if (getVtDisplayState() == VtClientDisplayStateInactive)
        newDisplayState = VtClientDisplayStateHidden;
      else
        newDisplayState = getVtDisplayState(); // if already in state hidden or active, nothing to do
    }
  }

  if (newDisplayState != getVtDisplayState())
  {
    men_displayState = newDisplayState;
    mrc_pool.eventDisplayActivation();
  }
}


void
VtClientServerCommunication_c::setObjectPoolUploadingLanguage()
{
  mi8_objectPoolUploadingLanguage = mi8_vtLanguage;
  mui16_objectPoolUploadingLanguageCode = 0x0000;
  if (mrc_pool.getWorkingSetObject().get_vtObjectWorkingSet_a().numberOfLanguagesToFollow > 0) // supporting multilanguage.
  { // only if the objectpool has 1 or more languages, it makes sense to add the language code to the version-name
    const int8_t ci8_realUploadingLanguage = (mi8_objectPoolUploadingLanguage < 0) ? 0 : mi8_objectPoolUploadingLanguage;
    const uint8_t* lang = mrc_pool.getWorkingSetObject().get_vtObjectWorkingSet_a().languagesToFollow[ci8_realUploadingLanguage].language;
    mui16_objectPoolUploadingLanguageCode = (lang [0] << 8) | lang[1];
  }
}

/** explicit conversion to reference of interface class type */
IsoAgLib::iVtClientServerCommunication_c& VtClientServerCommunication_c::toInterfaceReference()
{
  return static_cast<IsoAgLib::iVtClientServerCommunication_c&>(*this);
}
/** explicit conversion to reference of interface class type */
IsoAgLib::iVtClientServerCommunication_c* VtClientServerCommunication_c::toInterfacePointer()
{
  return static_cast<IsoAgLib::iVtClientServerCommunication_c*>(this);
}


uint16_t
VtClientServerCommunication_c::getVtObjectPoolSoftKeyWidth()
{
  return mrc_pool.getSkWidth();
}


uint16_t
VtClientServerCommunication_c::getVtObjectPoolDimension()
{
  return mrc_pool.getDimension();
}


uint16_t
VtClientServerCommunication_c::getVtObjectPoolSoftKeyHeight()
{
  return mrc_pool.getSkHeight();
}


} // end namespace __IsoAgLib
