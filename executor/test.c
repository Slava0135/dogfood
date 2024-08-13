#include "executor.h"
int fd_1 = -1, fd_2 = -1, fd_3 = -1, fd_4 = -1, __end_fd = -1;
int test_syscall()
{
srand(9838034);
do_create("/NMlADSeBZipvSFyFJufoo_1", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
do_hardlink("/NMlADSeBZipvSFyFJufoo_1", "/hxyrSheCeeFvzjUOcWFkglZnpgfoo_2"); 
do_symlink("/", "/KsnJcPLTpIeXLauQbFhdjgYBghIyQtqrAmfoo_3"); 
do_hardlink("/NMlADSeBZipvSFyFJufoo_1", "/lGIkfoo_4"); 
do_open(fd_1, "/NMlADSeBZipvSFyFJufoo_1", O_RDWR); 
do_write(fd_1, 3, 16384); 
do_mkdir("/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
do_close(fd_1); 
do_hardlink("/NMlADSeBZipvSFyFJufoo_1", "/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/QOmqiXkVbQHVQJJlWUnRBZsSwuZdOHRfoo_7"); 
do_symlink("/KsnJcPLTpIeXLauQbFhdjgYBghIyQtqrAmfoo_3", "/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/kcBahYhWgzSJfSXPYSQMwXLOVkWaElWpogahjITFofoo_13"); 
do_open(fd_2, "/NMlADSeBZipvSFyFJufoo_1", O_RDWR); 
do_write(fd_2, 1, 4096); 
do_open(fd_3, "/NMlADSeBZipvSFyFJufoo_1", O_RDWR); 
do_write(fd_3, 3, 53248); 
do_read(fd_3, 14, 24576); 
do_create("/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/qZtAfmfQNhuJaVGskyTTYsToUcRafoo_15", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
do_create("/aDRZgHkfoo_16", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
do_rename("/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/qZtAfmfQNhuJaVGskyTTYsToUcRafoo_15", "/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/HiderBwxhkxDSNGXiUYfoo_19"); 
do_read(fd_3, 13, 81920); 
do_open(fd_4, "/NMlADSeBZipvSFyFJufoo_1", O_RDWR | O_SYNC); 
do_rename("/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/kcBahYhWgzSJfSXPYSQMwXLOVkWaElWpogahjITFofoo_13", "/wnDNOYYsyKsmwlGPsXvJfoo_24"); 
do_close(fd_2); 
do_hardlink("/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/QOmqiXkVbQHVQJJlWUnRBZsSwuZdOHRfoo_7", "/GbdwxoPwioalJtBkIyCThypYbwEBrxGpQnefoo_25"); 
do_close(fd_4); 
do_create("/JDhmRHuIzyomqWtmoEXYAPFXPAEYbfPpqRlfoo_26", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
do_write(fd_3, 6, 49152); 
do_rename("/NMlADSeBZipvSFyFJufoo_1", "/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/icanlYMIudIBLjJTPZmXcStKyJafoo_30"); 
do_close(fd_3); 
do_mkdir("/dogTayIYQjlEgGsEFmhIFYMtuzcECXeVGYMJqmptsiqmmoTkdfoo_5/jhCCyUpUWOHjMKfoo_32", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
do_symlink("/", "/rXlrtCbBANYFVmDlcbSLmyuEtuvBJmFgSfNetumfoo_37"); 
return 0;
}
int concurrent_syscall_1() { return 0; }
int concurrent_syscall_2() { return 0; }
