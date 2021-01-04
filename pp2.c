

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int count = 0; // iteration counter
int sig_var = 0; // used to toggle b/w signals

void sigint_handler_1(int signum), sigusr1_handler(int signum3); // signal handler functions

int main() {

	// these catch the signals sent by parents
	signal(SIGINT, sigint_handler_1);
	signal(SIGUSR1, sigusr1_handler);

	while(1) { // the infinite loop
		sleep(2);
		// different response depending on each signal
		if (sig_var == 1)
			printf("# of Iterations: %d\n", count);
		else if (sig_var == 2)
			break;
		count++; // increment count by 1 when each iteration is over
	}
	exit(0);
}

/*
if sig_var = 2, terminate
if sig_var = 1, suppress print statements
if sig_var = 0, print count of iterations
*/

void sigint_handler_1(int signum1) {
	// toggle between 0 and 1
	if (sig_var == 0)
		sig_var = 1;
	else if (sig_var == 1)
		sig_var = 0;
}

void sigusr1_handler(int signum3) {
	sig_var = 2;
}
