#include <criterion/criterion.h>
#include "png_chunks.h"
#include "png_reader.h"
#include "util.h"
#include <string.h>

Test(ihdr, parse_valid_ihdr) {
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    /* Read IHDR chunk */
    png_chunk_t chunk;
    cr_assert_eq(png_read_chunk(fp, &chunk), 0, "Failed to read chunk");
    cr_assert_str_eq(chunk.type, "IHDR", "First chunk should be IHDR");
    cr_assert_eq(chunk.length, 13, "IHDR chunk should be 13 bytes");
    
    /* Parse IHDR */
    png_ihdr_t ihdr;
    cr_assert_eq(png_parse_ihdr(&chunk, &ihdr), 0, "Failed to parse IHDR");
    
    /* Verify parsed values (Large_batman_6.png should be a valid PNG) */
    cr_assert_gt(ihdr.width, 0, "Width should be positive");
    cr_assert_gt(ihdr.height, 0, "Height should be positive");
    cr_assert_lt(ihdr.bit_depth, 17, "Bit depth should be valid");
    cr_assert_lt(ihdr.color_type, 7, "Color type should be valid");
    
    png_free_chunk(&chunk);
    fclose(fp);
}

Test(ihdr, parse_invalid_file) {
    /* Test with NULL chunk pointer */
    png_ihdr_t ihdr;
    cr_assert_neq(png_parse_ihdr(NULL, &ihdr), 0, "Should fail with NULL chunk pointer");
    
    /* Test with NULL output pointer */
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    png_chunk_t chunk;
    cr_assert_eq(png_read_chunk(fp, &chunk), 0, "Failed to read chunk");
    
    cr_assert_neq(png_parse_ihdr(&chunk, NULL), 0, "Should fail with NULL output pointer");
    
    png_free_chunk(&chunk);
    fclose(fp);
}

Test(plte, parse_plte) {
    FILE *fp = png_open("tests/data/Large_batman_3.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    /* Read chunks until we find PLTE (if it exists) */
    png_chunk_t chunk;
    int found_plte = 0;
    
    while (png_read_chunk(fp, &chunk) == 0) {
        if (strcmp(chunk.type, "PLTE") == 0) {
            found_plte = 1;
            break;
        }
        png_free_chunk(&chunk);
        if (strcmp(chunk.type, "IEND") == 0) {
            break;
        }
    }
    
    if (found_plte) {
        png_color_t *colors = NULL;
        size_t count = 0;
        cr_assert_eq(png_parse_plte(&chunk, &colors, &count), 0, "Should parse PLTE");
        cr_assert_not_null(colors, "Colors should be allocated");
        cr_assert_gt(count, 0, "Should have at least one color");
        cr_assert_leq(count, 256, "Should have at most 256 colors");
        
        free(colors);
        png_free_chunk(&chunk);
    }
    
    fclose(fp);
}

Test(plte, parse_plte_null) {
    FILE *fp = png_open("tests/data/Large_batman_3.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    png_chunk_t chunk;
    cr_assert_eq(png_read_chunk(fp, &chunk), 0, "Failed to read chunk");
    
    cr_assert_neq(png_parse_plte(NULL, NULL, NULL), 0, "Should fail with NULL chunk");
    
    png_color_t *colors = NULL;
    size_t count = 0;
    cr_assert_neq(png_parse_plte(&chunk, NULL, &count), 0, "Should fail with NULL colors");
    cr_assert_neq(png_parse_plte(&chunk, &colors, NULL), 0, "Should fail with NULL count");
    
    png_free_chunk(&chunk);
    fclose(fp);
}
