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




int time; // Time since scheduler start.
int runningProcess; // Index of running process, -1 means no process is running.
int finishedCount; // The number of finished process.
int keepTime; // Time for RR
int queue[4096]; //queue for scheduling
int start, end;
pid_t defaultRunner;
//pid_t next; // The expected next process to run in RR.


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
	start = 0;
	end = 0;
	// Assign scheduler CPU number 0 and set a hight priority.
	assignCPU(getpid(), 0);
	wakeupProcess(getpid());

	// Run and set default running.
	defaultRunner = fork();
	if(defaultRunner < 0){
		fprintf(stderr, "[ERROR]	fork error\n");
		exit(0);
	}
	else if(defaultRunner == 0){
		while(1);
	}
	else{
		// Assign default runner CPU number 1 and set a hight priority.
		assignCPU(defaultRunner, 1);
		wakeupProcess(defaultRunner);
	}


	// Set other processes pid to -1.
	for(int i = 0; i < n; i++){
		process[i].pid = -1;
	}
	
	// Initialize
	time = 0;
	runningProcess = -1;
	finishedCount = 0;
	//next = -1;

	while(1){

		/* Check if there is a finished process. */
		if(runningProcess != -1 && process[runningProcess].executionTime == 0){
			//fprintf(stderr, "[FINISH]	%s was finished at time %d\n", process[runningProcess].name, time);
			waitpid(process[runningProcess].pid, NULL, 0);
			//fprintf(stderr, "[CONTEXTSWITCH]	to default runner\n");
			wakeupProcess(defaultRunner);
			finishedCount++;
			if(finishedCount == n){
				// When all processes are finished, kill default runner and break. 
				kill(defaultRunner, SIGKILL);
				break;
			}
			//next = (runningProcess + 1) % n;
			process[runningProcess].pid = -1;
			/*
			while(process[next].pid == -1 || process[next].executionTime == 0){
				if(next == runningProcess){
					break;
				}
				next = (next + 1) % n;
			}
			if(next == runningProcess){
				next = -1;
			}
			*/

			runningProcess = -1;
		}


		/* Check if there is a ready process. */
		for(int i = 0; i < n; i++){
			if(process[i].readyTime == time){
				process[i].pid = executeProcess(&process[i]);
				//fprintf(stderr, "[READY]	%s is ready and executed with pid %d at time %d\n", process[i].name, process[i].pid, time);		
				printf("%s %d\n", process[i].name, process[i].pid);
				fflush(stdout);
				queue[end] = i;
				end++;
				/*
				if(runningProcess == -1 && next == -1){
					next = i;
				}
				*/
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


/* Assign process pid to coreNumber processor. */
void assignCPU(pid_t pid, int coreNumber){
	if(coreNumber > get_nprocs()){
		fprintf(stderr, "[ERROR]	wrong core index\n");
		exit(0);
	}
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(coreNumber, &mask);

	// Set affinity to coreNumber core.
	if(sched_setaffinity(pid, sizeof(mask), &mask) < 0){
		fprintf(stderr, "[ERROR]	set affinity error with error number %d\n", errno);
		exit(0);
	}
	return;
}


/* Suspend process pid by setting a low priority. */
void suspendProcess(pid_t pid){
	struct  sched_param p;
	p.sched_priority = MINPRIORITY;	
	//fprintf(stderr, "[SUS]	pid %d is suspend\n", pid);
	if(sched_setscheduler(pid, SCHED_FIFO, &p) < 0){
		fprintf(stderr, "[ERROR]	Suspending error with error number %d\n", errno);
		exit(0);
	}
	return;
}


/* Wake up process pid by setting a hight priority. */
void wakeupProcess(pid_t pid){
	struct sched_param p;
	p.sched_priority = MAXPRIORITY;	
	//fprintf(stderr, "[UP]	pid %d is up\n", pid);
	if(sched_setscheduler(pid, SCHED_FIFO, &p) < 0){
		fprintf(stderr, "[ERROR]	Waking up error with error number %d\n", errno);
		exit(0);
	}
	return;
}


/* Comparasion function sort process from early ready time to late ready time.  */
int com(const void *p1, const void *p2){
	Process *process1 = (Process*)p1;
	Process *process2 = (Process*)p2;
	return process1->readyTime - process2->readyTime;
}


/* Execute a ready process and suspend it until scheduler wakes it up.  */
int executeProcess(Process *process){
	
	pid_t pid = fork();
	if(pid < 0){
		fprintf(stderr, "[ERROR]	fork error with error  number %d\n", errno);	
		exit(0);
	}
	//child
	else if(pid == 0){

		/*
		printf("child\n");
		processInfo(getppid());
		processInfo(getpid());	
		*/
		
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
		//fprintf(stderr, "%s\n", sendTodmsg);
		syscall(335, sendTodmsg);
		exit(0);
	}
	//parent
	else if(pid > 0){
		
		/*
		printf("parent\n");
		processInfo(getpid());
		processInfo(pid);
		*/

		//assign all process same index of CPU.
		assignCPU(pid, 0);
		wakeupProcess(pid);
		suspendProcess(getpid());
		assignCPU(pid, 1);
		suspendProcess(pid);
		return pid;
	}
}


/* According to policy, select the next process or keep this process running.*/
void nextProcess(Process *process, char *policy, int n, pid_t *runningProcess, int time, int *keepTime){
	
	int temp = *runningProcess;
	
	if(*runningProcess != -1 && ( (strcmp(policy, "FIFO")) == 0 || (strcmp(policy, "SJF")) == 0) ){
		/* There is a process running and policy is FIFO or SJF, no preemtion. */
		return; 
	}
	else if(strcmp(policy, "PSJF") == 0 || strcmp(policy, "SJF") == 0 ){
		/* Policy is PSJF or there is no process running and policy is SJF.
		   Choose the least execution time process to run. */
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
		/* Policy is FIFO and there is no process running. 
		   Choose the least ready time process to run. */
		if(temp != -1){
			// Shouldn't happend.
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
		/* Policy is RR, check if the running time chieve time quamtum.
		   if yes, choose next process to run. */
		if(temp == -1 && start != end){
			// There is no process running, let next pid to run.
			//temp = next;
			temp = queue[start];
			start++;
		}
		else if( (time - *keepTime) % 500 == 0 && start != end){
			// There is a process running, check how much time it has used to run.
			/*
			do{
				temp++;
				temp %= n;
			}while(process[temp].pid == -1 || process[temp].executionTime == 0);
			*/
			queue[end] = temp;
			end++;
			temp = queue[start];
			start++;

		}
	}
	
	if(*runningProcess == temp){
		// No context switch happens.
		return;
	}
	else{
		// Context switch happens.
		wakeupProcess(process[temp].pid);
		if(*runningProcess != -1){	
			//fprintf(stderr, "[CONTEXTSWITCH]	pid %d to pid %d\n", process[*runningProcess].pid, process[temp].pid);
			suspendProcess(process[*runningProcess].pid);
		}
		else{
			//fprintf(stderr, "[CONTEXTSWITCH]	to pid %d\n", process[temp].pid);
			suspendProcess(defaultRunner);
		}
		*runningProcess = temp;
		*keepTime = time;
		return;
	}

}



/* Check the process priority.  */
void processInfo(pid_t pid){
	struct sched_param p;
	if(sched_getparam(pid, &p) < 0){
		fprintf(stderr, "[ERROR]	get param error with error number %d\n", errno);
	}
	fprintf(stderr, "[INFO]	pid %d priority %d\n", pid, p.sched_priority);
	return;
}
