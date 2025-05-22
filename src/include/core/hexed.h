#pragma once

#include <stdint.h>
#include <sys/ioctl.h>
#include <modes/modes.h>
#include <config/config.h>

typedef struct {
	Configuration* config;
	Mode mode;
	FILE* fp;
	int start_line;
	int cursor_x;
	int cursor_y;
	int current_byte;
	struct winsize wsize;
	size_t size;
	char value_buffer[3];
	uint8_t* data;
} Hexed;

void scroll_up(Hexed* hexed);
void scroll_down(Hexed* hexed);
void scroll_left(Hexed* hexed);
void scroll_right(Hexed* hexed);
void render(Hexed* hexed);
void read_mode_event_handler(Hexed* hexed, char c);
void insert_mode_event_handler(Hexed* hexed, char c);
Hexed* hexed_init(void);
void hexed_open(Hexed* hexed, char* filename);
void hexed_close(Hexed* hexed, FILE* fp);
void hexed_exit(void);