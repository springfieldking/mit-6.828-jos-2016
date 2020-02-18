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


int
e1000_attachfn(struct pci_func *f)
{
    pci_func_enable(f);
    bar_va = mmio_map_region(f->reg_base[0], f->reg_size[0]);

    uint32_t *status_reg = (uint32_t *)E1000REG(E1000_STATUS);
    // cprintf("mmoi map pa=%08x to va=%08x, status addr=%08x, value=%08x\n", f->reg_base[0], bar_va, status_reg, *status_reg);
    assert(*status_reg == 0x80080783);
    e1000_transmit_init();

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