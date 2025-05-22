SRC_DIR = "src"

all: clean hex-editor

hex-editor:
	gcc -lncurses src/main.c src/core/hexed.c src/config/config.c src/utils/ini.c src/utils/utils.c -o $@ -I src/include -lncurses

clean:
	rm -f hex-editor