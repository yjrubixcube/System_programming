#include "threadtools.h"

// Please complete this three functions. You may refer to the macro function defined in "threadtools.h"

// Mountain Climbing
// You are required to solve this function via iterative method, instead of recursion.
void MountainClimbing(int thread_id, int number){
	/* Please fill this code section. */
	ThreadInit(thread_id, number);
	Current->w=number;
	int state = setjmp(Current->Environment);
	if (state==0){
		longjmp(MAIN, 1);
	}
	int a=1,b=0,i=0;
	for (;i<=Current->w;i++){
		//printf("task2\n");
		sleep(1);
		b=a+b;
		a=b-a;

		Current->i=i;
		Current->x=a;
		Current->y=b;
		printf("Mountain Climbing: %d\n", b);
		//check signal
		ThreadYield();
		i=Current->i;
		a=Current->x;
		b=Current->y;
		
		
	}
	
	//ans=b;
	//printf("1ans=%d\n", b);
	ThreadExit();
}

// Reduce Integer
// You are required to solve this function via iterative method, instead of recursion.
void ReduceInteger(int thread_id, int number){
	/* Please fill this code section. */
	ThreadInit(thread_id, number);
	Current->w=number;
	int state = setjmp(Current->Environment);
	if (state==0){
		longjmp(MAIN, 2);
	}
	int rep=0, n=Current->w;
	while (1){
		//printf("task1\n");
		sleep(1);
		printf("Reduce Integer: %d\n", rep);
		if (n==1) break;
		rep++;
		if (n==3){
			n-=1;
		}
		else if (n%2){
			if ((n+1)%4){
				n-=1;
			}
			else{
				n+=1;
			}
		}
		else{
			n/=2;
		}
		Current->i=rep;
		Current->x=n;
		
		//check signal
		ThreadYield();
		rep=Current->i;
		n=Current->x;
		
		
		
	}
	//printf("2ans=%d\n", rep);
	ThreadExit();
}

// Operation Count
// You are required to solve this function via iterative method, instead of recursion.
void OperationCount(int thread_id, int number){
	/* Please fill this code section. */
	//0,2,4,6,8,2n;
	ThreadInit(thread_id, number);
	//printf("task3-1\n");
	sleep(1);
	int state = setjmp(Current->Environment);
	if (state==0){
		Current->x=number;
		Current=Current->Next;
		longjmp(MAIN, 3);
	}
	//printf("task3-2\n");
	int n = Current->x;
	int res;
	if (n%2){
		res=(n-1)*(n+1)/4;
	}
	else{
		res=n*n/4;
	}
	Current->i=res;
	sleep(1);
	printf("Operation Count: %d\n", Current->i);
	//check signal
	ThreadYield();
	ThreadExit();
}
