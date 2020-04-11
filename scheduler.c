#define _GNU_SOURCE 
#include<sys/types.h>
#include"scheduler.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sched.h>
#include<errno.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<sys/wait.h>

int time; //time since scheduler start.
int runningProcess; //Index of running process, -1 means no process is running.
int finishedCount; //The number of finished process.


void scheduling(Process *process, char *policy, int n);
void assignCPU();
int blockProcess();
void wakeupProcess();
int com(const void *p1, const void *p2);
int executeProcess(Process *process);

void scheduling(Process *process, char *policy, int n){
	qsort(process, n, sizeof(Process), com);	
	
	//assign scheduler a CPU with number 0
	assignCPU(getpid(), 0);

	//set other pid to -1
	for(int i = 0; i < n; i++){
		process[i].pid = -1;
	}

	wakeupProcess(getpid());

	time = 0;
	runningProcess = -1;
	finishedCount = 0;

	while(1){
		//Check if there is a finished process
		if(runningProcess != -1 && process[runningProcess].executionTime == 0){
			fprintf(stderr, "[FINISH]	%s was finished at time %d\n", process[runningProcess].name, time);
			waitpid(process[runningProcess].pid, NULL, 0);
			runningProcess = -1;
			finishedCount++;
		}

		//Check if there is a ready process
		for(int i = 0; i < n; i++){
			if(process[i].readyTime == time){
				process[i].pid = executeProcess(&process[i]);

			}
		}
	}

	return;
}


void assignCPU(pid_t pid, int coreNumber){
	if(coreNumber > sizeof(cpu_set_t)){
		fprintf(stderr, "wrong core index\n");
		exit(0);
	}

	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(coreNumber, &mask);

	if(sched_setaffinity(pid, sizeof(mask), &mask) < 0){
		fprintf(stderr, "[ERROR]	set affinity error with error number %d\n", errno);
		exit(0);
	}
	return;
}
int blockProcess(){
	return 0;
}
void wakeupProcess(pid_t pid){
	struct sched_param p;
	p.sched_priority = 0;
	if(sched_setscheduler(pid, SCHED_OTHER, &p) < 0){
		fprintf(stderr, "[ERROR]	set scheduler error with error number %d\n", errno);
	}
	return;
}

int com(const void *p1, const void *p2){
	Process *process1 = (Process*)p1;
	Process *process2 = (Process*)p2;
	return process1->readyTime - process2->readyTime;
}

int executeProcess(Process *process){
	int pid = fork();

	if(pid < 0){
		fprintf(stderr, "[ERROR]	fork error with error  number %d\n", errno);	
		exit(0);
	}
	//child
	else if(pid == 0){
		unsigned long long startSec, startNSec, endSec, endNSec;
		char sendTodmsg[1024];
		syscall(334, &startSec, &startNSec); // get now time.
		int executionTime = process->executionTime;
		for(int i = 0; i < executionTime; i++){
			timeUnit();
		}
		syscall(334, &endSec, &endNSec); //get now time.
		sprintf(sendTodmsg, "[Project1] %d %llu.%09llu %llu.%09llu\n", getpid(), startSec, startNSec, endSec, endNSec);
		exit(0);
	}
	//parent
	else if(pid > 0){
		//assign all process same index of CPU.
		assignCPU(pid, 1);
		return pid;
	}
}
