#include <keyboard.h>

static const char normal[128] = {
	0, 27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
	'\t','q','w','e','r','t','y','i','o','p','[',']','\n',
	0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
	0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
	'*', 0, ' '
};

static const char shifted[128] = {
	0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
	'\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
	0, 'A','S','D','F','G','H','J','K','L',':','"','~',
	0, '|','Z','X','C','V','B','N','M','<','>','?', 0,
	'*', 0, ' '
};

/* Buffer circulaire partagé entre l'IRQ et le kernel -> volatile */
static volatile int		buffer[BUF_SIZE];
static volatile uint8_t	head = 0, tail = 0;

static int shift		= 0;
static int caps			= 0;
static int extended		= 0;

static void push_key(int key)
{
	uint8_t next = (head + 1) % BUF_SIZE;
	if (next != tail) {
		buffer[head] = key;
		head = next;
	}
}

static int translate(uint8_t sc)
{
	if (extended) {
		extended = 0;
		switch (sc)
		{
			case 0x48:
				return KEY_UP;
			case 0x50:
				return KEY_DOWN;
			case 0x4B:
				return KEY_LEFT;
			case 0x4D:
				return KEY_RIGHT;
			case 0x49:
				return KEY_PGUP;
			case 0x51:
				return KEY_PGDOWN;
			default:
				return 0;
		}
	}
	switch (sc)
	{
	case 0x3B:
		return KEY_F1;
	case 0x3C:
		return KEY_F2;
	case 0x3D:
		return KEY_F3;
	case 0x3E:
		return KEY_F4;
	}

	char c = shift ? shifted[sc] : normal[sc];
    if (caps && c >= 'a' && c <= 'z') c -= 32;
    else if (caps && c >= 'A' && c <= 'Z') c += 32;
    return c;
}

void keyboard_handler(void)
{
	uint8_t sc = inb(KBD_DATA);

	if (sc == 0xE0)
	{
		extended = 1;
	}
	else if (sc & 0x80)
	{
		uint8_t code = sc & 0x7F;
		if (code == 0x2A ||  code == 0x36)
			shift = 0;
		extended  = 0;
	}
	else if (sc == 0x2A || sc == 0x36)
	{
		shift = 1;
	}
	else if (sc == 0x3A)
	{
		caps = !caps;
	}
	else
	{
		int key = translate(sc);
		if (key)
			push_key(key);
	}

	pic_eoi(1);
}

int keyboard_getkey(void)
{
	for (;;)
	{
		__asm__ volatile ("cli");
		if (head != tail)
		{
			int key = buffer[tail];
			tail = (tail + 1) % BUF_SIZE;
			__asm__ volatile ("sti");
			return key;
		}
		__asm__ volatile ("sti; hlt");
	}
}
