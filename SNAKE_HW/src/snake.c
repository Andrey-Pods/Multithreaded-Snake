#include "snake.h"
#include "game_board.h"
#include "server.h"
#include "debug.h"

// validate a direction enum value
static int is_valid_direction(direction_t dir) {
	return dir == DIR_UP || dir == DIR_DOWN || dir == DIR_LEFT ||
	       dir == DIR_RIGHT;
}

// used to stop instant 180 turns 
static int directions_are_opposites(direction_t a, direction_t b) {
	return (a == DIR_UP && b == DIR_DOWN) ||
	       (a == DIR_DOWN && b == DIR_UP) ||
	       (a == DIR_LEFT && b == DIR_RIGHT) ||
	       (a == DIR_RIGHT && b == DIR_LEFT);
}

// queue a requested direction unless it is an immediate reverse
int snake_set_direction(snake_t *snake, direction_t dir) {
	if(snake == NULL || !is_valid_direction(dir) ||
	   !is_valid_direction(snake->direction)) {
		debug("snake_set_direction got invalid input");
		return -1;
	}

	if(!directions_are_opposites(snake->direction, dir)) {
		snake->next_direction = dir;
		debug("snake %d queued direction %d", snake->id, dir);
	} else {
		debug("snake %d tried to reverse direction, ignoring", snake->id);
	}

	return 0;
}

// move one snake forward by one tick and resolve collisions/apple eating
int snake_advance(struct game_board *board, int snake_id) {
	position_t new_head;
	position_t old_tail;
	cell_t destination;
	int i;
	snake_t *snake;

	if(board == NULL || board->cells == NULL || snake_id < 0 ||
	   snake_id >= board->max_snakes) {
		debug("snake_advance got invalid args for snake_id=%d", snake_id);
		return -1;
	}

	snake = &board->snakes[snake_id];
	if(!snake->alive || snake->length <= 0 ||
	   !is_valid_direction(snake->next_direction)) {
		debug("snake_advance called on bad snake state for id=%d", snake_id);
		return -1;
	}

	// apply whatever direction the client asked for last tick
	snake->direction = snake->next_direction;
	new_head = snake->body[0];

	// figure out where the head is going next
	switch(snake->direction) {
	case DIR_UP:
		new_head.y--;
		break;
	case DIR_DOWN:
		new_head.y++;
		break;
	case DIR_LEFT:
		new_head.x--;
		break;
	case DIR_RIGHT:
		new_head.x++;
		break;
	default:
		debug("snake %d has invalid direction", snake_id);
		return -1;
	}

	// out-of-bounds is treated as death
	if(new_head.x < 0 || new_head.x >= board->size ||
	   new_head.y < 0 || new_head.y >= board->size) {
		debug("snake %d moved out of bounds and died", snake_id);
		if(board_remove_snake(board, snake_id) != 0) {
			return -1;
		}
		return 1;
	}

	// hitting a wall or any snake body/head also kills this snake
	destination = board->cells[new_head.y * board->size + new_head.x];
	if(destination == CELL_WALL || destination >= CELL_SNAKE_0) {
		debug("snake %d hit something at (%d, %d) and died", snake_id,
		      new_head.x, new_head.y);
		if(board_remove_snake(board, snake_id) != 0) {
			return -1;
		}
		return 1;
	}

	// remember current tail in case this is a normal move
	old_tail = snake->body[snake->length - 1];
	if(destination == CELL_APPLE) {
		int old_length = snake->length;
		int shift_limit = old_length;

		// for an apple, we shift but do not clear the tail
		if(shift_limit >= MAX_SNAKE_LENGTH) {
			shift_limit = MAX_SNAKE_LENGTH - 1;
		}

		for(i = shift_limit; i > 0; i--) {
			snake->body[i] = snake->body[i - 1];
		}
		snake->body[0] = new_head;
		if(snake->length < MAX_SNAKE_LENGTH) {
			snake->length++;
		}
		board->cells[new_head.y * board->size + new_head.x] =
		    (cell_t)(CELL_SNAKE_0 + snake_id);
		debug("snake %d ate apple at (%d, %d), new length=%d", snake_id,
		      new_head.x, new_head.y, snake->length);
		(void)board_place_apple(board);
		return 2;
	}

	// normal move: clear old tail, shift body, put new head in front
	board->cells[old_tail.y * board->size + old_tail.x] = CELL_EMPTY;
	for(i = snake->length - 1; i > 0; i--) {
		snake->body[i] = snake->body[i - 1];
	}
	snake->body[0] = new_head;
	board->cells[new_head.y * board->size + new_head.x] =
	    (cell_t)(CELL_SNAKE_0 + snake_id);
	debug("snake %d moved to (%d, %d)", snake_id, new_head.x, new_head.y);
	return 0;
}