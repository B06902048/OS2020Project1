#include <sys/types.h>


#define timeUnit() for(volatile long long i = 0; i < 1000000UL; i++);
	
#define MAXNAME 32
typedef struct process{
	char name[MAXNAME];
	int readyTime, executionTime;
	pid_t pid;
} Process;

void scheduling(Process *process, char *policy, int n);
int assignCPU();
int blockProcess();
int wakeupProcess();
