#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <io.h>
#include <keyboard.h>

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This kernel needs to be compiled with a ix86-elf compiler"
#endif

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000 

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

static void terminal_update_cursor(void)
{
	uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	
	for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
		terminal_buffer[i] = vga_entry(' ', terminal_color);
	terminal_update_cursor();
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}


static void terminal_scroll(void)
{
	for (size_t y = 1; y < VGA_HEIGHT; y++)
	{
		for(size_t x = 0; x < VGA_WIDTH; x++)
			terminal_buffer[(y - 1) * VGA_WIDTH + 1] = vga_entry(' ', terminal_color);
	}

	for (size_t x = 0; x < VGA_WIDTH; x++)
		terminal_buffer[(VGA_HEIGHT -1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
	terminal_row = VGA_HEIGHT - 1;
}

static void terminal_newline(void)
{
	terminal_column = 0;
	if (++terminal_row == VGA_HEIGHT)
		terminal_scroll();
}

void terminal_putchar(char c) 
{
	switch (c)
	{
	case '\n':
		terminal_newline();
		break;

	case '\r':
		terminal_column = 0;
		break;

	case '\b':      /* Backspace : recule et efface */
		if (terminal_column > 0)
			terminal_column--;
		else if (terminal_row > 0)
		{
			terminal_row--;
			terminal_column = VGA_WIDTH - 1;
		}
		terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] =
			vga_entry(' ', terminal_color);
		break;

	case '\t':      /* Tabulation : aligne sur un multiple de 4 */
		terminal_column = (terminal_column + 4) & ~(size_t)3;
		if (terminal_column >= VGA_WIDTH)
			terminal_newline();
		break;

	default:        /* Caractère imprimable */
		terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] =
			vga_entry((unsigned char)c, terminal_color);
		if (++terminal_column == VGA_WIDTH)
			terminal_newline();
		break;
	}
	terminal_update_cursor();
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}

void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();
	
	gdt_init();	
	pic_init();	
	idt_set_gate(32, (uint32_t)irq0_stub);
	idt_init();
	__asm__ volatile("sti");
	
	terminal_writestring("Hello, 42!\n");

	while (1)
	{
		int key = keyboard_getkey();

		switch (key)
		{
			case KEY_F1:     terminal_setcolor(0x0F); break;
			case KEY_F2:     terminal_setcolor(0x0A); break;
			case KEY_F3:     terminal_setcolor(0x0C); break;
			case KEY_F4:     terminal_setcolor(0x0E); break;
			default:
				if (key < 0x100)
					terminal_putchar((char)key);
		}
	}
}
