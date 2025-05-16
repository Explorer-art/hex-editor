#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "utils/ini.h"

#define VERSION "0.1"
#define MAX_COLUMNS 16
#define KEYBOARD_UP 3
#define KEYBOARD_DOWN 2
#define KEYBOARD_LEFT 4
#define KEYBOARD_RIGHT 5

typedef struct {
	bool use_colors;
	int fg_color;
	int bg_color;
} Configuration;

size_t size = 0;
int start_line = 0;
int cursor_x = 11;
int cursor_y = 0;
struct winsize wsize;
Configuration config;

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

static int config_handler(void* user, const char* section, const char* name, const char* value) {
	Configuration* pconfig = (Configuration*)user;

	#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	if (MATCH("settings", "use_colors")) {
		pconfig->use_colors = (strcmp(value, "true") == 0);
	} else if (MATCH("settings", "fg_color")) {
		pconfig->fg_color = atoi(value);
	} else if (MATCH("settings", "bg_color")) {
		pconfig->bg_color = atoi(value);
	} else {
		return 0;
	}

	return 1;
}

void init_config() {
	char config_home[PATH_MAX];

	const char* xdg = getenv("XDG_CONFIG_HOME");
	const char* home = getenv("HOME");

	if (xdg && *xdg) {
		strncpy(config_home, xdg, sizeof(config_home) - 1);
		config_home[sizeof(config_home) - 1] = '\0';
	} else if (home && *home) {
		snprintf(config_home, sizeof(config_home), "%s/.config", home);
	} else {
		perror("Error config init");
		exit(1);
	}

	char config_dir_path[PATH_MAX];
	snprintf(config_dir_path, sizeof(config_dir_path), "%s/hex-editor", config_home);

	struct stat st = {0};

	if (stat(config_dir_path, &st) == -1) {
		if (mkdir(config_dir_path, 0700) == -1) {
			perror("Error creating directory");
		}
	}

	char config_path[PATH_MAX];
	snprintf(config_path, sizeof(config_path), "%s/config.ini", config_dir_path);

	if (stat(config_path, &st) == -1) {
		FILE* fp = fopen(config_path, "w");

		if (fp == NULL) {
			perror("Error creating config");
			exit(1);
		}

		fputs("[settings]\n", fp);
		fputs("use_colors = false\n", fp);
		fputs("fg_color = 7\n", fp);
		fputs("bg_color = 0\n", fp);

		fclose(fp);
	}

	if (ini_parse(config_path, config_handler, &config) < 0) {
		perror("Error load config.ini");
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	init_config();

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
			if (cursor_x > 11) {
				cursor_x -= 3;
			} else if (cursor_y != 0) {
				scroll_up();
				cursor_x = MAX_COLUMNS * 3 + 8;
			}
		} else if (c == KEYBOARD_RIGHT) {
			if (cursor_x < (size % MAX_COLUMNS) * 3 + 10) {
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

	if (config.use_colors)
		attroff(COLOR_PAIR(1));

	free(data);
	endwin();

	return 0;
}