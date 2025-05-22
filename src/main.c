/*
================================================
HEX Editor v0.1
Author: Truzme_ (https://github.com/Explorer-art)

Change log:
v0.1:
- Basic HEX viewer

v0.2:
- Read and insert modes
- Basic HEX editor
================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <core/hexed.h>
#include <config/config.h>

#define VERSION 		"0.2"

Hexed* hexed = NULL;

int main(int argc, char* argv[]) {
	config_init();

	if (argc < 2) {
		printf("Usage: %s [file]\n", argv[0]);
		exit(1);
	}

	if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
		printf("Version: HEX Editor %s\n", VERSION);
		exit(0);
	}

	hexed = hexed_init();

	hexed_open(argv[1]);

	// Основной цикл
	for (;;) {
		clear();

		mvprintw(0, 100, "Current byte: %d", hexed->current_byte);

		render();
		refresh();

		move(hexed->cursor_y, hexed->cursor_x);

		char c = getch();

		if (c == 'q') {
			break;
		} else if (c == 'i') {
			if (hexed->mode == 0) {
				hexed->mode = INSERT_MODE;
			} else {
				hexed->mode = READ_MODE;
			}
		}

		if (hexed->mode == READ_MODE) {
			read_mode_event_handler(c);
		} else if (hexed->mode == INSERT_MODE) {
			insert_mode_event_handler(c);
		}
	}

	hexed_exit();

	return 0;
}