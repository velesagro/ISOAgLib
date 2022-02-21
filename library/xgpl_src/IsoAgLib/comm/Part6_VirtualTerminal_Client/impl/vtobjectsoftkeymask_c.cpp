/*
  vtobjectsoftkeymask_c.cpp

  (C) Copyright 2009 - 2019 by OSB AG

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Usage under Commercial License:
  Licensees with a valid commercial license may use this file
  according to their commercial license agreement. (To obtain a
  commercial license contact OSB AG via <http://isoaglib.com/en/contact>)

  Usage under GNU General Public License with exceptions for ISOAgLib:
  Alternatively (if not holding a valid commercial license)
  use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/

#include "vtobjectsoftkeymask_c.h"
#ifdef CONFIG_USE_VTOBJECT_softkeymask
#include "../ivtobjectmacro_c.h"

#include "vtclient_c.h"

#ifdef ENABLE_SKM_HANDLER
#  ifdef DEFAULT_SKM_HANDLER_HEADER
#    include DEFAULT_SKM_HANDLER_HEADER
#  endif // DEFAULT_SKM_HANDLER_HEADER
#endif // ENABLE_SKM_HANDLER

namespace __IsoAgLib {



enum vtObjectSoftKeyMask_c::AttributeID:uint8_t
{
	BackgroundColour = 1,
};


struct vtObjectSoftKeyMask_c::iVtObjectSoftKeyMask_s: iVtObjectObject_s, iVtObjectwMacro_s {
	IsoAgLib::Colour backgroundColour;
	iVtObjectSoftKeyMask_s(
			IsoAgLib::ObjectID ID,
			IsoAgLib::Colour backgroundColour)
    : iVtObject_s(ID)
	, iVtObjectObject_s(ID)
	, iVtObjectwMacro_s(ID)
	, backgroundColour(backgroundColour)
	{
	}


};

uint16_t
vtObjectSoftKeyMask_c::stream(uint8_t* destMemory,
                              uint16_t maxBytes,
                              objRange_t sourceOffset)
{
  uint16_t curBytes=0; /* current bytes written */;

  const uint8_t numberOfObjectsToFollow = get_numberOfObjectsToFollow();

  if (getVtClientInstance4Comm().getClientByID (s_properties.clientId)
      .getVtServerInst().getVtCapabilities().skVirtual < numberOfObjectsToFollow)
  { // can't upload this SKM because it has more Keys than virtually supported
    return 0;
  }

  if (sourceOffset == 0) { // dump out constant sized stuff
    destMemory [0] = vtObject_a->ID & 0xFF;
    destMemory [1] = vtObject_a->ID >> 8;
    destMemory [2] = VT_OBJECT_TYPE_SOFT_KEY_MASK; // Object Type = Soft Key Mask
    destMemory [3] = getVtClientInstance4Comm().getClientByID (s_properties.clientId).getUserConvertedColor (vtObject_a->backgroundColour, this, IsoAgLib::BackgroundColour);
    destMemory [4] = numberOfObjectsToFollow;
    destMemory [5] = vtObject_a->numberOfMacrosToFollow;

    sourceOffset += 6;
    curBytes += 6;
  }

  if( numberOfObjectsToFollow )
  {
#ifdef ENABLE_SKM_HANDLER
    if( m_SkmHandler )
    {
      const objRange_t bytesBefore = 6;

      uint16_t nrObject = (sourceOffset - (bytesBefore)) / 2;
      while( (sourceOffset >= (bytesBefore)) && (sourceOffset < ((bytesBefore)+2U * numberOfObjectsToFollow)) && ((curBytes + 2) <= maxBytes) )
      {
        const iVtObject_c* vtObject = m_SkmHandler->objectsToFollow( nrObject );
        if( vtObject )
        {
          /* write out an object */
          if( vtObject != nullptr )
          {
              destMemory[curBytes] = vtObject->getID() & 0xFF;
              destMemory[curBytes + 1] = vtObject->getID() >> 8;
          }
          else
          {
              destMemory[curBytes] = 0xFF;
              destMemory[curBytes + 1] = 0xFF;
          }
          curBytes += 2;
        }
        sourceOffset += 2;
        nrObject++;
      }
    }
    else
#endif // ENABLE_SKM_HANDLER
    {
       MACRO_streamObject( 6 );
    }
  }

  MACRO_streamEventMacro(6+vtObject_a->numberOfObjectsToFollow*2U);
  return curBytes;
}


IsoAgLib::ObjectID vtObjectSoftKeyMask_c::getID() const {
	isoaglib_assert(vtObject_a);
	return vtObject_a->ID;
}

uint8_t vtObjectSoftKeyMask_c::get_numberOfObjectsToFollow() const
{
#ifdef ENABLE_SKM_HANDLER
  if( m_SkmHandler )
  {
    return m_SkmHandler->numberOfObjectsToFollow();
  }
  else
#endif // ENABLE_SKM_HANDLER
  {
    return vtObject_a->numberOfObjectsToFollow;
  }
}



vtObjectSoftKeyMask_c::vtObjectSoftKeyMask_c(
        iVtObjectSoftKeyMask_s *vtObjectSoftKeyMaskSROM, multiton ai_multitonInst)
        :vtObject_c(ai_multitonInst)
		,vtObject_a(vtObjectSoftKeyMaskSROM)
#ifdef ENABLE_SKM_HANDLER
  , m_SkmHandler( nullptr )
  , b_autoGenerated_SkmHandler( false )
#endif // ENABLE_SKM_HANDLER
{
}





vtObjectSoftKeyMask_c::~vtObjectSoftKeyMask_c()
{
#ifdef ENABLE_SKM_HANDLER
  if( b_autoGenerated_SkmHandler )
  {
    // Assume that the destructor below will call back unRegisterSkmHandler which will reset the m_SkmHandler
    delete m_SkmHandler;
    b_autoGenerated_SkmHandler = false;
  }

  isoaglib_assert( m_SkmHandler == nullptr );
#endif // ENABLE_SKM_HANDLER
}

#ifdef ENABLE_SKM_HANDLER
void vtObjectSoftKeyMask_c::registerSkmHandler_c( iSkmHandler_c* SkmHandler )
{
  isoaglib_assert( m_SkmHandler == nullptr );
  m_SkmHandler = SkmHandler;
}

void vtObjectSoftKeyMask_c::unRegisterSkmHandler_c( iSkmHandler_c* SkmHandler )
{
  isoaglib_assert( m_SkmHandler == SkmHandler );
  m_SkmHandler = nullptr;
}
#endif // ENABLE_SKM_HANDLER


uint32_t
vtObjectSoftKeyMask_c::fitTerminal() const
{
  const uint8_t skVirtual = getVtClientInstance4Comm().getClientByID( s_properties.clientId ).getVtServerInst().getVtCapabilities().skVirtual;

#ifdef ENABLE_SKM_HANDLER

#  ifdef DEFAULT_SKM_HANDLER_C
  // If this skm doesn't have a handler, but needs one, then auto-create it
  if (!m_SkmHandler)
  {
    if( skVirtual < vtObject_a->numberOfObjectsToFollow )
    {
      // Assume that the constructor below will callback registerSkmHandler which will set the m_SkmHandler
      new DEFAULT_SKM_HANDLER_C( getVtClientInstance4Comm().getClientByID( s_properties.clientId ).getPool(), *(IsoAgLib::iVtObjectSoftKeyMask_c*)this );
      b_autoGenerated_SkmHandler = true;
    }
  }
#  endif // DEFAULT_SKM_HANDLER_C

  if( m_SkmHandler )
  {
    m_SkmHandler->fitTerminal();
  }
#endif // ENABLE_SKM_HANDLER

  const uint8_t numberOfObjectsToFollow = get_numberOfObjectsToFollow();

  if ( skVirtual < numberOfObjectsToFollow )
  { // can't upload this SKM because it has more Keys than virtually supported
    return 0;
  }
  else
  { // okay, enough virtual SKs supported, so upload this SKM
    return 6 + (numberOfObjectsToFollow*2) + (vtObject_a->numberOfMacrosToFollow*2);
  }
}


void
vtObjectSoftKeyMask_c::setOriginSKM(bool /*b_SKM*/)
{
  for (int i=0; i<vtObject_a->numberOfObjectsToFollow; i++) {
    vtObject_a->objectsToFollow[i].vtObject->setOriginSKM (true);
  }
}



#ifdef CONFIG_USE_ISO_TERMINAL_GETATTRIBUTES
IsoAgLib::Colour
vtObjectSoftKeyMask_c::updateBackgroundColour(bool b_SendRequest)
{
  if (b_SendRequest)
    getAttribute(BackgroundColour);
  return vtObject_a->backgroundColour;
}

void
vtObjectSoftKeyMask_c::saveReceivedAttribute (uint8_t attrID, uint8_t* pui8_attributeValue)
{
  switch (attrID)
  {
    case BackgroundColour: vtObject_a->backgroundColour = convertLittleEndianStringColour(pui8_attributeValue); break;
    default: break;
  }
}

    void vtObjectSoftKeyMask_c::setBackgroundColour(IsoAgLib::Colour newValue, bool b_updateObject, bool b_enableReplaceOfCmd) {
    	if (b_updateObject)
    	    		vtObject_a->backgroundColour = newValue;
    	setAttribute ( BackgroundColour, getVtClientInstance4Comm().getClientByID (s_properties.clientId).getUserConvertedColor (newValue, this, IsoAgLib::BackgroundColour), b_enableReplaceOfCmd);
    }

    vtObjectSoftKeyMask_c::vtObjectSoftKeyMask_c(
            multiton ai_multitonInst,
            IsoAgLib::ObjectID ID,
            IsoAgLib::Colour backgroundColour)
	:vtObjectSoftKeyMask_c(
			new iVtObjectSoftKeyMask_s(
					ID,
					backgroundColour),
			ai_multitonInst)
	{
	}

#endif

} // __IsoAgLib

#endif //CONFIG_USE_VTOBJECT_softkeymask
