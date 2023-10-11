#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

extern int timeslice, switchmode;

typedef struct TCB_NODE *TCB_ptr;
typedef struct TCB_NODE{
    jmp_buf  Environment;
    int      Thread_id;
    TCB_ptr  Next;
    TCB_ptr  Prev;
    int i, N;
    int w, x, y, z;
} TCB;

extern jmp_buf MAIN, SCHEDULER;
extern TCB_ptr Head;
extern TCB_ptr Current;
extern TCB_ptr Work;
extern sigset_t base_mask, waiting_mask, tstp_mask, alrm_mask;

void sighandler(int signo);
void scheduler();

// Call function in the argument that is passed in
#define ThreadCreate(function, thread_id, number)                                         \
{                                                                                         \
	/* Please fill this code section. */												  \
    \
    int state = setjmp(MAIN); \
    if (state==0){ \
        /*printf("creating%d %d\n", thread_id, number); \
        fflush(stdout); */\
        function(thread_id, number); \
    } \
    \
}

// Build up TCB_NODE for each function, insert it into circular linked-list
#define ThreadInit(thread_id, number)                                                     \
{                                                                                         \
	/* Please fill this code section. */												  \
    if (thread_id==1){ \
        Head=(TCB_ptr)malloc(sizeof(TCB)); \
        Head->Thread_id=thread_id; \
        Head->Next=Head; \
        Head->Prev=Head; \
        Current=Head; \
    } \
    else{ \
        Work=(TCB_ptr)malloc(sizeof(TCB)); \
        Work->Thread_id=thread_id; \
        Work->Prev=Current; \
        Work->Next=Current->Next; \
        Current->Next->Prev=Work; \
        Current->Next=Work; \
        Current=Current->Next; \
    } \
}

// Call this while a thread is terminated
#define ThreadExit()                                                                      \
{                                                                                         \
	/* Please fill this code section. jumpto shce*/		\
    int last=0; \
    if (Current==Current->Next){ \
        Current=NULL; \
        longjmp(SCHEDULER, 7); \
    } \
    else{ \
        last = Current->Thread_id; \
        Current->Next->Prev=Current->Prev; \
        Current->Prev->Next=Current->Next; \
        Current=Current->Next; \
    } \
    longjmp(SCHEDULER, last); \
}

// Decided whether to "context switch" based on the switchmode argument passed in main.c
#define ThreadYield()                                                                     \
{                                                                                         \
	/* Please fill this code section. */												  \
    if (switchmode==0){ \
        state = setjmp(Current->Environment); \
        if (state==0){ \
            int last=Current->Thread_id; \
            Current=Current->Next; \
            /*printf("yeild %d\n", last); */\
            longjmp(SCHEDULER, last); \
        } \
    } \
    else{ \
        sigpending( &waiting_mask); \
        /*sigpending( &tstp_mask );*/ \
        if (sigismember (&waiting_mask, SIGTSTP)){ \
            /*printf("TSTP signal caught\n");*/ \
            /*sigprocmask(SIG_UNBLOCK, &tstp_mask, NULL);*/ \
            /*sigprocmask(SIG_BLOCK, &tstp_mask, NULL);*/ \
            state = setjmp(Current->Environment); \
            if (state==0){ \
                int last=Current->Thread_id; \
                Current=Current->Next; \
                /*printf("yeild %d\n", last); */\
                /*longjmp(SCHEDULER, last);*/ \
                sigprocmask(SIG_UNBLOCK, &tstp_mask, NULL); \
            } \
        } \
        else if (sigismember( &waiting_mask, SIGALRM)){ \
            /*alarm*/ \
            /*printf("ALRM signal caught!\n");*/ \
            /*sigprocmask(SIG_UNBLOCK, &alrm_mask, NULL);*/ \
            /*sigprocmask(SIG_BLOCK, &alrm_mask, NULL);*/ \
            /*alarm(timeslice);*/ \
            state = setjmp(Current->Environment); \
            if (state==0){ \
                int last=Current->Thread_id; \
                Current=Current->Next; \
                /*printf("yeild %d\n", last); */ \
                /*longjmp(SCHEDULER, last);*/ \
                sigprocmask(SIG_UNBLOCK, &alrm_mask, NULL); \
            } \
        } \
    } \
}
