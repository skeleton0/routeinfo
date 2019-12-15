#pragma once

#include <stdint.h>
#include <netinet/in.h>

#define MAX_INT_NAME 25

struct routeinfo
{
	int int_index; 			//index of interface
	char int_name[MAX_INT_NAME];	//name of interface
	struct in_addr int_ip; 		//interface IP address

	struct in_addr gateway_ip;	//gateway IP address
	struct in_addr dest_ip; 	//remote host IP address
};

struct msghdr build_request();
struct msghdr build_getroute_request();
struct msghdr build_getaddr_request();
void free_request(struct msghdr* msg);
int get_routeinfo(int fd, struct msghdr msg, struct routeinfo* info);
