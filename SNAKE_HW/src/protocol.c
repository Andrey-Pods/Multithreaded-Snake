#include "protocol.h"
#include "debug.h"
#include <arpa/inet.h>
#include <string.h>

// quick helper so direction checks are readable
static int is_valid_direction_value(uint8_t dir) {
	return dir <= DIR_RIGHT;
}

// serialize fixed-size welcome packet
int protocol_serialize_welcome(uint8_t *buf, size_t buf_len, int player_id,
                               int board_size, int max_players) {
	if(buf == NULL || buf_len < 4 || player_id < 0 || player_id >= MAX_PLAYERS ||
	   board_size < BOARD_SIZE_MIN || board_size > BOARD_SIZE_MAX ||
	   max_players < MAX_PLAYERS_MIN || max_players > MAX_PLAYERS_MAX) {
		debug("protocol_serialize_welcome got invalid input");
		return -1;
	}

	// WELCOME is fixed-size and super simple 
	buf[0] = MSG_WELCOME;
	buf[1] = (uint8_t)player_id;
	buf[2] = (uint8_t)board_size;
	buf[3] = (uint8_t)max_players;
	debug("serialized WELCOME for player %d", player_id);
	return 4;
}

// serialize full game snapshot with alive snakes and body segments
int protocol_serialize_game_state(uint8_t *buf, size_t buf_len,
                                  const game_board_t *board) {
	int i;
	int alive_count = 0;
	size_t offset = 0;
	size_t required = 6;

	if(buf == NULL || board == NULL || board->cells == NULL || board->size <= 0) {
		debug("protocol_serialize_game_state got invalid input");
		return -1;
	}

	// first pass: count alive snakes and compute exact byte size needed
	for(i = 0; i < MAX_PLAYERS; i++) {
		if(board->snakes[i].alive) {
			int length = board->snakes[i].length;

			if(length <= 0 || length > MAX_SNAKE_LENGTH) {
				debug("invalid snake length while serializing state: id=%d length=%d",
				      i, length);
				return -1;
			}
			required += 4 + (size_t)length * 4;
			alive_count++;
		}
	}

	if(required > buf_len || alive_count > 255) {
		debug("buffer too small for game state: need=%zu have=%zu", required,
		      buf_len);
		return -1;
	}

	buf[offset++] = MSG_GAME_STATE;
	buf[offset++] = (uint8_t)alive_count;

	// apple position goes in network byte order
	{
		uint16_t apple_x = htons((uint16_t)board->apple.x);
		uint16_t apple_y = htons((uint16_t)board->apple.y);

		memcpy(buf + offset, &apple_x, sizeof(apple_x));
		offset += sizeof(apple_x);
		memcpy(buf + offset, &apple_y, sizeof(apple_y));
		offset += sizeof(apple_y);
	}

	for(i = 0; i < MAX_PLAYERS; i++) {
		if(board->snakes[i].alive) {
			int seg;
			const snake_t *snake = &board->snakes[i];
			uint16_t net_len;

			// include snake metadata + every body segment
			if(!is_valid_direction_value((uint8_t)snake->direction)) {
				debug("invalid snake direction while serializing state: id=%d dir=%d",
				      i, snake->direction);
				return -1;
			}

			buf[offset++] = (uint8_t)snake->id;
			net_len = htons((uint16_t)snake->length);
			memcpy(buf + offset, &net_len, sizeof(net_len));
			offset += sizeof(net_len);
			buf[offset++] = (uint8_t)snake->direction;

			// each segment is serialized as 2-byte x and 2-byte y
			for(seg = 0; seg < snake->length; seg++) {
				uint16_t x = htons((uint16_t)snake->body[seg].x);
				uint16_t y = htons((uint16_t)snake->body[seg].y);

				memcpy(buf + offset, &x, sizeof(x));
				offset += sizeof(x);
				memcpy(buf + offset, &y, sizeof(y));
				offset += sizeof(y);
			}
		}
	}

	debug("serialized GAME_STATE for %d alive snakes (%zu bytes)", alive_count,
	      offset);
	return (int)offset;
}

// serialize single-player death notice
int protocol_serialize_dead(uint8_t *buf, size_t buf_len, int player_id) {
	if(buf == NULL || buf_len < 2 || player_id < 0 || player_id >= MAX_PLAYERS) {
		debug("protocol_serialize_dead got invalid input");
		return -1;
	}

	buf[0] = MSG_PLAYER_DEAD;
	buf[1] = (uint8_t)player_id;
	debug("serialized PLAYER_DEAD for player %d", player_id);
	return 2;
}

// serialize game-over packet with winner id or 0xFF for no winner
int protocol_serialize_game_over(uint8_t *buf, size_t buf_len, int winner_id) {
	if(buf == NULL || buf_len < 2 ||
	   !(winner_id == -1 || (winner_id >= 0 && winner_id < MAX_PLAYERS))) {
		debug("protocol_serialize_game_over got invalid input");
		return -1;
	}

	buf[0] = MSG_GAME_OVER;
	buf[1] = (winner_id == -1) ? 0xFF : (uint8_t)winner_id;
	debug("serialized GAME_OVER winner=%d", winner_id);
	return 2;
}

// serialize protocol-level error message
int protocol_serialize_error(uint8_t *buf, size_t buf_len, uint8_t error_code) {
	if(buf == NULL || buf_len < 2 ||
	   !(error_code == ERR_GAME_FULL || error_code == ERR_ALREADY_JOINED ||
	     error_code == ERR_INVALID_MSG)) {
		debug("protocol_serialize_error got invalid input");
		return -1;
	}

	buf[0] = MSG_ERROR;
	buf[1] = error_code;
	debug("serialized ERROR code=0x%02x", error_code);
	return 2;
}

// parse and validate a fixed-size client packet
int protocol_deserialize_client_msg(const uint8_t *buf, size_t buf_len,
                                    uint8_t *out_type, uint8_t *out_payload) {
	uint8_t type;
	uint8_t payload;

	if(buf == NULL || out_type == NULL || out_payload == NULL ||
	   buf_len < CLIENT_MSG_SIZE) {
		debug("protocol_deserialize_client_msg got invalid input");
		return -1;
	}

	type = buf[0];
	payload = buf[1];

	// validate allowed payload values per message type
	switch(type) {
	case MSG_JOIN:
		if(payload != 0x00) {
			debug("bad JOIN payload: 0x%02x", payload);
			return -1;
		}
		break;
	case MSG_DIRECTION:
		if(!is_valid_direction_value(payload)) {
			debug("bad DIRECTION payload: 0x%02x", payload);
			return -1;
		}
		break;
	case MSG_LEAVE:
		if(payload != 0x00) {
			debug("bad LEAVE payload: 0x%02x", payload);
			return -1;
		}
		break;
	default:
		debug("unknown client msg type: 0x%02x", type);
		return -1;
	}

	*out_type = type;
	*out_payload = payload;
	debug("deserialized client msg type=0x%02x payload=0x%02x", type, payload);
	return 0;
}
