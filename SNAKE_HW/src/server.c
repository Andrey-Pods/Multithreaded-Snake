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
#include <sys/types.h>
#include <unistd.h>

// Check if server is still running using atomic load to prevent data race conditions
static int server_is_running(const server_t *server) {
	return __atomic_load_n(&server->running, __ATOMIC_SEQ_CST);
}

// Atomically set the server running state across all threads
static void server_set_running(server_t *server, int value) {
	__atomic_store_n(&server->running, value, __ATOMIC_SEQ_CST);
}

// Initialize server with network socket, board state, and synchronization primitives
int server_init(server_t *server, int port, int board_size, int max_snakes, unsigned int seed) {
	int i;
	int optval = 1;
	struct sockaddr_in addr;

	if(server == NULL || port <= 0 || port > 65535) {
		debug("server_init got invalid args");
		return -1;
	}

	// Initialize server data structures
	server->listen_fd = -1;
	server_set_running(server, 0);
	for(i = 0; i < MAX_PLAYERS; i++) {
		server->client_fds[i] = -1;
		server->client_snake_ids[i] = -1;
	}

	// Initialize game board with specified dimensions and random seed
	if(board_init(&server->board, board_size, max_snakes, seed) != 0) {
		debug("server_init failed to initialize board");
		return -1;
	}

	// Create mutex to synchronize access to board state from multiple threads
	if(pthread_mutex_init(&server->board_mutex, NULL) != 0) {
		debug("server_init failed to initialize mutex");
		board_free(&server->board);
		return -1;
	}

	// Create TCP listening socket for accepting client connections
	server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server->listen_fd < 0) {
		debug("server_init socket() failed errno=%d", errno);
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	// Allow socket reuse to avoid TIME_WAIT issues during rapid restarts
	if(setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
	              sizeof(optval)) < 0) {
		debug("server_init setsockopt() failed errno=%d", errno);
		close(server->listen_fd);
		server->listen_fd = -1;
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	// Bind socket to wildcard address on specified port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((uint16_t)port);

	if(bind(server->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		debug("server_init bind() failed errno=%d", errno);
		close(server->listen_fd);
		server->listen_fd = -1;
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	// Start listening for incoming client connections

	if(listen(server->listen_fd, max_snakes) < 0) {
		debug("server_init listen() failed errno=%d", errno);
		close(server->listen_fd);
		server->listen_fd = -1;
		pthread_mutex_destroy(&server->board_mutex);
		board_free(&server->board);
		return -1;
	}

	server_set_running(server, 1);
	debug("server initialized on port %d", port);
	return 0;
}

// Game loop that runs in separate thread, ticking board and broadcasting state to clients
void *server_game_loop(void *arg) {
	server_t *server = (server_t *)arg;
	uint8_t state_buf[GAME_STATE_BUF_SIZE];
	uint8_t dead_buf[2];

	if(server == NULL) {
		debug("server_game_loop got NULL server");
		return NULL;
	}

	// Main game loop - runs until server shutdown
	while(server_is_running(server)) {
		int alive_before[MAX_PLAYERS] = {0};
		int dead_ids[MAX_PLAYERS];
		int dead_fds[MAX_PLAYERS];
		int dead_count = 0;
		int broadcast_fds[MAX_PLAYERS];
		int broadcast_count = 0;
		int game_state_len = -1;

		// Sleep for one game tick interval before processing next frame
		usleep((unsigned int)TICK_INTERVAL_MS * 1000U);
		if(!server_is_running(server)) {
			break;
		}

		// Acquire lock to safely access shared board state
		if(pthread_mutex_lock(&server->board_mutex) != 0) {
			debug("server_game_loop failed to lock board mutex");
			continue;
		}

		// Remember which snakes were alive before board update to detect deaths
		for(int i = 0; i < server->board.max_snakes; i++) {
			alive_before[i] = server->board.snakes[i].alive;
		}

		// Advance game state by one tick (move snakes, check collisions, grow snakes)
		if(board_tick(&server->board) != 0) {
			debug("server_game_loop: board_tick failed");
			pthread_mutex_unlock(&server->board_mutex);
			continue;
		}

		// Collect snakes that died this tick to alert their client connections
		// This must happen while we hold the board mutex
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
		if(dead_count > 0) {
			debug("board_tick: %d snakes died this tick", dead_count);
		}

		// Serialize current board state to send to all clients
		game_state_len = protocol_serialize_game_state(state_buf, sizeof(state_buf),
		                                              &server->board);

		// Snapshot all connected client file descriptors for broadcasting
		// Do this while holding lock to ensure consistent client list
		// Send actual messages after releasing lock to avoid holding it too long
		for(int slot = 0; slot < MAX_PLAYERS; slot++) {
			if(server->client_fds[slot] >= 0) {
				broadcast_fds[broadcast_count++] = server->client_fds[slot];
			}
		}

		pthread_mutex_unlock(&server->board_mutex);

		// Send death notifications to clients whose snakes just died
		// These are sent before board state to notify of death first
		for(int i = 0; i < dead_count; i++) {
			int dead_len =
			    protocol_serialize_dead(dead_buf, sizeof(dead_buf), dead_ids[i]);

			if(dead_len > 0) {
				(void)send_all(dead_fds[i], dead_buf, (size_t)dead_len);
			}
		}

		// Broadcast updated board state to all connected clients
		// All clients receive the same state update synchronously every tick
		if(game_state_len > 0) {
			for(int i = 0; i < broadcast_count; i++) {
				(void)send_all(broadcast_fds[i], state_buf, (size_t)game_state_len);
			}
			if(broadcast_count > 0) {
				debug("game_loop: broadcasted state to %d clients", broadcast_count);
			}
		}
	}

	debug("server_game_loop exited");
	return NULL;
}

// Handle one client connection - receives input commands and sends game state
// Each connected client runs this handler in its own detached thread
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

	// First message from client must be JOIN request, enforce strict protocol
	// Disconnect client immediately if protocol is violated
	if(recv_exact(client_fd, msg_buf, CLIENT_MSG_SIZE) != 0 ||
	   protocol_deserialize_client_msg(msg_buf, CLIENT_MSG_SIZE, &msg_type,
	                                   &msg_payload) != 0 ||
	   msg_type != MSG_JOIN) {
		debug("client fd=%d: invalid join request", client_fd);
		int err_len =
		    protocol_serialize_error(out_buf, sizeof(out_buf), ERR_INVALID_MSG);

		if(err_len > 0) {
			(void)send_all(client_fd, out_buf, (size_t)err_len);
		}
		close(client_fd);
		free(handler_arg);
		return NULL;
	}

	// Add snake to board while holding mutex to ensure atomic game state update
	// This must be protected as board state is shared with game loop thread
	lock_status = pthread_mutex_lock(&server->board_mutex);
	if(lock_status != 0) {
		debug("client handler mutex lock failed");
		close(client_fd);
		free(handler_arg);
		return NULL;
	}

	if(board_add_snake(&server->board, &snake_id) != 0) {
		debug("client fd=%d: game is full, cannot add snake", client_fd);
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

	// Build welcome message and register client in open slot while holding mutex
	// Both operations must be atomic to maintain consistent client state
	{
		int welcome_len = protocol_serialize_welcome(
		    out_buf, sizeof(out_buf), snake_id, server->board.size,
		    server->board.max_snakes);

		for(int i = 0; i < MAX_PLAYERS; i++) {
			if(server->client_fds[i] == -1) {
				server->client_fds[i] = client_fd;
				server->client_snake_ids[i] = snake_id;
				slot_index = i;
				break;
			}
		}

		pthread_mutex_unlock(&server->board_mutex);
		if(slot_index >= 0) {
			debug("client fd=%d: joined with snake_id=%d, slot=%d", client_fd, snake_id, slot_index);
		}

		// If welcome send failed, rollback the join to free the snake slot
		if(slot_index < 0 || welcome_len <= 0 ||
		   send_all(client_fd, out_buf, (size_t)welcome_len) != 0) {
			// Rollback: remove snake from board and clear client slot on failure
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

	// Handle incoming client messages - blocks on recv until client sends or disconnects
	// Client can send direction commands or LEAVE to disconnect gracefully
	while(server_is_running(server)) {
		recv_status = recv_exact(client_fd, msg_buf, CLIENT_MSG_SIZE);
		if(recv_status != 0) {
			break;
		}

		if(protocol_deserialize_client_msg(msg_buf, CLIENT_MSG_SIZE, &msg_type,
		                                  &msg_payload) != 0) {
			int err_len =
			    protocol_serialize_error(out_buf, sizeof(out_buf), ERR_INVALID_MSG);

			if(err_len > 0) {
				(void)send_all(client_fd, out_buf, (size_t)err_len);
			}
			continue;
		}

		if(msg_type == MSG_LEAVE) {
			// Client explicitly requesting to leave the game
			break;
		}

		// Reject duplicate JOIN attempts - client already joined
		if(msg_type == MSG_JOIN) {
			debug("client fd=%d snake_id=%d: duplicate JOIN attempt", client_fd, snake_id);
			int err_len = protocol_serialize_error(out_buf, sizeof(out_buf),
			                                     ERR_ALREADY_JOINED);

			if(err_len > 0) {
				(void)send_all(client_fd, out_buf, (size_t)err_len);
			}
			continue;
		}

		// Update direction for this client's snake if it's still alive
		if(msg_type == MSG_DIRECTION) {
			if(pthread_mutex_lock(&server->board_mutex) == 0) {
				if(snake_id >= 0 && snake_id < server->board.max_snakes &&
				   server->board.snakes[snake_id].alive) {
					debug("client fd=%d snake_id=%d: direction=%d", client_fd, snake_id, msg_payload);
					(void)snake_set_direction(
					    &server->board.snakes[snake_id],
					    (direction_t)msg_payload);
				}
				pthread_mutex_unlock(&server->board_mutex);
			}
		}
	}

	// Clean up client state: remove snake from board and free client slot
	// This must happen while holding mutex to avoid race conditions with game loop
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

// Start server - accepts client connections and runs game loop
// Blocks until server is shut down (typically by signal handler)
int server_start(server_t *server) {
	pthread_t game_thread;
	int had_error = 0;

	if(server == NULL || server->listen_fd < 0) {
		debug("server_start got invalid server state");
		return -1;
	}

	// Spawn game loop thread that runs simultaneously with accept loop
	if(pthread_create(&game_thread, NULL, server_game_loop, server) != 0) {
		debug("server_start failed to create game loop thread");
		return -1;
	}

	// Main accept loop - blocks waiting for new client connections
	while(server_is_running(server)) {
		int client_fd;
		client_handler_arg_t *handler_arg;
		pthread_t client_thread;

		// Accept incoming TCP connection from client
		client_fd = accept(server->listen_fd, NULL, NULL);
		if(client_fd < 0) {
			if(errno == EINTR) {
				continue;
			}
			if(!server_is_running(server)) {
				break;
			}
			debug("accept failed errno=%d", errno);
			had_error = 1;
			server_set_running(server, 0);
			break;
		}
		debug("accept: new client connection fd=%d", client_fd);

		// Allocate handler argument structure to pass to client thread
		handler_arg = malloc(sizeof(*handler_arg));
		if(handler_arg == NULL) {
			debug("malloc failed for client handler arg");
			close(client_fd);
			continue;
		}

		handler_arg->server = server;
		handler_arg->client_fd = client_fd;

		// Create detached thread to handle this client independently
		// Detached thread will clean up its own resources when done
		if(pthread_create(&client_thread, NULL, server_client_handler,
		                  handler_arg) != 0) {
			debug("failed to create client handler thread for fd=%d", client_fd);
			close(client_fd);
			free(handler_arg);
			continue;
		}
		debug("created client handler thread for fd=%d", client_fd);

		if(pthread_detach(client_thread) != 0) {
			debug("failed to detach client handler thread");
			// thread still running; don't free/close here
		}
	}

	// Server accept loop has exited - begin shutdown sequence
	server_set_running(server, 0);
	if(server->listen_fd >= 0) {
		close(server->listen_fd);
		server->listen_fd = -1;
	}

	// Wait for game loop thread to finish before continuing shutdown
	if(pthread_join(game_thread, NULL) != 0) {
		debug("failed to join game loop thread");
		return -1;
	}

	// Allow time for detached client handler threads to finish cleanup
	// They need to release their client slots before we fully shutdown
	for(int attempt = 0; attempt < 50; attempt++) {
		int active_clients = 0;

		if(pthread_mutex_lock(&server->board_mutex) != 0) {
			break;
		}
		for(int i = 0; i < MAX_PLAYERS; i++) {
			if(server->client_fds[i] >= 0) {
				active_clients = 1;
				break;
			}
		}
		pthread_mutex_unlock(&server->board_mutex);

		if(!active_clients) {
			break;
		}
		usleep(10000U);
	}

	return had_error ? -1 : 0;
}

// Free all server resources - call this after server_start returns
void server_cleanup(server_t *server) { 
	int i;

	if(server == NULL) {
		return;
	}

	// Signal server to stop running
	server_set_running(server, 0);

	// Close all active client connections
	for(i = 0; i < MAX_PLAYERS; i++) {
		if(server->client_fds[i] >= 0) {
			close(server->client_fds[i]);
			server->client_fds[i] = -1;
		}
		server->client_snake_ids[i] = -1;
	}

	// Close listening socket to stop accepting new connections
	if(server->listen_fd >= 0) {
		close(server->listen_fd);
		server->listen_fd = -1;
	}

	// Clean up game board resources and synchronization primitives
	board_free(&server->board);
	if(pthread_mutex_destroy(&server->board_mutex) != 0) {
		debug("server_cleanup: mutex still busy, skipping destroy");
	}
	debug("server cleanup completed");
}

// Receive exact number of bytes from socket - handles partial reads and signals
int recv_exact(int fd, uint8_t *buf, size_t len) {
	size_t total = 0;

	if(fd < 0 || buf == NULL) {
		debug("recv_exact got invalid input");
		return -1;
	}

	// Keep receiving until buffer is filled - handles partial reads from kernel
	// Retries on EINTR to handle interrupted system calls gracefully
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

// Send all bytes to socket - handles partial writes and signals
int send_all(int fd, const uint8_t *buf, size_t len) {
	size_t total = 0;

	if(fd < 0 || buf == NULL) {
		debug("send_all got invalid input");
		return -1;
	}

	// Keep sending until buffer is empty - handles partial sends from kernel
	// Retries on EINTR to handle interrupted system calls gracefully
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