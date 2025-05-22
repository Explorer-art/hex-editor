/*
================================================
Hexed v0.2
Author: Truzme_ (https://github.com/Explorer-art)

Change log:
v0.1:
- Basic HEX viewer

v0.2:
- Read and insert modes
- Basic HEX editor
- Code refact
================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <core/hexed.h>
#include <config/config.h>

#define VERSION 		"0.2"

int main(int argc, char* argv[]) {
	config_init();

	if (argc < 2) {
		printf("Usage: %s [file]\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
		printf("Version: Hexed %s\n", VERSION);
		return 0;
	}

	Hexed* hexed = hexed_init();

	hexed_open(hexed, argv[1]);

	// Основной цикл
	for (;;) {
		clear();

		mvprintw(0, 100, "Current byte: %d", hexed->current_byte);

		render(hexed);
		refresh();

		move(hexed->cursor_y, hexed->cursor_x);

		char c = getch();

		if (c == 'q') {
			break;
		} else if (c == 'i') {
			if (hexed->mode == READ_MODE) {
				hexed->mode = INSERT_MODE;
			} else {
				hexed->mode = READ_MODE;
			}
		}

		if (hexed->mode == READ_MODE) {
			read_mode_event_handler(hexed, c);
		} else if (hexed->mode == INSERT_MODE) {
			insert_mode_event_handler(hexed, c);
		}
	}

	hexed_close(hexed, hexed->fp);
	hexed_exit();

	return 0;
}