#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <errno.h>
#include <config/config.h>
#include <utils/ini.h>

static Configuration config;

static int config_handler(void* user, const char* section, const char* name, const char* value) {
	Configuration* pconfig = (Configuration*)user;

	#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	if (MATCH("general", "default_mode")) {
		pconfig->default_mode = atoi(value);
	} else if (MATCH("general", "octets")) {
		pconfig->octets = atoi(value);
	} else if (MATCH("general", "use_colors")) {
		pconfig->use_colors = (strcmp(value, "true") == 0);
	} else if (MATCH("general", "fg_color")) {
		pconfig->fg_color = atoi(value);
	} else if (MATCH("general", "bg_color")) {
		pconfig->bg_color = atoi(value);
	} else {
		return 0;
	}

	return 1;
}

Configuration* config_init() {
	char config_home[PATH_MAX];

	const char* xdg = getenv("XDG_CONFIG_HOME");
	const char* home = getenv("HOME");

	if (xdg && *xdg) {
		strncpy(config_home, xdg, sizeof(config_home) - 1);
		config_home[sizeof(config_home) - 1] = '\0';
	} else if (home && *home) {
		snprintf(config_home, sizeof(config_home), "%s/.config", home);
	} else {
		fprintf(stderr, "Error config init");
		exit(1);
	}

	char config_dir_path[PATH_MAX];

	if (snprintf(config_dir_path, sizeof(config_dir_path), "%s/hexed", config_home) >= sizeof(config_dir_path)) {
		fprintf(stderr, "Error: config_dir_path too long\n");
		exit(1);
	}

	struct stat st = {0};

	if (stat(config_dir_path, &st) == -1) {
		if (mkdir(config_dir_path, 0700) == -1) {
			perror("Error creating directory");
			exit(1);
		}
	}

	char config_path[PATH_MAX];

	if (snprintf(config_path, sizeof(config_path), "%s/config.ini", config_dir_path) >= sizeof(config_path)) {
		fprintf(stderr, "Error: config_path too long\n");
		exit(1);
	}

	if (stat(config_path, &st) == -1) {
		FILE* fp = fopen(config_path, "w");

		if (fp == NULL) {
			perror("Error creating config");
			exit(1);
		}

		fputs("[general]\n", fp);
		fputs("default_mode = 0\n", fp);
		fputs("octets = 1\n", fp);
		fputs("use_colors = false\n", fp);
		fputs("fg_color = 7\n", fp);
		fputs("bg_color = 0\n", fp);

		fclose(fp);
	}

	if (ini_parse(config_path, config_handler, &config) < 0) {
		perror("Error load config.ini");
		exit(1);
	}

	return &config;
}

Configuration* get_config(void) {
	return &config;
}