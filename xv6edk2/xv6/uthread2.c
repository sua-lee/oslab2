#include "types.h"
#include "stat.h"
#include "user.h"

/* Possible states of a thread; */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2
#define WAIT        0x3

#define STACK_SIZE  8192
#define MAX_THREAD  10

typedef struct thread thread_t, *thread_p;
typedef struct mutex mutex_t, *mutex_p;

struct thread {
  int        tid;    /* thread id */
  int        ptid;  /* parent thread id */
  int        sp;                /* saved stack pointer */
  char stack[STACK_SIZE];       /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE, WAIT */
};
static thread_t all_thread[MAX_THREAD];
thread_p  current_thread;
thread_p  next_thread;
extern void thread_switch(void);

static void 
thread_schedule(void)
{
  thread_p t;

  /* Find another runnable thread. */
  next_thread = 0;
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      break;
    }
  }

  if (t >= all_thread + MAX_THREAD && current_thread->state == RUNNABLE) {
    /* The current thread is the only runnable thread; run it. */
    next_thread = current_thread;
  }

  if (next_thread == 0) {
    printf(2, "thread_schedule: no runnable threads\n");
    exit();
  }

  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    current_thread->state = RUNNABLE;
    thread_switch();
  } else
    next_thread = 0;
}

void 
thread_init(void)
{
  uthread_init(thread_schedule);

  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
  current_thread->tid=0;
  current_thread->ptid=0;
}

void 
thread_create(void (*func)())
{
  thread_p t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->sp = (int) (t->stack + STACK_SIZE);   // set sp to the top of the stack
  t->sp -= 4;                              // space for return address
  //setting tid and ptid
  t->tid = t - all_thread;
  t->ptid = current_thread->tid; 

  * (int *) (t->sp) = (int)func;           // push return address on stack
  t->sp -= 32;                             // space for registers that thread_switch expects
  t->state = RUNNABLE;
}

static void 
thread_join_all(void)
{
  int has_child;

  while (1) {
    has_child = 0;

    for (int i = 0; i < MAX_THREAD; i++) {
      thread_p t = &all_thread[i];
      // 아직 살아 있는 자식 스레드가 있다면
      if (t->ptid == current_thread->tid && t->state != FREE) {
        has_child = 1;
        break;
      }
    }

    if (has_child) {
      // 자식이 살아있다면 현재 스레드를 WAIT으로 바꾸고 스케줄러에게 넘김
      current_thread->state = WAIT;
      thread_schedule();
    } else {
      // 자식이 모두 종료됨 → 더 이상 WAIT 필요 없음
      current_thread->state = RUNNABLE;
      break;
    }
  }

  // 마지막에 스케줄링 한번 더
  thread_schedule();
}

static void 
child_thread(void)
{
  int i;
  printf(1, "child thread running\n");
  for (i = 0; i < 100; i++) {
    printf(1, "child thread 0x%x\n", (int) current_thread);
  }
  printf(1, "child thread: exit\n");
  current_thread->state = FREE;
}

static void 
mythread(void)
{
  int i;
  printf(1, "my thread running\n");
  for (i = 0; i < 5; i++) {
    thread_create(child_thread);
  }
  thread_join_all();
  printf(1, "my thread: exit\n");
  current_thread->state = FREE;
}

int 
main(int argc, char *argv[]) 
{
  thread_init();
  thread_create(mythread);
  thread_join_all();
  return 0;
}