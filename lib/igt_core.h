/*
 * Copyright © 2007,2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Daniel Vetter <daniel.vetter@ffwll.ch>
 *
 */


#ifndef IGT_CORE_H
#define IGT_CORE_H

bool igt_only_list_subtests(void);

bool __igt_fixture(void);
void __igt_fixture_complete(void);
void __igt_fixture_end(void) __attribute__((noreturn));
/**
 * igt_fixture:
 *
 * Annotate global test fixture code
 *
 * Testcase with subtests often need to set up a bunch of global state as the
 * common test fixture. To avoid such code interferring with the subtest
 * enumeration (e.g. when enumerating on systemes without an intel gpu) such
 * blocks should be annotated with igt_fixture.
 */
#define igt_fixture for (int igt_tokencat(__tmpint,__LINE__) = 0; \
			 igt_tokencat(__tmpint,__LINE__) < 1 && \
			 __igt_fixture() && \
			 (setjmp(igt_subtest_jmpbuf) == 0); \
			 igt_tokencat(__tmpint,__LINE__) ++, \
			 __igt_fixture_complete())

/* subtest infrastructure */
jmp_buf igt_subtest_jmpbuf;
void igt_subtest_init(int argc, char **argv);
typedef int (*igt_opt_handler_t)(int opt, int opt_index);
struct option;
int igt_subtest_init_parse_opts(int argc, char **argv,
				const char *extra_short_opts,
				struct option *extra_long_opts,
				const char *help_str,
				igt_opt_handler_t opt_handler);

bool __igt_run_subtest(const char *subtest_name);
/**
 * igt_subtest:
 *
 * Denote a subtest code block
 *
 * Magic control flow which denotes a subtest code block. Within that codeblock
 * igt_skip|success will only bail out of the subtest. The _f variant accepts a
 * printf format string, which is useful for constructing combinatorial tests.
 */
#define igt_tokencat2(x, y) x ## y
#define igt_tokencat(x, y) igt_tokencat2(x, y)
#define __igt_subtest_f(tmp, format...) \
	for (char tmp [256]; \
	     snprintf( tmp , sizeof( tmp ), \
		      format), \
	     __igt_run_subtest( tmp ) && \
	     (setjmp(igt_subtest_jmpbuf) == 0); \
	     igt_success())

/**
 * igt_subtest_f:
 * @...: format string
 *
 * Denote a subtest code block
 *
 * Like #igt_subtest, but also accepts a printf format string
 */
#define igt_subtest_f(f...) \
	__igt_subtest_f(igt_tokencat(__tmpchar, __LINE__), f)
#define igt_subtest(name) for (; __igt_run_subtest((name)) && \
				   (setjmp(igt_subtest_jmpbuf) == 0); \
				   igt_success())
const char *igt_subtest_name(void);
#define igt_main \
	static void igt_tokencat(__real_main, __LINE__)(void); \
	int main(int argc, char **argv) { \
		igt_subtest_init(argc, argv); \
		igt_tokencat(__real_main, __LINE__)(); \
		igt_exit(); \
	} \
	static void igt_tokencat(__real_main, __LINE__)(void) \

/**
 * igt_simple_init:
 *
 * Init for simple tests without subtests
 */
void igt_simple_init(void);
#define igt_simple_main \
	static void igt_tokencat(__real_main, __LINE__)(void); \
	int main(int argc, char **argv) { \
		igt_simple_init(); \
		igt_tokencat(__real_main, __LINE__)(); \
		exit(0); \
	} \
	static void igt_tokencat(__real_main, __LINE__)(void) \

/**
 * igt_skip:
 *
 * Subtest aware test skipping
 *
 * For tests with subtests this will either bail out of the current subtest or
 * mark all subsequent subtests as SKIP (in case some global setup code failed).
 *
 * For normal tests without subtest it will directly exit.
 */
__attribute__((format(printf, 1, 2)))
void igt_skip(const char *f, ...) __attribute__((noreturn));
__attribute__((format(printf, 5, 6)))
void __igt_skip_check(const char *file, const int line,
		      const char *func, const char *check,
		      const char *format, ...) __attribute__((noreturn));
/**
 * igt_success:
 *
 * Complete a (subtest) as successfull
 *
 * This bails out of a subtests and marks it as successful. For global tests it
 * it won't bail out of anything.
 */
void igt_success(void);

/**
 * igt_fail:
 *
 * Fail a testcase
 *
 * For subtest it just bails out of the subtest, when run in global context it
 * will exit. Note that it won't attempt to keep on running further tests,
 * presuming that some mandatory setup failed.
 */
void igt_fail(int exitcode) __attribute__((noreturn));
__attribute__((format(printf, 6, 7)))
void __igt_fail_assert(int exitcode, const char *file,
		       const int line, const char *func, const char *assertion,
		       const char *format, ...)
	__attribute__((noreturn));
/**
 * igt_exit:
 *
 * exit() for igts
 *
 * This will exit the test with the right exit code when subtests have been
 * skipped. For normal tests it exits with a successful exit code, presuming
 * everything has worked out. For subtests it also checks that at least one
 * subtest has been run (save when only listing subtests.
 */
void igt_exit(void) __attribute__((noreturn));
/**
 * igt_assert:
 *
 * Fails (sub-)test if a condition is not met
 *
 * Should be used everywhere where a test checks results.
 */
#define igt_assert(expr) \
	do { if (!(expr)) \
		__igt_fail_assert(99, __FILE__, __LINE__, __func__, #expr , NULL); \
	} while (0)
#define igt_assert_f(expr, f...) \
	do { if (!(expr)) \
		__igt_fail_assert(99, __FILE__, __LINE__, __func__, #expr , f); \
	} while (0)
/**
 * igt_assert_cmptint:
 *
 * Like #igt_assert, but displays the values being compared on failure.
 */
#define igt_assert_cmpint(n1, cmp, n2) \
	do { \
		int __n1 = (n1), __n2 = (n2); \
		if (__n1 cmp __n2) ; else \
		__igt_fail_assert(99, __FILE__, __LINE__, __func__, \
				  #n1 " " #cmp " " #n2, \
				  "error: %d %s %d\n", __n1, #cmp, __n2); \
	} while (0)

/**
 * igt_require:
 *
 * Skip a (sub-)test if a condition is not met
 *
 * This is useful to streamline the skip logic since it allows for a more flat
 * code control flow.
 */
#define igt_require(expr) igt_skip_on(!(expr))
#define igt_skip_on(expr) \
	do { if ((expr)) \
		__igt_skip_check(__FILE__, __LINE__, __func__, #expr , NULL); \
	} while (0)
#define igt_require_f(expr, f...) igt_skip_on_f(!(expr), f)
#define igt_skip_on_f(expr, f...) \
	do { if ((expr)) \
		__igt_skip_check(__FILE__, __LINE__, __func__, #expr , f); \
	} while (0)

/* fork support code */
bool __igt_fork(void);
/**
 * igt_fork:
 * @child: name of the int variable with the child number
 * @num_children: number of children to fork
 *
 * Fork parallel test threads with fork()
 *
 * Joining all test threads should be done with igt_waitchildren to ensure that
 * the exit codes of all children are properly reflected in the test status.
 */
#define igt_fork(child, num_children) \
	for (int child = 0; child < (num_children); child++) \
		for (; __igt_fork(); exit(0))
void igt_waitchildren(void);

struct igt_helper_process {
	bool running;
	bool use_SIGKILL;
	pid_t pid;
	int id;
};
bool __igt_fork_helper(struct igt_helper_process *proc);
void igt_stop_helper(struct igt_helper_process *proc);
void igt_wait_helper(struct igt_helper_process *proc);
#define igt_fork_helper(proc) \
	for (; __igt_fork_helper(proc); exit(0))

/* exit handler code */
typedef void (*igt_exit_handler_t)(int sig);

/* reliable atexit helpers, also work when killed by a signal (if possible) */
void igt_install_exit_handler(igt_exit_handler_t fn);
void igt_enable_exit_handler(void);
void igt_disable_exit_handler(void);

/* helpers to automatically reduce test runtime in simulation */
bool igt_run_in_simulation(void);
#define SLOW_QUICK(slow,quick) (igt_run_in_simulation() ? (quick) : (slow))
/**
 * igt_skip_on_simulation:
 *
 * Skip tests when INTEL_SIMULATION env war is set
 *
 * Skip the test when running on simulation (and that's relevant only when
 * we're not in the mode where we list the subtests).
 *
 * This function is subtest aware (since it uses igt_skip) and so can be used to
 * skip specific subtests or all subsequent subtests.
 */
void igt_skip_on_simulation(void);

/* structured logging */
enum igt_log_level {
	IGT_LOG_DEBUG,
	IGT_LOG_INFO,
	IGT_LOG_WARN,
	IGT_LOG_NONE,
};
__attribute__((format(printf, 2, 3)))
void igt_log(enum igt_log_level level, const char *format, ...);
#define igt_debug(f...) igt_log(IGT_LOG_DEBUG, f)
#define igt_info(f...) igt_log(IGT_LOG_INFO, f)
#define igt_warn(f...) igt_log(IGT_LOG_WARN, f)
extern enum igt_log_level igt_log_level;

#define igt_warn_on(condition) do {\
		if (condition) \
			igt_warn("Warning on condition %s in fucntion %s, file %s:%i\n", \
				 #condition, __func__, __FILE__, __LINE__); \
	} while (0)
#define igt_warn_on_f(condition, f...) do {\
		if (condition) {\
			igt_warn("Warning on condition %s in fucntion %s, file %s:%i\n", \
				 #condition, __func__, __FILE__, __LINE__); \
			igt_warn(f); \
		} \
	} while (0)


#endif /* IGT_CORE_H */
