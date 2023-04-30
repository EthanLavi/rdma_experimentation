#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <cstdio>
#include <string>
#include <string.h>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 8009
#define MESSAGE_SIZE 24

void error(const char* message){
    fprintf(stderr, message);
    fprintf(stderr, "\n");
    exit(1);
}

int link(bool imserver, const char* ip){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0), result = 0;
    if (sockfd == -1) error("Cannot open socket");
    struct hostent* server = gethostbyname(ip);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    // Check if server is null (means could not resolve hostname)
    if (server == NULL) error("Could not resolve hostname");

    // Load socket information
    if (imserver){
        address.sin_addr.s_addr = INADDR_ANY;
        result = bind(sockfd, (struct sockaddr*) &address, sizeof(address));
        if (result == -1) error("Cannot bind to socket");
        result = listen(sockfd, 1);
        if (result == -1) error("Cannot listen on socket");
        int address_size = sizeof(address);
        result = accept(sockfd, (struct sockaddr*) &address, (socklen_t*) &address_size);
        if (result == -1) error("Cannot accept on new socket");
        close(sockfd);
        return result;
    } else {
        result = inet_pton(AF_INET, ip, &address.sin_addr);  // Pointer (to String) to Number.
        if (result <= 0) error("Cannot inet pton");
        result = connect(sockfd, (struct sockaddr*) &address, sizeof(address)); 
        if (result == -1) error("Cannot connect to foreign socket");
        return sockfd;
    }
}

void Write(int sockfd, char* data){
    int status = write(sockfd, data, MESSAGE_SIZE + 1);
    if (status == -1) error("Cannot send data over socket");
}

void Read(int sockfd, char* recv_buffer){
    int status = read(sockfd, recv_buffer, MESSAGE_SIZE + 1);
    if (status == -1) error("Cannot read data over socket");
}