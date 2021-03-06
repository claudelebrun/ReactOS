#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

//getc can be a macro
#undef getc

/*
 * @implemented
 */
int getc(FILE *fp)
{
        int c = -1;
// check for invalid stream

	if ( !__validfp (fp) ) {
		__set_errno(EINVAL);
		return EOF;
	}
// check for read access on stream

	if ( !OPEN4READING(fp) ) {
		__set_errno(EINVAL);
		return -1;
	}

	if(fp->_cnt > 0) {
		fp->_cnt--;
		c =  (int)(*fp->_ptr++ & 0377);
	} 
	else {
		c =  _filbuf(fp);
	}
	return c;
}

// not exported

wint_t  getwc(FILE *fp)
{
  wint_t c;
	
 // might check on multi bytes if text mode
 
  if(fp->_cnt > 0) {
        fp->_cnt -= sizeof(wchar_t);
        c = *((wchar_t *)(fp->_ptr));
        fp->_ptr += sizeof(wchar_t);
  } 
  else {
	c = _filwbuf(fp);
  }
  
  return c;
}




