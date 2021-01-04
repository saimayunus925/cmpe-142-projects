
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

// first, the program creates n zombies
// the zombies will be cleared once "kill -cont parent-id" is entered in terminal

void signal_handler (int signum) {
	// child sends signal to parent, this function starts right after that 
	printf("Signal with ID %d caught!\n", signum);
	sleep(1); // we want parent to live longer so child can become a zombie, sleep(1) makes parent pause even longer so child can definitely exit in time
}




int main(int argc, char **argv) {
	// register signal handler
	signal(SIGCONT, signal_handler);
	// create n zombie child processes

	// get n with getopt(), n is process_count
	int opt, option_exists = 0, process_count = 0;

	while ( (opt = getopt(argc, argv, "-n")) != -1) {
		switch (opt) {
			case 63 :
				// invalid case
				printf("ERROR - invalid argument. Please try './zombifier -n <process_count>'\n");
				printf("opt: %d\n", opt);
				exit(1);
				break;
			case 'n':
				option_exists = 1;
				break;
			default :
				break;
		}
	}

	if (option_exists == 1) {
		for (int j = 1; j < argc; j++) {
			if (atoi(argv[j]) != 0)
				process_count = atoi(argv[j]);
		}
	}
	if (process_count <= 0) {
		printf("ERROR - process count cannot be less than or equal to 0.\n");
		exit(1);
	}
	printf("# of child processes: %d\n", process_count);

	pid_t curr = getpid(); // curr: parent process ID
	for (int i = 0; i < process_count; i++) {
		// creating n processes
		pid_t pid = fork();
		if (pid == 0) {
			printf("Child ID: %d, Parent ID: %d\n", getpid(), getppid());
			sleep(1);
			// child sleeps for a bit before it exits
			exit(0);
		}
	}
	// parent process
	if (getpid() == curr) {
		printf("Parent ID: %d\n", curr);
		pause(); // pauses parent until signal handler function is complete
		printf("Resuming parent process...\n");
		for (int i = 0; i < argc; i++)
			wait(NULL);
		printf("Pause finished.\n");
	}
}


