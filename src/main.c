#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>

#define MAX_COLUMNS 16
#define KEYBOARD_UP 3
#define KEYBOARD_DOWN 2
#define KEYBOARD_LEFT 4
#define KEYBOARD_RIGHT 5

size_t size = 0;
int start_line = 0;
int cursor_x = 11;
int cursor_y = 0;
struct winsize wsize;

int is_ascii(unsigned char c) {
    return (c >= 32 && c <= 126);
}

void scroll_up() {
	if (cursor_y > 0) {
		cursor_y--;
	} else {
		if (start_line > 0) {
			start_line--;
		}
	}
}

void scroll_down() {
	if (start_line + wsize.ws_row < size / MAX_COLUMNS) {
		if (cursor_y < wsize.ws_row - 1) {
			cursor_y++;
		} else {
			start_line++;
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s [file name]\n", argv[0]);
		exit(1);
	}

	FILE* fp = fopen(argv[1], "rb");

	if (fp == NULL) {
		printf("Error opening %s\n", argv[1]);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char* data = (unsigned char*) malloc(size);

	fread(data, 1, size, fp);

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	int x, y;

	while (true) {
		clear();

		for (size_t i = MAX_COLUMNS * start_line; i < (start_line + wsize.ws_row) * MAX_COLUMNS && i < size; i++) {
			x = i % MAX_COLUMNS;
			y = i / MAX_COLUMNS - start_line;

			if (x == 0) {
				mvprintw(y, x, "%08X: ", i + MAX_COLUMNS);
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
			if (cursor_x > 11) {
				cursor_x -= 3;
			} else {
				scroll_up();
				cursor_x = MAX_COLUMNS * 3 + 8;
			}
		} else if (c == KEYBOARD_RIGHT) {
			if (size + 7 < MAX_COLUMNS * 3 + 8) {
				if (cursor_x < size * 3 + 7) {
					cursor_x += 3;
				}
			} else {
				if (cursor_x < MAX_COLUMNS * 3 + 8) {
					cursor_x += 3;
				} else {
					scroll_down();
					cursor_x = 11;
				}
			}
		}
	}

	printf("%d", round(size / MAX_COLUMNS));

	free(data);
	endwin();

	return 0;
}