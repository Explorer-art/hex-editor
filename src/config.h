typedef struct {
	bool use_colors;
	int fg_color;
	int bg_color;
} Configuration;

void init_config(Configuration* config);