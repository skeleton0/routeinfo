#pragma once

struct msghdr build_request();
void free_request(struct msghdr* msg);
