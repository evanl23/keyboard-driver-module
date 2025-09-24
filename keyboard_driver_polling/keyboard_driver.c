/*
 *  Simple Linux keyboard driver, polling version
 *  CS552 Fall 2025
 *  Evan Liu
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

/* initialize state savers for caps lock and shift */
int shift_pressed = 0;

/* Defines an ioctl number via macro call */
#define KEYBOARD_IOCTL _IOR(0, 6, char) // TODO

static int keyboard_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg);

// variable that holds available operations available for the ioctl call from user space
static struct file_operations proc_operations; 

// variable for /proc entry 
static struct proc_dir_entry *proc_entry;

static int __init initialization_routine(void) {
  printk("<1> Loading keyboard driver\n");

  proc_operations.ioctl = keyboard_ioctl; /* declares what kind of operation the user level can call */

  /* Start create proc entry */
  proc_entry = create_proc_entry("keyboard_ioctl", 0444, NULL);
  if(!proc_entry)
  {
    printk("<1> Error creating /proc entry.\n");
    return 1;
  }

  //proc_entry->owner = THIS_MODULE; <-- This is now deprecated
  proc_entry->proc_fops = &proc_operations;

  return 0;
}

/* inline assembly calls to read byte from port address */
static inline unsigned char inb( unsigned short usPort ) {

    unsigned char uch;
   
    asm volatile( "inb %1,%0" : "=a" (uch) : "Nd" (usPort) );
    return uch;
}

/* inline assembly calls to write to port addresss */
static inline void outb( unsigned char uch, unsigned short usPort ) {

    asm volatile( "outb %0,%1" : : "a" (uch), "Nd" (usPort) );
}

/* ioctl call for polling keyboard for characters */
char my_getchar ( void ) {
  static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    
  static char scancode_shift[128] = "\0\e!@#$%^&*()_+\177\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0\\ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  /* Poll keyboard status register at port 0x64 checking bit 0 to see if
   * output buffer is full. We continue to poll if the msb of port 0x60
   * (data port) is set, as this indicates out-of-band data or a release
   * keystroke
   */
  char c; 
  while( !(inb( 0x64 ) & 0x1) || ( ( c = inb( 0x60 ) ) & 0x80 ) );
  int pressed = !(c & 0x80);    // 1 if pressed
  
  // Track modifier state
  if (c == 0x2A || c == 0x36) {   // shift pressed 
    shift_pressed = pressed;
    return 0;
  }
 
  if (!pressed) {
    return 0;
  }
  
  if (shift_pressed) { // shift pressed or caps lock
    return scancode_shift[ (int)c ];
  }
  else {
    return scancode[ (int)c ];
  }
}

void my_printk(char *string)
{
  struct tty_struct *my_tty;

  my_tty = current->signal->tty;

  if (my_tty != NULL) {
    (*my_tty->driver->ops->write)(my_tty, string, strlen(string));
    (*my_tty->driver->ops->write)(my_tty, "\015\012", 2);
  }
} 

static void __exit cleanup_routine(void) {

  printk("<1> Dumping keyboard driver\n");
  remove_proc_entry("keyboard_ioctl", NULL);

  return;
}

/***
 * ioctl() entry point...
 */
static int keyboard_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
  char c; 
  switch (cmd) {

  case KEYBOARD_IOCTL:
    c = my_getchar();

    copy_to_user((char __user *)arg, &c, sizeof(c)); /* copy to the user the character picked up from my_getchar */
    printk("ioctl: call to KEYBOARD_IOCTL (%c)!\n", c);

    break;
  
  default:
    return -EINVAL;
    break;
  }
  
  return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 
