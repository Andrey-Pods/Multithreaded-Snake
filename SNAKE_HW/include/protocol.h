/*
 * DO NOT MODIFY THIS FILE
 * Network protocol definitions and function prototypes - CSE 320 HW2
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "game_board.h"
#include "global.h"

/* Client message size (all client messages are exactly 2 bytes) */
#define CLIENT_MSG_SIZE 2

/* ---------------------------------------------------------------------------
 * Serialization functions  (server -> client)
 * --------------------------------------------------------------------------- */

/*
 * Serialize a WELCOME message into buf.
 * Returns number of bytes written (4) on success, -1 on error.
 */
int protocol_serialize_welcome(uint8_t *buf, size_t buf_len,
                               int player_id, int board_size, int max_players);

/*
 * Serialize a GAME_STATE message from the current board.
 * Returns number of bytes written on success, -1 on error.
 */
int protocol_serialize_game_state(uint8_t *buf, size_t buf_len,
                                  const game_board_t *board);

/*
 * Serialize a PLAYER_DEAD message.
 * Returns number of bytes written (2) on success, -1 on error.
 */
int protocol_serialize_dead(uint8_t *buf, size_t buf_len, int player_id);

/*
 * Serialize a GAME_OVER message.
 * Returns number of bytes written (2) on success, -1 on error.
 */
int protocol_serialize_game_over(uint8_t *buf, size_t buf_len, int winner_id);

/*
 * Serialize an ERROR message.
 * Returns number of bytes written (2) on success, -1 on error.
 */
int protocol_serialize_error(uint8_t *buf, size_t buf_len, uint8_t error_code);

/* ---------------------------------------------------------------------------
 * Deserialization functions  (client -> server)
 * --------------------------------------------------------------------------- */

/*
 * Parse a 2-byte client message.
 * Returns 0 on success, -1 on error (unknown type, bad buffer, etc.).
 */
int protocol_deserialize_client_msg(const uint8_t *buf, size_t buf_len,
                                    uint8_t *out_type, uint8_t *out_payload);

#endif /* PROTOCOL_H */
