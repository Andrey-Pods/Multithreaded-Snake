#include <criterion/criterion.h>

#include <stdint.h>
#include <string.h>

#include "coverage_map.h"
#include "global.h"

TestSuite(coverage_map_tests, .timeout=1);

Test(coverage_map_tests, cov_map_initialization_0) {
    COVERAGE_MAP map = coverage_map_init();
    cr_assert_not_null(map, "Expected coverage_map_init to return non-null");
    coverage_map_fini(map);
}

Test(coverage_map_tests, cov_map_add_0) {
    COVERAGE_MAP map = coverage_map_init();

    uint64_t data[] = {
        0x0001042008001041ull,
        0x1080400010410400ull,
        0x0004100440040082ull,
        0x0210008802202000ull,
        0x8028001100020100ull,
        0x0842810080100000ull,
        0x0000224225008008ull,
        0x4500080000884810ull
    };
    const size_t N = sizeof(data) / sizeof(data[0]);
    char *buffers[sizeof(data) / sizeof(data[0])] = { 0 };
    for (size_t i = 0; i < N; ++i) {
        buffers[i] = calloc(COVERAGE_MAP_SIZE / 8, 1);
        memcpy(buffers[i], &data[i], sizeof(uint64_t));
    }

    for (size_t i = 0; i < N; ++i) {
        COVERAGE_PRIORITY actual_prio = coverage_map_add(map, buffers[i]);
        cr_assert(actual_prio == COV_HIGH_PRIO, "Expected prio returned from coverage_map_add to be COV_HIGH_PRIO");
    }

    for (size_t i = 0; i < N; ++i) {
        free(buffers[i]);
    }
    coverage_map_fini(map);
}