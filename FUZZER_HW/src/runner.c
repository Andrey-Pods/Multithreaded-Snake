#include "runner.h"

RUNNER runner_init() {
    return 0;
}

void runner_fini(RUNNER runner) {
    (void) runner;
}

char *runner_coverage_map(RUNNER runner) {
    (void) runner;
    return 0;
}

INPUT runner_get_active_input(RUNNER runner) {
    (void) runner;
    return 0;
}

int fuzzer_send_runner_input(RUNNER runner, INPUT input) {
    (void) runner;
    (void) input;
    return 0;
}

char * runner_receive_fuzzer_input(RUNNER runner) {
    (void) runner;
    return 0;
}

int runner_alert_fuzzer(RUNNER runner, RUNNER_STATE state, int data) {
    (void) runner;
    (void) state;
    (void) data;
    return 0;
}

RUNNER_STATE fuzzer_attempt_receive_status(RUNNER runner, int *data) {
    (void) runner;
    (void) data;
    return 0;
}

int runner_launch(RUNNER runner) {
    (void) runner;
    return 0;
}



RUNNERS runners_init(int job_count) {
    (void) job_count;
    return 0;
}

void runners_fini(RUNNERS runners) {
    (void) runners;
}

int runners_submit_input(RUNNERS runners, INPUT input) {
    (void) runners;
    (void) input;
    return 0;
}

int runners_has_jobs(RUNNERS runners) {
    (void) runners;
    return 0;
}

int runners_has_active_jobs(RUNNERS runners) {
    (void) runners;
    return 0;
}

int runners_has_done_jobs(RUNNERS runners) {
    (void) runners;
    return 0;
}

int runners_has_ready_jobs(RUNNERS runners) {
    (void) runners;
    return 0;
}

void runners_check_if_jobs_done(RUNNERS runners) {
    (void) runners;
}

RUNNER runners_process_result(RUNNERS runners, RUNNER_STATE *state, int *data) {
    (void) runners;
    (void) state;
    (void) data;
    return 0;
}

int runners_reap(RUNNERS runners) {
    (void) runners;
    return 0;
}
