/*
 * TCP_Socket.h
 *
 *  Created on: 1.12.2015
 *      Author: root
 */

#ifndef TCP_SOCKET_H_
#define TCP_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "HRLVEZ0.h"

#define CLIENT_ADDRESS "10.42.0.2"
#define PORT_NUMBER 51000

#define TRUE	1
#define FALSE	0

#define BUFFERSIZE	20

/* Function prototypes */
int TCP_SocketPollingServer(void);

#endif /* TCP_SOCKET_H_ */