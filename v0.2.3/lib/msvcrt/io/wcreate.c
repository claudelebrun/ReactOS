#include <msvcrt/io.h>
#include <msvcrt/fcntl.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


/*
 * @implemented
 */
int _wcreat(const wchar_t* filename, int mode)
{
    DPRINT("_wcreat('%S', mode %x)\n", filename, mode);
    return _wopen(filename,_O_CREAT|_O_TRUNC,mode);
}
