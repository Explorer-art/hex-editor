#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"

#define VERSION "0.1"
#define MAX_COLUMNS 16
#define KEYBOARD_UP 3
#define KEYBOARD_DOWN 2
#define KEYBOARD_LEFT 4
#define KEYBOARD_RIGHT 5

size_t size = 0;
int start_line = 0;
int cursor_x = 11;
int cursor_y = 0;
int current_byte = 0;
struct winsize wsize;
Configuration config;

int is_ascii(unsigned char c) {
    return (c >= 32 && c <= 126);
}

void scroll_up() {
	// Проверка для доп. безопасности
	if (current_byte < MAX_COLUMNS) return;

	if (cursor_y > 0) { // Прыгаем вверх если y > 0
		cursor_y--;
		current_byte -= MAX_COLUMNS;
	} else if (start_line > 0) { // Прыгаем вверх если сверху есть ещё строка
		start_line--;
		current_byte -= MAX_COLUMNS;
	}
}

void scroll_down() {
	// Проверка для доп. безопасности
	if (current_byte + MAX_COLUMNS >= size) return;

	// Прыгаем вниз если есть строка
	if (start_line + wsize.ws_row < size / MAX_COLUMNS) {
		if (cursor_y < wsize.ws_row - 1) {
			cursor_y++;
			current_byte += MAX_COLUMNS;
		} else {
			start_line++;
			current_byte += MAX_COLUMNS;
		}
	}
}

void scroll_left() {
	// Проверка для доп. безопасности
	if (current_byte == 0) return;

	if (cursor_x > 11) { // Перемещаемся влево если это не конец строки
		cursor_x -= 3;
		current_byte--;
	} else if (cursor_y > 0) { // Перемещаемся вверх и вправо если y > 0
		cursor_y--;
		current_byte--;
		cursor_x = MAX_COLUMNS * 3 + 8;
	} else if (start_line > 0) { // Перемещаемся вверх и вправо если сверху есть ещё строка
		start_line--;
		current_byte--;
		cursor_x = MAX_COLUMNS * 3 + 8;
	}
}

void scroll_right() {
	// Проверка для доп. безопасности
	if (current_byte + 1 >= size) return;

	if (cursor_x < MAX_COLUMNS * 3 + 8) { // Перемещаемся вправо если x < макс. столбцов
		if (cursor_x < size * 3 + 7) {
			cursor_x += 3;
			current_byte++;
		}
	} else if (start_line + wsize.ws_row < size / MAX_COLUMNS) { // Перемещаемся вниз и влево если снизу есть строка
		if (cursor_y < wsize.ws_row - 1) {
			cursor_y++;
			current_byte++;
		} else {
			start_line++;
			current_byte++;
		}

		cursor_x = 11;
	}
}

int main(int argc, char* argv[]) {
	init_config(&config);

	if (argc < 2) {
		printf("Usage: %s [file]\n", argv[0]);
		exit(1);
	}

	if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
		printf("Version: HEX Editor %s\n", VERSION);
		exit(0);
	}

	FILE* fp = fopen(argv[1], "rb");

	if (fp == NULL) {
		fprintf(stderr, "Error opening file %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}

	// Узнаем размер файла
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Выделяем память для данных
	unsigned char* data = (unsigned char*) malloc(size);

	fread(data, 1, size, fp);

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	if (config.use_colors) {
		start_color();
		init_pair(1, config.fg_color, config.bg_color);
	}

	int x, y;

	if (config.use_colors)
		attron(COLOR_PAIR(1));

	// Основной цикл
	while (true) {
		clear();

		mvprintw(0, 100, "%d", current_byte);

		for (size_t i = MAX_COLUMNS * start_line; i < (start_line + wsize.ws_row) * MAX_COLUMNS && i < size; i++) {
			x = i % MAX_COLUMNS;
			y = i / MAX_COLUMNS - start_line;

			if (x == 0) {
				mvprintw(y, x, "%08lX: ", i + MAX_COLUMNS);
			}

			mvprintw(y, x * 3 + 10, "%02X", data[i]);

			if (x == MAX_COLUMNS - 1) {
				int offset = x * 3 + 14;

				for (size_t j = i - (MAX_COLUMNS - 1); j <= i; j++) {
					if (is_ascii(data[j])) {
						mvprintw(y, offset, "%c", data[j]);
					} else {
						mvprintw(y, offset, ".");
					}

					offset++;
				}
			}
		}

		refresh();

		move(cursor_y, cursor_x);

		char c = getch();

		if (c == 'q') {
			break;
		} else if (c == KEYBOARD_UP) {
			scroll_up();
		} else if (c == KEYBOARD_DOWN) {
			scroll_down();
		} else if (c == KEYBOARD_LEFT) {
			scroll_left();
		} else if (c == KEYBOARD_RIGHT) {
			scroll_right();
		}
	}

	if (config.use_colors)
		attroff(COLOR_PAIR(1));

	free(data);
	endwin();

	return 0;
}