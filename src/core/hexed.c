#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <core/hexed.h>
#include <utils/utils.h>

#define MAX_COLUMNS 	16
#define KEYBOARD_UP 	3
#define KEYBOARD_DOWN 	2
#define KEYBOARD_LEFT 	4
#define KEYBOARD_RIGHT 	5
#define KEYBOARD_BACK	7

static Hexed hexed;

void scroll_up(Hexed* hexed) {
	// Проверка для доп. безопасности
	if (hexed->current_byte < MAX_COLUMNS) return;

	if (hexed->cursor_y > 0) { // Прыгаем вверх если y > 0
		hexed->cursor_y--;
		hexed->current_byte -= MAX_COLUMNS;
	} else if (hexed->start_line > 0) { // Прыгаем вверх если сверху есть ещё строка
		hexed->start_line--;
		hexed->current_byte -= MAX_COLUMNS;
	}
}

void scroll_down(Hexed* hexed) {
	// Проверка для доп. безопасности
	if (hexed->current_byte + MAX_COLUMNS >= hexed->size) return;

	// Прыгаем вниз если есть строка
	if (hexed->start_line + hexed->wsize.ws_row - 1 < hexed->size / MAX_COLUMNS) {
		if (hexed->cursor_y < hexed->wsize.ws_row - 2) { // -2 потому что у нас 1 строчка внизу занята под информацию
			hexed->cursor_y++;
			hexed->current_byte += MAX_COLUMNS;
		} else {
			hexed->start_line++;
			hexed->current_byte += MAX_COLUMNS;
		}
	}
}

void scroll_left(Hexed* hexed) {
	// Проверка для доп. безопасности
	if (hexed->current_byte == 0) return;

	if (hexed->cursor_x > 12) { // Перемещаемся влево если это не конец строки
		hexed->cursor_x -= 3;
		hexed->current_byte--;
	} else if (hexed->cursor_y > 0) { // Перемещаемся вверх и вправо если y > 0
		hexed->cursor_y--;
		hexed->current_byte--;
		hexed->cursor_x = MAX_COLUMNS * 3 + 8;
	} else if (hexed->start_line > 0) { // Перемещаемся вверх и вправо если сверху есть ещё строка
		hexed->start_line--;
		hexed->current_byte--;
		hexed->cursor_x = MAX_COLUMNS * 3 + 8;
	}
}

void scroll_right(Hexed* hexed) {
	// Проверка для доп. безопасности
	if (hexed->current_byte + 1 >= hexed->size) return;

	if (hexed->cursor_x < MAX_COLUMNS * 3 + 8) { // Перемещаемся вправо если x < макс. столбцов
		if (hexed->cursor_x < hexed->size * 3 + 7) {
			hexed->cursor_x += 3;
			hexed->current_byte++;
		}
	} else if (hexed->start_line + hexed->wsize.ws_row < hexed->size / MAX_COLUMNS) { // Перемещаемся вниз и влево если снизу есть строка
		if (hexed->cursor_y < hexed->wsize.ws_row - 1) {
			hexed->cursor_y++;
			hexed->current_byte++;
		} else {
			hexed->start_line++;
			hexed->current_byte++;
		}

		hexed->cursor_x = 11;
	}
}

void render(Hexed* hexed) {
	int x, y;

	for (size_t i = MAX_COLUMNS * hexed->start_line; i < (hexed->start_line + hexed->wsize.ws_row - 1) * MAX_COLUMNS && i < hexed->size; i++) {
		x = i % MAX_COLUMNS;
		y = i / MAX_COLUMNS - hexed->start_line;

		int group_size = (hexed->config->octets <= 1) ? 0 : (MAX_COLUMNS / hexed->config->octets);
		int space = (group_size > 0) ? (x / group_size) : 0;
		int offset = x * 3 + 11 + space;

		if (x == 0)
			mvprintw(y, x, "%08lX: ", i + MAX_COLUMNS);

		mvprintw(y, offset, "%02X", hexed->data[i]);

		if (x == MAX_COLUMNS - 1) {
			int offset = x * 3 + 15 + space;

			for (size_t j = i - (MAX_COLUMNS - 1); j <= i; j++) {
				if (is_ascii(hexed->data[j])) {
					mvprintw(y, offset, "%c", hexed->data[j]);
				} else {
					mvprintw(y, offset, ".");
				}

				offset++;
			}
		}
	}

	if (hexed->mode == READ_MODE)
		mvprintw(hexed->wsize.ws_row - 1, 0, "READ MODE");
	else if (hexed->mode == INSERT_MODE)
		mvprintw(hexed->wsize.ws_row - 1, 0, "INSERT MODE");
}

void read_mode_event_handler(Hexed* hexed, char c) {
	if (c == KEYBOARD_UP) {
		scroll_up(hexed);
	} else if (c == KEYBOARD_DOWN) {
		scroll_down(hexed);
	} else if (c == KEYBOARD_LEFT) {
		scroll_left(hexed);
	} else if (c == KEYBOARD_RIGHT) {
		scroll_right(hexed);
	}
}

void insert_mode_event_handler(Hexed* hexed, char c) {
	if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || c == KEYBOARD_BACK) {
		snprintf(hexed->value_buffer, sizeof(hexed->value_buffer), "%02X", hexed->data[hexed->current_byte]);

		hexed->value_buffer[0] = hexed->value_buffer[1];

		if (c == KEYBOARD_BACK) {
			hexed->value_buffer[1] = '0';
		} else {
			hexed->value_buffer[1] = c;
		}

		int value = 0;
		sscanf(hexed->value_buffer, "%x", &value);
		hexed->data[hexed->current_byte] = value;
	} else if (c == KEYBOARD_UP) {
		scroll_up(hexed);
	} else if (c == KEYBOARD_DOWN) {
		scroll_down(hexed);
	} else if (c == KEYBOARD_LEFT) {
		scroll_left(hexed);
	} else if (c == KEYBOARD_RIGHT) {
		scroll_right(hexed);
	}
}

Hexed* hexed_init(void) {
	hexed.config = get_config();
	hexed.cursor_x = 11;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &hexed.wsize);

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	hexed.mode = hexed.config->default_mode;

	if (hexed.config->use_colors) {
		start_color();
		init_pair(1, hexed.config->fg_color, hexed.config->bg_color);
		attron(COLOR_PAIR(1));
	}

	return &hexed;
}

void hexed_open(Hexed* hexed, char* filename) {
	hexed->fp = fopen(filename, "rb");

	if (hexed->fp == NULL) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		exit(1);
	}

	// Узнаем размер файла
	fseek(hexed->fp, 0, SEEK_END);
	hexed->size = ftell(hexed->fp);
	fseek(hexed->fp, 0, SEEK_SET);

	// Выделяем память для данных
	hexed->data = (unsigned char*) malloc(hexed->size);

	fread(hexed->data, 1, hexed->size, hexed->fp);
}

void hexed_close(Hexed* hexed, FILE* fp) {
	free(hexed->data);
	fclose(fp);
}

void hexed_exit(void) {
	if (hexed.config->use_colors)
		attroff(COLOR_PAIR(1));

	endwin();
}