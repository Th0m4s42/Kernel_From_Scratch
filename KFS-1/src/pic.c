#include <pic.h>

void pic_init(void)
{
	outb(PIC1_CMD, 0x11);
	io_wait();
	outb(PIC2_CMD, 0x11);
	io_wait();

	outb(PIC1_DATA, 0x20);
	io_wait();
	outb(PIC2_DATA, 0x28);
	io_wait();

	outb(PIC1_DATA, 0x04);
	io_wait();
	outb(PIC2_DATA, 0x02);
	io_wait();

	outb(PIC1_DATA, 0xFD);
	outb(PIC2_DATA, 0xFF);
}

void pic_eoi(uint8_t irq)
{
	if (irq >= 8)
		outb(PIC2_CMD, 0x20);
	outb(PIC1_CMD, 0x20);
}
