#include<linux/linkage.h>
#include<linux/kernel.h>

asmlinkage void sys_my_printk(char *messange){
	printk("%s\n", messange);
	return;
}
