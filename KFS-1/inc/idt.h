#ifndef IDT_H
#define IDT_H

# include <stdint.h>

struct s_idt_entry
{
	uint16_t    offset_low;
	uint16_t    selector;
	uint8_t     zero;
	uint8_t     type_attr;
	uint16_t    offset_high;
}   __attribute__((packed));

typedef struct s_idt_entry t_idt_entry;

struct s_idt_ptr
{
	uint16_t limit;
	uint32_t base;
}	__attribute__((packed));

typedef struct s_idt_ptr t_idt_ptr;

extern void idt_load(uint32_t idt_ptr_addr);	/* interrupts.asm */
extern void irq0_stub(void);					/* interrupts.asm */
extern void irq1_stub(void);					/* interrupts.asm */
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler);

#endif