# Custom keyboard interrupt handler for Linux 2.6.33.2
This repository contains code for a custom keyboard driver module written as a primer assignment for Boston University Professor [Richard West](https://www.cs.bu.edu/fac/richwest/)'s CS552 class. There are two versions, a polling version that simply polls the existing IRQ for keyboard inputs, and an interrupt version where the existing IRQ is disabled and replaced with a custom interrupt handler.

# Building and running
To build this module against your kernel source tree, clone this repository locally and enter the directory of the module you would like to make, either the interrupt version or the polling version. 

Then, simple enter:
```sh
make
insmod keyboard_driver.ko
```

To remove the module, simple enter:
```sh
rmmod keyboard_driver
```

# Requirements
These modules are written for Linux 2.6.33.

# Sources
Below are all references I poured over during the writing of these modules.

[The Linux Kernel Module Programming Guide](https://tldp.org/LDP/lkmpg/2.6/html/index.html)
[PS/2 keyboard scancodes - OSDev](https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Sets,_Scan_Codes_and_Key_Codes)
[tty_struct source files - Bootlin](https://elixir.bootlin.com/linux/v2.6.33.2/source/include/linux/tty.h#L252)
[Disabling default IRQ - Kernel.ord](https://www.kernel.org/doc/html/v4.14/core-api/kernel-api.html#c.free_irq)
[Keyboard interrupt handler giving null value - Stack Overflow](https://stackoverflow.com/questions/19147569/keyboard-interrupt-handler-giving-null-value)
[Work around for functions in/proc/kallsyms that are not exported - Stack Overflow](https://stackoverflow.com/questions/6455343/linux-module-call-functions-which-is-in-proc-kallsyms-but-not-exported)
[Using msleep for delaying activation - Kernel.org](https://docs.kernel.org/timers/delay_sleep_functions.html#c.msleep)
[request_irq() and free_irq() functions - Linux Kernel Labs](https://linux-kernel-labs.github.io/refs/heads/master/labs/interrupts.html)
[/proc/kallsyms - Linux Audit](https://linux-audit.com/what-is/proc-kallsyms/)

