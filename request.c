#include "request.h"
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

struct msghdr build_request()
{
	//ensuring 4 byte boundaries in case of some odd compiler
	const int nlmsg_bytes = NLMSG_SPACE(sizeof(struct rtmsg)) + RTA_SPACE(sizeof(uint32_t));

	struct nlmsghdr* nlmsg = (struct nlmsghdr*) malloc(nlmsg_bytes);
	memset(nlmsg, 0, nlmsg_bytes);

	nlmsg->nlmsg_len = nlmsg_bytes;
	nlmsg->nlmsg_type = RTM_GETROUTE;
	nlmsg->nlmsg_flags = NLM_F_REQUEST;
	nlmsg->nlmsg_pid = getpid();

	struct rtmsg* rtmsg = (struct rtmsg*) NLMSG_DATA(nlmsg);
	rtmsg->rtm_family = AF_INET;
	rtmsg->rtm_table = RT_TABLE_MAIN;
	rtmsg->rtm_type = RTN_UNICAST;

	struct rtattr* attr = RTM_RTA(rtmsg);
	attr->rta_len = RTA_LENGTH(sizeof(uint32_t));
	attr->rta_type = RTA_DST;

	struct sockaddr_nl* addr = (struct sockaddr_nl*) malloc(sizeof(struct sockaddr_nl));
	memset(addr, 0, sizeof(struct sockaddr_nl));
	addr->nl_family = AF_NETLINK;
	
	struct iovec* vec = (struct iovec*) malloc(sizeof(struct iovec));
	vec->iov_base = nlmsg;
	vec->iov_len = nlmsg_bytes;

	struct msghdr msg;
	memset(&msg, 0, sizeof(struct msghdr));
	
	msg.msg_name = addr;
	msg.msg_namelen = sizeof(struct sockaddr_nl);
	msg.msg_iov = vec;
	msg.msg_iovlen = 1;

	return msg;
}

void free_request(struct msghdr* msg)
{
	free(msg->msg_name);
	free(msg->msg_iov->iov_base);
	free(msg->msg_iov);

	msg->msg_name = NULL;
	msg->msg_iov = NULL;
}

int get_routeinfo(int fd, struct msghdr msg, struct routeinfo* info)
{
	//store IP addr in destination attribute
	int* dst_attr = (int*) RTA_DATA(RTM_RTA(NLMSG_DATA(msg.msg_iov->iov_base)));
	*dst_attr = info->dest_ip;

	if (sendmsg(fd, &msg, 0) < 0)
		return -1;
	
	const int MAX_BUF = 1024;
	void* buf = malloc(MAX_BUF); 
	memset(buf, 0, MAX_BUF);

	int bytes_read = recv(fd, buf, MAX_BUF, 0);
	if (bytes_read >= 1024 || bytes_read < 0)
		return -1;

	struct nlmsghdr* nlmsg = (struct nlmsghdr*) buf;
	if (!NLMSG_OK(nlmsg, bytes_read))
	       return -1;

	struct rtmsg* rtmsg = (struct rtmsg*) NLMSG_DATA(nlmsg);	

	int rtpayload = RTM_PAYLOAD(nlmsg);
	for (struct rtattr* attr = RTM_RTA(rtmsg); RTA_OK(attr, rtpayload); attr = RTA_NEXT(attr, rtpayload))
	{
		int* val;

		switch (attr->rta_type)
		{
			case RTA_OIF:
				info->int_index = *(int*) RTA_DATA(attr);
				break;
			case RTA_GATEWAY:
				info->gateway_ip = *(int*) RTA_DATA(attr);
				break;
			case RTA_PREFSRC:
				info->int_ip = *(int*) RTA_DATA(attr);
				break;
		}
	}



	return 0;
}
