#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VENDER_ID_82540EM 0x8086
#define E1000_DEV_ID_82540EM 0x100E

#define TXDESCS 	32
#define TX_PKT_SIZE 1518

#define E1000_STATUS   0x00008  /* Device Status - RO */
#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */
#define E1000_TXD_CMD_EOP    0x00000001 /* End of Packet */
#define E1000_TXD_CMD_RS     0x00000008 /* Report Status */

#define RXDESCS 			128
#define RX_PKT_SIZE 		1518
#define E1000_RCTL 			0x00100
#define E1000_RCTL_EN     	0x00000002    /* enable */
#define E1000_RCTL_BAM    	0x00008000    /* broadcast enable */
#define E1000_RCTL_SECRC  	0x04000000    /* Strip Ethernet CRC */
#define E1000_RDBAL    	  	0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    		0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    		0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      		0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      		0x02818  /* RX Descriptor Tail - RW */
#define E1000_RA       		0x05400  /* Receive Address - RW Array */
#define E1000_RAH_AV   		0x80000000        /* Receive descriptor valid */
#define E1000_RXD_STAT_DD  	0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP 	0x02    /* End of Packet */

enum {
    E_TRANSMIT_RETRY = 1,
};

/*transmit descriptor related*/
struct e1000_tx_desc
{
	uint64_t addr;      // Buffer Address
	uint16_t length;    // Length is per segment. 
	uint8_t cso;        //  Checksum Offset
	uint8_t cmd;        // Command field EOP IFCS IC RS RPS DEXT VLE IDE
	uint8_t status;     // Status field dd ec lc tu
	uint8_t css;        // Checksum Start Field
	uint16_t special;
}__attribute__((packed));

// Transmit Control register 
struct e1000_tctl {
	uint32_t rsv1:   1;
	uint32_t en:     1; // Transmit Enable
	uint32_t rsv2:   1;
	uint32_t psp:    1; // Pad Short Packets 0b = Do not pad. 1b = Pad short packets.
	uint32_t ct:     8; // Collision Threshold
	uint32_t cold:   10;// Collision Distance
	uint32_t swxoff: 1; // Software XOFF Transmission
	uint32_t rsv3:   1;
	uint32_t rtlc:   1; // Re-transmit on Late Collision
	uint32_t nrtu:   1; // No Re-transmit on underrun (82544GC/EI only)
	uint32_t rsv4:   6;
};

// Transmit InterPacket Gap 
struct e1000_tipg {
	uint32_t ipgt:   10;    // IPG Transmit Time
	uint32_t ipgr1:  10;    // IPG Receive Time 1
	uint32_t ipgr2:  10;    // IPG Receive Time 2
	uint32_t rsv:    2;
};

// Transmit Descriptor Length register (TDLEN)
struct e1000_tdlen {
	uint32_t zero: 7;   // Ignore on write. Reads back as 0b
	uint32_t len:  13;  // Descriptor Length. Number of bytes allocated to the transmit descriptor circular buffer.
	uint32_t rsv:  12;
};

// Transmit Descriptor Tail register 
struct e1000_tdt {
	uint16_t tdt;   // Transmit Descriptor Tail
	uint16_t rsv;
};

// Transmit Descriptor Head register (TDH)
struct e1000_tdh {
	uint16_t tdh;   // Transmit Descriptor Head
	uint16_t rsv;
};


// Receive Descriptor Layout
struct e1000_rx_desc {
	uint64_t addr;
	uint16_t length;
	uint16_t chksum;
	uint8_t  status;
	uint8_t  errors;
	uint16_t special;
}__attribute__((packed));

// Receive Descriptor Length register (RDLEN)
struct e1000_rdlen {
	unsigned zero: 7; // Zero value Ignore on write. Reads back as 0b.
	unsigned len: 13; // Receive Descriptor length Provides the number of receive descriptors (in a multiple of eight).
	unsigned rsv: 12; // 
};

// Receive Descriptor Head register (RDH)
struct e1000_rdh {
	uint16_t rdh;	// Receive Descriptor Head.
	uint16_t rsv;
};

// Receive Descriptor Tail register (RDT)
struct e1000_rdt {
	uint16_t rdt;	// Receive Descriptor Tail.
	uint16_t rsv;
};


int e1000_attachfn(struct pci_func *pcif);
static void e1000_transmit_init();
int e1000_transmit(void *data, size_t len);
static void e1000_receive_init();

#endif	// JOS_KERN_E1000_H
