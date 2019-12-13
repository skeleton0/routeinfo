#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <string.h>
#include <unistd.h>
#include "request.h"

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

	struct msghdr msg = build_request();

	close(nlsock);
	free_request(&msg);

	return 0;
}
