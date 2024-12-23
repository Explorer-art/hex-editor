SRC_DIR = "src"

all: clean hex-editor

hex-editor:
	gcc $(SRC_DIR)/main.c -o $@

clean:
	rm -f hex-editor