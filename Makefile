SRC_DIR = "src"

all: clean hex-editor

hex-editor:
	gcc -lncurses $(SRC_DIR)/main.c -o $@ -lncurses

clean:
	rm -f hex-editor