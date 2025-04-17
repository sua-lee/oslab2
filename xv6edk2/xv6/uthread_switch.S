	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
		  # 1. 현재 스레드 레지스터 저장
  pushal                          # 총 32 바이트 (eax~edi)

  # 2. current_thread->sp = esp
  movl current_thread, %eax
  movl %esp, (%eax)

  # 3. next_thread->sp → esp 복원
  movl next_thread, %eax
  movl (%eax), %esp

  # 4. current_thread = next_thread
  movl %eax, current_thread

  # 5. 레지스터 복원
  popal

  # 6. next_thread 초기화
  movl $0, next_thread

  # 7. ret = func으로 점프 (func → thread_exit)

	ret    /* return to ra */
