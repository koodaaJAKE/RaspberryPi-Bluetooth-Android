/*
 * TCP_Socket.cpp
 *
 *  Created on: 1.12.2015
 *      Author: root
 */
#include "TCP_Socket.h"
#include "LCD.h"
#include "SerializeDeserialize.h"

int TCP_SocketPollingServer(void)
{
	int on = 1;
    struct sockaddr_in server;
    struct pollfd fds[10];
    int timeout, close_conn, rc;
    int end_server = FALSE, compress_array = FALSE;
    int socket_fd = -1, new_socket_fd = -1, len;
    unsigned char sendBuffer[BUFFERSIZE];
    unsigned char recvBuffer[1];
    int nfds = 1, current_size = 0, i, j;
    unsigned char *ptr;
    HRLVEZ0_Data_t HRLVEZ0_Data;

    /*************************************************************/
	/* Open text file where to store the distance data           */
    /*************************************************************/
	FILE *f;

	f = fopen("/home/pi/koodit/data.txt", "w");
	if(NULL == f)
	{
		perror("Can't open file\n");
	}

    /*************************************************************/
    /* Create IPV4 socket                                        */
    /*************************************************************/
    socket_fd = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_fd == -1)
    {
        perror("Could not create socket\n");
    }

    /*************************************************************/
    /* Allow socket descriptor to be reusable                    */
    /*************************************************************/
    if(setsockopt(socket_fd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
    	perror("setsockopt error!\n");
    	close(socket_fd);
    	exit(EXIT_FAILURE);
    }

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    rc = ioctl(socket_fd, FIONBIO, (char *)&on);
    if(rc < 0)
    {
      perror("ioctl() error");
      close(socket_fd);
      exit(EXIT_FAILURE);
    }

    /*************************************************************/
    /* Bind the socket                                           */
    /*************************************************************/
    memset(&server, '0', sizeof(server));
    server.sin_addr.s_addr = inet_addr(CLIENT_ADDRESS);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_NUMBER);

    if(bind(socket_fd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
    	perror("Binding error!\n");
    	close(socket_fd);
    	exit(EXIT_FAILURE);
    }

    /*************************************************************/
    /* Set the listen back log                                   */
    /*************************************************************/
    if(listen(socket_fd, 10) < 0)
    {
    	perror("Listen error!\n");
    	close(socket_fd);
    	exit(EXIT_FAILURE);
    }

    /*************************************************************/
    /* Initialize the pollfd structure                           */
    /*************************************************************/
    memset(fds, 0 , sizeof(fds));

    /*************************************************************/
    /* Set up the initial listening socket                        */
    /*************************************************************/
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;

    /*************************************************************/
    /* Initialize the timeout to 2 minutes. If no                */
    /* activity after 2 minutes this program will end.           */
    /* timeout value is based on milliseconds.                   */
    /*************************************************************/
    timeout = (2 * 60 * 1000);

    /*************************************************************/
    /* Loop waiting for incoming connects or for incoming data   */
    /* on any of the connected sockets.                          */
    /*************************************************************/
    do
    {
      /***********************************************************/
      /* Call poll() and wait 2 minutes for it to complete.      */
      /***********************************************************/
      printf("Waiting on poll()...\n");
      rc = poll(fds, nfds, timeout);

      /***********************************************************/
      /* Check to see if the poll call failed.                   */
      /***********************************************************/
      if(rc < 0)
      {
        perror("  poll() failed");
        break;
      }

      /***********************************************************/
      /* Check to see if the 2 minute time out expired.          */
      /***********************************************************/
      if(rc == 0)
      {
        printf("poll() timed out.  End program.\n");
        break;
      }

      /***********************************************************/
      /* One or more descriptors are readable. Need to           */
      /* determine which ones they are.                          */
      /***********************************************************/
      current_size = nfds;
      for(i = 0; i < current_size; i++)
      {
        /*********************************************************/
        /* Loop through to find the descriptors that returned    */
        /* POLLIN and determine whether it's the listening       */
        /* or the active connection.                             */
        /*********************************************************/
        if(fds[i].revents == 0)
          continue;

        /*********************************************************/
        /* If revents is not POLLIN, it's an unexpected result,  */
        /* log and end the server.                               */
        /*********************************************************/
        if(fds[i].revents != POLLIN)
        {
          printf("  Error! revents = %d\n", fds[i].revents);
          end_server = TRUE;
          break;

        }
        if(fds[i].fd == socket_fd)
        {
          /*******************************************************/
          /* Listening descriptor is readable.                   */
          /*******************************************************/
          printf("  Listening socket is readable\n");

          /*******************************************************/
          /* Accept all incoming connections that are            */
          /* queued up on the listening socket before we         */
          /* loop back and call poll again.                      */
          /*******************************************************/
          do
          {
            /*****************************************************/
            /* Accept each incoming connection. If               */
            /* accept fails with EWOULDBLOCK, then we            */
            /* have accepted all of them. Any other              */
            /* failure on accept will cause us to end the        */
            /* server.                                           */
            /*****************************************************/
            new_socket_fd = accept(socket_fd, NULL, NULL);
            if(new_socket_fd < 0)
            {
              if (errno != EWOULDBLOCK)
              {
                perror("  accept() failed");
                end_server = TRUE;
              }
              break;
            }

            /*****************************************************/
            /* Add the new incoming connection to the            */
            /* pollfd structure                                  */
            /*****************************************************/
            printf(" New incoming connection - %d\n", new_socket_fd);
            fds[nfds].fd = new_socket_fd;
            fds[nfds].events = POLLIN;
            nfds++;

            /*****************************************************/
            /* Loop back up and accept another incoming          */
            /* connection                                        */
            /*****************************************************/
            }while (new_socket_fd != -1);
        }

        /*********************************************************/
        /* This is not the listening socket, therefore an        */
        /* existing connection must be readable                  */
        /*********************************************************/
        else
        {
          printf("  Descriptor %d is readable\n", fds[i].fd);
          close_conn = FALSE;

          /*******************************************************/
          /* Receive all incoming data on this socket            */
          /*******************************************************/
          while(!close_conn)
          {
            /*****************************************************/
            /* Receive data on this connection until the         */
            /* recv fails with EWOULDBLOCK. If any other         */
            /* failure occurs, we will close the                 */
            /* connection.                                       */
            /*****************************************************/

            rc = recv(fds[i].fd, recvBuffer, sizeof(recvBuffer), 0);
            if(rc < 0)
            {
            	if (errno != EWOULDBLOCK)
            	{
            		perror(" recv() failed");
            		close_conn = TRUE;
            	}
            	break;
            }

            /*****************************************************/
            /* Data was received                                 */
            /*****************************************************/
            len = rc;
            printf("%d bytes received\n", len);

            /*****************************************************/
            /* Check to see if the connection has been           */
            /* closed by the client                              */
            /*****************************************************/
            if(rc == 0)
            {
            	printf(" Connection closed\n");
                close_conn = TRUE;
                break;
            }

            	switch(recvBuffer[0])
                {
            		/*****************************************************/
           	    	/* Print "CONNECT" to the LCD Screen when received   */
                	/* the connect command character "C" from the UI     */
                	/*****************************************************/
                	case 'C':
                		printf("CONNECTED!\n");
                		printConnect();
                		break;

                    /*****************************************************/
                    /* Print "ALREADY CONNECTED" to the LCD Screen when  */
                    /* received the already connected command character  */
                    /* "A" from the UI                                   */
                    /*****************************************************/
                    case 'A':
                    	printf("ALREADY CONNECTED!\n");
                    	printAlreadyConnected();
                    	break;

                    /*****************************************************/
                    /* Disconnect when received the quit command         */
                    /* character "Q" from the UI                         */
                    /*****************************************************/
                    case 'Q':
                    	printf("Disconnecting the socket connection...!\n");
                        printDisconnect();
                        sleep(2);
                        close_conn = TRUE;
                        end_server = TRUE;
                        break;

                    /*****************************************************/
                    /* Clear the LCD when received the clear LCD command */
                    /* character "L" from the UI                         */
                    /*****************************************************/
                    case 'L':
                    	printf("LCD Screen cleared!\n");
                        clear_LCD();
                        break;

                    /*****************************************************/
                    /* Read the HRLVEZ0 data and send the data           */
                    /* to the client when received character "S"         */
                    /*****************************************************/
                    case 'S':

                    	measureHRLVEZ0_Data(&HRLVEZ0_Data);

                        //Serialize data before sending it through socket
                        ptr = Serialize_Struct(sendBuffer, &HRLVEZ0_Data);

                        printf("Distance: %ld\n", HRLVEZ0_Data.distance);
                        printf("Speed: %0.2f\n", HRLVEZ0_Data.speed);

                        rc = send(fds[i].fd, sendBuffer, ptr - sendBuffer, 0);
                        if(rc < 0)
                        {
                        	perror("send() failed");
                        	close_conn = TRUE;
                        	break;
                        }
                        else
                        {
                        	printf("\nSent %d bytes data\n", rc);
                        	recvBuffer[0] = 0;
                        }

                        /*******************************************/
            			/* Write distance data to the text file    */
                        /*******************************************/
            			rc = fprintf(f, "%ld\n", HRLVEZ0_Data.distance);
            			if(rc < 0)
            			{
            				perror("Write failed!\n");
            			}
            			break;

                   }
          }

          /*******************************************************/
          /* If the close_conn flag was turned on, we need       */
          /* to clean up this active connection. This            */
          /* clean up process includes removing the              */
          /* descriptor.                                         */
          /*******************************************************/
          if(close_conn)
          {
            close(fds[i].fd);
            fds[i].fd = -1;
            compress_array = TRUE;
          }

        }  /* End of existing connection is readable             */
      } /* End of loop through pollable descriptors              */

      /***********************************************************/
      /* If the compress_array flag was turned on, we need       */
      /* to squeeze together the array and decrement the number  */
      /* of file descriptors. We do not need to move back the    */
      /* events and revents fields because the events will always*/
      /* be POLLIN in this case, and revents is output.          */
      /***********************************************************/

      if (compress_array)
      {
        compress_array = FALSE;
        for (i = 0; i < nfds; i++)
        {
          if (fds[i].fd == -1)
          {
            for(j = i; j < nfds; j++)
            {
              fds[j].fd = fds[j+1].fd;
            }
            i--;
            nfds--;
          }
        }
      }

    } while (end_server == FALSE); /* End of serving running.    */

    fclose(f);
    return 0;
}