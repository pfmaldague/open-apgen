#ifndef _PERF_UTILITIES_
#define _PERF_UTILITIES_

// memory libraries
#include <sys/resource.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>
#endif

// timing stuff
#include <cstdio>
#include <ctime>
#include <sys/time.h>

class perf_report {
public:
static std::clock_t	startcputime;
static std::clock_t	lastcputime;
static double		wall_at_start;

static unsigned long int MemoryUsed();

static void	initialize() {
	startcputime = std::clock();
	lastcputime = startcputime;
	wall_at_start = wall_time();
}

static double cpu_duration() {
	lastcputime = std::clock();
	double dur = (lastcputime - startcputime) / (double)CLOCKS_PER_SEC;
	return dur;
}

static double cpu_duration_no_update() {
	double temp = std::clock();
	double dur = (temp - startcputime) / (double)CLOCKS_PER_SEC;
	return dur;
}

//
// since last time reported:
//
static double last_cpu_duration() {
	double temp = std::clock();
	double dur = (temp - lastcputime) / (double)CLOCKS_PER_SEC;
	lastcputime = temp;
	return dur;
}

static double wall_time() {
	struct timeval	temptime;
	gettimeofday(&temptime, NULL);
	return (double)temptime.tv_sec + (double)temptime.tv_usec * .000001;
}

static void profile();

};


#endif /* _PERF_UTILITIES_ */
