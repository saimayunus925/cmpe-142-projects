

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdbool.h>

// Peterson algorithm is implemented here 

int main() {
	// objective: parent and child processes send msgs to each other with a pipe

	int flag_shared_mem_id, turn_shared_mem_id;

	flag_shared_mem_id = shmget(1024, sizeof(bool) * 2, IPC_CREAT | 0660);
	turn_shared_mem_id = shmget(2048, sizeof(int), IPC_CREAT | 0660);

	// we want to use these IDs as flags
	if (flag_shared_mem_id < 0 || turn_shared_mem_id < 0) {
		printf("ERROR - couldn't create shared memory or get flag\n");
		exit(1);
	}

	// create pipe
	int p[2], status;
	int PIPE = pipe(p);
	char read1[100], read2[100];
	// exception handling
	if (PIPE == -1) {
		printf("ERROR - pipe creation failed.\n");
		exit(1);
	}

	int i = 0, j = 1; // shared variables
	// flag[j] = true; // setting first turn for shared memory to child

	bool *flag_before_fork = (bool *) shmat(flag_shared_mem_id, NULL, 0);
	flag_before_fork[i] = false;
	flag_before_fork[j] = false;

	// now create child process
	int pid = fork();

	if (pid == 0) {
		// printf("Child Process - i: %d, j: %d\n", i, j);
		// child process
		int child_id = getpid();
		char child_msg[100];
		sprintf(child_msg, "Daddy, my name is %d", child_id);

		// create two block-level variables
		bool *flag = (bool *) shmat(flag_shared_mem_id, NULL, 0);
		int *turn = (int *) shmat(turn_shared_mem_id, NULL, 0);

		if (flag == (bool *) - 1 || turn == (int *) - 1) {
			printf("ERROR - couldn't access shared memory\n");
			exit(1);
		}

		// read parent msg and print it
		read(p[0], &read1, sizeof(read1));
		printf("%s\n", read1);

		// now child waits for its turn with the shared memory
		// sleep(1);
		flag[j] = true;
		*turn = i;

		// child does not need to block because it only runs once, so its while loop is commented
		// while (flag[i] && *turn == i);
		// child just sends message and sets its flag to false to give the next turn to the parent

		// now, write child msg to parent process
		write(p[1], &child_msg, sizeof(child_msg));

		flag[j] = false; // child sets its own flag to false, so parent gets the control

		exit(5);
	}
	else if (pid < 0) {
		// more exception handling
		printf("ERROR - process creation failure.\n");
		exit(1);
	}
	else {
		// printf("Parent Process - i: %d, j: %d\n", i, j);
		// parent process
		int parent_id = getpid();
		char parent_msg[100];
		sprintf(parent_msg, "I am your daddy! and my name is %d", parent_id);

		bool *flag = (bool *) shmat(flag_shared_mem_id, NULL, 0);
		int *turn = (int *) shmat(turn_shared_mem_id, NULL, 0);

		flag[j] = true; // child has first turn to use the critical section

		if (flag == (bool *) - 1 || turn == (int *) - 1) {
			printf("ERROR - couldn't access shared memory\n");
			exit(1);
		}

		// write parent msg to child process
		write(p[1], &parent_msg, sizeof(parent_msg));

		// now parent blocks itself from reading child's msg
		flag[i] = true;
		*turn = j;

		while (flag[j] && *turn == j);

		// read child msg and print it
		read(p[0], &read2, sizeof(read2));
		flag[i] = false; // now blocking is done
		printf("%s\n", read2);

		int temp = wait(&status);

		if (WEXITSTATUS(status) != 5) {
			printf("ERROR - abnormal termination.\n");
			exit(1);
		}

		// printf("Child exit status: %d\n", WEXITSTATUS(status));
	}
}
