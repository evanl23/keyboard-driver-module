/* 
 * User level program for keyboard driver module
 * CS552
 * Evan Liu
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define KEYBOARD_IOCTL _IOR(0, 6, char)

int main() {
  char c;

  int fd = open("/proc/keyboard_ioctl", O_RDWR); // open file descripter for reading and writing 

  while (1) { 
    ioctl(fd, KEYBOARD_IOCTL, &c); 
    if (c>=32 && c<=126) {
      printf("%c", c);
    }
  } 
  
  return 0;
}
