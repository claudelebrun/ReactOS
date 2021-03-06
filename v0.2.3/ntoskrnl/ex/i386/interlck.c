/* $Id: interlck.c,v 1.6 2003/12/30 18:52:03 fireball Exp $
 *
 * reactos/ntoskrnl/ex/i386/interlck.c
 *
 */
#include <ddk/ntddk.h>


#if defined(__GNUC__)

INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedIncrementLong(IN PLONG Addend);

__asm__("\n\t.global @Exfi386InterlockedIncrementLong@4\n\t"
	"@Exfi386InterlockedIncrementLong@4:\n\t"
	"addl $1,(%ecx)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedIncrementLong(IN PLONG Addend)
{
	__asm add ecx, 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedDecrementLong(IN PLONG Addend);

__asm__("\n\t.global @Exfi386InterlockedDecrementLong@4\n\t"
	"@Exfi386InterlockedDecrementLong@4:\n\t"
	"subl $1,(%ecx)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedDecrementLong(IN PLONG Addend)
{
	__asm sub ecx, 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

ULONG FASTCALL
Exfi386InterlockedExchangeUlong(IN PULONG Target,
				IN ULONG Value);

__asm__("\n\t.global @Exfi386InterlockedExchangeUlong@8\n\t"
	"@Exfi386InterlockedExchangeUlong@8:\n\t"
	"xchgl %edx,(%ecx)\n\t"
	"movl  %edx,%eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
ULONG FASTCALL
Exfi386InterlockedExchangeUlong(IN PULONG Target,
				IN ULONG Value)
{
	__asm xchg [ecx], edx
	__asm mov  eax, edx
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

INTERLOCKED_RESULT STDCALL
Exi386InterlockedIncrementLong(IN PLONG Addend);

__asm__("\n\t.global _Exi386InterlockedIncrementLong@4\n\t"
	"_Exi386InterlockedIncrementLong@4:\n\t"
	"movl 4(%esp),%eax\n\t"
	"addl $1,(%eax)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret $4\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
INTERLOCKED_RESULT STDCALL
Exi386InterlockedIncrementLong(IN PLONG Addend)
{
	__asm mov eax, Addend
	__asm add [eax], 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret 4
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

INTERLOCKED_RESULT STDCALL
Exi386InterlockedDecrementLong(IN PLONG Addend);

__asm__("\n\t.global _Exi386InterlockedDecrementLong@4\n\t"
	"_Exi386InterlockedDecrementLong@4:\n\t"
	"movl 4(%esp),%eax\n\t"
	"subl $1,(%eax)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret $4\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
INTERLOCKED_RESULT STDCALL
Exi386InterlockedDecrementLong(IN PLONG Addend)
{
	__asm mov eax, Addend
	__asm sub [eax], 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret 4
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

ULONG STDCALL
Exi386InterlockedExchangeUlong(IN PULONG Target,
			       IN ULONG Value);

__asm__("\n\t.global _Exi386InterlockedExchangeUlong@8\n\t"
	"_Exi386InterlockedExchangeUlong@8:\n\t"
	"movl 4(%esp),%edx\n\t"
	"movl 8(%esp),%eax\n\t"
	"xchgl %eax,(%edx)\n\t"
	"ret $8\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
ULONG STDCALL
Exi386InterlockedExchangeUlong(IN PULONG Target,
			       IN ULONG Value)
{
	__asm mov edx, Value
	__asm mov eax, Target
	__asm xchg [edx], eax
	__asm ret 8
}

#else
#error Unknown compiler for inline assembler
#endif



/**********************************************************************
 * FASTCALL: @InterlockedIncrement@4
 * STDCALL : _InterlockedIncrement@4
 */
#if defined(__GNUC__)
LONG FASTCALL
InterlockedIncrement(PLONG Addend);
/*
 * FUNCTION: Increments a caller supplied variable of type LONG as an 
 * atomic operation
 * ARGUMENTS:
 *     Addend = Points to a variable whose value is to be increment
 * RETURNS: The incremented value
 */

__asm__("\n\t.global @InterlockedIncrement@4\n\t"
	"@InterlockedIncrement@4:\n\t"
	"movl $1,%eax\n\t"
	"xaddl %eax,(%ecx)\n\t"
	"incl %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
LONG FASTCALL
InterlockedIncrement(PLONG Addend)
{
	__asm mov eax, 1
	__asm xadd [ecx], eax
	__asm inc eax
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


/**********************************************************************
 * FASTCALL: @InterlockedDecrement@4
 * STDCALL : _InterlockedDecrement@4
 */
#if defined(__GNUC__)
LONG FASTCALL
InterlockedDecrement(PLONG Addend);

__asm__("\n\t.global @InterlockedDecrement@4\n\t"
	"@InterlockedDecrement@4:\n\t"
	"movl $-1,%eax\n\t"
	"xaddl %eax,(%ecx)\n\t"
	"decl %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
LONG FASTCALL
InterlockedDecrement(PLONG Addend)
{
	__asm mov eax, -1
	__asm xadd [ecx], eax
	__asm dec eax
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


/**********************************************************************
 * FASTCALL: @InterlockedExchange@8
 * STDCALL : _InterlockedExchange@8
 */

#if defined(__GNUC__)
LONG FASTCALL
InterlockedExchange(PLONG Target,
		    LONG Value);

__asm__("\n\t.global @InterlockedExchange@8\n\t"
	"@InterlockedExchange@8:\n\t"
	"xchgl %edx,(%ecx)\n\t"
	"movl  %edx,%eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
LONG FASTCALL
InterlockedExchange(PLONG Target,
		    LONG Value)
{
	__asm xchg [ecx], edx
	__asm mov eax, edx
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif

/**********************************************************************
 * FASTCALL: @InterlockedExchangeAdd@8
 * STDCALL: _InterlockedExchangeAdd@8
 */
#if defined(__GNUC__)

LONG FASTCALL
InterlockedExchangeAdd(PLONG Addend,
		       LONG Value);

__asm__("\n\t.global @InterlockedExchangeAdd@8\n\t"
	"@InterlockedExchangeAdd@8:\n\t"
	"xaddl %edx,(%ecx)\n\t"
	"movl %edx,%eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
LONG FASTCALL
InterlockedExchangeAdd(PLONG Addend,
		       LONG Value)
{
	__asm xadd [ecx], edx
	__asm mov eax, edx
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


/**********************************************************************
 * FASTCALL: @InterlockedCompareExchange@12
 * STDCALL: _InterlockedCompareExchange@12
 */
#if defined(__GNUC__)

LONG FASTCALL
InterlockedCompareExchange(PLONG Destination,
			   LONG Exchange,
			   LONG Comperand);

__asm__("\n\t.global @InterlockedCompareExchange@12\n\t"
	"@InterlockedCompareExchange@12:\n\t"
	"movl 4(%esp),%eax\n\t"
	"cmpxchg %edx,(%ecx)\n\t"
	"ret $4\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
LONG FASTCALL
InterlockedCompareExchange(PLONG Destination,
			   LONG Exchange,
			   LONG Comperand)
{
	__asm mov eax, Comperand
	__asm cmpxchg [ecx], edx
	__asm ret 4
}

#else
#error Unknown compiler for inline assembler
#endif

/* EOF */
