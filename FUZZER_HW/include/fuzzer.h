/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef FUZZER_H
#define FUZZER_H

#include <stdio.h>  // For FILE

#include "runner.h" // For RUNNER_STATE for event logging

/*
 * The entry point to the fuzzer program after parsing the command line arguments.
 * This function should set up signal handlers, initialize the input queue, coverage
 * map, and the runners job processes appropriately. Next, it should read inputs
 * from the seed file and add it to the input queue before being sent to the runner
 * job process where it will be executed on the target program. The function should
 * enter a loop where it will repeatedly dispatch mutated inputs to runners and retrieve
 * results from the the runners. The results will be used to guide the fuzzer via updating
 * the coverage map and input queue. The program should report whenever an input causes
 * the target program to crash. This will continue until either the fuzzer reaches its
 * maximum number of mutated inputs to test or a terminating signal has been sent to the
 * fuzzer. When the fuzzer has completed, this function should clean up any acquired or
 * allocated resource.
 *
 * See the assignment document for more specifics!
 *
 * @param seed_file     The file containing entries of initial inputs to feed the fuzzer.
 *                      Entries are separated by a newline.
 * @param job_count     The total number of job runner processes to start.
 * @param input_count   The total number of mutated inputs for the fuzzer to attempt 
 *                      before exiting.
 * @param time_limit    The time limit set for the target program before being treated
 *                      as the program timing out.
 * @param program       The target program command line arguments that the fuzzer will
 *                      fuzz. One of the program arguments must be the input placeholder @@.
 * @returns 0 if the fuzzer ran successfully, -1 otherwise.           
 */
int run_fuzzer(FILE *seed_file, int job_count, int input_count, int time_limit, char *target_program[]);

// ================================================================================================== //

/*
 * The following functions are event logging functions that you MUST call before or after
 * certain events described below. This will be used to test your final program, and
 * misusing these events could cause you to fail the assignment's test cases! Additionally,
 * do not add significant delay in calling these functions as that can also cause
 * you to fail our test cases. You have been warned.
 *
 * All of the functions below are implemented in the `lib/fzl.a` archive file which will
 * be linked to your program. Additionally, all of the functions below are not
 * async-signal-safe, and thus, should not be called within a signal handler.
 *
 * Specifics on when to call these event logging functions are described below and in
 * the assignment document. Be sure to read both!
 */

/*
 * This event logging function should be called immediately upon entering `run_fuzzer`.
 * It must be called from the main fuzzer process.
 *
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_init(const char *why);

/*
 * This function is not actually an event logger. It is will print the debug message
 * and will not send an event. You may use this function freely. Using this function
 * may be useful because when multiple processes write to the same standard out, the
 * outputs may interleave. This function synchronizes writes to standard out.
 * 
 * @param why       If non-null, the function will print this debugging message
 */
void fzl_debug(const char *why);

/*
 * This event logging function should be called immediately before the fuzzer
 * process sends an input to the runner. This must be called from the main
 * fuzzer process.
 *
 * @param runner_id The ID of the runner job
 * @param input     The input send by the fuzzer to that specific runner
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_sending_input(int runner_id, const char *input, const char *why);

/*
 * This event logging function should be called immediately when the runner child
 * process begins. This must be called from the runner child process that was
 * initiated.
 *
 * @param runner_id The ID of the runner job
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_runner_init(int runner_id, const char *why);

/*
 * This event logging function should be called immediately after the runner process
 * has received an input from the main fuzzer process. This must be called from the
 * runner process that has received the input.
 *
 * @param runner_id The ID of the runner job
 * @param input     The input that the runner job has received from the fuzzer
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_runner_received_input(int runner_id, const char *input, const char *why);

/*
 * This event logging function should be called immediately before the call to
 * `execvp` that is used to launch the target program in the runner job's child
 * process. This must be called from the child process of the runner.
 *
 * @param runner_id The ID of the runner job
 * @param argv      The argv that is supplied to the `execvp` with the substituted
 *                  input
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_runner_launch(int runner_id, char *argv[], const char *why);

/*
 * This event logging function should be called immediately before the runner job
 * sends the exit status of the target program that it has launched. This must be
 * called from the runner process which received the exit status.
 *
 * @param runner_id The ID of the runner job
 * @param state     The exit state of the target program: VALID, CRASH, or TIMEOUT
 * @param aux_data  Auxilary data supplied along with the exit state of the target
 *                  program. This is only meaningful if the state is VALID or CRASH.
 *                  If the state is VALID, the program exit code is passed in. If
 *                  the state is CRASH, the terminating signal is passed instead.
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_runner_sending_status(int runner_id, RUNNER_STATE state, int aux_data, const char *why);

/*
 * This event logging function should be called immediately before the runner job
 * process exits. This must be called from the runner process which is exiting
 * immediately after this call.
 *
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_runner_fini(int runner_id, const char *why);

/*
 * This event logging function should be called immediately after the fuzzer process
 * receives the exit status from the runner that the fuzzer has assigned to run an input.
 * This must be called from the main fuzzer process.
 *
 * @param runner_id The ID of the runner job which sent the status
 * @param state     The exit state of the target program from the runner: VALID, CRASH, or TIMEOUT
 * @param aux_data  Auxilary data supplied along with the exit state of the target
 *                  program. This is only meaningful if the state is VALID or CRASH.
 *                  This value must match the accompanying value passed in to 
 *                  `fzl_runner_sending_status`
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_received_status(int runner_id, RUNNER_STATE state, int aux_data, const char *why);

/*
 * This event logging function should be called immediately before the exiting the
 * `run_fuzzer` function either by return or the `exit` system call. This must be 
 * called from the main fuzzer process.
 *
 * @param why       If non-null, the function will print this debugging message along 
 *                  with the event.
 */
void fzl_fini(const char *why);

#endif