/*
 * DO NOT MODIFY THIS FILE
 * Server data structures and function prototypes - CSE 320 HW2
 */
#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <pthread.h>
#include "game_board.h"

/* ---------------------------------------------------------------------------
 * Server state
 * --------------------------------------------------------------------------- */
typedef struct {
    int               listen_fd;                     /* Listening socket fd               */
    game_board_t      board;                         /* Shared game board                 */
    pthread_mutex_t   board_mutex;                   /* Mutex protecting the board        */
    int               client_fds[MAX_PLAYERS];       /* Client socket fds (-1 = empty)    */
    int               client_snake_ids[MAX_PLAYERS]; /* Snake ID per client (-1 = empty)  */
    int               running;                       /* 1 = running, 0 = shutting down    */
} server_t;

/* ---------------------------------------------------------------------------
 * Argument struct passed to client handler threads
 * --------------------------------------------------------------------------- */
typedef struct {
    server_t *server;
    int       client_fd;
} client_handler_arg_t;

/* ---------------------------------------------------------------------------
 * Function prototypes
 * --------------------------------------------------------------------------- */

/*
 * Initialize the server: create listening socket, init board, init mutex.
 * Returns 0 on success, -1 on error.
 */
int server_init(server_t *server, int port, int board_size,
                int max_players, unsigned int seed);

/*
 * Game loop thread function.  Advances the game each tick and broadcasts state.
 * arg: pointer to server_t.
 */
void *server_game_loop(void *arg);

/*
 * Client handler thread function.  Manages one connected client.
 * arg: pointer to client_handler_arg_t (must be freed by this function).
 */
void *server_client_handler(void *arg);

/*
 * Main server loop: spawn game loop thread, then accept() in a loop.
 * Returns 0 on success, -1 on error.
 */
int server_start(server_t *server);

/*
 * Clean up all server resources.
 */
void server_cleanup(server_t *server);

/*
 * Helper: read exactly len bytes from fd into buf.
 * Returns 0 on success, -1 on error or disconnect.
 */
int recv_exact(int fd, uint8_t *buf, size_t len);

/*
 * Helper: send exactly len bytes from buf to fd.
 * Returns 0 on success, -1 on error.
 */
int send_all(int fd, const uint8_t *buf, size_t len);

#endif /* SERVER_H */
