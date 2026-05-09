#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "input.h"
#include "runner.h"
#include "fuzzer.h"

#include "macro.h"

#define TEST_PROGRAM_BIN_DIR "./programs/bin/"

static volatile sig_atomic_t sigusr1_flag = 0;
static volatile sig_atomic_t sigchld_flag = 0;

static void handle_sigusr1(int signum) {
    sigusr1_flag = 1;
}

static void handle_sigchld(int signum) {
    sigchld_flag = 1;
}

static void setup() {
    sigusr1_flag = 0;

    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigchld;
    sigaction(SIGCHLD, &sa, NULL);
}

static void fini() {
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
}

struct RunnerJob {
    const char *input;
    RUNNER_STATE expected_state;
    union {
        int expected_return_value;
        int expected_signal;
    };
};

#define TEST_JOBS_NAME(name) CAT(name, _jobs)
#define DECLARE_TEST_JOBS_ARRAY(name) static struct RunnerJob TEST_JOBS_NAME(name)[]

#define DEFINE_CRITERION_TEST()                                                                                             \
Test(runner_tests, TEST_NAME) {                                                                                             \
    char *__cmd_args[] = COMMAND;                                                                                           \
    cmd = __cmd_args[0];                                                                                                    \
    args = __cmd_args + 1;                                                                                                  \
    program_argc = sizeof(__cmd_args) / sizeof(char*) - 1;                                                                  \
    timeout = TIME_LIMIT;                                                                                                   \
                                                                                                                            \
    sigset_t block_mask, suspend_mask;                                                                                      \
    sigemptyset(&block_mask);                                                                                               \
    sigaddset(&block_mask, SIGUSR1);                                                                                        \
    sigaddset(&block_mask, SIGCHLD);                                                                                        \
    sigprocmask(SIG_BLOCK, &block_mask, &suspend_mask);                                                                     \
    sigdelset(&suspend_mask, SIGUSR1);                                                                                      \
    sigdelset(&suspend_mask, SIGCHLD);                                                                                      \
                                                                                                                            \
    int ret = 0;                                                                                                            \
    RUNNER runner = runner_init();                                                                                          \
    cr_assert_not_null(runner, "Expected runner to be non-null");                                                           \
    ret = runner_launch(runner);                                                                                            \
    cr_assert(ret != -1, "Expected runner_launch() to not fail with return value -1");                                      \
                                                                                                                            \
    size_t job_count = sizeof(TEST_JOBS_NAME(TEST_NAME)) / sizeof(struct RunnerJob);                                        \
    for (size_t i = 0; i < job_count; ++i) {                                                                                \
        INPUT input = make_input(TEST_JOBS_NAME(TEST_NAME)[i].input);                                                       \
        ret = fuzzer_send_runner_input(runner, input);                                                                      \
        cr_assert(ret != -1, "Expected fuzzer_send_runner_input to not fail with return value -1");                         \
        free_input(input);                                                                                                  \
                                                                                                                            \
        RUNNER_STATE state;                                                                                                 \
        int data;                                                                                                           \
        sigsuspend(&suspend_mask);                                                                                          \
        if (sigchld_flag) {                                                                                                 \
            sigchld_flag = 0;                                                                                               \
            runner_fini(runner);                                                                                            \
            cr_assert(1 == 0, "Runner process terminated unexpectedly");                                                    \
        }                                                                                                                   \
        state = fuzzer_attempt_receive_status(runner, &data);                                                               \
        cr_assert(state == TEST_JOBS_NAME(TEST_NAME)[i].expected_state,                                                     \
                "Expected the execution of command %s on input \"%s\" to be state %d. Got state %d instead",                \
                cmd, TEST_JOBS_NAME(TEST_NAME)[i].input, TEST_JOBS_NAME(TEST_NAME)[i].expected_state, state);               \
        if (TEST_JOBS_NAME(TEST_NAME)[i].expected_state == VALID) {                                                         \
            cr_assert(data == TEST_JOBS_NAME(TEST_NAME)[i].expected_return_value,                                           \
                "Expected the exit status of command %s on input \"%s\" to be %d. Got %d instead",                          \
                cmd, TEST_JOBS_NAME(TEST_NAME)[i].input, TEST_JOBS_NAME(TEST_NAME)[i].expected_return_value, data);         \
        }                                                                                                                   \
        else if (TEST_JOBS_NAME(TEST_NAME)[i].expected_state == CRASH) {                                                    \
            cr_assert(data == TEST_JOBS_NAME(TEST_NAME)[i].expected_signal,                                                 \
                "Expected the terminating signal of command %s on input \"%s\" to be %d. Got %d instead",                   \
                cmd, TEST_JOBS_NAME(TEST_NAME)[i].input, TEST_JOBS_NAME(TEST_NAME)[i].expected_signal, data);               \
        }                                                                                                                   \
    }                                                                                                                       \
    runner_fini(runner);                                                                                                    \
}


TestSuite(runner_tests, .init = setup, .fini = fini);

#define TEST_NAME run_N_jobs_0
#define COMMAND { "ls", "@@", NULL }
#define TIME_LIMIT 5
DECLARE_TEST_JOBS_ARRAY(TEST_NAME) = {
    { .input = ".",     .expected_state = VALID,    .expected_return_value = 0 },
    { .input = "..",    .expected_state = VALID,    .expected_return_value = 0 },
    { .input = "../..", .expected_state = VALID,    .expected_return_value = 0 }
};
DEFINE_CRITERION_TEST();
#undef TEST_NAME
#undef COMMAND
#undef TIME_LIMIT