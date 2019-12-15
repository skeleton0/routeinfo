#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <string.h>
#include <unistd.h>
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 500

int main()
{
	int nlsock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (nlsock < 0)
	{
		perror("Failed to open netlink socket");
		return 1;
	}

	struct sockaddr_nl nladdr;
	memset(&nladdr, 0, sizeof(struct sockaddr_nl));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();

	if (bind(nlsock, (struct sockaddr*) &nladdr, sizeof(struct sockaddr_nl)))
	{
		perror("Failed to bind netlink socket");
		return 1;
	}

	int listensock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listensock < 0)
	{
		perror("Failed to open unix domain socket");
		return 1;
	}

	struct sockaddr_un unaddr = {AF_UNIX, "/tmp/routeinfo"};

	unlink(unaddr.sun_path);
	if (bind(listensock, (struct sockaddr*) &unaddr, sizeof(struct sockaddr_un)))
	{
		perror("Failed to bind unix domain socket");
		return 1;
	}

	if (listen(listensock, 10))
	{
		perror("Failed to listen on unix domain socket");
		return 1;
	}

	//init unix domain socket list
	int maxfd = listensock;
	fd_set unsocks;
	FD_SET(listensock, &unsocks);

	//build data structures for netlink requests
	struct routeinfo info = {0, {0}, 0, 0, 0};
	struct msghdr routemsg = build_getroute_request();
	struct msghdr addrmsg = build_getaddr_request();

	char read_buf[MAX_BUFFER];

	//main loop
	for (;;)
	{
		fd_set rdsocks = unsocks;
		if (select(maxfd+1, &rdsocks, NULL, NULL, NULL) < 0)
		{
			perror("Select returned error");
			return 1;
		}

		for (int clientfd = 0; clientfd <= maxfd; ++clientfd)
		{
			if (!FD_ISSET(clientfd, &rdsocks))
				continue;

			if (clientfd == listensock)
			{
				int newsock = accept(listensock, NULL, NULL);
				if (newsock < 0)
					perror("Failed to accept client connection");
				else
				{
					printf("Accepted new client connection\n");
					FD_SET(newsock, &unsocks);
					if (newsock > maxfd)
						maxfd = newsock;
				}
			}
			else
			{
				int bytes_read = recv(clientfd, read_buf, MAX_BUFFER, 0);

				if (!bytes_read || bytes_read == MAX_BUFFER)
				{
					if (!bytes_read)
						printf("Client closed connection\n");
					else
						printf("Kicking client that sent exceedingly large amount of data\n");

					close(clientfd);
					FD_CLR(clientfd, &unsocks);
				}
				else
				{
					//null terminate the buffer
					read_buf[bytes_read] = 0;

					//clear routeinfo
					memset(&info, 0, sizeof(struct routeinfo));

					if (inet_aton(read_buf, &info.dest_ip) == 0)
					{
						fprintf(stderr, "Failed to parse IP address sent from client\n");	
						strcpy(read_buf, "Failed to parse IP address\n");
					}
					else if (get_routeinfo(nlsock, routemsg, &info) || get_routeinfo(nlsock, addrmsg, &info))
					{
						fprintf(stderr, "Failed to get route information from kernel\n");
						strcpy(read_buf, "Failed to get route information from kernel\n");
					}
					else
					{
						strcpy(read_buf, info.int_name);
						strcat(read_buf, "\n");
						
						if (info.gateway_ip.s_addr)
							strcat(read_buf, inet_ntoa(info.gateway_ip));
						else
							strcat(read_buf, "Target host is on local network");

						strcat(read_buf, "\n");
					}

					if (send(clientfd, read_buf, strlen(read_buf), 0) < 0)
					{
						perror("Error sending data to client");
					}
				}
			}
		}
	}

	return 0;
}
