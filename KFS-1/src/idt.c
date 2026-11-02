#include <idt.h>

static t_idt_entry idt[256];
static t_idt_ptr	ip;

void idt_set_gate(uint8_t num, uint32_t handler)
{
	idt[num].offset_low		= handler & 0x0FFFF;
	idt[num].offset_high	= (handler >> 16) & 0x0FFFF;
	idt[num].selector		= 0x08;
	idt[num].zero			= 0;
	idt[num].type_attr		= 0x8E;
}

void idt_init(void)
{
	ip.limit	= sizeof(idt) - 1;
	ip.base		= (uint32_t)&idt;

	idt_set_gate(33, (uint32_t)irq1_stub);

	idt_load((uint32_t)&ip);
}
