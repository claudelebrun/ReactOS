#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>

#define KERNEL_BASE (0xc0000000)

#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)

#define MULTIBOOT_HEADER_FLAGS (0x00010003)

#define V2P(x) (x - 0xc0000000 + 0x200000)

#ifdef MP

#define AP_MAGIC (0x12481020)

#endif /* MP */
	
.globl _NtProcessStartup
.globl _start
.globl _init_stack
.globl _init_stack_top
.globl _trap_stack
.globl _trap_stack_top	
.globl _unmap_me
.globl _unmap_me2
.globl _unmap_me3
.globl _unmap_me4
		
	/*
	 * This is called by the realmode loader, with protected mode
	 * enabled, paging disabled and the segment registers pointing
	 * a 4Gb, 32-bit segment starting at zero.
	 *
	 *    EAX = Multiboot magic or application processor magic
	 *
	 *    EBX = Points to a structure in lowmem with data from the
	 *    loader
	 */

_NtProcessStartup:
_start:
	jmp	_multiboot_entry

	/* Align 32 bits boundary */
	.align 4
	
	/* Multiboot header */
multiboot_header:
	/* magic */
	.long MULTIBOOT_HEADER_MAGIC
	/* flags */
	.long MULTIBOOT_HEADER_FLAGS
	/* checksum */
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	/* header_addr */
	.long (0x200000 + multiboot_header - KERNEL_BASE)
	/* load_addr */
	.long 0x200000
	/* load_end_addr */
	.long (__bss_start__ + 0x200000 - KERNEL_BASE)
	/* bss_end_addr */
	.long (__bss_end__ + 0x200000 - KERNEL_BASE)
	/* entry_addr */
	.long (0x200000 + _start - KERNEL_BASE)

_multiboot_entry:	
	/*
	 * This	must be PIC because we haven't set up paging yet
	 */	
	
	/*
	 * Gcc expects this at all times
	 */
	cld

#ifdef MP

	/*
	 * Save the multiboot or application processor magic
	 */
	movl    %eax, %edx

	cmpl    $AP_MAGIC, %edx
	je	.m1

#endif /* MP */

	/*
	 * Zero the BSS
	 */
	movl	%eax, %edx
	movl	$0, %eax
	movl	$__bss_end__, %ecx
	subl	$__bss_start__, %ecx
	shr	$2, %ecx
	movl	$__bss_start__, %edi
	subl	$0xc0000000, %edi
	addl	$0x200000, %edi
	rep
	stosl

	/*
	 * Initialize the page directory
	 */
	movl	$V2P(startup_pagedirectory), %esi
	movl	$(V2P(lowmem_pagetable) + 0x7), 0x0(%esi)
	movl	$(V2P(kernel_pagetable) + 0x7), 0xC00(%esi)
	movl	$(V2P(kernel_pagetable+4096) + 0x7), 0xC04(%esi)
	movl	$(V2P(kernel_pagetable+2*4096) + 0x7), 0xC08(%esi)
	movl	$(V2P(kernel_pagetable+3*4096) + 0x7), 0xC0c(%esi)
	movl	$(V2P(kernel_pagetable+4*4096) + 0x7), 0xC10(%esi)
	movl	$(V2P(kernel_pagetable+5*4096) + 0x7), 0xC14(%esi)
	movl	$(V2P(kernel_pagetable+6*4096) + 0x7), 0xC18(%esi)
	movl	$(V2P(kernel_pagetable+7*4096) + 0x7), 0xC1c(%esi)
	movl	$(V2P(kernel_pagetable+8*4096) + 0x7), 0xC20(%esi)
	movl	$(V2P(kernel_pagetable+9*4096) + 0x7), 0xC24(%esi)
	movl	$(V2P(kernel_pagetable+10*4096) + 0x7), 0xC28(%esi)
	movl	$(V2P(kernel_pagetable+11*4096) + 0x7), 0xC2c(%esi)
	movl	$(V2P(kernel_pagetable+12*4096) + 0x7), 0xC30(%esi)
	movl	$(V2P(kernel_pagetable+13*4096) + 0x7), 0xC34(%esi)
	movl	$(V2P(kernel_pagetable+14*4096) + 0x7), 0xC38(%esi)
	movl	$(V2P(kernel_pagetable+15*4096) + 0x7), 0xC3c(%esi)
	movl	$(V2P(kernel_pagetable+16*4096) + 0x7), 0xC40(%esi)
	movl	$(V2P(kernel_pagetable+17*4096) + 0x7), 0xC44(%esi)
	movl	$(V2P(kernel_pagetable+18*4096) + 0x7), 0xC48(%esi)
	movl	$(V2P(kernel_pagetable+19*4096) + 0x7), 0xC4c(%esi)
	movl	$(V2P(kernel_pagetable+20*4096) + 0x7), 0xC50(%esi)
	movl	$(V2P(kernel_pagetable+21*4096) + 0x7), 0xC54(%esi)
	movl	$(V2P(kernel_pagetable+22*4096) + 0x7), 0xC58(%esi)
	movl	$(V2P(kernel_pagetable+23*4096) + 0x7), 0xC5c(%esi)
	movl	$(V2P(kernel_pagetable+24*4096) + 0x7), 0xC60(%esi)
	movl	$(V2P(kernel_pagetable+25*4096) + 0x7), 0xC64(%esi)
	movl	$(V2P(kernel_pagetable+26*4096) + 0x7), 0xC68(%esi)
	movl	$(V2P(kernel_pagetable+27*4096) + 0x7), 0xC6c(%esi)
	movl	$(V2P(kernel_pagetable+28*4096) + 0x7), 0xC70(%esi)
	movl	$(V2P(kernel_pagetable+29*4096) + 0x7), 0xC74(%esi)
	movl	$(V2P(kernel_pagetable+30*4096) + 0x7), 0xC78(%esi)
	movl	$(V2P(kernel_pagetable+31*4096) + 0x7), 0xC7c(%esi)

	movl	$(V2P(startup_pagedirectory) + 0x7), 0xF00(%esi) 
#ifdef MP
	movl	$(V2P(apic_pagetable) + 0x7), 0xFEC(%esi)
#endif /* MP */
	movl	$(V2P(kpcr_pagetable) + 0x7), 0xFF0(%esi)

	/*
	 * Initialize the page table that maps low memory
	 */
	movl	$V2P(lowmem_pagetable), %esi
	movl	$0x7, %eax
	movl	$0, %edi
.l3:
	movl	%eax, (%esi, %edi)
	addl	$0x1000, %eax
	addl	$4, %edi
	cmpl	$4096, %edi
	jl	.l3

	/*
	 * Initialize the page table that maps kernel memory
	 */
	movl	$V2P(kernel_pagetable), %esi
	movl	$0x200007, %eax
	movl	$0, %edi
.l4:
	movl	%eax, (%esi, %edi)
	addl	$0x1000, %eax
	addl	$4, %edi
	cmpl	$6144, %edi
	jl	.l4
	
#ifdef  MP

	/*
	 * Initialize the page table that maps the APIC register address space
	 */
	 
	/* 
	 * FIXME: APIC register address space can be non-standard so do the 
	 * mapping later 
	 */
	movl	$V2P(apic_pagetable), %esi
	movl	$0, %edi
	movl	$0xFEC0001B, %eax
	movl	%eax, (%esi, %edi)
	movl	$0x800, %edi
	movl	$0xFEE0001B, %eax
	movl	%eax, (%esi, %edi)

#endif /* MP */

	/*
	 * Initialize the page table that maps the initial KPCR (at FF000000)
	 */
	movl	$V2P(kpcr_pagetable), %esi
	movl	$0, %edi
	movl	$0x1007, %eax
	movl	%eax, (%esi, %edi)

	/*
	 * Initialize a part of the same pagetable to map the vga frame buffer.
	 */
	movl	$0xa0007, %eax
	movl	$0x20, %ecx
	movl	$0xE80, %edi
.l9:	
	movl	%eax, (%esi, %edi)
	add	$4, %edi
	add	$0x1000, %eax
	loop	.l9

#ifdef MP

.m1:

#endif /* MP */

	/*
	 * Set up the PDBR
	 */	
	movl	$(V2P(startup_pagedirectory)), %eax
	movl	%eax, %cr3

	/*
	 * Enable paging and set write protect
	 */
	movl	%cr0, %eax
	orl	$0x80010000, %eax
	movl	%eax, %cr0

	/*
	 * Do an absolute jump because we now want to execute above 0xc0000000
	 */	
	movl	$.l2, %eax
	jmp	*%eax
.l2:
	
	/*
	 * Load the GDTR and IDTR with new tables located above
	 * 0xc0000000
	 */
	
	/* FIXME: Application processors should have their own GDT/IDT */
	lgdt	_KiGdtDescriptor
	lidt	_KiIdtDescriptor

	/*
	 * Reload the data segment registers 
	 */
	movl	$KERNEL_DS, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %gs
	movl	%eax, %ss
	movl	$0, %eax
	movl	%eax, %fs

#ifdef MP

	cmpl    $AP_MAGIC, %edx
	jne     .m2

	/*
	 * This is an application processor executing
	 */

	/*
	 * Initialize EFLAGS
	 */
	pushl	$0
	popfl

	/*
	 * Call the application processor initialization code
	 */
	pushl	$0
	pushl	$.l7
	pushl	$KERNEL_CS
	pushl	$_KiSystemStartup
	lret

	/*
	 * Catch illegal returns from KiSystemStartup
	 */
.l7:
	popl	%eax	
	pushl	$0
	call	_KeBugCheck@4
	popl	%eax
.l8:
	jmp	.l8

	
.m2:

#endif /* MP */

	/*
	 * Load the PCR selector
	 */
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs

	/*
	 * Load the initial kernel stack
	 */
	movl	$_init_stack_top, %esp

	/*
	 * Initialize EFLAGS
	 */
	pushl	$0
	popfl

	/*
	 * Call the main kernel initialization
	 */
	movl	$0, %ebp
	pushl	%ebx
	pushl	%edx
	pushl	$.l5
	pushl	$KERNEL_CS
	pushl	$__main
	lret
	
	/*
	 * Catch illegal returns from main, try bug checking the system,
	 * if that fails then loop forever.
	 */
.l5:
	popl	%eax
	popl	%eax
	pushl	$0
	call	_KeBugCheck@4
	popl	%eax
.l6:
	jmp	.l6

	
	/*
	 * This needs to be page aligned so put it at the beginning of the bss
	 * segment
	 */
.bss
startup_pagedirectory:
	.fill 4096, 1, 0

lowmem_pagetable:
	.fill 4096, 1, 0

kernel_pagetable:
	.fill 32*4096, 1, 0	

#ifdef MP
apic_pagetable:
	.fill 4096, 1, 0	
#endif /* MP */

kpcr_pagetable:
	.fill 4096, 1, 0	

_unmap_me:
	.fill 4096, 1, 0
	
_init_stack:
	.fill 3*4096, 1, 0
_init_stack_top: 

_unmap_me2:	
	.fill 4096, 1, 0
	
_trap_stack:	
	.fill 3*4096, 1, 0
_trap_stack_top:	
	
_unmap_me3:	
	.fill 4096, 1, 0			
