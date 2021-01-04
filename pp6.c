#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h> 
#include <string.h>
#include <unistd.h>

// the server response
char response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html> <html> <head> <title>Default Page</title> </head>\r\n"
"<body> <h1>Hello World</h1> </body> </html>\r\n";

// the error response if client HTML file isn't on server
char response_error[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html> <html> <head> <title>Default Page</title> </head>\r\n"
"<body> <h1>ERROR - HTML file could not be read.</h1> </body> </html>\r\n";

// the server response for the client HTML file
char response_header[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n";

// responses if no GET request or no HTML file
char response_nofile[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html> <html> <head> <title>Default Page</title> </head>\r\n"
"<body> <h1>ERROR - HTML file is not passed in URL.</h1> </body> </html>\r\n";
char response_nogetrequest[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html> <html> <head> <title>Default Page</title> </head>\r\n"
"<body> <h1>ERROR - Client did not send GET request.</h1> </body> </html>\r\n";

int main(int argc, char **argv) {
    // checking the command line arguments
    int port_num; // the port number for the server
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0)
            port_num = atoi(argv[i+1]);
    }
    if (port_num == 0) {
        printf("ERROR - port number not set. Please try again.\n");
        exit(1);
    }

    /* creating the socket */
    int socket_id = socket(AF_INET, SOCK_STREAM, 0); // returns file descriptor we can use in other socket calls
    if (socket_id == 0) {
        printf("Socket creation failed.\n");
        exit(1);
    }
    // setting socket options
    int opt = 1;
    int options = setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    // if 'options' =/= 0, there is an error and the options weren't set
    if (options != 0) {
        printf("ERROR - socket option setting failed.\n");
        exit(1);
    }
    // socket address struct
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // now socket can accept connections fron any IP address
    address.sin_port = htons(port_num); // port_num is now network byte order
    socklen_t addr_len = sizeof(address);

    /* binding the socket */
    if (bind(socket_id, (struct sockaddr *) &address, addr_len) < 0) {
        printf("ERROR - unable to bind socket to port number %d\n", port_num);
        exit(1);
    }

    /* listening for connections */
    if (listen(socket_id, 100) < 0) {
        printf("ERROR - listening failed.\n");
        exit(1);
    }
    printf("Server is listening at port #%d\n", port_num);

    // while loop: accept connections from clients

    char buffer[1024]; // array to get input from client
    while(1) {
        int client_id = accept(socket_id, (struct sockaddr *) &address, (socklen_t *) &addr_len);
        if (client_id < 0) { // if the connection fails
            printf("ERROR - client connection not accepted.\n");
            continue;
        }
        else {
            // else, write server response to client
            int count = read(client_id, buffer, sizeof(buffer));
            if (count > 0) { // process client request 
                char *get_ptr = strstr(buffer, "GET");
                // read GET as a separate token so that we can get HTML file in current directory
                char *token = strtok(get_ptr, " ");
                token = strtok(NULL, " "); // this should give the HTML file name
                char filename[100];
                if (token != NULL) {
                    sprintf(filename, ".%s", token); // if token isn't NULL, copy it into 'filename' variable
                    // 'filename' variable should now have HTML file name
                }
                else
                    sprintf(filename, "./"); // if 'token' is NULL, there was no file served 
                if (strcmp(filename, "./") == 0) {
                    // if no file is served, send error response to client
                    write(client_id, response_nofile, sizeof(response_nofile) - 1);
                }
                else
                {
                    FILE *fh = fopen(filename, "r");
                    if (fh == NULL) { 
							// write(client_id, response_error, sizeof(response_error) - 1);
                            char msg[255] = "";
                            sprintf(msg, "<html> <body> <h1>%s doesn't exist.</h1> </body> </html>", filename);
                            write(client_id, response_header, sizeof(response_header) - 1);
                            write(client_id, msg, sizeof(msg) - 1);
				    }
                    else {
                        char *file_text; // stores text of file
                        fseek(fh, 0L, SEEK_END); // ptr goes to end of file to get file length
                        long file_size = ftell(fh); // file length
                        rewind(fh); // ptr goes back to beginning of file to read it
                        file_text = malloc(file_size + 1); // 'file_text' is now a char array of length 'file_size'
                        fread(file_text, 1, file_size, fh); // reading content into file
                        fclose(fh);
                        write(client_id, response_header, sizeof(response_header) - 1);
                        write(client_id, file_text, file_size - 1);
                        free(file_text);
                    }

                }
            }
            else 
                write(client_id, response, sizeof(response) - 1);
        }
        close(client_id);
    }
}
