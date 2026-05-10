#include "global.h"
#include "server.h"
#include "debug.h"
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

// Main entry point - parse command line arguments and run game server
int main(int argc, char *argv[]) {
	int opt;
	int port = -1;
	int board_size = BOARD_SIZE_DEFAULT;
	int max_players = MAX_PLAYERS_DEFAULT;
	unsigned int seed = (unsigned int)time(NULL);
	int seed_set = 0;
	server_t server;

	// Parse command line options for server configuration
	while((opt = getopt(argc, argv, "p:b:s:m:h")) != -1) {
		switch(opt) {
		// Parse port number - required, must be in valid range
		case 'p': {
			char *endptr = NULL;
			long value = strtol(optarg, &endptr, 10);

			if(optarg == NULL || *optarg == '\0' || *endptr != '\0' || value <= 0 ||
			   value > 65535) {
				ERR_INVALID_PORT(optarg);
				return EXIT_FAILURE;
			}
			port = (int)value;
			break;
		}
		// Parse board size - square board dimension
		case 'b': {
			char *endptr = NULL;
			long value = strtol(optarg, &endptr, 10);

			if(optarg == NULL || *optarg == '\0' || *endptr != '\0' ||
			   value < BOARD_SIZE_MIN || value > BOARD_SIZE_MAX) {
				ERR_INVALID_BOARD_SIZE((int)value);
				return EXIT_FAILURE;
			}
			board_size = (int)value;
			break;
		}
		// Parse random seed - optional, defaults to current time
		case 's': {
			char *endptr = NULL;
			unsigned long value = strtoul(optarg, &endptr, 10);

			if(optarg == NULL || *optarg == '\0' || *endptr != '\0' ||
			   value > UINT_MAX) {
				ERR_MSG("Invalid seed value: %s", optarg);
				return EXIT_FAILURE;
			}
			seed = (unsigned int)value;
			seed_set = 1;
			break;
		}
		// Parse maximum number of players allowed to connect
		case 'm': {
			char *endptr = NULL;
			long value = strtol(optarg, &endptr, 10);

			if(optarg == NULL || *optarg == '\0' || *endptr != '\0' ||
			   value < MAX_PLAYERS_MIN || value > MAX_PLAYERS_MAX) {
				ERR_INVALID_MAX_PLAYERS((int)value);
				return EXIT_FAILURE;
			}
			max_players = (int)value;
			break;
		}
		// Print usage information
		case 'h':
			PRINT_USAGE();
			return EXIT_SUCCESS;
		default:
			// Invalid option provided
			PRINT_USAGE();
			return EXIT_FAILURE;
		}
	}

	// Ensure no positional arguments are provided
	if(optind < argc) {
		PRINT_USAGE();
		return EXIT_FAILURE;
	}

	// Port is required - error if not specified
	if(port < 0) {
		ERR_PORT_REQUIRED();
		return EXIT_FAILURE;
	}

	// If seed was not explicitly provided, use current time for randomization
	if(!seed_set) {
		seed = (unsigned int)time(NULL);
	}

	debug("arguments parsed: port=%d, board_size=%d, max_players=%d, seed=%u",
	      port, board_size, max_players, seed);

	// Initialize server with parsed configuration parameters
	if(server_init(&server, port, board_size, max_players, seed) != 0) {
		ERR_MSG("Failed to initialize server on port %d", port);
		return EXIT_FAILURE;
	}

	// Print startup message and run server main loop
	PRINT_SERVER_STARTED(port, board_size, max_players);

	// Start server - blocks here until shutdown signal is received
	if(server_start(&server) != 0) {
		ERR_MSG("Server runtime failure");
		server_cleanup(&server);
		return EXIT_FAILURE;
	}

	// Clean up server resources before exiting
	server_cleanup(&server);
	return EXIT_SUCCESS;
}