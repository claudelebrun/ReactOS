#define NTOS_MODE_KERNEL
#include <ntos.h>

NTSTATUS
LdrGetAddressInformation(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PULONG LineNumber,
  OUT PCH FileName  OPTIONAL,
  OUT PCH FunctionName  OPTIONAL);

CHAR
KdbTryGetCharKeyboard(PULONG ScanCode);
ULONG
KdbTryGetCharSerial(VOID);
VOID
KdbEnter(VOID);
VOID
DbgRDebugInit(VOID);
VOID
DbgShowFiles(VOID);
VOID
DbgEnableFile(PCH Filename);
VOID
DbgDisableFile(PCH Filename);
VOID
KdbInitProfiling();
VOID
KdbInitProfiling2();
VOID
KdbDisableProfiling();
VOID
KdbEnableProfiling();
VOID
KdbProfileInterrupt(ULONG_PTR Eip);

struct KDB_BPINFO {
    DWORD Addr;
    DWORD Type;
    DWORD Size;
    DWORD Enabled;
};
