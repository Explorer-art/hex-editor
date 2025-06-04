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

static Hexed hexed;

void move_up(Hexed* hexed, int file_index) {
	HexedFile* file = &hexed->files[file_index];

	// Проверка для доп. безопасности
	if (file->current_byte < MAX_COLUMNS) return;

	if (file->cursor_y > 0) { // Прыгаем вверх если y > 0
		file->cursor_y--;
		file->current_byte -= MAX_COLUMNS;
	} else if (file->start_line > 0) { // Прыгаем вверх если сверху есть ещё строка
		file->start_line--;
		file->current_byte -= MAX_COLUMNS;
	}
}

void move_down(Hexed* hexed, int file_index) {
	HexedFile* file = &hexed->files[file_index];

	// Проверка для доп. безопасности
	if (file->current_byte + MAX_COLUMNS >= hexed->files[file_index].size) return;

	// Прыгаем вниз если есть строка
	if (file->start_line + hexed->wsize.ws_row - 1 < file->size / MAX_COLUMNS) {
		if (file->cursor_y < hexed->wsize.ws_row - 2) { // -2 потому что у нас 1 строчка внизу занята под информацию
			file->cursor_y++;
			file->current_byte += MAX_COLUMNS;
		} else {
			file->start_line++;
			file->current_byte += MAX_COLUMNS;
		}
	}
}

void move_left(Hexed* hexed, int file_index) {
	HexedFile* file = &hexed->files[file_index];

	// Проверка для доп. безопасности
	if (file->current_byte == 0) return;

	if (file->cursor_x > 12) { // Перемещаемся влево если это не конец строки
		int x = file->current_byte % MAX_COLUMNS;
		int group_size = (hexed->config->octets <= 1) ? 0 : (MAX_COLUMNS / hexed->config->octets);
		int space = (group_size > 0) ? (x / group_size) : 0;

		file->cursor_x -= 3 + space;
		file->current_byte--;
	} else if (file->cursor_y > 0) { // Перемещаемся вверх и вправо если y > 0
		file->cursor_y--;
		file->current_byte--;
		file->cursor_x = MAX_COLUMNS * 3 + 8;
	} else if (file->start_line > 0) { // Перемещаемся вверх и вправо если сверху есть ещё строка
		file->start_line--;
		file->current_byte--;
		file->cursor_x = MAX_COLUMNS * 3 + 8;
	}
}

void move_right(Hexed* hexed, int file_index) {
	HexedFile* file = &hexed->files[file_index];

	// Проверка для доп. безопасности
	if (file->current_byte + 1 >= hexed->files[file_index].size) return;

	if (file->cursor_x < MAX_COLUMNS * 3 + 8) { // Перемещаемся вправо если x < макс. столбцов
		if (file->cursor_x < hexed->files[file_index].size * 3 + 7) {
			int x = file->current_byte % MAX_COLUMNS;
			int group_size = (hexed->config->octets <= 1) ? 0 : (MAX_COLUMNS / hexed->config->octets);
			int space = (group_size > 0) ? (x / group_size) : 0;

			file->cursor_x += 3 + space;
			file->current_byte++;
		}
	} else if (file->start_line + hexed->wsize.ws_row < file->size / MAX_COLUMNS) { // Перемещаемся вниз и влево если снизу есть строка
		if (file->cursor_y < hexed->wsize.ws_row - 1) {
			file->cursor_y++;
			file->current_byte++;
		} else {
			file->start_line++;
			file->current_byte++;
		}

		file->cursor_x = 11;
	}
}

void render(Hexed* hexed, int file_index) {
	HexedFile* file = &hexed->files[file_index];

	int x, y;
	int start = MAX_COLUMNS * file->start_line;
	int end = (file->start_line + hexed->wsize.ws_row - 1) * MAX_COLUMNS;

	for (size_t i = start; i < end && i < file->size; i++) {
		x = i % MAX_COLUMNS;
		y = i / MAX_COLUMNS - file->start_line;

		int group_size = (hexed->config->octets <= 1) ? 0 : (MAX_COLUMNS / hexed->config->octets);
		int space = (group_size > 0) ? (x / group_size) : 0;
		int offset = x * 3 + 11 + space;

		if (x == 0)
			mvprintw(y, x, "%08lX: ", i + MAX_COLUMNS);

		mvprintw(y, offset, "%02X", file->data[i]);

		if (is_ascii(file->data[i])) {
			mvprintw(y, MAX_COLUMNS * 3 + 15 + x, "%c", file->data[i]);
		} else {
			mvprintw(y, MAX_COLUMNS * 3 + 15 + x, ".");
		}
	}

	if (hexed->mode == READ_MODE)
		mvprintw(hexed->wsize.ws_row - 1, 0, "READ MODE");
	else if (hexed->mode == INSERT_MODE)
		mvprintw(hexed->wsize.ws_row - 1, 0, "INSERT MODE");
}

void read_mode_event_handler(Hexed* hexed, int file_index, char c) {
	if (c == KEYBOARD_UP) {
		move_up(hexed, file_index);
	} else if (c == KEYBOARD_DOWN) {
		move_down(hexed, file_index);
	} else if (c == KEYBOARD_LEFT) {
		move_left(hexed, file_index);
	} else if (c == KEYBOARD_RIGHT) {
		move_right(hexed, file_index);
	}
}

void insert_mode_event_handler(Hexed* hexed, int file_index, char c) {
	HexedFile* file = &hexed->files[file_index];

	if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || c == KEYBOARD_BACK) {
		snprintf(file->value_buffer, sizeof(file->value_buffer), "%02X", file->data[file->current_byte]);

		file->value_buffer[0] = file->value_buffer[1];

		if (c == KEYBOARD_BACK) {
			file->value_buffer[1] = '0';
		} else {
			file->value_buffer[1] = c;
		}

		int value = 0;
		sscanf(file->value_buffer, "%x", &value);
		file->data[file->current_byte] = value;
	} else if (c == KEYBOARD_UP) {
		move_up(hexed, file_index);
	} else if (c == KEYBOARD_DOWN) {
		move_down(hexed, file_index);
	} else if (c == KEYBOARD_LEFT) {
		move_left(hexed, file_index);
	} else if (c == KEYBOARD_RIGHT) {
		move_right(hexed, file_index);
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
	FILE* fp = fopen(filename, "r+b");

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
		.cursor_x = 12,
		.cursor_y = 0,
		.current_byte = 0,
		.value_buffer = {0},
		.data = data
	};

	hexed->files[hexed->files_count] = new_file;
	hexed->files_count++;

	return hexed->files_count - 1;
}

void hexed_save(Hexed* hexed, int file_index) {
	HexedFile* file = &hexed->files[file_index];

	fseek(file->fp, 0, SEEK_SET);
	fwrite(file->data, 1, file->size, file->fp);
}

void hexed_close(Hexed* hexed, int file_index) {
	HexedFile* file = &hexed->files[file_index];

	free(file->data);
	fclose(file->fp);

	// Удаляем структуру из массива
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