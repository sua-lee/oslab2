#include "types.h"
#include "stat.h"
#include "user.h"

/* Possible states of a thread; */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4

typedef struct thread thread_t, *thread_p;
typedef struct mutex mutex_t, *mutex_p;

struct thread {
  int        sp;                /* saved stack pointer */
  char stack[STACK_SIZE];       /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
};
static thread_t all_thread[MAX_THREAD];
thread_p  current_thread;
thread_p  next_thread;
extern void thread_switch(void);
void thread_schedule(void);
void uthread_init(void (*func)());    // 커널에 user-level 스케줄러 주소를 등록하는 시스템콜

void thread_exit(void) {
  current_thread->state = FREE;
  thread_schedule();
  exit();  // trap fallback
}

void 
thread_schedule(void)
{
  thread_p t;
  int cur_index = current_thread - all_thread;

  next_thread = 0;
  for (int i = 1; i < MAX_THREAD; i++) {
    int idx = (cur_index + i) % MAX_THREAD;
    t = &all_thread[idx];

    if (idx == 0)  // main thread 제외
      continue;

    if (t->state == RUNNABLE) {
      next_thread = t;
      break;
    }
  }
  // 현재 스레드가 유일한 runnable인 경우, 자기 자신 다시 선택
  if (!next_thread && cur_index != 0 && current_thread->state == RUNNABLE) {
    next_thread = current_thread;
  }

  if (!next_thread) {
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
}

void 
thread_create(void (*func)())
{
  thread_p t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }

  if (t >= all_thread + MAX_THREAD) // 사용 가능한 스레드가 없으면 그냥 return
  return;

  t->sp = (int) (t->stack + STACK_SIZE);   // set sp to the top of the stack
  t->sp -= 4;                              // space for return address
  * (int *) (t->sp) = (int)func;           // push return address on stack
  t->sp -= 32;                             // space for registers that thread_switch expects
  t->state = RUNNABLE;
  uthread_count(1);
}

static void 
mythread(void)
{
  int i;
  printf(1, "my thread running\n");
  for (i = 0; i < 100; i++) {
    printf(1, "my thread 0x%x\n", (int) current_thread);
    for (volatile int j = 0; j < 10000000; j++);  // 지연시키기 위해
  }
  printf(1, "my thread: exit\n");
  uthread_count(-1);
  thread_exit();
}


int 
main(int argc, char *argv[]) 
{
  thread_init();
  thread_create(mythread);
  thread_create(mythread);
  thread_schedule();
  exit();
}