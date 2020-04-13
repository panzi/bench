#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <getopt.h>
#include <spawn.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
#	define __WINDOWS__
#endif

#if defined(__WINDOWS__)
#	define DEV_NULL "NUL"
#else
#	define DEV_NULL "/dev/null"
#endif

#define NANO_DIVISOR 1000000000

static void usage(int argc, char *const argv[]) {
	printf("Usage: %s [OPTIONS] [--] COMMAND...\n", argc > 0 ? argv[0] : "bench");
	printf(
		"\n"
		"OPTIONS:\n"
		"\n"
		"\t-h, --help             Print this help message.\n"
		"\t-t, --times=N          Repeat command N times. (default: 100)\n"
	);
}

extern char **environ;

int timecmp(const struct timespec *time1, const struct timespec *time2) {
	if (time1->tv_sec < time2->tv_sec) {
		return -1;
	}

	if (time1->tv_sec > time2->tv_sec) {
		return 1;
	}

	if (time1->tv_nsec < time2->tv_nsec) {
		return -1;
	}

	if (time1->tv_nsec > time2->tv_nsec) {
		return 1;
	}

	return 0;
}

int qsort_timecmp(const void *time1, const void *time2) {
	return timecmp(time1, time2);
}

struct timespec timediv(const struct timespec time, size_t divisor) {
	const time_t sec = time.tv_sec / divisor;
	const time_t sec_delta = time.tv_sec - sec * divisor;

	struct timespec result = {
		.tv_sec  = sec,
		.tv_nsec = (time.tv_nsec + sec_delta * NANO_DIVISOR) / divisor,
	};

	if (result.tv_nsec >= NANO_DIVISOR) {
		const long sec = result.tv_nsec / NANO_DIVISOR;
		result.tv_sec  += sec;
		result.tv_nsec -= sec * NANO_DIVISOR;
	}

	return result;
}

struct timespec timeadd(const struct timespec time1, const struct timespec time2) {
	struct timespec result = {
		.tv_sec  = time1.tv_sec  + time2.tv_sec,
		.tv_nsec = time1.tv_nsec + time2.tv_nsec,
	};

	if (result.tv_nsec >= NANO_DIVISOR) {
		long sec = result.tv_nsec / NANO_DIVISOR;
		result.tv_sec  += sec;
		result.tv_nsec -= sec * NANO_DIVISOR;
	}

	return result;
}

struct timespec timesub(const struct timespec time1, const struct timespec time2) {
	struct timespec result = {
		.tv_sec  = time1.tv_sec  - time2.tv_sec,
		.tv_nsec = time1.tv_nsec - time2.tv_nsec,
	};

	if (result.tv_nsec < 0) {
		result.tv_sec  -= 1;
		result.tv_nsec += NANO_DIVISOR;
	}

	return result;
}

int main(int argc, char *argv[]) {
	struct option long_options[] = {
		{"help",  no_argument,       0, 'h'},
		{"times", required_argument, 0, 't'},
		{0,       0,                 0,  0 },
	};

	int status = 0;
	size_t times = 100;
	char *endptr = NULL;
	struct timespec *timestamps = NULL;
	posix_spawn_file_actions_t actions;

	for(;;) {
		int c = getopt_long(argc, argv, "ht:", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				usage(argc, argv);
				goto error;

			case 't':
				endptr = NULL;
				errno = 0;
				long long value = strtoll(optarg, &endptr, 10);
				if (errno != 0) {
					fprintf(stderr, "illegal value for --times: %s (%s)\n", optarg, strerror(errno));
					usage(argc, argv);
					return 1;
				} else if (!*optarg || *endptr || value <= 0
#if ULONG_MAX < LLONG_MAX
					|| value > (long long)ULONG_MAX
#endif
				) {
					fprintf(stderr, "illegal value for --times: %s\n", optarg);
					usage(argc, argv);
					goto error;
				}
				times = (size_t) value;
				break;

			case '?':
				usage(argc, argv);
				goto error;
		}
	}

	if (argc - optind <= 0) {
		fprintf(stderr, "no command specified\n");
		usage(argc, argv);
		return 1;
	}

	timestamps = calloc(times, sizeof(struct timespec));

	if (!times) {
		perror("allocating array of times");
		goto error;
	}

	printf("runs:    %6zu\n", times);

	int errnum = posix_spawn_file_actions_init(&actions);
	if (errnum != 0) {
		fprintf(stderr, "posix_spawn_file_actions_init: %s\n", strerror(errnum));
	}

	errnum = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO,  DEV_NULL, O_RDONLY, 0);
	if (errnum != 0) {
		fprintf(stderr, "posix_spawn_file_actions_addopen(stdin): %s\n", strerror(errnum));
	}
	
	errnum = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, DEV_NULL, O_WRONLY, 0);
	if (errnum != 0) {
		fprintf(stderr, "posix_spawn_file_actions_addopen(stdout): %s\n", strerror(errnum));
	}
	
	errnum = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, DEV_NULL, O_WRONLY, 0);
	if (errnum != 0) {
		fprintf(stderr, "posix_spawn_file_actions_addopen(stderr): %s\n", strerror(errnum));
	}

	struct timespec start_time = { .tv_sec = 0, .tv_nsec = 0 };
	if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) {
		perror("clock_gettime(CLOCK_MONOTONIC)");
		goto error;
	}

	for (size_t index = 0; index < times; ++ index) {
		pid_t child_pid = 0;
		int child_status = 0;

		errnum = posix_spawnp(&child_pid, argv[optind], &actions, NULL, &argv[optind], environ);
		if (errnum != 0) {
			fprintf(stderr, "[run %zu] %s: %s\n", index + 1, argv[optind], strerror(errnum));
			goto error;
		}

		for (;;) {
			if (waitpid(child_pid, &child_status, 0) == -1) {
				perror(argv[optind]);
				goto error;
			}

			if (WIFEXITED(child_status)) {
				int exit_status = WEXITSTATUS(child_status);
				if (exit_status != 0) {
					fprintf(stderr, "[run %zu] child exited with status %d\n", index + 1, exit_status);
					goto error;
				}
				break;
			} else if (WIFSIGNALED(child_status)) {
				fprintf(stderr, "[run %zu] child killed by signal %d\n", index + 1, WTERMSIG(child_status));
				goto error;
			}
		}

		if (clock_gettime(CLOCK_MONOTONIC, &timestamps[index]) != 0) {
			fprintf(stderr, "[run %zu] clock_gettime(CLOCK_MONOTONIC): %s\n", index + 1, strerror(errnum));
			goto error;
		}
	}

	const struct timespec sum_time = timesub(timestamps[times - 1], start_time);
	const struct timespec avg_time = timediv(sum_time, times);
	struct timespec min_time = { .tv_sec = 0, .tv_nsec = 0 };
	struct timespec max_time = { .tv_sec = 0, .tv_nsec = 0 };

	struct timespec prev_timestamp = start_time;

	for (size_t index = 0; index < times; ++ index) {
		const struct timespec tmp = timesub(timestamps[index], prev_timestamp);

		if (index == 0 || timecmp(&min_time, &tmp) > 0) {
			min_time = tmp;
		}

		if (index == 0 || timecmp(&max_time, &tmp) < 0) {
			max_time = tmp;
		}

		prev_timestamp = timestamps[index];
		timestamps[index] = tmp;
	}

	qsort(timestamps, times, sizeof(struct timespec), qsort_timecmp);

	const struct timespec med_time = times % 2 == 0 ?
		timediv(timeadd(timestamps[times / 2], timestamps[times / 2 - 1]), 2) :
		timestamps[times / 2];

	printf("sum:     %6lu.%09lu sec\n", sum_time.tv_sec, sum_time.tv_nsec);
	printf("minimum: %6lu.%09lu sec\n", min_time.tv_sec, min_time.tv_nsec);
	printf("maximum: %6lu.%09lu sec\n", max_time.tv_sec, max_time.tv_nsec);
	printf("average: %6lu.%09lu sec\n", avg_time.tv_sec, avg_time.tv_nsec);
	printf("median:  %6lu.%09lu sec\n", med_time.tv_sec, med_time.tv_nsec);

	goto cleanup;

error:
	status = 1;

cleanup:

	if (timestamps) {
		posix_spawn_file_actions_destroy(&actions);
		free(timestamps);
	}

	return status;
}
