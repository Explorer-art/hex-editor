#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>

#define WIDTH 100
#define HEIGHT 50
#define MAX_LINES 20
#define MAX_COLUMNS 16

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

	initscr();
	cbreak();
	noecho();

	int start_line = 0;
	int x, y;

	for (size_t i = MAX_COLUMNS * start_line; i < MAX_COLUMNS * MAX_LINES; i++) {
		x = i % MAX_COLUMNS;
		y = i / MAX_COLUMNS;

		if (x == 0) {
			mvprintw(y, x, "%08X: ", i + MAX_COLUMNS);
		}

		mvprintw(y, x * 3 + 10, "%02X", data[i]);

		if (x == MAX_COLUMNS - 1) {
			int c = 0;

			for (size_t j = i - (MAX_COLUMNS - 1); j <= i; j++) {
				if (is_ascii(data[j])) {
					mvprintw(y, x * 3 + 14 + c, "%c", data[j]);
				} else {
					mvprintw(y, x * 3 + 14 + c, ".");
				}

				c++;
			}
		}
	}

	refresh();
	getch();

	free(data);
	endwin();

	return 0;
}