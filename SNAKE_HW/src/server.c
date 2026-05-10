#include "server.h"
#include "debug.h"
#include "global.h"
#include "protocol.h"
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// set up server state, board state, and listening socket
int server_init(server_t *server, int port, int board_size, int max_snakes, unsigned int seed) {
	int i;
	int optval = 1;
	struct sockaddr_in addr;

	if(server == NULL || port <= 0 || port > 65535) {
		debug("server_init got invalid args");
		return -1;
	}

	// clear runtime fields so cleanup paths are always safe
	server->listen_fd = -1;
	server->running = 0;
	for(i = 0; i < MAX_PLAYERS; i++) {
		server->client_fds[i] = -1;
		server->client_snake_ids[i] = -1;
	}

	// board must be ready before clients can join
	if(board_init(&server->board, board_size, max_snakes, seed) != 0) {
		debug("server_init failed to initialize board");
		return -1;
	}

	// one shared mutex protects board + client slot bookkeeping
	if(pthread_mutex_init(&server->board_mutex, NULL) != 0) {
		debug("server_init failed to initialize mutex");
		board_free(&server->board);
		return -1;
	}

	// create TCP listener
	server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server->listen_fd < 0) {
		debug("server_init socket() failed errno=%d", errno);
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	// allow quick restart after process exits
	if(setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
	              sizeof(optval)) < 0) {
		debug("server_init setsockopt() failed errno=%d", errno);
		close(server->listen_fd);
		server->listen_fd = -1;
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((uint16_t)port);

	// bind listener to all interfaces on requested port
	if(bind(server->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		debug("server_init bind() failed errno=%d", errno);
		close(server->listen_fd);
		server->listen_fd = -1;
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	// start accepting up to max_snakes queued connections
	if(listen(server->listen_fd, max_snakes) < 0) {
		debug("server_init listen() failed errno=%d", errno);
		close(server->listen_fd);
		server->listen_fd = -1;
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	server->running = 1;
	debug("server initialized on port %d", port);
	return 0;
}

// implemented as a separate thread so it can run concurrently with client handlers
void *server_game_loop(void *arg) {
	server_t *server = (server_t *)arg;
	uint8_t state_buf[GAME_STATE_BUF_SIZE];
	uint8_t dead_buf[2];

	if(server == NULL) {
		debug("server_game_loop got NULL server");
		return NULL;
	}

	while(server->running) {
		int alive_before[MAX_PLAYERS] = {0};
		int dead_ids[MAX_PLAYERS];
		int dead_fds[MAX_PLAYERS];
		int dead_count = 0;
		int broadcast_fds[MAX_PLAYERS];
		int broadcast_count = 0;
		int game_state_len = -1;

		usleep((unsigned int)TICK_INTERVAL_MS * 1000U);
		if(!server->running) {
			break;
		}

		if(pthread_mutex_lock(&server->board_mutex) != 0) {
			debug("server_game_loop failed to lock board mutex");
			continue;
		}

		/* remember who was alive before ticking */
		for(int i = 0; i < server->board.max_snakes; i++) {
			alive_before[i] = server->board.snakes[i].alive;
		}

		if(board_tick(&server->board) != 0) {
			debug("server_game_loop: board_tick failed");
			pthread_mutex_unlock(&server->board_mutex);
			continue;
		}

		/* collect snakes that just died and their client fds */
		for(int i = 0; i < server->board.max_snakes; i++) {
			if(alive_before[i] && !server->board.snakes[i].alive) {
				for(int slot = 0; slot < MAX_PLAYERS; slot++) {
					if(server->client_snake_ids[slot] == i &&
					   server->client_fds[slot] >= 0) {
						dead_ids[dead_count] = i;
						dead_fds[dead_count] = server->client_fds[slot];
						dead_count++;
						break;
					}
				}
			}
		}

		game_state_len = protocol_serialize_game_state(state_buf, sizeof(state_buf),
		                                              &server->board);

		/* snapshot recipients so we can send after releasing lock */
		for(int slot = 0; slot < MAX_PLAYERS; slot++) {
			if(server->client_fds[slot] >= 0) {
				broadcast_fds[broadcast_count++] = server->client_fds[slot];
			}
		}

		pthread_mutex_unlock(&server->board_mutex);

		/* notify clients whose snakes died this tick */
		for(int i = 0; i < dead_count; i++) {
			int dead_len =
			    protocol_serialize_dead(dead_buf, sizeof(dead_buf), dead_ids[i]);

			if(dead_len > 0) {
				(void)send_all(dead_fds[i], dead_buf, (size_t)dead_len);
			}
		}

		/* then broadcast the board state to all connected clients */
		if(game_state_len > 0) {
			for(int i = 0; i < broadcast_count; i++) {
				(void)send_all(broadcast_fds[i], state_buf, (size_t)game_state_len);
			}
		}
	}

	debug("server_game_loop exited");
	return NULL;
}

// handle one connected client from join until disconnect
void *server_client_handler(void *arg) {
	client_handler_arg_t *handler_arg = (client_handler_arg_t *)arg;
	server_t *server;
	int client_fd;
	int snake_id = -1;
	int slot_index = -1;
	uint8_t msg_buf[CLIENT_MSG_SIZE];
	uint8_t out_buf[GAME_STATE_BUF_SIZE];
	uint8_t msg_type;
	uint8_t msg_payload;
	int recv_status;
	int lock_status;

	if(handler_arg == NULL || handler_arg->server == NULL) {
		debug("server_client_handler got invalid arg");
		free(handler_arg);
		return NULL;
	}

	server = handler_arg->server;
	client_fd = handler_arg->client_fd;

	/* first message must be JOIN */
	// protocol requires client to join before sending commands
	if(recv_exact(client_fd, msg_buf, CLIENT_MSG_SIZE) != 0 ||
	   protocol_deserialize_client_msg(msg_buf, CLIENT_MSG_SIZE, &msg_type,
	                                   &msg_payload) != 0 ||
	   msg_type != MSG_JOIN) {
		int err_len =
		    protocol_serialize_error(out_buf, sizeof(out_buf), ERR_INVALID_MSG);

		if(err_len > 0) {
			(void)send_all(client_fd, out_buf, (size_t)err_len);
		}
		close(client_fd);
		free(handler_arg);
		return NULL;
	}

	/* add snake while holding board mutex */
	// lock once for add + slot registration so board/client state stays consistent
	lock_status = pthread_mutex_lock(&server->board_mutex);
	if(lock_status != 0) {
		debug("client handler mutex lock failed");
		close(client_fd);
		free(handler_arg);
		return NULL;
	}

	// no free snake slot means game is full for now
	if(board_add_snake(&server->board, &snake_id) != 0) {
		pthread_mutex_unlock(&server->board_mutex);
		{
			int err_len =
			    protocol_serialize_error(out_buf, sizeof(out_buf), ERR_GAME_FULL);

			if(err_len > 0) {
				(void)send_all(client_fd, out_buf, (size_t)err_len);
			}
		}
		close(client_fd);
		free(handler_arg);
		return NULL;
	}

	/* build welcome and register slot while board state is stable */
	// send assigned id and current board settings back to client
	{
		int welcome_len = protocol_serialize_welcome(
		    out_buf, sizeof(out_buf), snake_id, server->board.size,
		    server->board.max_snakes);

		// reserve a server slot for this client fd <-> snake id mapping
		for(int i = 0; i < MAX_PLAYERS; i++) {
			if(server->client_fds[i] == -1) {
				server->client_fds[i] = client_fd;
				server->client_snake_ids[i] = snake_id;
				slot_index = i;
				break;
			}
		}

		pthread_mutex_unlock(&server->board_mutex);

		if(slot_index < 0 || welcome_len <= 0 ||
		   send_all(client_fd, out_buf, (size_t)welcome_len) != 0) {
			/* rollback join if we cannot register/send welcome */
			if(pthread_mutex_lock(&server->board_mutex) == 0) {
				if(snake_id >= 0 && snake_id < server->board.max_snakes &&
				   server->board.snakes[snake_id].alive) {
					(void)board_remove_snake(&server->board, snake_id);
				}
				if(slot_index >= 0) {
					server->client_fds[slot_index] = -1;
					server->client_snake_ids[slot_index] = -1;
				}
				pthread_mutex_unlock(&server->board_mutex);
			}
			close(client_fd);
			free(handler_arg);
			return NULL;
		}
	}

	/* handle client messages */
	// after join, only DIRECTION and LEAVE are expected from the client
	while(server->running) {
		recv_status = recv_exact(client_fd, msg_buf, CLIENT_MSG_SIZE);
		if(recv_status != 0) {
			// disconnect or recv error
			break;
		}

		if(protocol_deserialize_client_msg(msg_buf, CLIENT_MSG_SIZE, &msg_type,
		                                  &msg_payload) != 0) {
			// malformed packet: notify client but keep connection alive
			int err_len =
			    protocol_serialize_error(out_buf, sizeof(out_buf), ERR_INVALID_MSG);

			if(err_len > 0) {
				(void)send_all(client_fd, out_buf, (size_t)err_len);
			}
			continue;
		}

		if(msg_type == MSG_LEAVE) {
			// graceful client exit
			break;
		}

		if(msg_type == MSG_JOIN) {
			// already joined once; reject duplicate joins
			int err_len = protocol_serialize_error(out_buf, sizeof(out_buf),
			                                     ERR_ALREADY_JOINED);

			if(err_len > 0) {
				(void)send_all(client_fd, out_buf, (size_t)err_len);
			}
			continue;
		}

		if(msg_type == MSG_DIRECTION) {
			// queue next direction for this snake under mutex
			if(pthread_mutex_lock(&server->board_mutex) == 0) {
				if(snake_id >= 0 && snake_id < server->board.max_snakes &&
				   server->board.snakes[snake_id].alive) {
					(void)snake_set_direction(
					    &server->board.snakes[snake_id],
					    (direction_t)msg_payload);
				}
				pthread_mutex_unlock(&server->board_mutex);
			}
		}
	}

	/* cleanup this client: remove snake, clear slot, close fd */
	if(pthread_mutex_lock(&server->board_mutex) == 0) {
		if(snake_id >= 0 && snake_id < server->board.max_snakes &&
		   server->board.snakes[snake_id].alive) {
			(void)board_remove_snake(&server->board, snake_id);
		}

		if(slot_index >= 0 && slot_index < MAX_PLAYERS) {
			server->client_fds[slot_index] = -1;
			server->client_snake_ids[slot_index] = -1;
		} else {
			for(int i = 0; i < MAX_PLAYERS; i++) {
				if(server->client_fds[i] == client_fd) {
					server->client_fds[i] = -1;
					server->client_snake_ids[i] = -1;
					break;
				}
			}
		}

		pthread_mutex_unlock(&server->board_mutex);
	}

	close(client_fd);
	free(handler_arg);
	debug("client handler exited for fd=%d", client_fd);
	return NULL;
}

// placeholder for accept loop + thread startup
int server_start(server_t *server) {
	/* accept loop will be implemented later */
	debug("server_start stub called");
	(void)server;
	return 0;
}

// close sockets and release board resources
void server_cleanup(server_t *server) { 
	int i;

	if(server == NULL) {
		return;
	}

	server->running = 0;

	for(i = 0; i < MAX_PLAYERS; i++) {
		if(server->client_fds[i] >= 0) {
			close(server->client_fds[i]);
			server->client_fds[i] = -1;
		}
		server->client_snake_ids[i] = -1;
	}

	if(server->listen_fd >= 0) {
		close(server->listen_fd);
		server->listen_fd = -1;
	}

	board_free(&server->board);
	(void)pthread_mutex_destroy(&server->board_mutex);
	debug("server cleanup completed");
}

// read exactly len bytes unless peer disconnects or error occurs
int recv_exact(int fd, uint8_t *buf, size_t len) {
	size_t total = 0;

	if(fd < 0 || buf == NULL) {
		debug("recv_exact got invalid input");
		return -1;
	}

	/* loop until we read exactly len bytes */
	while(total < len) {
		ssize_t got = recv(fd, buf + total, len - total, 0);

		if(got == 0) {
			debug("recv_exact: peer closed connection");
			return -1;
		}
		if(got < 0) {
			if(errno == EINTR) {
				continue;
			}
			debug("recv_exact failed errno=%d", errno);
			return -1;
		}
		total += (size_t)got;
	}

	debug("recv_exact read %zu bytes", len);
	return 0;
}

// write exactly len bytes unless send fails
int send_all(int fd, const uint8_t *buf, size_t len) {
	size_t total = 0;

	if(fd < 0 || buf == NULL) {
		debug("send_all got invalid input");
		return -1;
	}

	/* loop until everything is sent */
	while(total < len) {
		ssize_t sent = send(fd, buf + total, len - total, 0);

		if(sent < 0) {
			if(errno == EINTR) {
				continue;
			}
			debug("send_all failed errno=%d", errno);
			return -1;
		}
		if(sent == 0) {
			debug("send_all sent 0 bytes unexpectedly");
			return -1;
		}
		total += (size_t)sent;
	}

	debug("send_all wrote %zu bytes", len);
	return 0;
}