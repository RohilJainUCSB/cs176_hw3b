//Used help from Google searches for making a UDP client and time-based responses
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

//This is a helper function to help find the differences in milliseconds between two times
double diff_ms(struct timeval *start, struct timeval *end)
{
    return (end -> tv_sec - start -> tv_sec) * 1000.0 + (end -> tv_usec - start -> tv_usec) / 1000.0;
}

int main(int argc, char * argv[])
{
    char * server_ip = argv[1];
    int server_port = atoi(argv[2]);
    
    int client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    struct timeval tv;
    tv.tv_sec = 1;  //1 second timeout
    tv.tv_usec = 0; //0 msec for the timeout
    setsockopt(client_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); //This will tell the client to give up if a timeout is reached and move on

    socklen_t server_addr_len = sizeof(server_addr);

    int sent = 0, received = 0;
    double min = 1e9, max = 0, sum = 0;

    for (int seq = 1; seq <= 10; ++seq)
    {
        struct timeval t1, t2;
        gettimeofday(&t1, NULL); //Set t1 to the send time
        char msg[1024];
        snprintf(msg, sizeof(msg), "PING %d %ld.%06ld", seq, (long)t1.tv_sec, (long)t1.tv_usec);

        sendto(client_socket_fd, msg, strlen(msg), 0, (struct sockaddr *) & server_addr, server_addr_len);
        sent ++;

        char buff[1024];
        ssize_t n = recvfrom(client_socket_fd, buff, sizeof(buff) - 1, 0, (struct sockaddr *) & server_addr, & server_addr_len);
        gettimeofday(&t2, NULL); //Set t2 to the receive time

        if (n < 0) //If the recvfrom doesn't receive anything, it would set n to be -1 from its return value
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) //This -1 in n sets errno to be one of these two values which mean the request timed out
            {
                printf("Request timeout for seq#=%d\n", seq);
            }
        }
        else
        {
            buff[n] = 0;
            double rtt = diff_ms(&t1, &t2); //The rtt is the difference between the send and receive times
            printf("PING received from %s: seq#=%d time=%.3f ms\n", server_ip, seq, rtt);
            received ++;
            if (rtt < min) //Track the min rtt
            {
                min = rtt;
            }
            if (rtt > max) //Track the max rtt
            {
                max = rtt;
            }
            sum += rtt;
        }
        sleep(1); //Wait ~1s between pings
    }

    double loss = 100.0 * (sent - received) / sent; //Loss percentage
    double avg = (received)? (sum / received) : 0;
    printf("--- %s ping statistics ---\n", server_ip);
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n", sent, received, loss);
    if (received) //A packet at least was received
    {
        printf("rtt min/avg/max = %.3f %.3f %.3f ms\n", min, avg, max);
    }
    else //If nothing was received, we should do a default report of 0s
    {
        printf("rtt min/avg/max = 0.000 0.000 0.000 ms\n");
    }

    close(client_socket_fd);
    return 0;
}