#include <criterion/criterion.h>
#include "png_overlay.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
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

Test(overlay, overlay_type6) {
    const char *large = "tests/data/Large_batman_6.png";
    const char *small = "tests/data/Small_batman_6.png";
    const char *output = "tests/data/test_overlay_type6.png";
    const char *answer = "tests/data/answer_test_overlay_type6.png";

    /* Paste small image onto large at (50, 50) */
    int ret = png_overlay_paste(large, small, output, 50, 50);
    cr_assert_eq(ret, 0, "Paste should succeed");

    FILE *fp = fopen(output, "rb");
    cr_assert_not_null(fp, "Output file should be created");
    fclose(fp);

    /* Compare with answer file */
    ret = compare_files(output, answer);
    cr_assert_eq(ret, 0, "Output file should match answer file");

    unlink(output);
}

Test(overlay, overlay_type3) {
    const char *large = "tests/data/Large_batman_3.png";
    const char *small = "tests/data/Small_arrow_3.png";
    const char *output = "tests/data/test_overlay_type3.png";
    const char *answer = "tests/data/answer_test_overlay_type3.png";

    /* Paste small image onto large at (0, 0) */
    int ret = png_overlay_paste(large, small, output, 0, 0);
    cr_assert_eq(ret, 0, "Paste should succeed");

    FILE *fp = fopen(output, "rb");
    cr_assert_not_null(fp, "Output file should be created");
    fclose(fp);

    /* Compare with answer file */
    ret = compare_files(output, answer);
    cr_assert_eq(ret, 0, "Output file should match answer file");

    unlink(output);
}

Test(overlay, overlay_null_parameters) {
    const char *input = "tests/data/Large_batman_6.png";
    const char *output = "tests/data/test_overlay_null.png";

    /* Test null parameters for png_overlay_paste */
    cr_assert_neq(png_overlay_paste(NULL, input, output, 0, 0), 0);
    cr_assert_neq(png_overlay_paste(input, NULL, output, 0, 0), 0);
    cr_assert_neq(png_overlay_paste(input, input, NULL, 0, 0), 0);
}
