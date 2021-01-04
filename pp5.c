#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
 #include <netdb.h> 
#include <unistd.h>
#include <netinet/in.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/tcp.h>
#include <arpa/inet.h>
 
#define MessageLength 120
#define BuffSize 10


int buffer_index=0; //index of buffer
int gracefull_exit=0; //variable for signal handler
 
int u=0; /* if 0, then shared memory. if 1, then Unix socket. */
int p=0; /* p=1, process runs as producer */
int c=0; /* c=1, process runs as consumer */
int message_buffer_id; // ID of msg buffer
int PortNumber=8080; //socket port number
int MaxConnectionCount=100; //socket can have maximum 100 client-server connections
char buffer[MessageLength]; //buffer used to read/write socket msg

void producer(char *message);
void consumer(char *message);
 
struct Message{
   char  msg[MessageLength];
} ;
struct Message *message_buffer;	// buffer of msgs, one to write to memory (producer) and other to read from memory (consumer)

sem_t *full, *empty, *mutex; //semaphores
int *process_count;
int *in_index;
int *out_index;
 
int running_as=0; /* if 1, runs as consumer. if 2, runs as producer. */
int graceful_exit;
int iam_waiting=0;
int msg_print=1;
int buffer_size = 10; // default buffer size is 10 but it will be overwritten by -q <buffer_size>
void signal_handler(int sig_num) 
{ 
	printf("sigint called");
 	graceful_exit=1;
	/*
	FOR DEBUGGING PURPOSES
	if (*((int *) full)==0 && iam_waiting)
	{	
		printf("\nSIGINT handler: I am exiting once my wait for semaphor full is over\n");
		//sem_post(full);
	}
	if (*((int *) empty)==0 && iam_waiting)
	{	
		printf("\nSIGINT handler: I am exiting once my wait for semaphor empty is over\n");
		//sem_post(empty);
	}
	if (*((int *) mutex)==0 && iam_waiting)
	{	
		printf("\nSIGINT handler: I am exiting once my wait for semaphor mutex  is over\n");
	    //sem_post(mutex);
	}
*/	
}


int main(int argc, char **argv) {
	int m, q, s, e; // flags for checking if each type of argument has a value
	char message[120]=""; // where we pass the message to
	graceful_exit=0;
	signal(SIGINT, signal_handler); 
	for (int i = 1; i < argc; i++) { // compares argv arguments to argument options
		if (strcmp(argv[i], "-p") == 0)
			p = 1;
		else if (strcmp(argv[i], "-c") == 0)
			c = 1;
		else if (strcmp(argv[i], "-m") == 0) {
			m = 1;
			strcpy(message, argv[i+1]); // we are assuming argument will be like "-m "hello world""
		}
		else if (strcmp(argv[i], "-q") == 0) {
			q = 1;
			buffer_size = atoi(argv[i+1]); // we are assuming the argument will be like "-q 10", for example
		}
		else if (strcmp(argv[i], "-u") == 0)
			u = 1;
		else if (strcmp(argv[i], "-s") == 0)
			s = 1;
		else if (strcmp(argv[i], "-e") == 0)
			msg_print= 1;
	}
	 
	if (p == 0 && c==0) {  
		printf("ERROR: -p or -c option is required to launch producer or consumer\n");
		exit(1);
	}
	else if (buffer_size == 0) { // -q isn't there so no size
		printf("ERROR - buffer size cannot be 0.\n");
		exit(1);
	}
	else if (strcmp(message, "") == 0) { // 'message' string is empty b/c here there's no -m
		printf("ERROR : -m <msg> option is required \n");
		exit(1);
	}
	
	// key for 'full' semaphore
	key_t sem_full_key = ftok("/tmp", 'f');
	int sem_full_id = shmget(sem_full_key, sizeof(sem_t), IPC_CREAT | 0666);
	if (sem_full_id < 0) {
          printf("sem_full_id ERROR\n");
          exit(1);
	}
	full = (sem_t *) shmat(sem_full_id, NULL, 0);

	// do same code for 'empty' and 'mutex' semaphores
	key_t sem_empty_key = ftok("/tmp", 'e');
	int sem_empty_id = shmget(sem_empty_key, sizeof(sem_t), IPC_CREAT | 0666);
	if (sem_empty_id < 0) {
          printf("sem_empty_id ERROR\n");
          exit(1);
	}
	empty = (sem_t *) shmat(sem_empty_id, NULL, 0);
	key_t sem_mutex_key = ftok("/tmp", 'm');
	int sem_mutex_id = shmget(sem_mutex_key, sizeof(sem_t), IPC_CREAT | 0666);
	if (sem_mutex_id < 0) {
          printf("sem_mutex_id ERROR\n");
          exit(1);
	}
	mutex = (sem_t *) shmat(sem_mutex_id, NULL, 0);
	
	// same process for process_count, in_index, and out_index
	
	key_t process_count_key = ftok("/tmp", 'i');
	int process_count_id = shmget(process_count_key, sizeof(int), IPC_CREAT | 0666);
	if (process_count_id < 0) {
		printf("process_count_id ID ERROR\n");
		exit(1);
	}
	// process count: tally of current processes. when last process has exited, this is 0, so resources can be freed
	process_count = (int *) shmat(process_count_id, NULL, 0);
	
	// in_index: writes message into buffer
	key_t in_index_key = ftok("/tmp", 'j');
	int in_index_id= shmget(in_index_key, sizeof(int), IPC_CREAT | 0666);
	if (in_index_id < 0) {
		printf("in_index_id ID ERROR\n");
		exit(1);
	}
	in_index = (int *) shmat(in_index_id, NULL, 0);
	
	// out_index: reads entry from buffer
	key_t out_index_key = ftok("/tmp", 'k');
	int out_index_id= shmget(out_index_key, sizeof(int), IPC_CREAT | 0666);
	if (out_index_id < 0) {
		printf("out_index_id ID ERROR\n");
		exit(1);
	}
	out_index = (int *) shmat(out_index_id, NULL, 0);
	
	if (u!=1)
	{ 
		// code for shared mem
		//declare out index to write msg into buffer
		key_t message_buffer_key= ftok("/tmp", 'l');
		message_buffer_id= shmget(message_buffer_key, sizeof(struct Message)*buffer_size, IPC_CREAT | 0666);
		if (message_buffer_id < 0) {
			printf("message_buffer_id ID ERROR\n");
			exit(1);
		}
		message_buffer = (struct Message	*) shmat(message_buffer_id, NULL, 0);
	}
	else { // code for Unix socket
		message_buffer = (struct Message	*)NULL;
		*process_count=*process_count+1;
	}
	if (*process_count==1 || p==1) // first process running semaphores again: initialize semaphores
	{
		// full = 0
		sem_init(full, 1, 0);
		// empty = buffer size
		sem_init(empty, 1, 10);
		// mutex = 1 so that someone can run it
		sem_init(mutex, 1, 1);
	}
		
	  
	if (u==1 && c==1) // when only consumer has the mutex (this happens in the Unix socket)
		sem_init(mutex, 1, 1);
	/*
	printf("full = %d, mutex = %d, empty = %d\n", *((int *) full), *((int *) mutex), *((int *) empty));
	printf("Process Count: %d\n", *process_count);
	printf("MESSAGE: %s\n", message);
	printf("QUEUE SIZE: %d\n", buffer_size);
	*/
	if (p == 1) // program acts as a producer
		producer(message);
	else if (c == 1) // program acts as a consumer
		consumer(message);
 
	if (*process_count<=1) // process_count <= 1: after consumer/producer stuff is done, if starter process is done, free resources
	{
		// printf("\nresetting resources ...\n");
		*process_count=0;
		sem_destroy(full);
		sem_destroy(empty);
		sem_destroy(mutex);
		shmdt(full);
		shmdt(empty);
		shmdt(mutex);
		shmdt(in_index);
		shmdt(out_index);
		shmdt(message_buffer);
		
		shmctl(sem_full_id, IPC_RMID, NULL);
		shmctl(sem_empty_id, IPC_RMID, NULL);
		shmctl(sem_mutex_id, IPC_RMID, NULL);
		
		shmctl(in_index_id, IPC_RMID, NULL);
		shmctl(out_index_id, IPC_RMID, NULL);
		if (u==0)
			shmctl(message_buffer_id, IPC_RMID, NULL); //check it later
	}
	else // if this isn't last process, decrement process_count by 1 b/c a process has already happened 
		*process_count=*process_count-1;
}
void producer(char *message)
 {
	 if (u==1)
	 { // Unix socket
		  int fd = socket(AF_INET, SOCK_STREAM,   0); // start the Unix socket   
		  if (fd < 0) 
		  {
				perror("\nSocket creation problem\n");
				exit(-1);  
		  }
		  struct sockaddr_in saddr; // socket address
		  memset(&saddr, 0, sizeof(saddr)); // initialize address to 0
		  saddr.sin_family = AF_INET; // set type of connection: here it's Internet, because 'AF_INET'
		  saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // defining server at localhost
		  saddr.sin_port = htons(PortNumber);  // listens at the PortNumber (8080)
		  if (bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) // if binding failed
		  {
				perror("Binding problem\n");
				 exit(-1);  
		  }
		/* listening on port 8080 */
		  if (listen(fd, 100) < 0) /* maximum 100 clients */
		  {	
			perror("Listening on port is a problem"); 
			exit(-1);  
		  }
		//printf( "\nListening on port %d for clients...\n", PortNumber);
			
		while(1)
		{
			if (gracefull_exit==1) break; // this exit happens on Ctrl+C

			struct sockaddr_in caddr; /* client address */
			int len = sizeof(caddr);  /* length of client address */
			int client_fd = accept(fd, (struct sockaddr*) &caddr, &len);  /* client connection accepted by server */
			if (client_fd < 0) { // if client connection fails
				printf("Client Connection issue\n");
			  continue;
			}
			// no need of semaphore here because client connection is blocking; it waits until the connection happens
			write(client_fd, message, MessageLength); // write 'message' content to client
			close(client_fd); 
			 if (msg_print==1)
				 printf( "%s\n",message);
		} // 'while' loop has ended
	} 
	else { // shared memory
		while (1) 
		{
				if (gracefull_exit==1) break;
				iam_waiting=1; // starts waiting (semaphores are 0)
				// semaphore wait empty
				sem_wait(empty);
				// semaphore wait mutex
				sem_wait(mutex);
				// write message to buffer[in_index]
				sprintf(message_buffer[*in_index].msg,"%s",message);
			    if (msg_print==1)
					printf("%s\n",message_buffer[*in_index].msg);
				*in_index = (*in_index+1)%buffer_size; // now that msg is printed, increase in_index to next slot
				// now signal mutex and full
				sem_post(mutex);
				sem_post(full);
				iam_waiting=0; // the wait is over
		}
	}
}
void consumer(char *message) 
{

	while (1) 
	{
		if (gracefull_exit==1) break;
		if (u==1)
		{ // Unix socket
			// just one mutex needed
				sem_wait(mutex);
				int sockfd = socket(AF_INET, SOCK_STREAM,  0); // begin socket      
				if (sockfd < 0) {perror("Error: gethostbyname returned null\n"); exit(-1); }
				/* host server address */
				struct hostent* hptr = gethostbyname("localhost");//gethostbyname(Host); /* localhost: 127.0.0.1 */
				if (!hptr)  // if server doesn't exist
				{perror("Error: gethostbyname returned null\n"); exit(-1); }

				if (hptr->h_addrtype != AF_INET) // if host server isn't a server       
				{perror("Error: bad address fo server\n"); exit(-1); }
				/* else, connect to the server: configure server's address 1st */
				struct sockaddr_in saddr; // server address
				memset(&saddr, 0, sizeof(saddr));
				saddr.sin_family = AF_INET;
				saddr.sin_addr.s_addr =
				((struct in_addr*) hptr->h_addr_list[0])->s_addr;
				saddr.sin_port = htons(PortNumber); // port number 
				if (connect(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0) // connects to server, fetches message
				{perror("Error: connection problem\n"); exit(-1); }
				if (read(sockfd, buffer, MessageLength) > 0) // here, we read the message from the server
					printf("%s\n",buffer);
				close(sockfd); //close the connection 
				sem_post(mutex);
		}
		else
		{
			if (gracefull_exit==1) break; // set in SIGINT handler
			iam_waiting=1; // start waiting
			// semaphore wait full
			sem_wait(full);
			// semaphore wait mutex
			sem_wait(mutex);
		    if (msg_print==1)
				printf("%s\n",message_buffer[*out_index].msg); // if msg_print = 1, print the fetched msg
			*out_index = (*out_index+1)%buffer_size; // now that msg is read, increase out_index to next slot
			// now signal mutex and empty
			sem_post(mutex);
			sem_post(empty);
			iam_waiting=0; // the wait is over
		}
	}
}




