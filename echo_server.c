#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
/***************************************************************************************/
#define NWK_ERR             -1
#define BACKLOG             10
#define BUFFER_SIZE         1024
#define MAX_CONNECTIONS     2
#define POLL_INDEFINITE       -1
#define SERVER_FULL_MSG     "The server is full\n"
/***************************************************************************************/
/**
 * @brief - simple function that prints the usage of this program
 * 
 * @param exec_name - the string containing the name of this program's executable file
 * @return int - returns 1 always
 */
int print_usage (char *exec_name)
{
    printf("usage: %s <port_num>\n", exec_name);
    return 1;
}

/**
 * @brief - Creates a TCP socket, sets the SO_REUSEADDR flag, binds it to the specified port on this machine
            and sets up the listen backlog size to BACKLOG
 * 
 * @param port - then port number to which this server should be bound
 * @return int - returns the fd
 */
int setup_socket (short port)
{

    int fd = socket (AF_INET, SOCK_STREAM, 0);
    if(fd == NWK_ERR) {
        perror("socket");
        return fd;
    }

    //set SO_REUSEADDR
    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == NWK_ERR) {
        perror("setsockopt");
        close(fd);
        return NWK_ERR;
    }

    //bind to the local ipv4 addr and given port
    struct sockaddr_in addr = {0,};
    addr.sin_family     = AF_INET;
    addr.sin_addr.s_addr= htonl (INADDR_ANY);
    addr.sin_port       = htons (port);

    if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == NWK_ERR) {
        perror("bind");
        close(fd);
        return NWK_ERR;
    }

    //setup connection backlog
    if(listen(fd, BACKLOG) == NWK_ERR) {
        perror("listen");
        close(fd);
        return NWK_ERR;
    }
    return fd;

}

/**
 * @brief - simple function that prints the IP address and port of the new client
 * 
 * @param addr - pointer to the struct sockaddr_in instance containing the IPv4 address of the client
 */
void print_new_conn (struct sockaddr_in *addr)
{
    printf("new connection from %s:%hu\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
}

/**
 * @brief - simple function that logs when an fd is closed
 * 
 * @param fd - the fd that is going to be closed
 */
void print_conn_closed (int fd)
{
    printf("connection at fd=%d closed\n",fd);
}

/**
 * @brief - simple function that logs the number of bytes received at an fd
 * 
 * @param fd - the fd through whih the data was received
 * @param bytes - the number of bytes received
 */
void print_data_recvd (int fd, int bytes)
{
    printf("received %d bytes from fd=%d\n", fd, bytes);
}


int main (int argc, char *argv[])
{
    if(argc != 2) {
        return print_usage (argv[0]);
    }

    //parse the port and setup the socket
    short port = (short) strtol (argv[1], NULL, 0);
    int server_fd = setup_socket (port);

    //setup the array of file descriptors for poll()
    struct pollfd pfds[MAX_CONNECTIONS+1];
    pfds[0].fd = server_fd;
    pfds[0].events = POLLIN;

    int ret,i,success_count,client_fd,curr_count,bytes;
    int fds_count = 1;  //the server fd
    char *buffer = calloc (BUFFER_SIZE, 1);

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while(1) {
        //we poll on the set of fd's for an indefinite period of time
        ret = poll (pfds, fds_count, POLL_INDEFINITE);
        if(ret == NWK_ERR) {
            perror("poll");
            return 1;
        }

        //this must never happen since our timeout value is POLL_INDEFINITE
        if(ret == 0) {
            continue;
        }

        //loop through the list of fd's
        for(i=0,success_count=0,curr_count=fds_count; i<curr_count && success_count<ret; i++) {
            //we found our fd that's ready
            if(pfds[i].revents & POLLIN) {
                success_count++;
                //if the fd is our server connection, we need to accept a connection
                if(pfds[i].fd == server_fd) {
                    client_fd = accept (pfds[i].fd, (struct sockaddr*)&client_addr, &len);
                    if(client_fd == NWK_ERR) {
                        perror("accept");
                    }
                    //check if the server is full, if so, close this connection
                    //we use MAX_CONNECTIONS+1 since 1 item in pfds[] is the server fd itself
                    if(fds_count == MAX_CONNECTIONS+1) {
                        send (client_fd, SERVER_FULL_MSG, strlen(SERVER_FULL_MSG), 0);
                        close(client_fd);
                    }
                    //else add to the back of the set of fd's
                    else {
                        pfds[fds_count].fd = client_fd;
                        pfds[fds_count].events = POLLIN;
                        print_new_conn (&client_addr);
                        fds_count++;
                    }
                }       //end of if(pfds[i].fd == server_fd)
                //else it is a message from a client
                else {
                        bytes = recv (pfds[i].fd, buffer, BUFFER_SIZE-1, 0);
                        if(bytes == NWK_ERR) {
                            perror("recv");
                        }
                        //recv returns 0 if the connection is closed
                        else if(bytes == 0) {
                            print_conn_closed (pfds[i].fd);
                            close (pfds[i].fd);
                            //move the last fd to this position and decrement the pfds count
                            pfds[i].fd = pfds[fds_count-1].fd;
                            fds_count--;
                            //continue to the next iteration of this loop because we've rearranged the structure
                            continue;
                        }
                        //else we received some data
                        else {
                            print_data_recvd (pfds[i].fd, bytes);
                            //send it back
                            send (pfds[i].fd, buffer, bytes, 0);
                            //clear the buffer
                            memset (buffer, 0, bytes);
                        }
                }
            }           //end of if(pfds[i].revents & POLLIN)
        }               //end of for loop
    }                   //end of while(1)

    free(buffer);
    close(server_fd);
    return 0;
}