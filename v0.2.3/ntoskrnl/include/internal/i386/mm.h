/*
 * Lowlevel memory managment definitions
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H

struct _EPROCESS;

#if 0
/*
 * Page access attributes (or these together)
 */
#define PA_READ            (1<<0)
#define PA_WRITE           ((1<<0)+(1<<1)) 
#define PA_EXECUTE         PA_READ
#define PA_PCD             (1<<4)
#define PA_PWT             (1<<3)

/*
 * Page attributes
 */
#define PA_USER            (1<<2)
#define PA_SYSTEM          (0)
#endif

// #define PAGE_SIZE (4096)

PULONG MmGetPageEntry(PVOID Address);

#define KERNEL_BASE        (0xc0000000)

#if defined(__GNUC__)

#define FLUSH_TLB   {				\
			unsigned int tmp;	\
			__asm__ __volatile__(	\
			    "movl %%cr3,%0\n\t"	\
			    "movl %0,%%cr3\n\t"	\
			    : "=r" (tmp)	\
			    :: "memory");	\
		    }

#elif defined(_MSC_VER)
/* TODO: Need some way to tell the compiler this is a memory barrier. */
#define FLUSH_TLB __asm mov eax, cr3  __asm mov cr3, eax;
#else
#error Unknown compiler for inline assembler
#endif


PULONG MmGetPageDirectory(VOID);

/*
 * Amount of memory that can be mapped by a page table
 */
#define PAGE_TABLE_SIZE (4*1024*1024)

#define PAGE_MASK(x) ((x)&(~0xfff))
#define VADDR_TO_PT_OFFSET(x)  ((((x)/1024)%4096))
#define VADDR_TO_PD_OFFSET(x)  ((x)/(4*1024*1024))

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H */
