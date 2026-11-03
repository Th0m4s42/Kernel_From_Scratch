#include <stddef.h>
#include <stdint.h>
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <io.h>
#include <keyboard.h>
#include <terminal.h>

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This kernel needs to be compiled with a ix86-elf compiler"
#endif

void kernel_main(void)
{
	terminal_initialize();

	/* Message de bienvenue sur chaque terminal */
	for (size_t i = 0; i < NB_TERMINALS; i++) {
		terminal_switch(i);
		terminal_writestring("Hello, 42! - terminal ");
		terminal_putchar('1' + i);
		terminal_writestring("\n> ");
	}
	terminal_switch(0);

	gdt_init();
	idt_init();                              /* d'abord l'IDT...            */
	idt_set_gate(32, (uint32_t)irq0_stub);   /* ...puis les gates           */
	pic_init();
	__asm__ volatile ("sti");

	while (1) {
		int key = keyboard_getkey();

		switch (key) {
		case 0:				__asm__ volatile ("hlt");	break;
		case KEY_F1:		terminal_switch(0);			break;
		case KEY_F2:		terminal_switch(1);			break;
		case KEY_F3:		terminal_switch(2);			break;
		case KEY_F4:		terminal_switch(3);			break;
		case KEY_F5:		terminal_next_color();		break;
		case KEY_UP:		terminal_move_cursor(0, -1);break;
		case KEY_DOWN:		terminal_move_cursor(0, 1);	break;
		case KEY_LEFT:		terminal_move_cursor(-1, 0);break;
		case KEY_RIGHT:		terminal_move_cursor(1, 0);	break;
		case KEY_PGUP:		terminal_scroll_view(VGA_HEIGHT - 1);	break;
		case KEY_PGDOWN:	terminal_scroll_view(-(VGA_HEIGHT - 1));break;
		default:
			if (key < 0x100)
				terminal_putchar((char)key);
		}
	}
}
