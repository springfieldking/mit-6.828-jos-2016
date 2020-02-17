#include <kern/e1000.h>

// LAB 6: Your driver code here

int
e1000_attachfn(struct pci_func *f)
{
    pci_func_enable(f);
    return 0;
}
