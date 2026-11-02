bits 32

section .text

global	gdt_flush
global	idt_load
global	irq1_stub
global	irq0_stub
extern	keyboard_handler

; void gdt_flush(uint32_t gdt_ptr)
gdt_flush:
	mov		eax, [esp + 4]
	lgdt	[eax]
	mov		ax, 0x10		; segment data
	mov		ds, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax
	mov		ss, ax
	jmp		0x08:.reload_cs	; far jump pour recharger CS
.reload_cs:
	ret

; void idt_load(uint32_t idt_ptr)
idt_load:
	mov		eax, [esp + 4]
	lidt	[eax]
	ret

; Stub IRQ1: sauvegarde les registres, appelle le C, revient avec iret
irq1_stub:
	pushad
	cld						; l'ABI C suppose DF = 0
	call keyboard_handler
	popad
	iretd

irq0_stub:
    push eax
    mov al, 0x20
    out 0x20, al      ; EOI
    pop eax
    iret
