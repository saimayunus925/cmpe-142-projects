

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

// global variables for Peterson's algorithm

int flag[2], turn, loop;

// thread functions

void* thread_1_fn();
void* thread_2_fn();

// signal handler for SIGINT
void signal_handler(int signum);

int main() {

	signal(SIGINT, signal_handler);

	// initialize global variables
	flag[0] = 0;
	flag[1] = 0;
	turn = 0;
	loop = 1;

	pthread_t th_id_1, th_id_2;

	// create thread 1
	pthread_create(&th_id_1, NULL, &thread_1_fn, NULL);

	// create thread 2
	pthread_create(&th_id_2, NULL, &thread_2_fn, NULL);

	// wait for each thread to stop
	pthread_join(th_id_1, NULL);
	pthread_join(th_id_2, NULL);

	return 0;
}

// simple signal handler
void signal_handler(int signum) {
	loop = 0;
	// printf("Signal with ID %d caught!\n", signum);
}

// critical-section problem: how to make sure that at any time, only one process is using its critical section
// critical section - code segment where program can get shared variables, only one process can execute in the section at a time
// Peterson's algorithm solves this problem

// i: thread 1
// j: thread 2

void* thread_1_fn() {
	// Peterson's algorithm
	int i = 0, j = 1;
	int first_msg = 1; // resets after sending first msg
	while (loop) {
		flag[i] = 1; // i's turn to use critical section
		turn = j; // it is j's turn by default here
		while (flag[j] == 1 && turn == j); // as long as it's j's turn, keep running (also till flag[j] == 0)
		// now flag[j] is 0, and it is i's turn

		// if flag = 1, we reset it to 0 (this happens the first time): else, we print the thread's response to receiving msg
		if (first_msg == 1)
			first_msg = 0;
		else
			printf("thread 1: pong! thread 2 ping received\n");
		printf("thread 1: ping thread 2\n"); // sends this to thread 2
		flag[i] = 0; // now i's turn is done, j gets a chance
	}
}

void* thread_2_fn() {
	// Peterson's algorithm
	int i = 0, j = 1;
	while (loop) {
		flag[j] = 1; // j's turn to use critical section
		turn = i; // it is i's turn by default here
		while (flag[i] == 1 && turn == i); // as long as it's i's turn, keep running (also till flag[i] == 0)
		// now flag[i] is 0, and it is j's turn

		// no flag needed here because thread 2 is always sending and receiving messages

		printf("thread 2: pong! thread 1 ping received\n");
		printf("thread 2: ping thread 1\n");
		flag[j] = 0; // now j's turn is done, i gets a chance
	}
}
