#include "server.h"
#include "debug.h"
#include "global.h"

int server_init(server_t *server, int port, int board_size, int max_snakes, unsigned int seed) {
	(void)server;
	(void)port;
	(void)board_size;
	(void)max_snakes;
	(void)seed;
	return 0;
}

void *server_game_loop(void *arg) {
	(void)arg;
	return NULL;
}

void *server_client_handler(void *arg) {
	(void)arg;
	return NULL;
}

int server_start(server_t *server) {
	(void)server;
	return 0;
}

void server_cleanup(server_t *server) { 
    (void)server;
}

int recv_exact(int fd, uint8_t *buf, size_t len) {
	(void)fd;
	(void)buf;
	(void)len;
	return 0;
}

int send_all(int fd, const uint8_t *buf, size_t len) {
	(void)fd;
	(void)buf;
	(void)len;
	return 0;
}