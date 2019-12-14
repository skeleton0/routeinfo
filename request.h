#pragma once

#include <stdint.h>

struct routeinfo
{
	int int_index; 			//index of interface
	char* int_name; 		//name of interface
	struct in_addr int_ip; 		//interface IP address

	struct in_addr gateway_ip;	//gateway IP address
	struct in_addr dest_ip; 	//remote host IP address
};

void free_routeinfo(struct routeinfo info);
struct msghdr build_request();
struct msghdr build_getroute_request();
struct msghdr build_getaddr_request();
void free_request(struct msghdr* msg);
int get_routeinfo(int fd, struct msghdr msg, struct routeinfo* info);
