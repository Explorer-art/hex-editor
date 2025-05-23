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

void scroll_up(Hexed* hexed, int file_index) {
	// Проверка для доп. безопасности
	if (hexed->files[file_index].current_byte < MAX_COLUMNS) return;

	if (hexed->files[file_index].cursor_y > 0) { // Прыгаем вверх если y > 0
		hexed->files[file_index].cursor_y--;
		hexed->files[file_index].current_byte -= MAX_COLUMNS;
	} else if (hexed->files[file_index].start_line > 0) { // Прыгаем вверх если сверху есть ещё строка
		hexed->files[file_index].start_line--;
		hexed->files[file_index].current_byte -= MAX_COLUMNS;
	}
}

void scroll_down(Hexed* hexed, int file_index) {
	// Проверка для доп. безопасности
	if (hexed->files[file_index].current_byte + MAX_COLUMNS >= hexed->files[file_index].size) return;

	// Прыгаем вниз если есть строка
	if (hexed->files[file_index].start_line + hexed->wsize.ws_row - 1 < hexed->files[file_index].size / MAX_COLUMNS) {
		if (hexed->files[file_index].cursor_y < hexed->wsize.ws_row - 2) { // -2 потому что у нас 1 строчка внизу занята под информацию
			hexed->files[file_index].cursor_y++;
			hexed->files[file_index].current_byte += MAX_COLUMNS;
		} else {
			hexed->files[file_index].start_line++;
			hexed->files[file_index].current_byte += MAX_COLUMNS;
		}
	}
}

void scroll_left(Hexed* hexed, int file_index) {
	// Проверка для доп. безопасности
	if (hexed->files[file_index].current_byte == 0) return;

	if (hexed->files[file_index].cursor_x > 12) { // Перемещаемся влево если это не конец строки
		hexed->files[file_index].cursor_x -= 3;
		hexed->files[file_index].current_byte--;
	} else if (hexed->files[file_index].cursor_y > 0) { // Перемещаемся вверх и вправо если y > 0
		hexed->files[file_index].cursor_y--;
		hexed->files[file_index].current_byte--;
		hexed->files[file_index].cursor_x = MAX_COLUMNS * 3 + 8;
	} else if (hexed->files[file_index].start_line > 0) { // Перемещаемся вверх и вправо если сверху есть ещё строка
		hexed->files[file_index].start_line--;
		hexed->files[file_index].current_byte--;
		hexed->files[file_index].cursor_x = MAX_COLUMNS * 3 + 8;
	}
}

void scroll_right(Hexed* hexed, int file_index) {
	// Проверка для доп. безопасности
	if (hexed->files[file_index].current_byte + 1 >= hexed->files[file_index].size) return;

	if (hexed->files[file_index].cursor_x < MAX_COLUMNS * 3 + 8) { // Перемещаемся вправо если x < макс. столбцов
		if (hexed->files[file_index].cursor_x < hexed->files[file_index].size * 3 + 7) {
			hexed->files[file_index].cursor_x += 3;
			hexed->files[file_index].current_byte++;
		}
	} else if (hexed->files[file_index].start_line + hexed->wsize.ws_row < hexed->files[file_index].size / MAX_COLUMNS) { // Перемещаемся вниз и влево если снизу есть строка
		if (hexed->files[file_index].cursor_y < hexed->wsize.ws_row - 1) {
			hexed->files[file_index].cursor_y++;
			hexed->files[file_index].current_byte++;
		} else {
			hexed->files[file_index].start_line++;
			hexed->files[file_index].current_byte++;
		}

		hexed->files[file_index].cursor_x = 11;
	}
}

void render(Hexed* hexed, int file_index) {
	int x, y;

	for (size_t i = MAX_COLUMNS * hexed->files[file_index].start_line; i < (hexed->files[file_index].start_line + hexed->wsize.ws_row - 1) * MAX_COLUMNS && i < hexed->files[file_index].size; i++) {
		x = i % MAX_COLUMNS;
		y = i / MAX_COLUMNS - hexed->files[file_index].start_line;

		int group_size = (hexed->config->octets <= 1) ? 0 : (MAX_COLUMNS / hexed->config->octets);
		int space = (group_size > 0) ? (x / group_size) : 0;
		int offset = x * 3 + 11 + space;

		if (x == 0)
			mvprintw(y, x, "%08lX: ", i + MAX_COLUMNS);

		mvprintw(y, offset, "%02X", hexed->files[file_index].data[i]);

		if (x == MAX_COLUMNS - 1) {
			int offset = x * 3 + 15 + space;

			for (size_t j = i - (MAX_COLUMNS - 1); j <= i; j++) {
				if (is_ascii(hexed->files[file_index].data[j])) {
					mvprintw(y, offset, "%c", hexed->files[file_index].data[j]);
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

void read_mode_event_handler(Hexed* hexed, int file_index, char c) {
	if (c == KEYBOARD_UP) {
		scroll_up(hexed, file_index);
	} else if (c == KEYBOARD_DOWN) {
		scroll_down(hexed, file_index);
	} else if (c == KEYBOARD_LEFT) {
		scroll_left(hexed, file_index);
	} else if (c == KEYBOARD_RIGHT) {
		scroll_right(hexed, file_index);
	}
}

void insert_mode_event_handler(Hexed* hexed, int file_index, char c) {
	if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || c == KEYBOARD_BACK) {
		snprintf(hexed->files[file_index].value_buffer, sizeof(hexed->files[file_index].value_buffer), "%02X", hexed->files[file_index].data[hexed->files[file_index].current_byte]);

		hexed->files[file_index].value_buffer[0] = hexed->files[file_index].value_buffer[1];

		if (c == KEYBOARD_BACK) {
			hexed->files[file_index].value_buffer[1] = '0';
		} else {
			hexed->files[file_index].value_buffer[1] = c;
		}

		int value = 0;
		sscanf(hexed->files[file_index].value_buffer, "%x", &value);
		hexed->files[file_index].data[hexed->files[file_index].current_byte] = value;
	} else if (c == KEYBOARD_UP) {
		scroll_up(hexed, file_index);
	} else if (c == KEYBOARD_DOWN) {
		scroll_down(hexed, file_index);
	} else if (c == KEYBOARD_LEFT) {
		scroll_left(hexed, file_index);
	} else if (c == KEYBOARD_RIGHT) {
		scroll_right(hexed, file_index);
	}
}

Hexed* hexed_init(void) {
	hexed.config = get_config();

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &hexed.wsize);

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	hexed.mode = hexed.config->default_mode;
	hexed.files_count = 0;
	hexed.files = NULL;

	if (hexed.config->use_colors) {
		start_color();
		init_pair(1, hexed.config->fg_color, hexed.config->bg_color);
		attron(COLOR_PAIR(1));
	}

	return &hexed;
}

int hexed_open(Hexed* hexed, char* filename) {
	FILE* fp = fopen(filename, "rb");

	if (fp == NULL) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return -1;
	}

	// Узнаем размер файла
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	uint8_t* data = (uint8_t*) malloc(size); // Выделяем память для данных
	fread(data, 1, size, fp); // Читаем файл

	// Выделяем место для массива структур
	hexed->files = realloc(hexed->files, (hexed->files_count + 1) * sizeof(HexedFile));

	// Создаем структуру файла
	HexedFile new_file = {
		.fp = fp,
		.size = size,
		.start_line = 0,
		.cursor_x = 11,
		.cursor_y = 0,
		.current_byte = 0,
		.value_buffer = {0},
		.data = data
	};

	hexed->files[hexed->files_count] = new_file;

	hexed->files_count++;

	return hexed->files_count - 1;
}

void hexed_close(Hexed* hexed, int file_index) {
	free(hexed->files[file_index].data);
	fclose(hexed->files[file_index].fp);

	for (int i = file_index; i < hexed->files_count; i++) {
		hexed->files[i] = hexed->files[i + 1];
	}

	hexed->files = realloc(hexed->files, hexed->files_count * sizeof(HexedFile));

	hexed->files_count--;
}

void hexed_exit(Hexed* hexed) {
	free(hexed->files);

	if (hexed->config->use_colors)
		attroff(COLOR_PAIR(1));

	endwin();
}