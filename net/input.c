#include "ns.h"

extern union Nsipc nsipcbuf;

#define RX_PKT_SIZE 1518

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

	while(1) {
		int recv_len;
		char buf[RX_PKT_SIZE];
		if ((recv_len = sys_pkt_recv(buf, RX_PKT_SIZE)) < 0) {
			continue;
		}
		memcpy(nsipcbuf.pkt.jp_data, buf, recv_len);
		nsipcbuf.pkt.jp_len = recv_len;
		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P|PTE_U|PTE_W);

		sys_yield();
	}
}
