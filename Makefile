SRC_DIR = "src"

all: clean hexed

hexed:
	gcc -lncurses src/main.c src/core/hexed.c src/config/config.c src/utils/ini.c src/utils/utils.c -o $@ -I src/include -lncurses

clean:
	rm -f hexed