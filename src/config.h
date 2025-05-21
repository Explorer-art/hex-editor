#pragma once

#include "modes.h"

typedef struct {
	Mode default_mode;
	unsigned int octets;
	bool use_colors;
	int fg_color;
	int bg_color;
} Configuration;

void init_config(Configuration* config);