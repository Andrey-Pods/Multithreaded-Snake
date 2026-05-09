/*
 * DO NOT MODIFY THIS FILE
 * Global definitions for the Snake Server - CSE 320 HW2
 */
#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdlib.h>
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Game constants
 * --------------------------------------------------------------------------- */
#define TICK_INTERVAL_MS    2000      /* Milliseconds between game ticks */

#define BOARD_SIZE_MIN      10
#define BOARD_SIZE_MAX      50
#define BOARD_SIZE_DEFAULT  20

#define MAX_PLAYERS_MIN     1
#define MAX_PLAYERS_MAX     8
#define MAX_PLAYERS_DEFAULT 4

/* Maximum buffer size for serialized game state messages */
#define GAME_STATE_BUF_SIZE 8192

/* ---------------------------------------------------------------------------
 * Protocol message types  (client -> server)
 * --------------------------------------------------------------------------- */
#define MSG_JOIN        0x01
#define MSG_DIRECTION   0x02
#define MSG_LEAVE       0x03

/* ---------------------------------------------------------------------------
 * Protocol message types  (server -> client)
 * --------------------------------------------------------------------------- */
#define MSG_WELCOME     0x10
#define MSG_GAME_STATE  0x20
#define MSG_PLAYER_DEAD 0x30
#define MSG_GAME_OVER   0x40
#define MSG_ERROR       0xF0

/* ---------------------------------------------------------------------------
 * Error codes sent inside MSG_ERROR
 * --------------------------------------------------------------------------- */
#define ERR_GAME_FULL       0x01
#define ERR_ALREADY_JOINED  0x02
#define ERR_INVALID_MSG     0x03

/* ---------------------------------------------------------------------------
 * Macros for formatted output
 * --------------------------------------------------------------------------- */
#define PRINT_USAGE() \
    printf("Usage: bin/snake_server [options]\n" \
           "Options:\n" \
           "  -p port        Port number to listen on (required)\n" \
           "  -b board_size  Board size NxN (default: %d, min: %d, max: %d)\n" \
           "  -s seed        Random seed for apple placement (default: time-based)\n" \
           "  -m max_players Maximum concurrent players (default: %d, min: %d, max: %d)\n" \
           "  -h             Print this help message\n", \
           BOARD_SIZE_DEFAULT, BOARD_SIZE_MIN, BOARD_SIZE_MAX, \
           MAX_PLAYERS_DEFAULT, MAX_PLAYERS_MIN, MAX_PLAYERS_MAX)

#define PRINT_SERVER_STARTED(port, size, max) \
    printf("Snake server started on port %d (board: %dx%d, max players: %d)\n", \
           (port), (size), (size), (max))

#define PRINT_CLIENT_CONNECTED(addr, port) \
    debug("Client connected from %s:%d", (addr), (port))

#define PRINT_CLIENT_DISCONNECTED(id) \
    debug("Player %d disconnected", (id))

/* ---------------------------------------------------------------------------
 * Error message macros (print to stderr)
 * --------------------------------------------------------------------------- */
#define ERR_MSG(fmt, ...) \
    fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__)

#define ERR_PORT_REQUIRED() \
    ERR_MSG("Port number is required (-p flag)")

#define ERR_INVALID_PORT(p) \
    ERR_MSG("Invalid port number: %s", (p))

#define ERR_INVALID_BOARD_SIZE(s) \
    ERR_MSG("Invalid board size: %d (must be %d-%d)", (s), BOARD_SIZE_MIN, BOARD_SIZE_MAX)

#define ERR_INVALID_MAX_PLAYERS(m) \
    ERR_MSG("Invalid max players: %d (must be %d-%d)", (m), MAX_PLAYERS_MIN, MAX_PLAYERS_MAX)

#define ERR_BIND_FAILED(port) \
    ERR_MSG("Failed to bind to port %d", (port))

#endif /* GLOBAL_H */
