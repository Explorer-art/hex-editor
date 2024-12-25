#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_COLUMNS 16
#define KEYBOARD_UP 3
#define KEYBOARD_DOWN 2
#define KEYBOARD_LEFT 4
#define KEYBOARD_RIGHT 5

int is_ascii(unsigned char c) {
    return (c >= 32 && c <= 126);
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
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char* data = (unsigned char*) malloc(size);

	fread(data, 1, size, fp);

	struct winsize wsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);

	printf("Width: %d\n", wsize.ws_row);
	printf("Height: %d\n", wsize.ws_col);

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	int start_line = 0;
	int x, y;
	int select_x = 11;
	int select_y = 0;

	while (true) {
		for (size_t i = MAX_COLUMNS * start_line; i < MAX_COLUMNS * wsize.ws_row; i++) {
			x = i % MAX_COLUMNS;
			y = i / MAX_COLUMNS;

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

		move(select_y, select_x);

		char c = getch();

		if (c == 'q') {
			break;
		} else if (c == KEYBOARD_UP) {
			if (select_y > 0) {
				select_y--;
			}
		} else if (c == KEYBOARD_DOWN) {
			if (select_y < wsize.ws_row) {
				select_y++;
			}
		} else if (c == KEYBOARD_LEFT) {
			if (select_x > 11) {
				select_x -= 3;
			}
		} else if (c == KEYBOARD_RIGHT) {
			if (select_x < MAX_COLUMNS * 3 + 7) {
				select_x += 3;
			}
		}
	}

	free(data);
	endwin();

	return 0;
}