#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <string.h>
#include <unistd.h>
#include "request.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	//get socket descripter
	int nlsock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	//create src netlink address
	struct sockaddr_nl src_addr;
	memset(&src_addr, 0, sizeof(struct sockaddr_nl));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();

	//bind socket
	bind(nlsock, (struct sockaddr*)&src_addr, sizeof(struct sockaddr_nl));

	struct routeinfo info = {0, NULL, 0, 0, 784808664};
	struct msghdr routemsg = build_getroute_request();
	struct msghdr addrmsg = build_getaddr_request();

	get_routeinfo(nlsock, routemsg, &info);
	get_routeinfo(nlsock, addrmsg, &info);

	printf("***ROUTE INFO***\n");
	printf("Interface index: %d\n", info.int_index);
	printf("Interface name: %s\n", info.int_name);
	printf("Interface ip: %u\n", info.int_ip);
	printf("Gateway ip: %u\n", info.gateway_ip);
	printf("Destination ip: %u\n", info.dest_ip);

	close(nlsock);
	free_request(&routemsg);
	free_request(&addrmsg);
	free(info.int_name);

	return 0;
}
