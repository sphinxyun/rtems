/**
 * @file
 *
 * @brief Inlined Routines in the Priority Handler Bit Map Implementation
 *
 * This file contains the static inline implementation of all inlined
 * routines in the Priority Handler bit map implementation
 */

/*
 *  COPYRIGHT (c) 1989-2010.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_PRIORITYBITMAPIMPL_H
#define _RTEMS_SCORE_PRIORITYBITMAPIMPL_H

#include <rtems/score/prioritybitmap.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ScorePriority
 */
/**@{**/

/**
 *  This table is used by the generic bitfield routines to perform
 *  a highly optimized bit scan without the use of special CPU
 *  instructions.
 */
extern const unsigned char _Bitfield_Leading_zeros[256];

/**
 *  @brief Gets the @a _bit_number of the first bit set in the specified value.
 *
 *  This routine returns the @a _bit_number of the first bit set
 *  in the specified value.  The correspondence between @a _bit_number
 *  and actual bit position is processor dependent.  The search for
 *  the first bit set may run from most to least significant bit
 *  or vice-versa.
 *
 *  @param[in] _value is the value to bit scan.
 *  @param[in] _bit_number is the position of the first bit set.
 *
 *  @note This routine is used when the executing thread is removed
 *  from the ready state and, as a result, its performance has a
 *  significant impact on the performance of the executive as a whole.
 *
 *  @note This routine must be a macro because if a CPU specific version
 *  is used it will most likely use inline assembly.
 */
#if ( CPU_USE_GENERIC_BITFIELD_CODE == FALSE )
#define _Bitfield_Find_first_bit( _value, _bit_number ) \
        _CPU_Bitfield_Find_first_bit( _value, _bit_number )
#else
#define _Bitfield_Find_first_bit( _value, _bit_number ) \
  { \
    unsigned int __value = (_value); \
    const unsigned char *__p = _Bitfield_Leading_zeros; \
    \
    if ( __value < 0x100 ) \
      (_bit_number) = (Priority_bit_map_Word)( __p[ __value ] + 8 );  \
    else \
      (_bit_number) = (Priority_bit_map_Word)( __p[ __value >> 8 ] ); \
  }
#endif

/**
 * @brief Returns the priority bit mask for the specified major or minor bit
 * number.
 *
 * @param bit_number The bit number for which we need a mask.
 *
 * @return The priority bit mask.
 */
RTEMS_INLINE_ROUTINE Priority_bit_map_Word _Priority_Mask(
  unsigned int bit_number
)
{
#if ( CPU_USE_GENERIC_BITFIELD_CODE == FALSE )
  return _CPU_Priority_Mask( bit_number );
#else
  return (Priority_bit_map_Word) ( 0x8000u >> bit_number );
#endif
}

#if ( CPU_USE_GENERIC_BITFIELD_CODE == FALSE )
/**
 *  This method returns the bit index position for the specified priority.
 *
 *  @param[in] _priority is the priority for which we need the index.
 *
 *  @retval This method returns the array index into the priority bit map.
 *
 *  @note This may simply be a pass through to a CPU dependent implementation.
 */
#define _Priority_Bits_index( _priority ) \
  _CPU_Priority_bits_index( _priority )
#endif

/**
 * This function returns the major portion of the_priority.
 */

RTEMS_INLINE_ROUTINE unsigned int _Priority_Major( unsigned int the_priority )
{
  return the_priority / 16;
}

/**
 * This function returns the minor portion of the_priority.
 */

RTEMS_INLINE_ROUTINE unsigned int _Priority_Minor( unsigned int the_priority )
{
  return the_priority % 16;
}

#if ( CPU_USE_GENERIC_BITFIELD_CODE == TRUE )

/**
 * This function translates the bit numbers returned by the bit scan
 * of a priority bit field into something suitable for use as
 * a major or minor component of a priority.
 */

RTEMS_INLINE_ROUTINE unsigned int _Priority_Bits_index(
  unsigned int bit_number
)
{
  return bit_number;
}

#endif

RTEMS_INLINE_ROUTINE void _Priority_bit_map_Initialize(
  Priority_bit_map_Control *bit_map
)
{
  memset( bit_map, 0, sizeof( *bit_map ) );
}

/**
 * Priority Queue implemented by bit map
 */

RTEMS_INLINE_ROUTINE void _Priority_bit_map_Add (
  Priority_bit_map_Control     *bit_map,
  Priority_bit_map_Information *bit_map_info
)
{
  *bit_map_info->minor |= bit_map_info->ready_minor;
  bit_map->major_bit_map |= bit_map_info->ready_major;
}

RTEMS_INLINE_ROUTINE void _Priority_bit_map_Remove (
  Priority_bit_map_Control     *bit_map,
  Priority_bit_map_Information *bit_map_info
)
{
  *bit_map_info->minor &= bit_map_info->block_minor;
  if ( *bit_map_info->minor == 0 )
    bit_map->major_bit_map &= bit_map_info->block_major;
}

RTEMS_INLINE_ROUTINE unsigned int _Priority_bit_map_Get_highest(
  const Priority_bit_map_Control *bit_map
)
{
  unsigned int minor;
  unsigned int major;

  /* Avoid problems with some inline ASM statements */
  unsigned int tmp;

  tmp = bit_map->major_bit_map;
  _Bitfield_Find_first_bit( tmp, major );

  tmp = bit_map->bit_map[ major ];
  _Bitfield_Find_first_bit( tmp, minor );

  return (_Priority_Bits_index( major ) << 4) +
          _Priority_Bits_index( minor );
}

RTEMS_INLINE_ROUTINE bool _Priority_bit_map_Is_empty(
  const Priority_bit_map_Control *bit_map
)
{
  return bit_map->major_bit_map == 0;
}

RTEMS_INLINE_ROUTINE void _Priority_bit_map_Initialize_information(
  Priority_bit_map_Control     *bit_map,
  Priority_bit_map_Information *bit_map_info,
  unsigned int                  new_priority
)
{
  unsigned int major;
  unsigned int minor;
  Priority_bit_map_Word mask;

  major = _Priority_Major( new_priority );
  minor = _Priority_Minor( new_priority );

  bit_map_info->minor = &bit_map->bit_map[ _Priority_Bits_index( major ) ];

  mask = _Priority_Mask( major );
  bit_map_info->ready_major = mask;
  bit_map_info->block_major = (Priority_bit_map_Word) ~mask;

  mask = _Priority_Mask( minor );
  bit_map_info->ready_minor = mask;
  bit_map_info->block_minor = (Priority_bit_map_Word) ~mask;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
/* end of include file */
