/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/internal/file.h>

/*
 * @implemented
 */
int
fprintf(register FILE *iop, const char *fmt, ...)
{
  int len;
  char localbuf[BUFSIZ];
  va_list a=0;
  

  va_start( a, fmt ); 
  if (iop->_flag & _IONBF)
  {
    iop->_flag &= ~_IONBF;
    iop->_ptr = iop->_base = localbuf;
    iop->_bufsiz = BUFSIZ;
    len = vfprintf(iop,fmt,a);
    fflush(iop);
    iop->_flag |= _IONBF;
    iop->_base = NULL;
    iop->_bufsiz = 0;
    iop->_cnt = 0;
  }
  else
    len = vfprintf(iop, fmt, a);
  return ferror(iop) ? EOF : len;
}

/*
 * @implemented
 */
int
fwprintf(register FILE *iop, const wchar_t *fmt, ...)
{
  int len;
  wchar_t localbuf[BUFSIZ];
  va_list a=0;
  

  va_start( a, fmt ); 
  if (iop->_flag & _IONBF)
  {
    iop->_flag &= ~_IONBF;
    iop->_ptr = iop->_base = (char *)localbuf;
    iop->_bufsiz = BUFSIZ;
    len = vfwprintf(iop,fmt,a);
    fflush(iop);
    iop->_flag |= _IONBF;
    iop->_base = NULL;
    iop->_bufsiz = 0;
    iop->_cnt = 0;
  }
  else
    len = vfwprintf(iop, fmt, a);
  return ferror(iop) ? EOF : len;
}
