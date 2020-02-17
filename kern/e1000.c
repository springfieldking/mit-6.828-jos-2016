#include <kern/e1000.h>

// LAB 6: Your driver code here
#include <kern/pmap.h>

#define E1000_STATUS 0x00008

int
e1000_attachfn(struct pci_func *f)
{
    pci_func_enable(f);
    void *va = mmio_map_region(f->reg_base[0], f->reg_size[0]);

    uint32_t * reg_addr = (uint32_t *)(va + E1000_STATUS);
    cprintf("mmoi map pa=%08x to va=%08x, status addr=%08x, value=%08x\n", f->reg_base[0], va, reg_addr, *reg_addr);
    return 0;
}
