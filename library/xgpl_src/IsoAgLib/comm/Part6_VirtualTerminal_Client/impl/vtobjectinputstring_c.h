/*
  vtobjectinputstring_c.h

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
#ifndef VTOBJECTINPUTSTRING_C_H
#define VTOBJECTINPUTSTRING_C_H

#include <IsoAgLib/isoaglib_config.h>
#include "../ivtobjectstring_c.h"


#ifdef CONFIG_USE_VTOBJECT_inputstring

#include "../ivtobjectfontattributes_c.h"
#include "../ivtobjectinputattributes_c.h"
#include "../ivtobjectstringvariable_c.h"

namespace __IsoAgLib {

class vtObjectInputString_c : public IsoAgLib::iVtObjectString_c
{
private:
	enum AttributeID:uint8_t;
	// Internal implementation class
	struct iVtObjectInputString_s;

	// Pointer to the internal implementation
	std::unique_ptr<iVtObjectInputString_s> vtObject_a;


	vtObjectInputString_c(iVtObjectInputString_s* vtObjectInputStringSROM , multiton ai_multitonInst);

public:


  vtObjectInputString_c() = delete;
  ~vtObjectInputString_c();

  explicit vtObjectInputString_c(
		multiton ai_multitonInst,
		IsoAgLib::ObjectID ID,
  	    uint16_t width,
  	    uint16_t height,
		IsoAgLib::Colour backgroundColour,
		IsoAgLib::iVtObjectFontAttributes_c* fontAttributes,
		IsoAgLib::iVtObjectInputAttributes_c* inputAttributes,
		IsoAgLib::iVtObjectStringOptions options,
		IsoAgLib::iVtObjectStringVariable_c* variableReference,
		IsoAgLib::Justification justification,
  	    char* value, /* size length+1 (0-termination intern!) */
		uint8_t enabled
  		);


  uint16_t stream(uint8_t* destMemory,
                 uint16_t maxBytes,
                 objRange_t sourceOffset);
  IsoAgLib::ObjectID getID() const;


  uint32_t fitTerminal() const;
  const char* getString();
  void setValueRef(char* newValue, bool b_updateObject= false, bool b_enableReplaceOfCmd=false);
  void setValueCopy(char* newValue, bool b_updateObject= false, bool b_enableReplaceOfCmd=false);

#ifdef USE_VT_UNICODE_SUPPORT
  void setValueCopyUTF8(const char* newValue, bool b_updateObject= false, bool b_enableReplaceOfCmd=false);
  void setValueCopyUTF16(const char* newValue, uint16_t length, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
#endif

  void setSize(uint16_t newWidth, uint16_t newHeight, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setWidth(uint16_t newValue, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setHeight(uint16_t newValue, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setBackgroundColour(IsoAgLib::Colour newValue, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setFontAttributes(IsoAgLib::iVtObjectFontAttributes_c* newFontAttributes, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setInputAttributes(IsoAgLib::iVtObjectInputAttributes_c* newInputAttributes, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setOptions(IsoAgLib::iVtObjectStringOptions newOptions, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setVariableReference(IsoAgLib::iVtObjectStringVariable_c* newVariableRef, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);
  void setJustification(IsoAgLib::Justification newJustification, bool b_updateObject=false, bool b_enableReplaceOfCmd=false);

#ifdef CONFIG_USE_ISO_TERMINAL_GETATTRIBUTES
  // ///////////////////////// getter for attributes
  /** that attribute is in parentheses in the spec, so commented out here
  uint8_t updateObjectType() const { return 8; }
  */

  uint16_t updateWidth(bool b_SendRequest=false);
  uint16_t updateHeight(bool b_SendRequest=false);
  IsoAgLib::Colour updateBackgroundColour(bool b_SendRequest=false);
  IsoAgLib::iVtObjectFontAttributes_c* updateFontAttributes(bool b_SendRequest=false);
  IsoAgLib::iVtObjectInputAttributes_c* updateInputAttributes(bool b_SendRequest=false);
  IsoAgLib::iVtObjectStringOptions updateOptions(bool b_SendRequest=false);
  IsoAgLib::iVtObjectStringVariable_c* updateVariableReference(bool b_SendRequest=false);
  IsoAgLib::Justification updateJustification(bool b_SendRequest=false);

  /** that attribute is in parentheses in the spec, so commented out here
  uint8_t updateEnabled(bool b_SendRequest=false);
  */

  void saveReceivedAttribute (uint8_t attrID, uint8_t* pui8_attributeValue);
#endif
private:
  void updateEnable(uint8_t aui8_enOrDis);
};

} // __IsoAgLib

#endif //CONFIG_USE_VTOBJECT_inputstring

#endif //VTOBJECTINPUTSTRING_C_H
