#include "protocol.h"
#include "debug.h"

int protocol_serialize_welcome(uint8_t *buf, size_t buf_len, int player_id,
                               int board_size, int max_players) {
	(void)buf;
	(void)buf_len;
	(void)player_id;
	(void)board_size;
	(void)max_players;
	return 0;
}

int protocol_serialize_game_state(uint8_t *buf, size_t buf_len,
                                  const game_board_t *board) {
	(void)buf;
	(void)buf_len;
	(void)board;
	return 0;
}

int protocol_serialize_dead(uint8_t *buf, size_t buf_len, int player_id) {
	(void)buf;
	(void)buf_len;
	(void)player_id;
	return 0;
}

int protocol_serialize_game_over(uint8_t *buf, size_t buf_len, int winner_id) {
	(void)buf;
	(void)buf_len;
	(void)winner_id;
	return 0;
}

int protocol_serialize_error(uint8_t *buf, size_t buf_len, uint8_t error_code) {
	(void)buf;
	(void)buf_len;
	(void)error_code;
	return 0;
}

int protocol_deserialize_client_msg(const uint8_t *buf, size_t buf_len,
                                    uint8_t *out_type, uint8_t *out_payload) {
	(void)buf;
	(void)buf_len;
	(void)out_type;
	(void)out_payload;
	return 0;
}
