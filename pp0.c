#include <stdio.h>                                                                            
#include <unistd.h>                                                                                                     
#include <stdlib.h>                                                                                                                               
#include <signal.h>                                                                                                                               
#include <sys/wait.h>                                                                                                          

void child_signal_handler(int n) {                                             
	// this function runs when parent sends SGCONT code to child, since SGCONT is in the child process                                         
	// so, the child process pauses, then sleeps for 5 seconds, then exits                      
	sleep(5);                                                                      
}

int main() {    
	pid_t pid;                     
	int status;               
	if ((pid = fork()) == 0) {         
		// child process                   
		signal(SIGCONT, child_signal_handler);      
		// printing child process ID and then \n     
		printf("Child Process ID: %d\n", getpid());          
		// pausing and then exiting with code 5        
		pause();                                     
		exit(5);                                   
	}                   
	else {              
		// parent process  
		sleep(1);            
		// parent process sending "terminate" signal to child process
		kill(pid, SIGCONT);   
		// pid is 0 here, so waitpid will wait for the child process to terminate, since the child's process group ID is 
		// equal to that of the calling process                       
		int child_pid = waitpid(0, &status, WCONTINUED);      
		// WEXITSTATUS macro finds exit code in last byte of "status" variable and prints it 
		printf("childpid: %d, exitstatus:%d\n", child_pid, WEXITSTATUS(status));  
	}   
	return 0; 
}      