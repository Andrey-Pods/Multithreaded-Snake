#include <criterion/criterion.h>
#include "png_steg.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* Helper function to compare two files byte by byte */
static int compare_files(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "rb");
    FILE *fp2 = fopen(file2, "rb");
    
    if (fp1 == NULL || fp2 == NULL) {
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        return -1;
    }
    
    /* Get file sizes */
    fseek(fp1, 0, SEEK_END);
    fseek(fp2, 0, SEEK_END);
    long size1 = ftell(fp1);
    long size2 = ftell(fp2);
    
    if (size1 != size2) {
        fclose(fp1);
        fclose(fp2);
        return -1;
    }
    
    /* Compare byte by byte */
    rewind(fp1);
    rewind(fp2);
    
    int match = 1;
    for (long i = 0; i < size1; i++) {
        int c1 = fgetc(fp1);
        int c2 = fgetc(fp2);
        if (c1 != c2) {
            match = 0;
            break;
        }
    }
    
    fclose(fp1);
    fclose(fp2);
    
    return match ? 0 : -1;
}

/* ========== ENCODE TESTS ========== */

Test(steg_encode, encode_lsb_basic_type3) {
    const char *input = "tests/data/Batman.png";  /* Color type 3 (palette) */
    const char *output = "tests/data/test_steg_encode_basic_type3.png";
    const char *answer = "tests/data/answer_test_steg_encode_basic_type3.png";
    const char *secret = "Hello World";

    /* Encode secret into PNG */
    int ret = png_encode_lsb(input, output, secret);
    cr_assert_eq(ret, 0, "Encoding should succeed for color type 3");

    FILE *fp = fopen(output, "rb");
    cr_assert_not_null(fp, "Output file should be created");
    fclose(fp);

    /* Compare with answer file */
    ret = compare_files(output, answer);
    cr_assert_eq(ret, 0, "Output file should match answer file");

    unlink(output);
}

Test(steg_encode, encode_lsb_basic_type6) {
    const char *input = "tests/data/Large_batman_6.png";  /* Color type 6 (RGBA) */
    const char *output = "tests/data/test_steg_encode_basic_type6.png";
    const char *answer = "tests/data/answer_test_steg_encode_basic_type6.png";
    const char *secret = "Hello World";

    /* Encode secret into PNG */
    int ret = png_encode_lsb(input, output, secret);
    cr_assert_eq(ret, 0, "Encoding should succeed for color type 6");

    FILE *fp = fopen(output, "rb");
    cr_assert_not_null(fp, "Output file should be created");
    fclose(fp);

    /* Compare with answer file */
    ret = compare_files(output, answer);
    cr_assert_eq(ret, 0, "Output file should match answer file");

    unlink(output);
}

Test(steg_encode, encode_long_secret_type3) {
    const char *input = "tests/data/Batman.png";  /* Color type 3 (palette) */
    const char *output = "tests/data/test_steg_long_type3.png";
    const char *answer = "tests/data/answer_test_steg_long_type3.png";
    char long_secret[1000];

    /* Create a long secret string */
    for (int i = 0; i < 999; i++) {
        long_secret[i] = 'A' + (i % 26);
    }
    long_secret[999] = '\0';

    /* Try to encode - might fail if image is too small */
    int ret = png_encode_lsb(input, output, long_secret);
    if (ret == 0) {
        FILE *fp = fopen(output, "rb");
        cr_assert_not_null(fp, "Output file should be created");
        fclose(fp);

        /* Compare with answer file */
        ret = compare_files(output, answer);
        cr_assert_eq(ret, 0, "Output file should match answer file");
        unlink(output);
    }
    /* If encoding failed due to image size, that's acceptable */
}

Test(steg_encode, encode_long_secret_type6) {
    const char *input = "tests/data/Large_batman_6.png";  /* Color type 6 (RGBA) */
    const char *output = "tests/data/test_steg_long_type6.png";
    const char *answer = "tests/data/answer_test_steg_long_type6.png";
    char long_secret[1000];

    /* Create a long secret string */
    for (int i = 0; i < 999; i++) {
        long_secret[i] = 'A' + (i % 26);
    }
    long_secret[999] = '\0';

    /* Try to encode - might fail if image is too small */
    int ret = png_encode_lsb(input, output, long_secret);
    if (ret == 0) {
        FILE *fp = fopen(output, "rb");
        cr_assert_not_null(fp, "Output file should be created");
        fclose(fp);

        /* Compare with answer file */
        ret = compare_files(output, answer);
        cr_assert_eq(ret, 0, "Output file should match answer file");
        unlink(output);
    }
    /* If encoding failed due to image size, that's acceptable */
}

Test(steg_encode, encode_null_terminated_type3) {
    const char *input = "tests/data/Batman.png";  /* Color type 3 (palette) */
    const char *output = "tests/data/test_steg_null_type3.png";
    const char *answer = "tests/data/answer_test_steg_null_type3.png";
    const char *secret = "Test\0Hidden\0Message";

    /* Encode secret (will only encode up to first null) */
    int ret = png_encode_lsb(input, output, secret);
    cr_assert_eq(ret, 0, "Encoding should succeed for color type 3");

    FILE *fp = fopen(output, "rb");
    cr_assert_not_null(fp, "Output file should be created");
    fclose(fp);

    /* Compare with answer file */
    ret = compare_files(output, answer);
    cr_assert_eq(ret, 0, "Output file should match answer file");

    unlink(output);
}

Test(steg_encode, encode_null_terminated_type6) {
    const char *input = "tests/data/Large_batman_6.png";  /* Color type 6 (RGBA) */
    const char *output = "tests/data/test_steg_null_type6.png";
    const char *answer = "tests/data/answer_test_steg_null_type6.png";
    const char *secret = "Test\0Hidden\0Message";

    /* Encode secret (will only encode up to first null) */
    int ret = png_encode_lsb(input, output, secret);
    cr_assert_eq(ret, 0, "Encoding should succeed for color type 6");

    FILE *fp = fopen(output, "rb");
    cr_assert_not_null(fp, "Output file should be created");
    fclose(fp);

    /* Compare with answer file */
    ret = compare_files(output, answer);
    cr_assert_eq(ret, 0, "Output file should match answer file");

    unlink(output);
}

Test(steg_encode, encode_null_parameters) {
    const char *input = "tests/data/Large_batman_6.png";
    const char *output = "tests/data/test_steg_null_params.png";
    const char *secret = "Test";

    /* Test null parameters */
    cr_assert_neq(png_encode_lsb(NULL, output, secret), 0);
    cr_assert_neq(png_encode_lsb(input, NULL, secret), 0);
    cr_assert_neq(png_encode_lsb(input, output, NULL), 0);
}

/* ========== DECODE/EXTRACT TESTS ========== */

Test(steg_extract, extract_lsb_basic_type3) {
    const char *input = "tests/data/answer_test_steg_encode_basic_type3.png";
    const char *expected = "Hello World";
    char extracted[256] = {0};

    /* Extract from answer file (pre-encoded with "Hello World") */
    int ret = png_extract_lsb(input, extracted, sizeof(extracted));
    cr_assert_gt(ret, 0, "Extraction should succeed");
    cr_assert_str_eq(extracted, expected, "Extracted secret should match expected");
}

Test(steg_extract, extract_lsb_basic_type6) {
    const char *input = "tests/data/answer_test_steg_encode_basic_type6.png";
    const char *expected = "Hello World";
    char extracted[256] = {0};

    /* Extract from answer file (pre-encoded with "Hello World") */
    int ret = png_extract_lsb(input, extracted, sizeof(extracted));
    cr_assert_gt(ret, 0, "Extraction should succeed");
    cr_assert_str_eq(extracted, expected, "Extracted secret should match expected");
}

Test(steg_extract, extract_long_secret_type3) {
    const char *input = "tests/data/answer_test_steg_long_type3.png";
    char expected[1000];
    char extracted[1000] = {0};

    /* Create expected long secret string */
    for (int i = 0; i < 999; i++) {
        expected[i] = 'A' + (i % 26);
    }
    expected[999] = '\0';

    /* Extract from answer file */
    int ret = png_extract_lsb(input, extracted, sizeof(extracted));
    cr_assert_gt(ret, 0, "Extraction should succeed");
    cr_assert_str_eq(extracted, expected, "Extracted secret should match expected");
}

Test(steg_extract, extract_long_secret_type6) {
    const char *input = "tests/data/answer_test_steg_long_type6.png";
    char expected[1000];
    char extracted[1000] = {0};

    /* Create expected long secret string */
    for (int i = 0; i < 999; i++) {
        expected[i] = 'A' + (i % 26);
    }
    expected[999] = '\0';

    /* Extract from answer file */
    int ret = png_extract_lsb(input, extracted, sizeof(extracted));
    cr_assert_gt(ret, 0, "Extraction should succeed");
    cr_assert_str_eq(extracted, expected, "Extracted secret should match expected");
}

Test(steg_extract, extract_null_terminated_type3) {
    const char *input = "tests/data/answer_test_steg_null_type3.png";
    const char *expected = "Test";
    char extracted[256] = {0};

    /* Extract from answer file (encoded with "Test\0Hidden\0Message", should extract "Test") */
    int ret = png_extract_lsb(input, extracted, sizeof(extracted));
    cr_assert_gt(ret, 0, "Extraction should succeed");
    cr_assert_str_eq(extracted, expected, "Should extract up to first null");
}

Test(steg_extract, extract_null_terminated_type6) {
    const char *input = "tests/data/answer_test_steg_null_type6.png";
    const char *expected = "Test";
    char extracted[256] = {0};

    /* Extract from answer file (encoded with "Test\0Hidden\0Message", should extract "Test") */
    int ret = png_extract_lsb(input, extracted, sizeof(extracted));
    cr_assert_gt(ret, 0, "Extraction should succeed");
    cr_assert_str_eq(extracted, expected, "Should extract up to first null");
}

Test(steg_extract, extract_null_parameters) {
    const char *input = "tests/data/answer_test_steg_encode_basic_type6.png";
    char extracted[256] = {0};

    /* Test null parameters */
    cr_assert_neq(png_extract_lsb(NULL, extracted, sizeof(extracted)), 0);
    cr_assert_neq(png_extract_lsb(input, NULL, sizeof(extracted)), 0);
    cr_assert_neq(png_extract_lsb(input, extracted, 0), 0);
}
