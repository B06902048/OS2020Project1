#define _GNU_SOURCE 
#include<sys/types.h>
#include"scheduler.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sched.h>

void scheduling(Process *process, char *policy, int n);
int assignCPU();
int blockProcess();
int wakeupProcess();
int com(const void *p1, const void *p2);

void scheduling(Process *process, char *policy, int n){
	qsort(process, n, sizeof(Process), com);	
	printf("CPU size = %d\n", sizeof(cpu_set_t));	
	return;
}


int assignCPU(){
	return 0;
}
int blockProcess(){
	return 0;
}
int wakeupProcess(){
	return 0;
}

int com(const void *p1, const void *p2){
	Process *process1 = (Process*)p1;
	Process *process2 = (Process*)p2;
	return process1->readyTime - process2->readyTime;
}
