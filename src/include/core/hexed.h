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

void scroll_up(void);
void scroll_down(void);
void scroll_left(void);
void scroll_right(void);
void render(void);
void read_mode_event_handler(char c);
void insert_mode_event_handler(char c);
Hexed* hexed_init(void);
void hexed_open(char* filename);
void hexed_close(FILE* fp);
void hexed_exit(void);