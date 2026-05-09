#include <arpa/inet.h>
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>

#include "game_board.h"
#include "global.h"
#include "protocol.h"

/* ========================================================================= */
/*  protocol_serialize_welcome tests                                         */
/* ========================================================================= */

Test(protocol_suite, welcome_basic) {
	uint8_t buf[16];
	int ret = protocol_serialize_welcome(buf, sizeof(buf), 2, 20, 4);
	cr_assert_eq(ret, 4, "Welcome message should be 4 bytes");
	cr_assert_eq(buf[0], MSG_WELCOME, "Byte 0 should be MSG_WELCOME (0x10)");
	cr_assert_eq(buf[1], 2, "Byte 1 should be player_id (2)");
	cr_assert_eq(buf[2], 20, "Byte 2 should be board_size (20)");
	cr_assert_eq(buf[3], 4, "Byte 3 should be max_players (4)");
}