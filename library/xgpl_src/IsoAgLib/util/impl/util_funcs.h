/*
  util_funcs.h
    utils functions for checking time distances smaller than 1min.
    [msec.]

  (C) Copyright 2009 - 2010 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/

#ifndef UTIL_FUNCS_H
#define UTIL_FUNCS_H

#include <IsoAgLib/hal/hal_typedef.h>
#ifdef USE_DATASTREAMS_IO
class StreamInput_c;
#endif
#include <cstdlib>	// Include before vector or else CNAMESPACE stuff is screwed up for Tasking
#include <cstring>
#include <vector>
#ifdef USE_VT_UNICODE_SUPPORT
#include <string>
#endif

// Begin Namespace __IsoAgLib
namespace __IsoAgLib
{
  template <class T> inline T abs(const T& val)
  {
    return (val < 0) ? (-val) : val;
  }

  uint8_t bcd2dec(uint8_t ab_bcd);
  uint8_t dec2bcd(uint8_t ab_dec);
/**
  calculate res = (mul_1 / div_1) * (mul_2 / div_2) with 4 uint8_t
  integers without overflow problems caused by (mul_1 * mul_2)
  and without rounding errors by (mul_1 / div_1) * (mul_2 / div_2)
  @param ai32_mul_1 first factor for multiplication
  @param ai32_div_1 first divisor for first factor
  @param ai32_mul_2 second factor for multiplication
  @param ai32_div_2 second divisor for first factor
  @return result as int32_t value
*/
int32_t mul1Div1Mul2Div2(int32_t ai32_mul_1, int32_t ai32_div_1, int32_t ai32_mul_2, int32_t ai32_div_2);

/**
  copy 4 uint8_t data string into pointer to float value;
  use e.g. to access uint8_t memory representation of
  int32_t value as float value;
  often needed for CanPkg_c formating
  @param pvFrom source data string
  @param pf_to target float value
*/
void littleEndianStream2FloatVar(const void *const pvFrom, float *const pf_to);
/**
  copy float value to 4 uint8_t data string into pointer;
  use e.g. to access uint8_t memory representation of
  float value as int32_t value;
  often needed for CanPkg_c formating
  @param pf_from source float value
  @param pvTo target data string
*/
void floatVar2LittleEndianStream(const float *const pf_from, void *const pvTo);

/** calculate the total allocated HEAP for:
  - slist<T> with given size of T
  - add the overhead per node for slist<T> ( pointer to next item )
  - add the overhead for malloc_alloc Allocator which calls malloc for each single node ( HEAP block pointer )
  - add the overhead for alignment based on sizeof(int)
  @param aui16_sizeT sizeof(T) -> size of the stored class
  @param aui16_cnt amount of items ( default: 1 )
  @return amount of corresponding byte in heap
*/
uint16_t sizeSlistTWithMalloc( uint16_t aui16_sizeT, uint16_t aui16_cnt = 1 );
/** calculate the total allocated HEAP for:
  - list<T> with given size of T
  - add the overhead per node for list<T> ( TWO pointer to next and prev item )
  - add the overhead for malloc_alloc Allocator which calls malloc for each single node ( HEAP block pointer )
  - add the overhead for alignment based on sizeof(int)
  @param aui16_sizeT sizeof(T) -> size of the stored class
  @param aui16_cnt amount of items ( default: 1 )
  @return amount of corresponding byte in heap
*/
uint16_t sizeListTWithMalloc( uint16_t aui16_sizeT, uint16_t aui16_cnt = 1 );
/** calculate the total allocated HEAP for:
  - vector<T> with given size of T
  - add the overhead for malloc_alloc Allocator which calls malloc for each vector instance ( HEAP block pointer )
  - add the overhead for alignment based on sizeof(int)
  @param aui16_sizeT sizeof(T) -> size of the stored class
  @param aui16_capacity reserved space for vector<T> ( >= amount of currently stored items )
  @return amount of corresponding byte in heap
*/
uint16_t sizeVectorTWithMalloc( uint16_t aui16_sizeT, uint16_t aui16_capacity );
/** calculate the total allocated HEAP for:
  - slist<T> with given size of T
  - add the overhead per node for slist<T> ( pointer to next item )
  - add the overhead caused by allocation large chunks of each 40 items
  - add overhead for linking the HEAP block by the lowloevel malloc
  - add the overhead for alignment based on sizeof(int)
  @param aui16_sizeT sizeof(T) -> size of the stored class
  @param aui16_cnt amount of items ( default: 1 )
  @return amount of corresponding byte in heap
*/
uint16_t sizeSlistTWithChunk( uint16_t aui16_sizeT, uint16_t aui16_cnt = 1 );
/** calculate the total allocated HEAP for:
  - list<T> with given size of T
  - add the overhead per node for slist<T> ( pointers to next+prev item )
  - add the overhead caused by allocation large chunks of each 40 items
  - add overhead for linking the HEAP block by the lowloevel malloc
  - add the overhead for alignment based on sizeof(int)
  @param aui16_sizeT sizeof(T) -> size of the stored class
  @param aui16_cnt amount of items ( default: 1 )
  @return amount of corresponding byte in heap
*/
uint16_t sizeListTWithChunk( uint16_t aui16_sizeT, uint16_t aui16_cnt = 1 );
/** calculate the total allocated HEAP for:
  - vector<T> with given size of T
  - add the overhead for malloc_alloc Allocator which calls malloc for each vector instance ( HEAP block pointer )
  - add the overhead for alignment based on sizeof(int)
  @param aui16_sizeT sizeof(T) -> size of the stored class
  @param aui16_capacity reserved space for vector<T> ( >= amount of currently stored items )
  @return amount of corresponding byte in heap
*/
uint16_t sizeVectorTWithChunk( uint16_t aui16_sizeT, uint16_t aui16_capacity );


/** convert little endian byte string into an unsigned variable */
template<class T> void convertLittleEndianString( const uint8_t* apui8_src, T& r_result )
{
  #if defined(NO_8BIT_CHAR_TYPE)
  for ( int ind = sizeof(T)*2-1; ind >= 0; ind-- )
  {
    r_result |= (T(apui8_src[ind] & 0xFF) << (8*ind));
  }
  #elif defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN)
  CNAMESPACE::memcpy( &r_result, apui8_src, sizeof(T) );
  #else
  for ( int ind = sizeof(T)-1; ind >= 0; ind-- )
  {
    r_result |= (T(apui8_src[ind]) << (8*ind));
  }

  #endif
}

/** convert receive multistream into an unsigned variable */
uint8_t convertLittleEndianStringUi8( const uint8_t* apui8_src );
/** convert receive multistream into an unsigned variable */
inline int8_t convertLittleEndianStringI8( const uint8_t* apui8_src ){ return (int8_t)(apui8_src[0]);};
/** convert receive multistream into an unsigned variable */
uint16_t convertLittleEndianStringUi16( const uint8_t* apui8_src );
/** convert receive multistream into an unsigned variable */
int16_t convertLittleEndianStringI16( const uint8_t* apui8_src );
/** convert receive multistream into an unsigned variable */
uint32_t convertLittleEndianStringUi32( const uint8_t* apui8_src );
/** convert receive multistream into an unsigned variable */
int32_t convertLittleEndianStringI32( const uint8_t* apui8_src );
/** convert receive multistream into a float variable */
float convertLittleEndianStringFloat( const uint8_t* apui8_src );

/** convert number reference variable to little endian byte string */
template<class T> void numberRef2LittleEndianString( const T& acrc_src, uint8_t* pui8_target )
{
  #if defined(NO_8BIT_CHAR_TYPE)
  const unsigned int BitSize = sizeof(T) * 16;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
   *pui8_target = ((acrc_src >> ind) & 0xFF);
    pui8_target++;
  }
  #elif defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN)
  CNAMESPACE::memcpy( pui8_target, &acrc_src, sizeof(T) );
  #else
  const unsigned int BitSize = sizeof(T) * 8;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    *pui8_target = ((acrc_src >> ind) & 0xFF);
    pui8_target++;
  }
  #endif
}
/** convert number call-by-val variable to little endian byte string */
template<class T> void number2LittleEndianString( const T at_src, uint8_t* pui8_target )
{
  #if defined(NO_8BIT_CHAR_TYPE)
  const unsigned int BitSize = sizeof(T) * 16;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
   *pui8_target = ((at_src >> ind) & 0xFF);
    pui8_target++;
  }
  #elif defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN)
  CNAMESPACE::memcpy( pui8_target, &at_src, sizeof(T) );
  #else
  const unsigned int BitSize = sizeof(T) * 8;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    *pui8_target = ((at_src >> ind) & 0xFF);
    pui8_target++;
  }
  #endif
}


/** convert number reference variable to little endian byte string
 * @return iterator so that write can be continued directly after last written position
*/
template<class T> void numberRef2LittleEndianString( const T& acrc_src, STL_NAMESPACE::vector<uint8_t>& acrc_target )
{
#if defined(NO_8BIT_CHAR_TYPE)
  const unsigned int BitSize = sizeof(T) * 16;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    acrc_target.push_back((acrc_src >> ind) & 0xFF);
  }
#elif defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN)
  const uint8_t* pui8_src = &acrc_src;
  const unsigned int size = sizeof(T);
  for ( unsigned int ind = 0; ind < size; ind++ )
  {
    acrc_target.push_back( pui8_src[ind] );
  }
#else
  const unsigned int BitSize = sizeof(T) * 8;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    acrc_target.push_back((acrc_src >> ind) & 0xFF);
  }
#endif
}
/** convert number call-by-val variable to little endian byte string
 * @return iterator so that write can be continued directly after last written position
*/
template<class T> void number2LittleEndianString( const T at_src, STL_NAMESPACE::vector<uint8_t>& rc_target )
{
#if defined(NO_8BIT_CHAR_TYPE)
  const unsigned int BitSize = sizeof(T) * 16;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    rc_target.push_back((at_src >> ind) & 0xFF);
  }
#elif defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN)
  const uint8_t* pui8_src = (const uint8_t*)(&at_src);
  const unsigned int size = sizeof(T);
  for ( unsigned int ind = 0; ind < size; ind++ )
  {
    rc_target.push_back( pui8_src[ind] );
  }
#else
  const unsigned int BitSize = sizeof(T) * 8;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    rc_target.push_back((at_src >> ind) & 0xFF);
  }
#endif
}

/** convert number call-by-val variable to little endian byte string
 * @return iterator so that write can be continued directly after last written position
*/
template<class T> void number2LittleEndianString( const T at_src, STL_NAMESPACE::vector<uint8_t>& rc_target, uint16_t aui16_bytePos)
{
#if defined(NO_8BIT_CHAR_TYPE)
  const unsigned int BitSize = sizeof(T) * 16;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    rc_target[ aui16_bytePos ] = (at_src >> ind) & 0xFF;
    aui16_bytePos++;
  }
#elif defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN)
  const uint8_t* pui8_src = (const uint8_t*)(&at_src);
  const unsigned int size = sizeof(T);
  for ( unsigned int ind = 0; ind < size; ind++ )
  {
    rc_target[ aui16_bytePos ]= pui8_src[ind];
    aui16_bytePos++;
  }
#else
  const unsigned int BitSize = sizeof(T) * 8;
  for ( unsigned int ind = 0; ( ind < BitSize ); ind += 8 )
  {
    rc_target[ aui16_bytePos ] = (at_src >> ind) & 0xFF;
    aui16_bytePos++;
  }
#endif
}

/** convert big endian textual number representation into little endian uint8_t string of specified size */
void bigEndianHexNumberText2CanString( const char* ac_src, uint8_t* pui8_target, unsigned int size );

/** convert big endian textual unsigned int 8Bit number representation into little endian uint8_t string */
void bigEndianHexNumberText2CanStringUint8( const char* ac_src, uint8_t* pui8_target );
/** convert big endian textual unsigned int 16Bit number representation into little endian uint8_t string of specified size */
void bigEndianHexNumberText2CanStringUint16( const char* ac_src, uint8_t* pui8_target );
/** convert big endian textual unsigned int 32Bit number representation into little endian uint8_t string of specified size */
void bigEndianHexNumberText2CanStringUint32( const char* ac_src, uint8_t* pui8_target );
#if 0
Currently disabled - see implementation for more information
/** convert big endian textual unsigned int 64Bit number representation into little endian uint8_t string of specified size */
void bigEndianHexNumberText2CanStringUint64( const char* ac_src, uint8_t* pui8_target );
#endif
/** convert big endian textual unsigned int up to 16Bit number representation into little endian uint8_t string of specified size */
void bigEndianDecNumberText2CanStringUint( const char* ac_src, uint8_t* pui8_target );


class Flexible4ByteString_c;
class Flexible8ByteString_c;

/** helper class with some inline functions to enable flexible, efficient
    and safe access to 4-Byte strings for the 4-Byte part of a process data package as:
    + uint8_t  array of size 4 -> name uint8
    + uint16_t array of size 2 -> name uint16
    + uint32_t array of size 1 -> name uint32 (( only for CPU with 16Bit or more)
  IMPORTANT: The internal order of bytes is LITTLE ENDIAN, so that the bytes are directly
             used for sending of CAN messages.
   ==>> for big endian CPU all assignment and retrieval functions have to use the helper
        like numberRef2LittleEndianString for safe data conversion / adaptation
  */
class Flexible4ByteString_c {

 public:
  Flexible4ByteString_c( const Flexible4ByteString_c& acrc_src )
  #ifndef NO_8BIT_CHAR_TYPE
  {uint32[0] = acrc_src.uint32[0];};
  #else
  {for (uint8_t index = 0; index < 4; index++) uint8[index] = acrc_src.uint8[index];};
  #endif

  /**
    simply set a Flexible4ByteString_c at a specific value index with.
    Important: Flexible4ByteString_c and Flexible8ByteString_c have
               the SAME byte order as a CAN message byte string.
    ==>> for BIG ENDIAN systems, this value can NOT directly be used as intger value
    @param aui8_ind position of set Flexible4ByteString_c [0..1]
  */
  void setFlexible4DataValueInd(uint8_t aui8_ind, const Flexible8ByteString_c& ac_value );

  /** constructor for INIT directly from a flexible positioned Stream.
      IMPORTANT: this works also when the string starts at ODD position!
  */
  Flexible4ByteString_c( const uint8_t* apui8_srcStream = NULL );
  /** set this object from a optionally odd addressed string */
  void setDataFromString( const uint8_t* apui8_srcStream, uint8_t aui8_len = 4 )
  { if (apui8_srcStream != NULL ) CNAMESPACE::memcpy(uint8, apui8_srcStream, aui8_len );};

  /** copy contents of this object to a optionally odd addressed string */
  void getDataToString( uint8_t* pui8_targetStream, uint8_t aui8_len = 4 ) const
  { if (pui8_targetStream != NULL ) CNAMESPACE::memcpy( pui8_targetStream, uint8, aui8_len );};

  /** set this object from a optionally odd addressed string */
  void setDataFromString( uint8_t aui8_offset, const uint8_t* apui8_srcStream, uint8_t aui8_len = 4 );
  /** copy contents of this object to a optionally odd addressed string */
  void getDataToString( uint8_t aui8_offset, uint8_t* pui8_targetStream, uint8_t aui8_len = 4 ) const;


  /** assignment */
  const Flexible4ByteString_c& operator=( const Flexible4ByteString_c& acrc_src )
  {
  #ifndef NO_8BIT_CHAR_TYPE  
    uint32[0] = acrc_src.uint32[0]; 
  #else
    for (uint8_t index = 0; index < 4; index++)
      uint8[index] = acrc_src.uint8[index];
  #endif
    return *this; 
  };
  /** compare for EQUAL */
  bool operator==( const Flexible4ByteString_c& acrc_cmp ) const
  { // use inline implementation for this case
  #ifndef NO_8BIT_CHAR_TYPE
    return ( uint32[0] == acrc_cmp.uint32[0] );
  #else
    for (uint8_t index = 0; index < 4; index++)
      if ( uint8[index] != acrc_cmp.uint8[index] )
        return false;

    return true;
  #endif
  };
  /** compare for DIFFERENT */
  bool operator!=( const Flexible4ByteString_c& acrc_cmp ) const
  {
  #ifndef NO_8BIT_CHAR_TYPE
    return ( uint32[0] != acrc_cmp.uint32[0] );
  #else
    for (uint8_t index = 0; index < 4; index++)
      if ( uint8[index] != acrc_cmp.uint8[index] )
        return true;

    return false;
  #endif
  };

  #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && !defined(NO_8BIT_CHAR_TYPE)
  /** compare for LOWER */
  bool operator<( const Flexible4ByteString_c& acrc_cmp ) const
  { // use inline implementation for this case
    return ( uint32[0] < acrc_cmp.uint32[0] );
  };
  /** compare for LARGER */
  bool operator>( const Flexible4ByteString_c& acrc_cmp ) const
  { // use inline implementation for this case
    return ( uint32[0] > acrc_cmp.uint32[0] );
  };
  /** compare with:
    @return 0 == equal;
           +1 == this item has higher value than par;
           -1 == this item has lower value than par */
  int compare( const Flexible4ByteString_c& acrc_cmp ) const
  {
    if      (uint32[0] < acrc_cmp.uint32[0]) return -1;
    else if (uint32[0] > acrc_cmp.uint32[0]) return +1;
    return 0;
  };
  #else
  /** compare for LOWER */
  bool operator<( const Flexible4ByteString_c& acrc_cmp ) const;
  /** compare for LARGER */
  bool operator>( const Flexible4ByteString_c& acrc_cmp ) const;
  /** compare with:
    @return 0 == equal;
           +1 == this item has higher value than par;
           -1 == this item has lower value than par */
  int compare( const Flexible4ByteString_c& acrc_cmp ) const;
  #endif


  /**
    set an uint8_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..7]
    @param aui8_val uint8_t value to set
  */
  void setUint8Data( uint8_t aui8_pos, uint8_t aui8_val) { uint8[aui8_pos] = aui8_val;};
  /**
    simply deliver a uint8_t from a specific position with operator[].
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint8_t [0..7]
    @return uint8_t balue in CAN data string at pos aui8_pos
  */
  uint8_t operator[](uint8_t aui8_pos) const {return uint8[aui8_pos];};
  /**
    simply deliver a uint8_t from a specific position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint8_t [0..7]
    @return uint8_t balue in CAN data string at pos aui8_pos
  */
  uint8_t getUint8Data(uint8_t aui8_pos) const {return uint8[aui8_pos];};

  #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && !defined(NO_8BIT_CHAR_TYPE)
  /**
    set an uint16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 2
    @param aui8_pos Byte position [0..2]
    @param aui16_val uint16_t value to set
  */
  void setUint16Data( uint8_t aui8_pos, uint16_t aui16_val);
  /**
    set an int16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 2
    @param aui8_pos Byte position [0..2]
    @param ai16_val int16_t value to set
  */
  void setInt16Data( uint8_t aui8_pos, int16_t ai16_val);
  /**
    simply deliver a uint16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 2
    @param aui8_pos position of delivered uint16_t [0..2]
    @return uint16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  uint16_t getUint16Data(uint8_t aui8_pos) const;
  /**
    simply deliver a int16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 2
    @param aui8_pos position of delivered int16_t [0..2]
    @return int16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  int16_t getInt16Data(uint8_t aui8_pos) const;
  #else
  /**
    set an uint16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..2]
    @param aui16_val uint16_t value to set
  */
  void setUint16Data( uint8_t aui8_pos, uint16_t aui16_val)
    {numberRef2LittleEndianString( aui16_val, (uint8+aui8_pos) );};
  /**
    set an int16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..2]
    @param ai16_val int16_t value to set
  */
  void setInt16Data( uint8_t aui8_pos, int16_t ai16_val)
    {numberRef2LittleEndianString( ai16_val, (uint8+aui8_pos) );};

  /**
    simply deliver a uint16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint16_t [0..2]
    @return uint16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  uint16_t getUint16Data(uint8_t aui8_pos) const { return convertLittleEndianStringUi16(uint8+aui8_pos);};
  /**
    simply deliver a int16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered int16_t [0..2]
    @return int16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  int16_t getInt16Data(uint8_t aui8_pos) const { return convertLittleEndianStringI16(uint8+aui8_pos);};
  #endif

  /** set an uint32_t value
    @param aui32_val uint32_t value to set
  */
  void setUint32Data( uint32_t aui32_val)
    #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && !defined(NO_8BIT_CHAR_TYPE)
    { uint32[0] = aui32_val;};
    #else
    {numberRef2LittleEndianString( aui32_val, uint8 );};
    #endif
  /** set an int32_t value
    @param ai32_val int32_t value to set
  */
  void setInt32Data( int32_t ai32_val)
    #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && !defined(NO_8BIT_CHAR_TYPE)
    { int32[0] = ai32_val;};
    #else
    {numberRef2LittleEndianString( ai32_val, uint8 );};
    #endif

  /** simply deliver a uint32_t
    @return uint32_t balue in CAN data string at pos aui32_pos
  */
  uint32_t getUint32Data() const
    #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && !defined(NO_8BIT_CHAR_TYPE)
    {return uint32[0];};
    #else
    { return convertLittleEndianStringUi32(uint8);};
    #endif
  /** simply deliver a int32_t
    @return int32_t balue in CAN data string at pos aui32_pos
  */
  int32_t getInt32Data() const
    #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && !defined(NO_8BIT_CHAR_TYPE)
    {return int32[0];};
    #else
    { return convertLittleEndianStringI32(uint8);};
    #endif

  /** define the values as anonymous union */
  union {
    uint8_t  uint8 [4];
    int8_t    int8 [4];
#ifndef NO_8BIT_CHAR_TYPE
    uint16_t uint16[2];
    int16_t   int16[2];
    uint32_t uint32[1];
    int32_t   int32[1];
#endif
  };
};


/** helper class with some inline functions to enable flexible, efficient
    and safe access to 8-Byte strings for ISONAME and CanPkg_c as:
    + uint8_t  array of size 8 -> name uint8
    + uint16_t array of size 4 -> name uint16
    + uint32_t array of size 2 -> name uint32 (( only for CPU with 16Bit or more)
  ( + uint64_t array of size 1 -> name uint64 ONLY FOR CPU_WORD_SIZE >= 4 )
  IMPORTANT: The internal order of bytes is LITTLE ENDIAN, so that the bytes are directly
             used for sending of CAN messages.
   ==>> for big endian CPU all assignment and retrieval functions have to use the helper
        like numberRef2LittleEndianString for safe data conversion / adaptation
  */
class Flexible8ByteString_c {

 public:
  Flexible8ByteString_c( const Flexible8ByteString_c& acrc_src );
  /** constructor for INIT directly from a flexible positioned Stream.
      IMPORTANT: this works also when the string starts at ODD position!
  */
  Flexible8ByteString_c( const uint8_t* apui8_srcStream = NULL );
  /** set this object from a optionally odd addressed string */
  void setDataFromString( const uint8_t* apui8_srcStream, uint8_t aui8_len = 8 )
  { if (apui8_srcStream != NULL ) CNAMESPACE::memcpy(uint8, apui8_srcStream, aui8_len );};

  /** copy contents of this object to a optionally odd addressed string */
  void getDataToString( uint8_t* pui8_srcStream, uint8_t aui8_len = 8 ) const
  { if (pui8_srcStream != NULL ) CNAMESPACE::memcpy( pui8_srcStream, uint8, aui8_len );};

  /** set this object from a optionally odd addressed string */
  void setDataFromString( uint8_t aui8_offset, const uint8_t* apui8_srcStream, uint8_t aui8_len = 8 );
  /** copy contents of this object to a optionally odd addressed string */
  void getDataToString( uint8_t aui8_offset, uint8_t* pui8_targetStream, uint8_t aui8_len = 8 ) const;

  /** retrieve a raw const pointer to uint8_t data string from given offset position onwards */
  const uint8_t* getUint8DataConstPointer( uint8_t aui8_positionOffset ) const
    { return uint8+aui8_positionOffset;};
  /** retrieve a raw const pointer to uint8_t data string */
  const uint8_t* getUint8DataConstPointer() const { return uint8;};
  /** retrieve a raw const pointer to uint8_t data string from given offset position onwards */
  uint8_t* getUint8DataPointer( uint8_t aui8_positionOffset )
    { return uint8+aui8_positionOffset;};
  /** retrieve a raw const pointer to uint8_t data string */
  uint8_t* getUint8DataPointer() { return uint8;};


#if (SIZEOF_INT >= 4) && !defined( __IAR_SYSTEMS_ICC__ )
  /** assignment */
  const Flexible8ByteString_c& operator=( const Flexible8ByteString_c& acrc_src )
  { uint64[0] = acrc_src.uint64[0]; return *this; };
  /** compare for EQUAL */
  bool operator==( const Flexible8ByteString_c& acrc_cmp ) const
  { // use inline implementation for this case
    return ( uint64[0] == acrc_cmp.uint64[0] );
  };
  /** compare for DIFFERENT */
  bool operator!=( const Flexible8ByteString_c& acrc_cmp ) const
  {
    return ( uint64[0] != acrc_cmp.uint64[0] );
  };
  #else
  /** assignment */
  const Flexible8ByteString_c& operator=( const Flexible8ByteString_c& acrc_src );
  /** compare for EQUAL */
  bool operator==( const Flexible8ByteString_c& acrc_cmp ) const;
  /** compare for DIFFERENT */
  bool operator!=( const Flexible8ByteString_c& acrc_cmp ) const;
  #endif


  #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && SIZEOF_INT >= 4
  /** compare for LOWER */
  bool operator<( const Flexible8ByteString_c& acrc_cmp ) const
  { // use inline implementation for this case
    return ( uint64[0] < acrc_cmp.uint64[0] );
  };
  /** compare for LARGER */
  bool operator>( const Flexible8ByteString_c& acrc_cmp ) const
  { // use inline implementation for this case
    return ( uint64[0] > acrc_cmp.uint64[0] );
  };
  /** compare with:
    @return 0 == equal;
           +1 == this item has higher value than par;
           -1 == this item has lower value than par */
  int compare( const Flexible8ByteString_c& acrc_cmp ) const
  {
    if      (uint64[0] < acrc_cmp.uint64[0]) return -1;
    else if (uint64[0] > acrc_cmp.uint64[0]) return +1;
    return 0;
  };
  #else
  /** compare for LOWER */
  bool operator<( const Flexible8ByteString_c& acrc_cmp ) const;
  /** compare for LARGER */
  bool operator>( const Flexible8ByteString_c& acrc_cmp ) const;
  /** compare with:
    @return 0 == equal;
           +1 == this item has higher value than par;
           -1 == this item has lower value than par */
  int compare( const Flexible8ByteString_c& acrc_cmp ) const;
  #endif

  /**
    set an uint8_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..7]
    @param aui8_val uint8_t value to set
  */
  void setUint8Data( uint8_t aui8_pos, uint8_t aui8_val) { uint8[aui8_pos] = aui8_val;};
  /**
    simply deliver a uint8_t from a specific position with operator[].
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint8_t [0..7]
    @return uint8_t balue in CAN data string at pos aui8_pos
  */
  uint8_t operator[](uint8_t aui8_pos) const {return uint8[aui8_pos];};
  /**
    simply deliver a uint8_t from a specific position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint8_t [0..7]
    @return uint8_t balue in CAN data string at pos aui8_pos
  */
  uint8_t getUint8Data(uint8_t aui8_pos) const {return uint8[aui8_pos];};

  /**
    simply deliver a float from a specific position with.
    IMPORTANT: position 0 matches to the least significant byte,
    as the string is ordered in LittleEndian order,
    identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint8_t [0..7]
    @return float value in CAN data string at pos aui8_pos
   */
  float getFloatData(uint8_t aui8_pos) const;

  /**
    set a float value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
    as the string is ordered in LittleEndian order,
    identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos Byte position [0..4]
    @param af_val float value to set
   */
  void setFloatData(uint8_t aui8_pos, const float af_val);

  #if defined(OPTIMIZE_NUMBER_CONVERSIONS_FOR_LITTLE_ENDIAN) && !defined(NO_8BIT_CHAR_TYPE)
  /**
    set an uint16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos Byte position [0..6]
    @param aui16_val uint16_t value to set
  */
  void setUint16Data( uint8_t aui8_pos, uint16_t aui16_val);
  /**
    set an int16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos Byte position [0..6]
    @param ai16_val int16_t value to set
  */
  void setInt16Data( uint8_t aui8_pos, int16_t ai16_val);
  /**
    simply deliver a uint16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos position of delivered uint16_t [0..6]
    @return uint16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  uint16_t getUint16Data(uint8_t aui8_pos) const;
  /**
    simply deliver a int16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos position of delivered int16_t [0..6]
    @return int16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  int16_t getInt16Data(uint8_t aui8_pos) const;
  /**
    set an uint32_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos Byte position [0..4]
    @param aui32_val uint32_t value to set
  */
  void setUint32Data( uint8_t aui8_pos, uint32_t aui32_val);
  /**
    set an int32_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos Byte position [0..4]
    @param ai32_val int32_t value to set
  */
  void setInt32Data( uint8_t aui8_pos, int32_t ai32_val);
  /**
    simply deliver a uint32_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos position of delivered uint32_t [0..4]
    @return uint32_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  uint32_t getUint32Data(uint8_t aui8_pos) const;
  /**
    simply deliver a int32_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    Possible Error: <iLibErr_c::Range, iLibErr_c::Can> when aui8_pos > 6
    @param aui8_pos position of delivered int32_t [0..4]
    @return int32_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  int32_t getInt32Data(uint8_t aui8_pos) const;
  #else
  /**
    set an uint16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..6]
    @param aui16_val uint16_t value to set
  */
  void setUint16Data( uint8_t aui8_pos, uint16_t aui16_val)
    {numberRef2LittleEndianString( aui16_val, (uint8+aui8_pos) );};
  /**
    set an int16_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..6]
    @param ai16_val int16_t value to set
  */
  void setInt16Data( uint8_t aui8_pos, int16_t ai16_val)
    {numberRef2LittleEndianString( ai16_val, (uint8+aui8_pos) );};

  /**
    simply deliver a uint16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint16_t [0..6]
    @return uint16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  uint16_t getUint16Data(uint8_t aui8_pos) const { return convertLittleEndianStringUi16(uint8+aui8_pos);};
  /**
    simply deliver a int16_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered int16_t [0..6]
    @return int16_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  int16_t getInt16Data(uint8_t aui8_pos) const { return convertLittleEndianStringI16(uint8+aui8_pos);};
  /**
    set an uint32_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..4]
    @param aui32_val uint32_t value to set
  */
  void setUint32Data( uint8_t aui8_pos, uint32_t aui32_val)
    {numberRef2LittleEndianString( aui32_val, (uint8+aui8_pos) );};
  /**
    set an int32_t value at specified position in string.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos Byte position [0..4]
    @param ai32_val int32_t value to set
  */
  void setInt32Data( uint8_t aui8_pos, int32_t ai32_val)
    {numberRef2LittleEndianString( ai32_val, (uint8+aui8_pos) );};

  /**
    simply deliver a uint32_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered uint32_t [0..4]
    @return uint32_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  uint32_t getUint32Data(uint8_t aui8_pos) const { return convertLittleEndianStringUi32(uint8+aui8_pos);};
  /**
    simply deliver a int32_t from a specific starting position with.
    IMPORTANT: position 0 matches to the least significant byte,
               as the string is ordered in LittleEndian order,
               identic to the order which is used for CAN messages
    @param aui8_pos position of delivered int32_t [0..4]
    @return int32_t balue in CAN data string at pos (aui8_pos, aui8_pos+1) read Low/High order
  */
  int32_t getInt32Data(uint8_t aui8_pos) const { return convertLittleEndianStringI32(uint8+aui8_pos);};
  #endif
  /**
    simply deliver a Flexible4ByteString_c from a specific value index with.
    Important: Flexible4ByteString_c has the SAME byte order as a CAN message byte string.
    ==>> for BIG ENDIAN systems, this value can NOT directly be used as intger value
    @param aui8_ind position of delivered uint16_t [0..1]
    @return Flexible4ByteString_c balue in CAN data string at pos aui16_pos
  */
  void setFlexible4DataValueInd(uint8_t aui8_ind, const Flexible4ByteString_c& ac_value )
#ifndef NO_8BIT_CHAR_TYPE
    {uint32[aui8_ind] = ac_value.uint32[0];};
#else
    {for (uint8_t index = 0; index < 4; index++) uint8[4*aui8_ind+index] = ac_value.uint8[index];};
#endif


  /** define the values as anonymous union */
  union {
    uint8_t  uint8 [8];
    int8_t    int8 [8];
#ifndef NO_8BIT_CHAR_TYPE
    uint16_t uint16[4];
    int16_t   int16[4];
    uint32_t uint32[2];
    int32_t   int32[2];
    #if (SIZEOF_INT >= 4) && !defined( __IAR_SYSTEMS_ICC__ )
    uint64_t uint64[1];
    int64_t   int64[1];
    #endif
#endif
  };
};


#ifdef USE_DATASTREAMS_IO
/** convert receive multistream (LE) into an unsigned 16-bit variable */
uint16_t convertIstreamUi16( StreamInput_c& rc_stream );
/** convert receive multistream (LE) into a signed 16-bit variable */
inline
int16_t convertIstreamI16( StreamInput_c& rc_stream )
{ return static_cast <int16_t>(convertIstreamUi16 (rc_stream)); }

/** convert receive multistream (LE) into an unsigned 32-bit variable */
uint32_t convertIstreamUi32( StreamInput_c& rc_stream );
/** convert receive multistream (LE) into a signed 32-bit variable */
inline
int32_t convertIstreamI32( StreamInput_c& rc_stream )
{ return static_cast <int32_t>(convertIstreamUi32 (rc_stream)); }
#endif


/// Some CString functions
/// (replacing the need for C standard library calls)

int
getCStringLength (const char *acstr_text);

void
addCStringWithoutTermination (char **dest, const char *src);

int
getCStringCount (const char *src, char countChar);




#ifdef USE_VT_UNICODE_SUPPORT
  /// Define UNICODE REPLACEMENT CHARACTER
  #define DEF_UNICODE_REPLACEMENT_CHAR 0xFFFD
  /// Define ISO-8859 REPLACEMENT CHARACTER (for any iso8859)
  #define DEF_ISO8859_REPLACEMENT_CHAR 0x20

  // forward declarations for inline functions!
  extern const uint16_t p96ui16_8859_2 [96];
  extern const uint16_t p96ui16_8859_4 [96];
  extern const uint16_t p96ui16_8859_5 [96];
  extern const uint16_t p96ui16_8859_7 [96];
  extern const uint16_t p96ui16_8859_15 [96];
  extern const uint16_t* pp96ui16_8859s [8];

  inline uint16_t convert8859ToUnicode (uint8_t aui8_char, uint8_t aui8_encoding)
  {
    if ((aui8_char < 0xA0) || (aui8_encoding == 0))
    { // direct mapping for [0x00..0x9F] and latin-1
      return uint16_t (aui8_char);
    }
    else // (aui8_char >= 0xA0) && (aui8_encoding != 0)
    { // lookup!
      return pp96ui16_8859s [aui8_encoding] [aui8_char - 0xA0]; // checking for valid encoding is omitted for performance reasons - there shouldn't be wrong entries in FontAttributes anyway!
    }
  }

  /// @todo ON REQUEST OPTIMIZE: Reverse lookup up to now done a slow but non-space-consuming way. To be optimized later on using a generated reverse lookup table */
  /// keep inlined, as it will be optimized, so it's smaller then!
  inline uint8_t convertUnicodeTo8859 (uint16_t aui16_unicode, uint8_t aui8_encoding)
  {
    if (aui8_encoding == 0)
    {
      if (aui16_unicode >= 0x100)
        return DEF_ISO8859_REPLACEMENT_CHAR; // return iso8859-Replacement Character
      else
        return uint8_t (aui16_unicode);
    }
    const uint16_t* p96ui16_8859s = pp96ui16_8859s [aui8_encoding];
    if (aui16_unicode != DEF_UNICODE_REPLACEMENT_CHAR)
    {
      for (uint16_t ui16_cur=0xA0; ui16_cur < 0x100; ui16_cur++)
      {
        if (*p96ui16_8859s++ == aui16_unicode)
        {
          return uint8_t (ui16_cur);
        }
      }
    }
    return DEF_ISO8859_REPLACEMENT_CHAR; // default to iso8859-Replacement Character in case the char's not found;
  }

  void convertString8859ToUnicode (const STL_NAMESPACE::string& rrefstr_iso8859, STL_NAMESPACE::string& rrefstr_unicode, uint8_t aui8_encoding);
  void convertStringUnicodeTo8859 (const char* apui8_stringUnicode, uint16_t aui16_stringUnicodeLength, char* apui8_iso8859buffer, uint8_t aui8_encoding);
  void push_backUTF8 (STL_NAMESPACE::string& rrefstr_string, uint16_t aui16_unicode);
#endif

class VtFontScaling
{
public:
  static uint8_t getScaledFont(uint8_t aui8_originalSize, int32_t ai32_vtDimension, int32_t ai32_opDimension, uint16_t aui16_vtSupportedFonts,
                               bool ab_buttonParent, bool ab_inSkm,
                               uint16_t aui16_opButtonWidth, uint16_t aui16_opButtonHeight,
                               uint16_t aui16_opSoftKeyWidth, uint16_t aui16_opSoftKeyHeight, uint32_t aui32_vtSoftKeyWidth, uint32_t aui32_vtSoftKeyHeight);
private:
  static uint8_t marr_font2PixelDimensionTableW[15];
  static uint8_t marr_font2PixelDimensionTableH[15];
};


class Subsystem_c
{
public:
  Subsystem_c()
    : mb_initialized (false)
  {}

  void setInitialized()   { mb_initialized = true; }
  void setClosed()        { mb_initialized = false; }
  bool initialized() const { return mb_initialized; }

private:
  bool mb_initialized;
};

} // end of namespace __IsoAgLib
#endif
