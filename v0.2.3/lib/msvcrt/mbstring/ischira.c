/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/ischira.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>
#include <msvcrt/mbctype.h>


/*
 * @implemented
 */
int _ismbchira( unsigned int c )
{
  return ((c>=0x829F) && (c<=0x82F1));
}

/*
 * @implemented
 */
int _ismbckata( unsigned int c )
{
  return ((c>=0x8340) && (c<=0x8396));
}

/*
 * @unimplemented
 */
unsigned int _mbctohira( unsigned int c )
{
  return c;
}

/*
 * @unimplemented
 */
unsigned int _mbctokata( unsigned int c )
{
  return c;
}

