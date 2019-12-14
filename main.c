#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <string.h>
#include <unistd.h>
#include "request.h"
#include <stdio.h>

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
	struct msghdr msg = build_request();
	get_routeinfo(nlsock, msg, &info);

	close(nlsock);
	free_request(&msg);

	printf("***ROUTE INFO***\n");
	printf("Interface index: %d\n", info.int_index);
	printf("Interface ip: %u\n", info.int_ip);
	printf("Gateway ip: %u\n", info.gateway_ip);
	printf("Destination ip: %u\n", info.dest_ip);

	return 0;
}
