SRC_DIR = "src"

all: clean hex-editor

hex-editor:
	gcc -lncurses $(SRC_DIR)/main.c $(SRC_DIR)/utils/ini.c -o $@ -lncurses

clean:
	rm -f hex-editor