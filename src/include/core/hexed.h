#pragma once

#include <stdint.h>
#include <sys/ioctl.h>
#include <modes/modes.h>
#include <config/config.h>

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

void scroll_up(Hexed* hexed, int file_index);
void scroll_down(Hexed* hexed, int file_index);
void scroll_left(Hexed* hexed, int file_index);
void scroll_right(Hexed* hexed, int file_index);
void render(Hexed* hexed, int file_index);
void read_mode_event_handler(Hexed* hexed, int file_index, char c);
void insert_mode_event_handler(Hexed* hexed, int file_index, char c);
Hexed* hexed_init(void);
int hexed_open(Hexed* hexed, char* filename);
void hexed_close(Hexed* hexed, int file_index);
void hexed_exit(Hexed* hexed);