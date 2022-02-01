/*
  vtobjectalarmmask_c.h

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
#ifndef VTOBJECTALARMMASK_C_H
#define VTOBJECTALARMMASK_C_H

#include <IsoAgLib/isoaglib_config.h>


#include "../ivtobjectmask_c.h"
#include "../ivtobjectsoftkeymask_c.h"
#include "vtclient_c.h"


namespace __IsoAgLib {

class vtObjectAlarmMask_c : public IsoAgLib::iVtObjectMask_c
{
public:
  int16_t stream(uint8_t* destMemory,
                 uint16_t maxBytes,
                 objRange_t sourceOffset);

  vtObjectAlarmMask_c(const iVtObjectAlarmMask_s* vtObjectAlarmMaskSROM , int ai_multitonInst);
  iVtObjectAlarmMask_s* get_vtObjectAlarmMask_a();
  vtObjectAlarmMask_c();
  uint32_t fitTerminal() const;
  void setBackgroundColour(uint8_t newValue,  bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setSoftKeyMask(IsoAgLib::iVtObjectSoftKeyMask_c* newSoftKeyMask, bool b_updateObject= false, bool b_enableReplaceOfCmd=false);
  void setPriority(uint8_t newValue,  bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setAcousticSignal(uint8_t newValue,  bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  bool moveChildLocation(IsoAgLib::iVtObject_c* apc_childObject, int8_t dx, int8_t dy, bool b_updateObject=false);
  bool setChildPosition(IsoAgLib::iVtObject_c* apc_childObject, int16_t dx, int16_t dy, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);

#ifdef USE_ISO_TERMINAL_GETATTRIBUTES
  /** that attribute is in parentheses in the spec, so commented out here
  uint8_t updateObjectType() const { return 2; }
  */

  uint8_t updateBackgroundColour(bool b_SendRequest=false);
  uint16_t updateSoftKeyMask(bool b_SendRequest=false);
  uint8_t updatePriority(bool b_SendRequest=false);
  uint8_t updateAcousticSignal(bool b_SendRequest=false);
  void saveReceivedAttribute (uint8_t attrID, uint8_t* pui8_attributeValue);
#endif
};

} // __IsoAgLib

#endif

