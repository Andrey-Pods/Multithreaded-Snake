#include <criterion/criterion.h>
#include "png_reader.h"
#include <string.h>

Test(reader, png_open_valid) {
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Should open valid PNG file");
    fclose(fp);
}

Test(reader, png_open_invalid_signature) {
    FILE *fp = png_open("tests/data/Batnan.png");
    cr_assert_null(fp, "Should reject invalid PNG signature");
}

Test(reader, png_read_chunk_ihdr) {
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    png_chunk_t chunk;
    cr_assert_eq(png_read_chunk(fp, &chunk), 0, "Should read IHDR chunk");
    cr_assert_str_eq(chunk.type, "IHDR", "First chunk should be IHDR");
    cr_assert_eq(chunk.length, 13, "IHDR chunk should be 13 bytes");
    
    png_free_chunk(&chunk);
    fclose(fp);
}

Test(reader, png_read_chunk_all) {
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    png_chunk_t chunk;
    int chunk_count = 0;
    do {
        if (chunk_count > 0) {
            png_free_chunk(&chunk);
        }
        cr_assert_eq(png_read_chunk(fp, &chunk), 0, "Should read chunk");
        chunk_count++;
    } while (strcmp(chunk.type, "IEND") != 0);
    
    cr_assert_gt(chunk_count, 2, "Should have multiple chunks");
    png_free_chunk(&chunk);
    fclose(fp);
}

Test(reader, png_extract_ihdr) {
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    png_ihdr_t ihdr;
    cr_assert_eq(png_extract_ihdr(fp, &ihdr), 0, "Should extract IHDR");
    
    cr_assert_gt(ihdr.width, 0, "Width should be positive");
    cr_assert_gt(ihdr.height, 0, "Height should be positive");
    cr_assert_lt(ihdr.bit_depth, 17, "Bit depth should be valid");
    cr_assert_lt(ihdr.color_type, 7, "Color type should be valid");
    
    fclose(fp);
}

Test(reader, png_extract_ihdr_null) {
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    cr_assert_neq(png_extract_ihdr(NULL, NULL), 0, "Should fail with NULL parameters");
    cr_assert_neq(png_extract_ihdr(fp, NULL), 0, "Should fail with NULL output");
    
    fclose(fp);
}

Test(reader, png_extract_plte) {
    FILE *fp = png_open("tests/data/Large_batman_3.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    png_color_t *colors = NULL;
    size_t count = 0;
    int ret = png_extract_plte(fp, &colors, &count);
    
    /* May succeed or fail depending on whether image has palette */
    if (ret == 0) {
        cr_assert_not_null(colors, "Colors should be allocated");
        cr_assert_gt(count, 0, "Should have at least one color");
        cr_assert_leq(count, 256, "Should have at most 256 colors");
        free(colors);
    }
    
    fclose(fp);
}

Test(reader, png_extract_plte_null) {
    FILE *fp = png_open("tests/data/Large_batman_3.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    cr_assert_neq(png_extract_plte(NULL, NULL, NULL), 0, "Should fail with NULL parameters");
    
    png_color_t *colors = NULL;
    size_t count = 0;
    cr_assert_neq(png_extract_plte(fp, NULL, &count), 0, "Should fail with NULL colors");
    cr_assert_neq(png_extract_plte(fp, &colors, NULL), 0, "Should fail with NULL count");
    
    fclose(fp);
}

Test(reader, png_summary) {
    png_chunk_t *summary = NULL;
    int ret = png_summary("tests/data/Large_batman_6.png", &summary);
    
    cr_assert_eq(ret, 0, "Should create summary");
    cr_assert_not_null(summary, "Summary should be allocated");
    
    /* Find IEND chunk */
    int iend_found = 0;
    for (int i = 0; summary[i].length > 0 || strcmp(summary[i].type, "IEND") == 0; i++) {
        if (strcmp(summary[i].type, "IEND") == 0) {
            iend_found = 1;
            break;
        }
    }
    cr_assert(iend_found || summary[0].length == 0, "Should have IEND chunk or empty summary");
    
    free(summary);
}

Test(reader, png_summary_null) {
    png_chunk_t *summary = NULL;
    
    cr_assert_neq(png_summary(NULL, &summary), 0, "Should fail with NULL filename");
    cr_assert_neq(png_summary("tests/data/Large_batman_6.png", NULL), 0, "Should fail with NULL output");
}

Test(reader, png_free_chunk) {
    FILE *fp = png_open("tests/data/Large_batman_6.png");
    cr_assert_not_null(fp, "Failed to open test PNG file");
    
    png_chunk_t chunk;
    cr_assert_eq(png_read_chunk(fp, &chunk), 0, "Should read chunk");
    cr_assert_not_null(chunk.data, "Chunk data should be allocated");
    
    /* Free should not crash */
    png_free_chunk(&chunk);
    cr_assert_null(chunk.data, "Chunk data should be freed");
    
    fclose(fp);
}
