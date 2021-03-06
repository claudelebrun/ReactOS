; * 
; * COPYRIGHT:       See COPYING in the top level directory
; * PROJECT:         ReactOS kernel
; * FILE:            i386_RtlZeroMemory.asm
; * PURPOSE:         Memory functions
; * PROGRAMMER:      Magnus Olsen (magnusolsen@greatlord.com)
; * UPDATE HISTORY:
; *                  Created 20/07-2003
; * 


	  BITS 32
        GLOBAL _RtlZeroMemory@8         ; (no bug) (max optimze code)       
	  SECTION .text

; * 
; * [1] VOID STDCALL RtlZeroMemory (PVOID Destination, ULONG Length); 
; * 
                   
_RtlZeroMemory@8:           
         
        mov ecx,dword [esp + 8 ]               ; Length                                             
        cmp ecx,0                              ; if (Length==0) goto .zero  
	  je  .zero	        

        pushad                                 ; Save all register on the stack      
        mov edi, dword [esp + (4 + 32)]        ; Destination           
        xor eax,eax                            ; ZeroFillByte = 0
        
       
   
; code for take four byte each time it loop
      mov ebx,ecx                              ; temp_Length = Length              
      shr ecx,2                                ; Length = Length / sizeof(ULONG)      
      jz .1byte                                ; if (Length==0) goto .1byte
       
      shl ecx,2                                ; Length = Length * sizeof(ULONG)  
      sub ebx,ecx                              ; temp_Length = temp_Length - Length; 
      jz .4bytes                               ; if (temp_Length==0) goto .4byte

; move 4byte and 1byte
      shr ecx,2                                ; Length = Length / sizeof(ULONG)      
      cld                                      ; clear d flag 
      rep stosd                                ; while (Length!=0) { (ULONG *) Destination[Length-1]=ZeroFillByte; Legnth = Legnth - 1 }            
      mov ecx,ebx                              ; Length = temp_Length 
      rep stosb                                ; while (Length!=0) { (UCHAR *) Destination[Length-1]=ZeroFillByte; Legnth = Legnth - 1 }            
      popad                                    ; restore register 
      ret 8                                    ; return  
      
; move 1byte            
.1byte:   
       mov ecx,dword [esp + (12 +32) ]        ; Length                                               
       cld                                    ; clear d flag 
       rep stosb                              ; while (Length!=0) { (UCHAR *) Destination[Length-1]=ZeroFillByte; Legnth = Legnth - 1 }
       popad                                  ; restore register
       ret 8                                  ; return

; move 4bytes     
.4bytes:
       shr ecx,2                              ; Length = Length / sizeof(ULONG)      
       cld                                    ; clear d flag 
       rep stosd                              ; while (Length!=0) { (ULONG *) Destination[Length-1]=ZeroFillByte; Legnth = Legnth - 1 } 
       popad                                  ; restore register 
.zero:
       ret 8                                  ; return
