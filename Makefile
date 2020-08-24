all: build

build:
	$(CC) echo_server.c -o echo_server -O2 -Wall -Werror

debug:
	$(CC) echo_server.c -o echo_server -O0 -ggdb3 -Wall

clean:
	@rm -rf echo_server