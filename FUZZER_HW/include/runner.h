/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef RUNNER_H
#define RUNNER_H

#include "input.h"

/*
 * This type represents a runner job. It contains all the data necessary
 * for managing the runner process and for the interprocess communication
 * involving the runner.
 *
 * See the assignment document for more details!
 */
typedef struct runner * RUNNER;

/*
 * This enum represents the different states that the result of running
 * the target program can end in.
 *      (1) NO_STATE: Used as a return code
 *      (2) VALID: The target program completed normally
 *      (3) CRASH: The target program crashed due to a terminating signal
 *      (4) TIMEOUT: The target program has timed out
 */
typedef enum runner_state {
    NO_STATE = -1,
    VALID,
    CRASH,
    TIMEOUT
} RUNNER_STATE;

/*
 * Initializes a runner instance. This function does not launch the child
 * process itself, but it sets up the runner including the IPC used
 * for communication between processes involving the runner.
 *
 * See the assignment document for more details on what this function
 * should do and what IPC is necessary.
 *
 * @returns the newly initialized runner instance.
 */
RUNNER runner_init();

/*
 * Finalizes a runner instance. This function should release/deallocate
 * all resources and memory that the runner instance holds. Additionally,
 * this process should terminate and reap the runner process if the
 * runner has been launched.
 *
 * @param runner    The runner instance to finalalize and free
 */
void runner_fini(RUNNER runner);

/*
 * Retrieves the address of the shared memory used for the coverage-feedback
 * data for a specific runner instance. The shared memory should be memory
 * mapped into the address space, so that it can be accessed like a buffer.
 *
 * @param runner    The runner instance
 * @returns the address of the memory-mapped shared memory used to store
 *          coverage-feedback data
 */
char *runner_coverage_map(RUNNER runner);

/*
 * Retrieves the input that the runner process is currently handling if it
 * exists.
 *
 * @param runner    The runner instance
 * @returns the input that the runner instance is currently executing. NULL
 *          if the runner is not currently running an input.
 */
INPUT runner_get_active_input(RUNNER runner);

/*
 * Handles the logic for the main fuzzer process sending an input to a runner
 * process. The main fuzzer process should write the input to the appropriate
 * pipe to the target runner. 
 *
 * @param runner    The runner instance
 * @param input     The input to send to the runner instance
 * @return 0 if the fuzzer successfully wrote to the runner. -1 otherwise.
 */
int fuzzer_send_runner_input(RUNNER runner, INPUT input);

/*
 * Handles the logic for the runner process to receive an input from the
 * main fuzzer process. The runner process should read from the appropriate pipe
 * to obtain the input from the fuzzer. This will become the active input, and
 * the runner will execute the target program with the given input. If a 
 * SIGTERM/SIGINT/SIGHUP is received during this function, the function 
 * returns NULL. 
 *
 * See the assignment document for more information.
 *
 * @param runner    The runner instance
 * @returns the read input from the main fuzzer process or NULL
 */
char * runner_receive_fuzzer_input(RUNNER runner);

/*
 * Handles the logic for the runner process to alert the main fuzzer process
 * of the results of running the target program on a given input. This involves
 * writing to the appropriate pipe and then sending SIGUSR1 signal afterwards
 * to the main process.
 *
 * @param runner    The runner instance
 * @param state     The exit state of the target program on the input
 * @param data      Any auxiliary data along with the state
 * @returns 0 if the runner successfully notified the main process. -1 otherwise.
 */
int runner_alert_fuzzer(RUNNER runner, RUNNER_STATE state, int data);

/*
 * Handles the logic for the main fuzzer process to receive the exit state and
 * data from a runner. This involves attempting to read from a non-blocking pipe
 * after the main fuzzer process received the SIGUSR1 signal. 
 *
 * @param runner    The runner instance
 * @param data      The address to an integer to store auxiliary data
 * @returns the exit state of the program. If there is nothing in the pipe, NO_STATE
 */
RUNNER_STATE fuzzer_attempt_receive_status(RUNNER runner, int *data); 

/*
 * Launches the runner process by forking. It should finish setting up the IPC
 * which includes closing the appropriate pipes and redirecting file descriptors.
 * The runner process needs to install the appropriate signal handlers and then
 * enter a loop. In the loop, the runner will continuously retrieve inputs from
 * the main fuzzer process, executing the input on the target program, and then
 * reporting the results back to the fuzzer process. The runner process terminates
 * when it receives signals such as SIGTERM, SIGINT, and SIGHUP.
 *
 * See the assignment document for more specific details on the behavior of the
 * runner process.
 *
 * @param runner    The runner instance
 * @returns 0 if the runner process has successfully launched. -1 otherwise.
 * @note Only the process who calls this function should receive the return value!
 */
int runner_launch(RUNNER runner);

// ----------------------------------------------------------------------------- //

/*
 * This type represents a set of runner processes. The operations provided perform
 * operations on these groups of processes. The runner processes are separated into
 * three categories: ready, active, and done. See the assignment document for more 
 * details on these three categories. 
 */
typedef struct runners * RUNNERS;

/*
 * Initializes the runners instance which is composed of many runner processes. 
 * This function needs to initialize and launch the runners. Additionally, it
 * must add the runners into the ready queue meaning that the runners are ready
 * to accept input from the fuzzer. 
 *
 * @param job_count The total number of runner job processes
 * @returns the newly initialized and launched set of runner proceses
 */
RUNNERS runners_init(int job_count);

/*
 * Finalizes the runners instance. This function is responsible for reaping
 * and finalizing the individual runner instances that make up the runners 
 * object.
 *
 * @param runners   the set of runners to finalize
 */
void runners_fini(RUNNERS runners);

/*
 * Submits an input to one of the runner processes in the ready queue.
 * The runner process which received this input will be moved to the
 * active queue.
 *
 * @param runners   The set of runners
 * @param input     The input to send to a ready runner process
 * @return 0 if successful. -1 otherwise.
 */
int runners_submit_input(RUNNERS runners, INPUT input);

/*
 * @param runners   The set of runners
 * @returns a boolean on if there are still runner processes 
 *          running
 */
int runners_has_jobs(RUNNERS runners);

/*
 * @param runners   The set of runners
 * @returns a boolean on if there are any active runner processes 
 */
int runners_has_active_jobs(RUNNERS runners);

/*
 * @param runners   The set of runners
 * @returns a boolean on if there are any done runner processes
 */
int runners_has_done_jobs(RUNNERS runners);

/*
 * @param runners   The set of runners
 * @return a boolean on if there are any ready runner processes
 */
int runners_has_ready_jobs(RUNNERS runners);

/*
 * Checks each active runner process and determines if the process is
 * done executing its supplied input. If the process is done, which
 * can be determined by checking the contents of the pipe from the
 * runner, then the process is moved into the done queue from the
 * active queue.
 *
 * @param runner    The set of runners
 */
void runners_check_if_jobs_done(RUNNERS runners);

/*
 * Processes the results of one of the runner process that is in the
 * done queue. This function returns to the caller the program exit state,
 * any auxiliary data such as exit code and the terminating signal as well
 * as a reference to the runner instance itself. This process is moved from
 * the done queue to the ready queue.
 *
 * @param runners   The set of runners
 * @param state     The exit state of the target program the runner was executing
 * @param data      The address to store the auxiliary data
 * @returns a reference to the runner instance that has been processed and moved
 *          to the ready queue.
 */
RUNNER runners_process_result(RUNNERS runners, RUNNER_STATE *state, int *data);

/*
 * Goes through all the processes and determines if the runner process has been
 * terminated. If so, this function will reap the runner process and finalize its
 * corresponding runner which include removing it from any queue the runner is
 * currently in. 
 *
 * @param runners   The set of runners
 * @returns 0 if either no process was reaped or reaping the terminated processes
 *          was successful. -1 if reaping the terminated process was unsuccessful.
 */
int runners_reap(RUNNERS runners);

#endif