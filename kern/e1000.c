#include <kern/e1000.h>

// LAB 6: Your driver code here
#include <kern/pmap.h>
#include <inc/string.h>

volatile void *bar_va;
#define E1000REG(offset) (void *)(bar_va + offset)

struct e1000_tdh *tdh;
struct e1000_tdt *tdt;
struct e1000_tx_desc tx_desc_array[TXDESCS];
char tx_buffer_array[TXDESCS][TX_PKT_SIZE];

struct e1000_rdh *rdh;
struct e1000_rdt *rdt;
struct e1000_rx_desc rx_desc_array[RXDESCS];
char rx_buffer_array[RXDESCS][RX_PKT_SIZE];

uint32_t E1000_MAC[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

int
e1000_attachfn(struct pci_func *f)
{
    pci_func_enable(f);
    bar_va = mmio_map_region(f->reg_base[0], f->reg_size[0]);

    uint32_t *status_reg = (uint32_t *)E1000REG(E1000_STATUS);
    // cprintf("mmoi map pa=%08x to va=%08x, status addr=%08x, value=%08x\n", f->reg_base[0], bar_va, status_reg, *status_reg);
    assert(*status_reg == 0x80080783);

    e1000_transmit_init();
    e1000_receive_init();
    // test
    // char *data = "transmit test";
    // e1000_transmit(data, 13);
    return 0;
}


static void
e1000_transmit_init()
{
    // Allocate a region of memory for the transmit descriptor list. Software should insure this memory is
    // aligned on a paragraph (16-byte) boundary. Program the Transmit Descriptor Base Address
    // (TDBAL/TDBAH) register(s) with the address of the region. TDBAL is used for 32-bit addresses
    // and both TDBAL and TDBAH are used for 64-bit addresses
    int i;
	for (i = 0; i < TXDESCS; i++) {
		tx_desc_array[i].addr = PADDR(tx_buffer_array[i]);
		tx_desc_array[i].cmd = 0;
		tx_desc_array[i].status |= E1000_TXD_STAT_DD;
	}

    uint32_t *tdbal = (uint32_t *)E1000REG(E1000_TDBAL);
    *tdbal = PADDR(tx_desc_array);

    uint32_t *tdbah = (uint32_t *)E1000REG(E1000_TDBAH);
    *tdbah = 0;

    // Set the Transmit Descriptor Length (TDLEN) register to the size (in bytes) of the descriptor ring.
    // This register must be 128-byte aligned.
    struct e1000_tdlen *tdlen = (struct e1000_tdlen *)E1000REG(E1000_TDLEN);
    tdlen->len = TXDESCS;

    // The Transmit Descriptor Head and Tail (TDH/TDT) registers are initialized (by hardware) to 0b
    // after a power-on or a software initiated Ethernet controller reset. Software should write 0b to both
    // these registers to ensure this.
    tdh = (struct e1000_tdh *)E1000REG(E1000_TDH);
    tdh->tdh = 0;
    tdt = (struct e1000_tdt *)E1000REG(E1000_TDT);
    tdt->tdt = 0;

    // Initialize the Transmit Control Register (TCTL) for desired operation to include the following:
    // • Set the Enable (TCTL.EN) bit to 1b for normal operation.
    // • Set the Pad Short Packets (TCTL.PSP) bit to 1b.
    // • Configure the Collision Threshold (TCTL.CT) to the desired value. Ethernet standard is 10h.
    //       This setting only has meaning in half duplex mode.
    // • Configure the Collision Distance (TCTL.COLD) to its expected value. For full duplex
    //      operation, this value should be set to 40h. For gigabit half duplex, this value should be set to
    //      200h. For 10/100 half duplex, this value should be set to 40h.
    struct e1000_tctl *tctl = (struct e1000_tctl *)E1000REG(E1000_TCTL);
    tctl->en = 1;
    tctl->psp = 1;
    tctl->ct = 0x10;
    tctl->cold = 0x40;

    // Program the Transmit IPG (TIPG) register with the following decimal values to get the minimum
    // legal Inter Packet Gap: ipgt=10，ipgr1=4(ipgr2 * 2/3)，ipgr2=6
    struct e1000_tipg *tipg = (struct e1000_tipg *)E1000REG(E1000_TIPG);
    tipg->ipgt = 10;
    tipg->ipgr1 = 4;
    tipg->ipgr2 = 6;
}

int
e1000_transmit(void *data, size_t len)
{
    uint32_t current = tdt->tdt;
    // cprintf("e1000_transmit current = %0x8\n", current);
    if(!(tx_desc_array[current].status & E1000_TXD_STAT_DD)) {
        return -E_TRANSMIT_RETRY;
    }

    // cprintf("e1000_transmit set status\n");
    tx_desc_array[current].length = len;
    tx_desc_array[current].status &= ~E1000_TXD_STAT_DD;
    tx_desc_array[current].cmd |= (E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS);
    memcpy(tx_buffer_array[current], data, len);
    tdt->tdt = (current + 1) % TXDESCS;
    return 0;
}


static void
e1000_receive_init()
{
    // Allocate a region of memory for the receive descriptor list. Software should insure this memory is
    // aligned on a paragraph (16-byte) boundary. Program the Receive Descriptor Base Address
    // (RDBAL/RDBAH) register(s) with the address of the region. RDBAL is used for 32-bit addresses
    // and both RDBAL and RDBAH are used for 64-bit addresses.

    int i;
	for (i = 0; i < TXDESCS; i++) {
		rx_desc_array[i].addr = PADDR(rx_buffer_array[i]);
	}

    uint32_t *rdbal = (uint32_t *)E1000REG(E1000_RDBAL);
    *rdbal = PADDR(rx_desc_array);

    uint32_t *rdbah = (uint32_t *)E1000REG(E1000_RDBAH);
    *rdbah = 0;


    // Set the Receive Descriptor Length (RDLEN) register to the size (in bytes) of the descriptor ring.
    // This register must be 128-byte aligned.
    struct e1000_rdlen *rdlen = (struct e1000_rdlen *)E1000REG(E1000_RDLEN);
    rdlen->len = RXDESCS;

    // The Receive Descriptor Head and Tail registers are initialized (by hardware) to 0b after a power-on
    // or a software-initiated Ethernet controller reset. Receive buffers of appropriate size should be
    // allocated and pointers to these buffers should be stored in the receive descriptor ring. Software
    // initializes the Receive Descriptor Head (RDH) register and Receive Descriptor Tail (RDT) with the
    // appropriate head and tail addresses. Head should point to the first valid receive descriptor in the
    // descriptor ring and tail should point to one descriptor beyond the last valid descriptor in the
    // descriptor ring.
    rdh = (struct e1000_rdh *)E1000REG(E1000_RDH);
    rdh->rdh = 0;
    rdt = (struct e1000_rdt *)E1000REG(E1000_RDT);
    rdt->rdt = RXDESCS - 1;

    /*
    Program the Receive Control (RCTL) register with appropriate values for desired operation to
    include the following:
    • Set the receiver Enable (RCTL.EN) bit to 1b for normal operation. However, it is best to leave
      the Ethernet controller receive logic disabled (RCTL.EN = 0b) until after the receive
      descriptor ring has been initialized and software is ready to process received packets.
    • Set the Long Packet Enable (RCTL.LPE) bit to 1b when processing packets greater than the
      standard Ethernet packet size. For example, this bit would be set to 1b when processing Jumbo
      Frames.
    • Loopback Mode (RCTL.LBM) should be set to 00b for normal operation.
    • Configure the Receive Descriptor Minimum Threshold Size (RCTL.RDMTS) bits to the
      desired value.
    • Configure the Multicast Offset (RCTL.MO) bits to the desired value.
    • Set the Broadcast Accept Mode (RCTL.BAM) bit to 1b allowing the hardware to accept
      broadcast packets.
    • Configure the Receive Buffer Size (RCTL.BSIZE) bits to reflect the size of the receive buffers
      software provides to hardware. Also configure the Buffer Extension Size (RCTL.BSEX) bits if
      receive buffer needs to be larger than 2048 bytes.
    • Set the Strip Ethernet CRC (RCTL.SECRC) bit if the desire is for hardware to strip the CRC
      prior to DMA-ing the receive packet to host memory.
    • For the 82541xx and 82547GI/EI, program the Interrupt Mask Set/Read (IMS) register to
      enable any interrupt the driver wants to be notified of when the even occurs. Suggested bits
      include RXT, RXO, RXDMT, RXSEQ, and LSC. There is no immediate reason to enable the
      transmit interrupts. Plan to optimize interrupts later, including programming the interrupt
      moderation registers TIDV, TADV, RADV and IDTR.
    • For the 82541xx and 82547GI/EI, if software uses the Receive Descriptor Minimum
      Threshold Interrupt, the Receive Delay Timer (RDTR) register should be initialized with the
      desired delay time.
    */
    uint32_t *rctl = (uint32_t *)E1000REG(E1000_RCTL);
    *rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC;

    // get ra address
    uint32_t ral = 0, rah = 0;
    for(i = 0; i < 4; i ++)
        ral |= E1000_MAC[i] << (8 * i);
    for(i = 4; i < 6; i ++)
        rah |= E1000_MAC[i] << (8 * i);

    // set ra address
    uint32_t *ra = (uint32_t *)E1000REG(E1000_RA);
    ra[0] = ral;
    ra[1] = rah;
}

int 
e1000_receive(void *data, size_t len)
{
    static int32_t next = 0;
    if(!(rx_desc_array[next].status & E1000_RXD_STAT_DD)) {
        return -E_RECEIVE_RETRY;
    }

    if(rx_desc_array[next].errors) {
        return -E_RECEIVE_RETRY;
    }

    size_t recv_len = rx_desc_array[next].length;
    memcpy(data, rx_buffer_array[next], recv_len);

    rdt->rdt = (rdt->rdt + 1) % RXDESCS;
    next = (next + 1) % RXDESCS;

    return recv_len;
}