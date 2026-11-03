#include <terminal.h>
#include <io.h>

typedef struct {
	uint16_t lines[HISTORY_LINES][VGA_WIDTH];
	size_t   nb_lines;   /* lignes utilisées (toujours >= VGA_HEIGHT) */
	size_t   row;        /* curseur, relatif à l'écran vivant         */
	size_t   column;
	size_t   scroll;     /* 0 = en bas, sinon nb de lignes remontées  */
	uint8_t  color;
} terminal_t;

static terminal_t         terminals[NB_TERMINALS];
static terminal_t        *current;
static volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;

static const uint8_t palette[] = {
	0x07, 0x0F, 0x0A, 0x0B, 0x0C, 0x0E, 0x1F, 0x4F,
};
#define PALETTE_SIZE (sizeof(palette) / sizeof(palette[0]))

/* ------------------------------------------------------------------ */

size_t strlen(const char *str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

static inline uint16_t vga_entry(unsigned char c, uint8_t color)
{
	return (uint16_t)c | ((uint16_t)color << 8);
}

/* Ligne y (0..24) de l'écran vivant */
static uint16_t *screen_line(terminal_t *t, size_t y)
{
	return t->lines[t->nb_lines - VGA_HEIGHT + y];
}

/* Vrai si ce terminal est affiché ET en bas de l'historique */
static int is_live(terminal_t *t)
{
	return t == current && t->scroll == 0;
}

static void clear_line(uint16_t *line, uint8_t color)
{
	for (size_t x = 0; x < VGA_WIDTH; x++)
		line[x] = vga_entry(' ', color);
}

static void update_cursor(void)
{
	uint16_t pos;

	if (current->scroll)            /* on regarde l'historique : curseur caché */
		pos = VGA_WIDTH * VGA_HEIGHT;
	else
		pos = current->row * VGA_WIDTH + current->column;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* Affiche la portion visible du terminal actif */
static void flush(void)
{
	size_t start = current->nb_lines - VGA_HEIGHT - current->scroll;

	for (size_t y = 0; y < VGA_HEIGHT; y++)
		for (size_t x = 0; x < VGA_WIDTH; x++)
			vga[y * VGA_WIDTH + x] = current->lines[start + y][x];
	update_cursor();
}

static void put_at(terminal_t *t, size_t x, size_t y, uint16_t entry)
{
	screen_line(t, y)[x] = entry;
	if (is_live(t))
		vga[y * VGA_WIDTH + x] = entry;
}

/* Ajoute une ligne vide en bas ; la plus ancienne disparaît si l'historique est plein */
static void push_line(terminal_t *t)
{
	if (t->nb_lines < HISTORY_LINES)
		t->nb_lines++;
	else
		for (size_t i = 0; i < HISTORY_LINES - 1; i++)
			for (size_t x = 0; x < VGA_WIDTH; x++)
				t->lines[i][x] = t->lines[i + 1][x];

	clear_line(t->lines[t->nb_lines - 1], t->color);
	if (t == current)
		flush();
}

static void newline(terminal_t *t)
{
	t->column = 0;
	if (t->row < VGA_HEIGHT - 1)
		t->row++;
	else
		push_line(t);
}

/* Si on regardait l'historique, revient en bas */
static void back_to_live(terminal_t *t)
{
	if (t->scroll) {
		t->scroll = 0;
		flush();
	}
}

/* ------------------------------------------------------------------ */

void terminal_initialize(void)
{
	for (size_t n = 0; n < NB_TERMINALS; n++) {
		terminal_t *t = &terminals[n];

		t->row = 0;
		t->column = 0;
		t->scroll = 0;
		t->color = palette[0];
		t->nb_lines = VGA_HEIGHT;
		for (size_t y = 0; y < HISTORY_LINES; y++)
			clear_line(t->lines[y], t->color);
	}
	current = &terminals[0];
	flush();
}

void terminal_switch(size_t id)
{
	if (id >= NB_TERMINALS || current == &terminals[id])
		return;
	current = &terminals[id];
	flush();
}

void terminal_setcolor(uint8_t color)
{
	current->color = color;
}

void terminal_next_color(void)
{
	size_t i = 0;

	while (i < PALETTE_SIZE && palette[i] != current->color)
		i++;
	current->color = palette[(i + 1) % PALETTE_SIZE];

	for (size_t y = 0; y < current->nb_lines; y++)
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			unsigned char c = current->lines[y][x] & 0xFF;
			current->lines[y][x] = vga_entry(c, current->color);
		}
	flush();
}

/* Flèches : déplace le curseur dans l'écran vivant, avec retour à la ligne */
void terminal_move_cursor(int dx, int dy)
{
	terminal_t *t = current;
	int col = (int)t->column + dx;
	int row = (int)t->row + dy;

	back_to_live(t);

	if (col < 0) {                       /* ← en début de ligne : ligne précédente */
		if (row > 0) { row--; col = VGA_WIDTH - 1; }
		else col = 0;
	} else if (col >= VGA_WIDTH) {       /* → en fin de ligne : ligne suivante */
		if (row < VGA_HEIGHT - 1) { row++; col = 0; }
		else col = VGA_WIDTH - 1;
	}

	if (row < 0)               row = 0;
	if (row >= VGA_HEIGHT)     row = VGA_HEIGHT - 1;

	t->column = (size_t)col;
	t->row = (size_t)row;
	update_cursor();
}

/* Page Up / Page Down : défile dans l'historique */
void terminal_scroll_view(int delta)
{
	terminal_t *t = current;
	int max = (int)(t->nb_lines - VGA_HEIGHT);
	int s = (int)t->scroll + delta;

	if (s < 0)   s = 0;
	if (s > max) s = max;
	if ((size_t)s == t->scroll)
		return;
	t->scroll = (size_t)s;
	flush();
}

void terminal_putchar(char c)
{
	terminal_t *t = current;

	back_to_live(t);   /* taper une touche ramène en bas */

	switch (c) {
	case '\n':
		newline(t);
		break;

	case '\r':
		t->column = 0;
		break;

	case '\b':
		if (t->column > 0)
			t->column--;
		else if (t->row > 0) {
			t->row--;
			t->column = VGA_WIDTH - 1;
		}
		put_at(t, t->column, t->row, vga_entry(' ', t->color));
		break;

	case '\t':
		t->column = (t->column + 4) & ~(size_t)3;
		if (t->column >= VGA_WIDTH)
			newline(t);
		break;

	default:
		put_at(t, t->column, t->row, vga_entry((unsigned char)c, t->color));
		if (++t->column == VGA_WIDTH)
			newline(t);
		break;
	}
	update_cursor();
}

void terminal_write(const char *data, size_t size)
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char *data)
{
	terminal_write(data, strlen(data));
}
