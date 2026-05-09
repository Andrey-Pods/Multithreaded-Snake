#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "global.h"
#include "our_zlib.h"

int main(int argc, char** argv) {
	char* filename = NULL;
	char* output_filename = NULL;
	int mode = -1;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			PRINT_USAGE(argv[0]);
			return 0;
		}
		if (strcmp(argv[i], "-i") == 0) {
			if (i < argc - 1) {
				filename = argv[i + 1];
				i++;
			}
		}
		else if (strcmp(argv[i], "-o") == 0) {
			if (i < argc - 1) {
				output_filename = argv[i + 1];
				i++;
			}
		}
		else if (strcmp(argv[i], "-m") == 0) {
			if (mode >= 0) {
				PRINT_ERROR_REQUIRE_ONE_OF_MCD();
				return 1;
			}
			mode = M_INFO;
		}
		else if (strcmp(argv[i], "-c") == 0) {
			if (mode >= 0) {
				PRINT_ERROR_REQUIRE_ONE_OF_MCD();
				return 1;
			}
			mode = M_DEFLATE;
		}
		else if (strcmp(argv[i], "-d") == 0) {
			if (mode >= 0) {
				PRINT_ERROR_REQUIRE_ONE_OF_MCD();
				return 1;
			}
			mode = M_INFLATE;
		}
	}

	if (filename == NULL) {
		PRINT_ERROR_MISSING_I_FLAG();
		return 1;
	}
	if (mode < 0) {
		PRINT_ERROR_REQUIRE_ONE_OF_MCD();
		return 1;
	}
	if ((mode == M_DEFLATE || mode == M_INFLATE) && output_filename == NULL) {
		PRINT_ERROR_MISSING_O_FLAG();
		return 1;
	}

	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		PRINT_ERROR_OPEN_FILE(filename);
		return 1;
	}

	gz_header_t info = {0};
	if (mode != M_DEFLATE) {
		if (parse_member(file, &info) != 0) {
			PRINT_ERROR_BAD_HEADER();
			fclose(file);
			return 1;
		}
	}

	switch (mode) {
		case M_INFO: {
			rewind(file);
			PRINT_MEMBER_SUMMARY_HEADER(filename);
			int member_idx = 0;
			while (parse_member(file, &info) == 0) {
				char label[256];
				if (info.name && info.name[0])
					snprintf(label, sizeof(label), "%s", info.name);
				else
					snprintf(label, sizeof(label), "%d", member_idx);
				/* TODO: set from actual CRC/HCRC verification when implemented */
				int crc_valid = 1;
				PRINT_MEMBER_LINE(label, info.cm, info.mtime, info.os,
					(unsigned)info.extra_len, info.comment, info.full_size, crc_valid);
				member_idx++;
				/* Multi-member: would need to skip compressed data to next member */
				break;
			}
			break;
		}
		case M_DEFLATE: {
			if (fseek(file, 0, SEEK_END) != 0) {
				fclose(file);
				return 1;
			}
			long file_size = ftell(file);
			if (file_size < 0) {
				fclose(file);
				return 1;
			}
			rewind(file);
			char* input = malloc((size_t)file_size);
			if (!input || fread(input, 1, (size_t)file_size, file) != (size_t)file_size) {
				free(input);
				fclose(file);
				return 1;
			}
			size_t out_len = 0;
			char* out_buf = deflate(filename, input, (size_t)file_size, &out_len);
			free(input);
			fclose(file);
			if (!out_buf) {
				return 1;
			}
			FILE* out = fopen(output_filename, "wb");
			if (!out) {
				PRINT_ERROR_OPEN_FILE(output_filename);
				free(out_buf);
				return 1;
			}
			if (out_len > 0 && fwrite(out_buf, 1, out_len, out) != out_len) {
				fclose(out);
				free(out_buf);
				return 1;
			}
			fclose(out);
			free(out_buf);
			break;
		}
		case M_INFLATE: {
			if (fseek(file, 0, SEEK_END) != 0) {
				fclose(file);
				return 1;
			}
			long file_size = ftell(file);
			if (file_size < 8) {
				fclose(file);
				return 1;
			}
			rewind(file);
			if (skip_gz_header_to_compressed_data(file, &info) != 0) {
				fclose(file);
				return 1;
			}
			long comp_start = ftell(file);
			if (comp_start < 0 || comp_start + 8 > file_size) {
				fclose(file);
				return 1;
			}
			size_t comp_len = (size_t)(file_size - 8 - comp_start);
			char* comp_buf = malloc(comp_len);
			if (!comp_buf || fread(comp_buf, 1, comp_len, file) != comp_len) {
				free(comp_buf);
				fclose(file);
				return 1;
			}
			fclose(file);
			char* out_buf = inflate(comp_buf, comp_len);
			free(comp_buf);
			if (!out_buf) {
				return 1;
			}
			FILE* out = fopen(output_filename, "wb");
			if (!out) {
				PRINT_ERROR_OPEN_FILE(output_filename);
				free(out_buf);
				return 1;
			}
			if (info.full_size > 0 && fwrite(out_buf, 1, info.full_size, out) != info.full_size) {
				fclose(out);
				free(out_buf);
				return 1;
			}
			fclose(out);
			free(out_buf);
			break;
		}
		default:
			PRINT_ERROR_REQUIRE_ONE_OF_MCD();
			break;
	}

	if (mode != M_DEFLATE && mode != M_INFLATE)
		fclose(file);
	return 0;
}
