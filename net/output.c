#include "ns.h"
#include "inc/lib.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	
	int32_t req;
	uint32_t whom;
	int i, perm;

	while(1) {
		req = ipc_recv((int32_t *)&whom, &nsipcbuf, &perm);
		if(req != NSREQ_OUTPUT) {
			cprintf("not a nsreq output\n");
			continue;
		}

		struct jif_pkt *pkt = &(nsipcbuf.pkt);
		while (sys_pkt_send(pkt->jp_data, pkt->jp_len)) {
			sys_yield();
		}
	}
}
