#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_COLUMNS 16

int is_ascii(unsigned char c) {
    return (c >= 32 && c <= 127);
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
	unsigned char* data_ptr = data;

	fread(data, 1, size, fp);

	for (size_t i = 0; i < size; i++) {
		if (i % MAX_COLUMNS == 0) {
			printf("%08x: ", i + MAX_COLUMNS);
		}

		printf("%02x ", *data_ptr);
		data_ptr++;

		if (i % MAX_COLUMNS == MAX_COLUMNS - 1) {
			unsigned char* c = data_ptr - MAX_COLUMNS;

			printf(" ");

			for (int i = 0; i < MAX_COLUMNS; i++) {
				if (is_ascii(*c)) {
					printf("%c", *c);
				} else {
					printf(".");
				}

				c++;
			}

			printf("\n");
		}
	}

	free(data);

	printf("\n");

	return 0;
}