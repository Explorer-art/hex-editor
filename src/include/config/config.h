#pragma once

#include <stdbool.h>
#include <modes/modes.h>

typedef struct {
	Mode default_mode;
	unsigned int octets;
	bool use_colors;
	int fg_color;
	int bg_color;
} Configuration;

Configuration* config_init(void);
Configuration* get_config(void);