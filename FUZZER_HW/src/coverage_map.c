#include "coverage_map.h"

COVERAGE_MAP coverage_map_init() {
    return 0;
}

void coverage_map_fini(COVERAGE_MAP map) {
    (void) map;
}

COVERAGE_PRIORITY coverage_map_add(COVERAGE_MAP map, char *cov_data) {
    (void) map;
    (void) cov_data;
    return -1;
}
