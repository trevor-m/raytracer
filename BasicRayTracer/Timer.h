#ifndef TIMER_H
#define TIMER_H

#include <Windows.h>
#include <stdio.h>

class Timer {
private:
	LARGE_INTEGER freq;
	LARGE_INTEGER t_start, t_end;
	LARGE_INTEGER total_time;
public:
	Timer() {
		 QueryPerformanceFrequency(&freq);
	}
	
	void resetTimer(void) {
		total_time.QuadPart = 0;
	}

	void unpauseTimer(void) {
		QueryPerformanceCounter(&t_start);
	}

	void pauseTimer(void) {
		QueryPerformanceCounter(&t_end);
		total_time.QuadPart += (t_end.QuadPart - t_start.QuadPart);
	}

	void startTimer(void) {
		QueryPerformanceCounter(&t_start);
	}

	void stopTimer(void) {
		QueryPerformanceCounter(&t_end);
		total_time.QuadPart = (t_end.QuadPart - t_start.QuadPart);
	}

	void printTime(void) {
		fprintf(stderr,"%lf\n",((double) total_time.QuadPart) /((double) freq.QuadPart));
		fflush(stderr);
	}

	double getTime(void) const{
		return ((double) total_time.QuadPart) /((double) freq.QuadPart);
	}

};



#endif		/* TIMER_H */