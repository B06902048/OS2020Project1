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
#include<sys/sysinfo.h>




int time; //time since scheduler start.
int runningProcess; //Index of running process, -1 means no process is running.
int finishedCount; //The number of finished process.
int keepTime; //time for RR
pid_t defaultRunner;

void scheduling(Process *process, char *policy, int n);
void assignCPU(pid_t pid, int coreNumber);
void suspendProcess(pid_t pid);
void wakeupProcess(pid_t pid);
void initialProcess(pid_t pid);
int com(const void *p1, const void *p2);
int executeProcess(Process *process);
void nextProcess(Process *process, char *policy, int n, pid_t *runningProcess, int time, int *keepTime);
void processInfo(pid_t pid);


void scheduling(Process *process, char *policy, int n){
	//printf("%d processors and %d available\n", get_nprocs_conf(), get_nprocs());
	//exit(0);
	

	qsort(process, n, sizeof(Process), com);	
	
	//assign scheduler a CPU with number 0
	assignCPU(getpid(), 0);

	defaultRunner = fork();
	if(defaultRunner < 0){
		fprintf(stderr, "[ERROR]	fork error\n");
	}
	else if(defaultRunner == 0){
		while(1);
	}
	else{
		assignCPU(defaultRunner, 1);
		wakeupProcess(defaultRunner);
	}

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
			//fprintf(stderr, "[FINISH]	%s was finished at time %d\n", process[runningProcess].name, time);
			waitpid(process[runningProcess].pid, NULL, 0);
			runningProcess = -1;
			wakeupProcess(defaultRunner);
			finishedCount++;
			if(finishedCount == n){
				kill(defaultRunner, SIGKILL);
				break;
			}
		}

		//Check if there is a ready process
		for(int i = 0; i < n; i++){
			if(process[i].readyTime == time){
				process[i].pid = executeProcess(&process[i]);
				//fprintf(stderr, "[READY]	%s is ready and executed with pid %d at time %d\n", process[i].name, process[i].pid, time);		
				fprintf(stderr, "%s %d\n", process[i].name, process[i].pid);
			}
			if(process[i].readyTime > time){
				break;
			}
		}

		//Select next process to run
		nextProcess(process, policy, n, &runningProcess, time, &keepTime);

		//Run a time of unit
		timeUnit();
		if(runningProcess != -1){
			process[runningProcess].executionTime--;
			//fprintf(stderr, "[ONEUNIT]	pid %d executionTime %d\n", process[runningProcess].pid, process[runningProcess].executionTime);
		}
		time++;
	}

	return;
}


void assignCPU(pid_t pid, int coreNumber){
	if(coreNumber > sizeof(cpu_set_t)){
		fprintf(stderr, "[ERROR]	wrong core index\n");
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
void suspendProcess(pid_t pid){
	struct  sched_param p;
	p.sched_priority = 1;	
	//fprintf(stderr, "[SUS]	pid %d is suspend\n", pid);
	if(sched_setscheduler(pid, SCHED_FIFO, &p) < 0){
		fprintf(stderr, "[ERROR]	In suspend process set scheduler eooro with error number %d\n", errno);
		exit(0);
	}
	return;
}
void wakeupProcess(pid_t pid){
	struct sched_param p;
	p.sched_priority = 99;	
	//fprintf(stderr, "[UP]	pid %d is up\n", pid);
	if(sched_setscheduler(pid, SCHED_FIFO, &p) < 0){
		fprintf(stderr, "[ERROR]	In wake up process set scheduler error with error number %d\n", errno);
		exit(0);
	}
	return;
}

int com(const void *p1, const void *p2){
	Process *process1 = (Process*)p1;
	Process *process2 = (Process*)p2;
	return process1->readyTime - process2->readyTime;
}

int executeProcess(Process *process){
	pid_t pid = fork();

	if(pid < 0){
		fprintf(stderr, "[ERROR]	fork error with error  number %d\n", errno);	
		exit(0);
	}
	//child
	else if(pid == 0){
		printf("child\n");
		processInfo(getppid());
		processInfo(getpid());	
		//fprintf(stderr, "[FORK] Child pid %d\n", getpid());
		unsigned long long startSec, startNSec, endSec, endNSec;
		char sendTodmsg[1024];
		syscall(334, &startSec, &startNSec); // get now time.
		wakeupProcess(getppid());
		suspendProcess(getpid());
		int executionTime = process->executionTime;
		for(int i = 0; i < executionTime; i++){
			timeUnit();
		}
		syscall(334, &endSec, &endNSec); //get now time.
		sprintf(sendTodmsg, "[Project1] %d %llu.%09llu %llu.%09llu", getpid(), startSec, startNSec, endSec, endNSec);
		fprintf(stderr, "%s\n", sendTodmsg);
		exit(0);
	}
	//parent
	else if(pid > 0){
		printf("parent\n");
		processInfo(getpid());
		processInfo(pid);
		//assign all process same index of CPU.
		assignCPU(pid, 0);
		wakeupProcess(pid);
		suspendProcess(getpid());
		assignCPU(pid, 1);
		suspendProcess(pid);
		return pid;
	}
}


void nextProcess(Process *process, char *policy, int n, pid_t *runningProcess, int time, int *keepTime){
	int temp = *runningProcess;
	if(*runningProcess != -1 && ( (strcmp(policy, "FIFO")) == 0 || (strcmp(policy, "SJF")) == 0) ){
		return; 
	}
	else if(strcmp(policy, "PSJF") == 0 || strcmp(policy, "SJF") == 0 ){
		for(int i = 0; i < n; i++){
			if(process[i].pid == -1 || process[i].executionTime == 0){
				continue;
			}
			if(temp == -1 || process[i].executionTime < process[temp].executionTime){
				temp = i;
			}
		}
	}
	else if(strcmp(policy, "FIFO") == 0){
		if(temp != -1){
			fprintf(stderr, "[ERROR]	Why runnning process is not -1????????\n");
			exit(0);
		}
		for(int i = 0; i < n; i++){
			if(process[i].pid == -1 || process[i].executionTime == 0){
				continue;
			}
			else if(temp == -1 || process[i].readyTime < process[temp].readyTime){
				temp = i;
			}
		}
	}	
	else if(strcmp(policy, "RR") == 0){
		if(temp == -1){
			for(int i = 0; i < n; i++){
				if(process[i].pid != -1 && process[i].executionTime > 0){
					temp = i;
					break;
				}
			}
		}
		else if( (time - *keepTime) % 500 == 0){
			do{
				temp++;
				temp %= n;
			}while(process[temp].pid == -1 || process[temp].executionTime == 0);
		}
	}
	
	if(*runningProcess == temp){
		return;
	}
	else{
		//context switch
		if(*runningProcess != -1){
			fprintf(stderr, "[CONTEXTSWITCH]	pid %d to pid %d\n", process[*runningProcess].pid, process[temp].pid);
		}
		else{
			fprintf(stderr, "[CONTEXTSWITCH]	to pid %d\n", process[temp].pid);
		}
		wakeupProcess(process[temp].pid);
		if(*runningProcess != -1){
			suspendProcess(process[*runningProcess].pid);
		}
		else{
			suspendProcess(defaultRunner);
		}
		*runningProcess = temp;
		*keepTime = time;
		return;
	}

}


void initialProcess(pid_t pid){
	struct sched_param p;
	p.sched_priority = 50;
	if(sched_setscheduler(pid, SCHED_FIFO, &p) < 0){
		fprintf(stderr, "[ERROR]	In wake up process set scheduler error with error number %d\n", errno);
		exit(0);
	}
	fprintf(stderr, "[INITIAL]	pid %d is up\n", pid);
	return;
}


void processInfo(pid_t pid){
	struct sched_param p;
	if(sched_getparam(pid, &p) < 0){
		fprintf(stderr, "[ERROR]	get param error with error number %d\n", errno);
	}
	fprintf(stderr, "[INFO]	pid %d priority %d\n", pid, p.sched_priority);
}
