#pragma once

#include <stdint.h>
#include <sys/ioctl.h>
#include <modes.h>
#include <config/config.h>

#define MAX_COLUMNS 	16
#define KEYBOARD_UP 	3
#define KEYBOARD_DOWN 	2
#define KEYBOARD_LEFT 	4
#define KEYBOARD_RIGHT 	5
#define KEYBOARD_BACK	7

typedef struct {
	FILE* fp;
	size_t size;
	int start_line;
	int cursor_x;
	int cursor_y;
	int current_byte;
	char value_buffer[3];
	uint8_t* data;
} HexedFile;

typedef struct {
	Configuration* config;
	Mode mode;
	struct winsize wsize;
	int files_count;
	HexedFile* files;
} Hexed;

void move_up(Hexed* hexed, int file_index);
void move_down(Hexed* hexed, int file_index);
void move_left(Hexed* hexed, int file_index);
void move_right(Hexed* hexed, int file_index);
void render(Hexed* hexed, int file_index);
void read_mode_event_handler(Hexed* hexed, int file_index, char c);
void insert_mode_event_handler(Hexed* hexed, int file_index, char c);
Hexed* hexed_init(void);
int hexed_open(Hexed* hexed, char* filename);
void hexed_save(Hexed* hexed, int file_index);
void hexed_close(Hexed* hexed, int file_index);
void hexed_exit(Hexed* hexed);