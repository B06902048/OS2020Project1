#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"scheduler.h"

#define MAXPOLICY 8

/*
#define MAXNAME 8
typedef struct process{
	char name[MAXNAME];
	int readyTime, exectionTime;
	pid_t pid
} Process;
*/

int checkPolicy(char *policy){
	if(strlen(policy) > 4 || (strcmp(policy, "FIFO") && strcmp(policy, "RR") && strcmp(policy, "SJF") && strcmp(policy, "PSJF"))){
		return 1;
	}
	return 0;

}

int main(int argc, char **argv){
	char schedulePolicy[MAXPOLICY] = {};
	int n;
	
	scanf("%s", schedulePolicy);
	if(checkPolicy(schedulePolicy)){
		fprintf(stderr, "Undefine Policy name : %s\n", schedulePolicy);
		exit(0);
	}

	scanf("%d", &n);

	Process *process = (Process*)malloc(sizeof(Process) * n);
	for(int i = 0; i < n; i++){
		scanf("%s %d %d", process[i].name, &process[i].readyTime, &process[i].executionTime);
	}
	
	scheduling(process, schedulePolicy, n);

	return 0;
	

}
