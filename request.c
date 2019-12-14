#include "request.h"
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_BUF 1024

void free_routeinfo(struct routeinfo info)
{
	if (info.int_name != NULL)
	{
		free(info.int_name);
		info.int_name = NULL;
	}
}

struct msghdr build_request()
{
	struct sockaddr_nl* addr = (struct sockaddr_nl*) malloc(sizeof(struct sockaddr_nl));
	memset(addr, 0, sizeof(struct sockaddr_nl));
	addr->nl_family = AF_NETLINK;
	
	struct msghdr msg;
	memset(&msg, 0, sizeof(struct msghdr));
	
	msg.msg_name = addr;
	msg.msg_namelen = sizeof(struct sockaddr_nl);
	msg.msg_iov = malloc(sizeof(struct iovec));
	msg.msg_iovlen = 1;

	return msg;
}

struct msghdr build_getroute_request()
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

	struct msghdr msg = build_request();
	msg.msg_iov->iov_base = nlmsg;
	msg.msg_iov->iov_len = nlmsg_bytes;

	return msg;
}

struct msghdr build_getaddr_request()
{
	//ensuring 4 byte boundaries in case of some odd compiler
	const int nlmsg_bytes = NLMSG_SPACE(sizeof(struct ifaddrmsg));

	struct nlmsghdr* nlmsg = (struct nlmsghdr*) malloc(nlmsg_bytes);
	memset(nlmsg, 0, nlmsg_bytes);

	nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	nlmsg->nlmsg_type = RTM_GETADDR;
	nlmsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_MATCH;
	nlmsg->nlmsg_pid = getpid();

	((struct ifaddrmsg*) NLMSG_DATA(nlmsg))->ifa_family = AF_INET;

	struct msghdr msg = build_request();
	msg.msg_iov->iov_base = nlmsg;
	msg.msg_iov->iov_len = nlmsg_bytes;

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
	int msgtype = ((struct nlmsghdr*) msg.msg_iov->iov_base)->nlmsg_type;

	//populate message with dynamic data
	if (msgtype == RTM_GETROUTE)
	{
		//store IP addr in destination attribute
		int* dst_attr = (int*) RTA_DATA(RTM_RTA(NLMSG_DATA(msg.msg_iov->iov_base)));
		*dst_attr = info->dest_ip;
	}

	if (sendmsg(fd, &msg, 0) < 0)
		return -1;
	
	char buf[MAX_BUF] = {0}; 

	int bytes_read = recv(fd, buf, MAX_BUF, 0);
	if (bytes_read >= 1024 || bytes_read < 0)
		return -1;

	struct nlmsghdr* nlmsg = (struct nlmsghdr*) buf;
	if (!NLMSG_OK(nlmsg, bytes_read) || nlmsg->nlmsg_type == NLMSG_ERROR)
	       return -1;

	//get attributes start and length
	struct rtattr* attr;
	int rtpayload;
	if (msgtype == RTM_GETROUTE)
	{
		attr = RTM_RTA(NLMSG_DATA(nlmsg));	
		rtpayload = RTM_PAYLOAD(nlmsg);
	}
	else if (msgtype == RTM_GETADDR)
	{
		int foundmsg = 0;
		//find the message for the interface index we're looking for
		for (; NLMSG_OK(nlmsg, bytes_read); nlmsg = NLMSG_NEXT(nlmsg, bytes_read))
		{
			if (((struct ifaddrmsg*) NLMSG_DATA(nlmsg))->ifa_index == info->int_index)
			{
				foundmsg = 1;
				break;
			}
		}

		if (!foundmsg)
			return -1;

		attr = IFA_RTA(NLMSG_DATA(nlmsg));
		rtpayload = IFA_PAYLOAD(nlmsg);
	}

	//parse attributes
	for (; RTA_OK(attr, rtpayload); attr = RTA_NEXT(attr, rtpayload))
	{
		if (msgtype == RTM_GETROUTE)
		{
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
		else if (msgtype == RTM_GETADDR && attr->rta_type == IFA_LABEL)
		{
			info->int_name = malloc(RTA_PAYLOAD(attr) + 1);
			memset(info->int_name, 0, RTA_PAYLOAD(attr) + 1);
			memcpy(info->int_name, RTA_DATA(attr), RTA_PAYLOAD(attr));
			break;
		}
	}

	return 0;
}
