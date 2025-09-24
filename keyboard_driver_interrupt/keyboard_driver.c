/*
 *  Simple Linux keyboard driver, interrupt version
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
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");

#define MY_WORK_QUEUE_NAME "WQsched.c"

/* static variables for old handler */
static irq_handler_t default_handler;
static void * default_dev_id;

/* initialize a queue for */
static struct workqueue_struct *my_workqueue;

/* initialize state savers for shift key modifier and key_release */
int shift_pressed = 0;
static int key_release = 0;

/* global static for storing last read key */
static char last_key;

/* Defines an ioctl number via macro call */
#define KEYBOARD_IOCTL _IOR(0, 6, char)

static int keyboard_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg);

/* declare tty struct */
struct tty_struct *my_tty;

/* variable that holds available operations available for the ioctl call from user space */
static struct file_operations proc_operations; 

/* variable for /proc entry */
static struct proc_dir_entry *proc_entry;

/* custom irq_to_desc function using address found in kallsyms */
int (*my_irq_to_desc)(int param1) = (void *)0xc104728c;

/* inline assembly calls to read byte from port address */
static inline unsigned char inb( unsigned short usPort ) {
    unsigned char uch; 
    asm volatile( "inb %1,%0" : "=a" (uch) : "Nd" (usPort) );
    return uch;
}

/* custom printk to print to tty */
static void my_printk(char *string)
{
  if (my_tty != NULL) {
    ((my_tty->driver)->ops->write) (my_tty, string, strlen(string)); 
  } else {
    printk("<1> my_tty is null");
  } 
} 

/* declare struct for get_char work */
struct getchar_info {
    struct work_struct work;
    unsigned int scancode;
};
// static struct getchar_info gci; // Statically declare or use kmalloc()

/* 
 * This will get called by the kernel as soon as it's safe
 * to do everything normally allowed by kernel modules.
 */
static void got_char(struct work_struct *work)
{
  static char scancode[128] = "\0\e1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    
  static char scancode_shift[128] = "\0\e!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0\\ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  struct getchar_info *info = container_of(work, struct getchar_info, work);
  unsigned char code = info->scancode; 
  unsigned char sc = code & 0x7F;
 
  int pressed = !(code & 0x80); // 1 if pressed
  
  // Track modifier state
  if (sc == 0x2A || sc == 0x36) { // shift pressed 
    shift_pressed = pressed;
    return;
  }
  
  if (!pressed) {
    return;
  }

  char c;
  if (shift_pressed) // shift pressed
    c = scancode_shift[sc];
  else
    c = scancode[sc];
  
  if (c) {
      if (sc == 0x0E) { // check if backspace
        char del[] = "\b \b";
        my_printk(&del); 
      } else {
        last_key = c;
        my_printk(&last_key);
      }
      key_release = 1;
      printk(KERN_INFO "Scan Code %x %s.\n",
	       code & 0x7F,
	       (code & 0x80) ? "Released" : "Pressed"); 
  }
  kfree(info);
}

/* 
 * This function services keyboard interrupts. It reads the relevant
 * information from the keyboard and then puts the non time critical
 * part into the work queue. This will be run when the kernel considers it safe.
 */
irqreturn_t irq_handler(int irq, void *dev_id) 
{
  struct getchar_info *info;
  info = kmalloc(sizeof(*info), GFP_ATOMIC);
	/* 
	 * This variables are static because they need to be
	 * accessible (through pointers) to the bottom half routine.
	 */
	static unsigned char scancode;
	unsigned char status;
  
  //Read keyboard status
	status = inb(0x64);
	scancode = inb(0x60);
  info->scancode = scancode;
	
  // initialize work
  INIT_WORK(&info->work, got_char);
	queue_work(my_workqueue, &info->work);

	return IRQ_HANDLED;
}

static int __init initialization_routine(void) {
  printk("<1> Loading interrupt keyboard driver\n");

  proc_operations.ioctl = keyboard_ioctl; /* declares what kind of operation the user level can call */

  /* Start create proc entry */
  proc_entry = create_proc_entry("keyboard_ioctl", 0444, NULL);
  if(!proc_entry)
  {
    printk("<1> Error creating /proc entry.\n");
    return 1;
  }
  
  my_tty = current->signal->tty;
  
  if (!my_tty) {
    printk("<1> my_tty not found");
    return 0;
  }
  
  //proc_entry->owner = THIS_MODULE; <-- This is now deprecated
  proc_entry->proc_fops = &proc_operations;
 
  // create work queue for interrupt handler
  my_workqueue = create_workqueue( MY_WORK_QUEUE_NAME );

  // save old irq handler and dev_id
  struct irq_desc *desc = (struct irq_desc *)my_irq_to_desc(1);
  default_handler = desc->action->handler;
  default_dev_id = desc->action->dev_id;

  // disabling IRQ1 
  free_irq(1, default_dev_id);
  
  // request irq to be rounted to our own irq handler
  return request_irq(1,
                    irq_handler,
                    IRQF_SHARED,
                    "my_keyboard_irq_handler",
                    default_dev_id);
}

static void __exit cleanup_routine(void) {

  printk("<1> Dumping interrupt keyboard driver\n");
  remove_proc_entry("keyboard_ioctl", NULL);

  flush_workqueue(my_workqueue);
  destroy_workqueue(my_workqueue);
  
  // command for resuming control to default keyboard driver
  free_irq(1, default_dev_id);

  request_irq(1,
              default_handler,
              IRQF_SHARED,
              "default_keyboard_irq_handler",
              default_dev_id);
}

/***
 * ioctl() entry point...
 */
static int keyboard_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
  
  switch (cmd) {

  case KEYBOARD_IOCTL:
    if (key_release == 1) {
      copy_to_user((char __user *)arg, &last_key, sizeof(last_key)); /* copy to the user the character picked up from got_char */
      key_release = 0;
      printk("ioctl: call to KEYBOARD_IOCTL (%c)!\n", last_key);
    }
    break;
  
  default:
    return -EINVAL;
    break;
  }
  
  return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 
