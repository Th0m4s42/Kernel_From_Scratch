#ifndef KEYBOARD_H
# define KEYBOARD_H

# include <io.h>
# include <pic.h>

# define KBD_DATA 0x60
# define BUF_SIZE 64

enum {
	KEY_UP = 0x100, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_PGUP, KEY_PGDOWN,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4,
	KEY_F5, KEY_F6, KEY_F7, KEY_F8,
	KEY_F9, KEY_F10, KEY_F11, KEY_F12,
};

void	keyboard_handler(void);	/* appelé par irql_stub */
int		keyboard_getkey(void);	/* bloquant (dort avec hit) */

#endif