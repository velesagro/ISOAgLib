/*
  proc_c.h: class with special (enum) types for ProcessData management

  (C) Copyright 2009 - 2013 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/
#ifndef IPROCDATA_H
#define IPROCDATA_H

#include <IsoAgLib/isoaglib_config.h>
#include <IsoAgLib/hal/hal_typedef.h>
#include <IsoAgLib/util/impl/bitfieldwrapper_c.h>


namespace IsoAgLib {

namespace ProcData {

  enum RemoteType_t {
    RemoteTypeTaskController,
    RemoteTypeDataLogger };

  enum MeasurementCommand_t { 
    MeasurementCommandTimeProp =          0x4,
    MeasurementCommandDistProp =          0x5,
    MeasurementCommandMinimumThreshold =  0x6,
    MeasurementCommandMaximumThreshold =  0x7,
    MeasurementCommandOnChange =          0x8 };

  enum NackResponse_t { 
    NackProcessDataCommandNotSupported =          0x01, // Bit 0 = 1 Process Data Command not supported
    NackInvalidElementNumber =                    0x02, // Bit 1 = 1 Invalid Element Number
    NackDDINoSupportedByElement =                 0x04, // Bit 2 = 1 DDI not supported by element
    NackTriggerMethodNotSupported =               0x08, // Bit 3 = 1 Trigger method not supported
    NackProcessDataNotSetable =                   0x10, // Bit 4 = 1 Process Data not setable
    NackInvalidOrUnsupportedIntervalOrThreshold = 0x20, // Bit 5 = 1 Invalid or unsupported interval or threshold
    NackReserved1 =                               0x40,
    NackReserved2 =                               0x80,
    NackUndefined };


  /** enum type for special DDIs */
  enum SpecialDDI_t {DefaultDataLoggingDDI = 0xDFFF};

  enum DeviceObjectType_t { // exact order as needed in array lookup
    ObjectTypeDVC,
    ObjectTypeDET,
    ObjectTypeDPD,
    ObjectTypeDPT,
    ObjectTypeDVP };

  enum DeviceElementType_t {
    ElementTypeDevice = 1,
    ElementTypeFunction,
    ElementTypeBin,
    ElementTypeSection,
    ElementTypeUnit,
    ElementTypeConnector,
    ElementTypeNavigationReference };

  enum TriggerMethod_t { 
    TimeInterval = 0,
    DistInterval = 1,
    ThresholdLimit = 2, 
    OnChange = 3,
    Total = 4 };

  struct TriggerMethod_s {
    typedef TriggerMethod_t enum_type;
    enum { number_of_bits = 5 };
  };

  typedef BitFieldWrapperRight_c<TriggerMethod_s> Methods_t;

  inline bool isMethodSet( uint8_t value, TriggerMethod_t method ) { return ((value & (0x1 << method)) != 0 ); }

  enum Property_t {
    Defaultset = 0,
    Settable = 1,
    ControlSource = 2 };

  struct Property_s {
    typedef Property_t enum_type;
    enum { number_of_bits = 3 };
  };

  typedef BitFieldWrapperRight_c<Property_s> Properties_t;
}

}

#endif
