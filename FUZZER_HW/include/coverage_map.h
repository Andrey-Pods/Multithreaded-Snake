/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef COVERAGE_MAP_H
#define COVERAGE_MAP_H

/*
 * The coverage map is composed of two data structures: a bitmap and
 * a hash set. The coverage map stores the coverage-feedback data 
 * obtained by running the target program on a supplied input. The 
 * goal of the coverage map is to filter out inputs which exhibit 
 * behavior that has already been observed.
 *
 * See the assignment document for more details!
 */
typedef struct coverage_map * COVERAGE_MAP;

/*
 * An input is considered:
 *  (1) COV_HIGH_PRIO if the coverage-feedback data contains a new
 *                    branch that has never been executed in a prior
 *                    execution of the target program.
 *  (2) COV_LOW_PRIO  if the set of branches in the coverage-feedback 
 *                    data has never been seen before in a prior 
 *                    execution of the target program.
 *  (3) COV_NO_PRIO   if neither of the above is true.
 * 
 * You should check for COV_HIGH_PRIO before COV_LOW_PRIO.
 */
typedef enum coverage_priority {
    COV_NO_PRIO = -1,
    COV_LOW_PRIO,
    COV_HIGH_PRIO
} COVERAGE_PRIORITY;

/*
 * Initializes the coverage map. This should acquire/allocate any
 * resource or memory necessary to create the coverage map.
 *
 * @returns the handler to the newly initialized coverage map.
 */
COVERAGE_MAP coverage_map_init();

/*
 * Finalizes the coverage map. This should release/deallocate all
 * resources and memory that the coverage map is currently holding.
 *
 * @param map       The coverage map to finalize and free.
 */
void coverage_map_fini(COVERAGE_MAP map);

/*
 * Adds the coverage-feedback data to the coverage map and determines
 * the priority of the input based on the inserted data. The 
 * coverage-feedback data is added to both the bitmap and the hashset.
 * The function should determine the priority of the input using
 * the inserted coverage data and the data structures used here.
 *
 * @param map       The coverage map to insert the coverage-feedback 
 *                  data into
 * @param cov_data  The address of the coverage-feedback data to
 *                  insert into the coverage map
 * @returns the priority of the input based on the coverage data
 *          provided
 */
COVERAGE_PRIORITY coverage_map_add(COVERAGE_MAP map, char *cov_data);

#endif