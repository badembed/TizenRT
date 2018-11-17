/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * kernel/init/os_start.c
 *
 *   Copyright (C) 2007-2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include  <sys/types.h>
#include  <stdbool.h>
#include  <string.h>
#include  <assert.h>
#include  <debug.h>

#include  <tinyara/arch.h>
#include  <tinyara/compiler.h>
#include  <tinyara/sched.h>
#include  <tinyara/fs/fs.h>
#include  <tinyara/net/net.h>
#include  <tinyara/lib.h>
#include  <tinyara/mm/mm.h>
#include  <tinyara/mm/shm.h>
#include  <tinyara/kmalloc.h>
#include  <tinyara/init.h>

#include  "sched/sched.h"
#include  "signal/signal.h"
#include  "wdog/wdog.h"
#include  "semaphore/semaphore.h"
#ifndef CONFIG_DISABLE_MQUEUE
#include "mqueue/mqueue.h"
#endif
#ifndef CONFIG_DISABLE_PTHREAD
#include "pthread/pthread.h"
#endif
#include  "clock/clock.h"
#include  "timer/timer.h"
#include  "irq/irq.h"
#ifdef HAVE_TASK_GROUP
#include  "group/group.h"
#endif
#include  "init/init.h"
#ifdef CONFIG_DEBUG_SYSTEM
#include <tinyara/debug/sysdbg.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/* Task Lists ***************************************************************/
/* The state of a task is indicated both by the task_state field of the TCB
 * and by a series of task lists.  All of these tasks lists are declared
 * below. Although it is not always necessary, most of these lists are
 * prioritized so that common list handling logic can be used (only the
 * g_readytorun, the g_pendingtasks, and the g_waitingforsemaphore lists need
 * to be prioritized).
 */

/* This is the list of all tasks that are ready to run.  The head of this
 * list is the currently active task; the tail of this list is always the
 * IDLE task.
 */

volatile dq_queue_t g_readytorun;

/* This is the list of all tasks that are ready-to-run, but cannot be placed
 * in the g_readytorun list because:  (1) They are higher priority than the
 * currently active task at the head of the g_readytorun list, and (2) the
 * currently active task has disabled pre-emption.
 */

volatile dq_queue_t g_pendingtasks;

/* This is the list of all tasks that are blocked waiting for a semaphore */

volatile dq_queue_t g_waitingforsemaphore;

/* This is the list of all tasks that are blocked waiting for a signal */

#ifndef CONFIG_DISABLE_SIGNALS
volatile dq_queue_t g_waitingforsignal;
#endif

/* This is the list of all tasks that are blocked waiting for a message
 * queue to become non-empty.
 */

#ifndef CONFIG_DISABLE_MQUEUE
volatile dq_queue_t g_waitingformqnotempty;
#endif

/* This is the list of all tasks that are blocked waiting for a message
 * queue to become non-full.
 */

#ifndef CONFIG_DISABLE_MQUEUE
volatile dq_queue_t g_waitingformqnotfull;
#endif

/* This is the list of all tasks that are blocking waiting for a page fill */

#ifdef CONFIG_PAGING
volatile dq_queue_t g_waitingforfill;
#endif

/* This the list of all tasks that have been initialized, but not yet
 * activated. NOTE:  This is the only list that is not prioritized.
 */

volatile dq_queue_t g_inactivetasks;

/* These are lists of dayed memory deallocations that need to be handled
 * within the IDLE loop or worker thread.  These deallocations get queued
 * by sched_kufree and sched_kfree() if the OS needs to deallocate memory
 * while it is within an interrupt handler.
 */

volatile sq_queue_t g_delayed_kufree;

#if (defined(CONFIG_BUILD_PROTECTED) || defined(CONFIG_BUILD_KERNEL)) && \
	 defined(CONFIG_MM_KERNEL_HEAP)
volatile sq_queue_t g_delayed_kfree;
#endif

/* This gives number of alive tasks at any point of time in the system.
 * If the system is already running CONFIG_MAX_TASKS, Creating new
 * task is not supported.
 */
volatile uint8_t g_alive_taskcount;

/* This is the value of the last process ID assigned to a task */

volatile pid_t g_lastpid;

/* The following hash table is used for two things:
 *
 * 1. This hash table greatly speeds the determination of
 *    a new unique process ID for a task, and
 * 2. Is used to quickly map a process ID into a TCB.
 * It has the side effects of using more memory and limiting
 *
 * the number of tasks to CONFIG_MAX_TASKS.
 */

struct pidhash_s g_pidhash[CONFIG_MAX_TASKS];

/* This is a table of task lists.  This table is indexed by
 * the task state enumeration type (tstate_t) and provides
 * a pointer to the associated static task list (if there
 * is one) as well as a boolean indication as to if the list
 * is an ordered list or not.
 */

const struct tasklist_s g_tasklisttable[NUM_TASK_STATES] = {
	{NULL,                    false},	/* TSTATE_TASK_INVALID */
	{&g_pendingtasks,         true },	/* TSTATE_TASK_PENDING */
	{&g_readytorun,           true },	/* TSTATE_TASK_READYTORUN */
	{&g_readytorun,           true },	/* TSTATE_TASK_RUNNING */
	{&g_inactivetasks,        false},	/* TSTATE_TASK_INACTIVE */
	{&g_waitingforsemaphore,  true }	/* TSTATE_WAIT_SEM */
#ifndef CONFIG_DISABLE_SIGNALS
	,
	{&g_waitingforsignal,     false}	/* TSTATE_WAIT_SIG */
#endif
#ifndef CONFIG_DISABLE_MQUEUE
	,
	{&g_waitingformqnotempty, true },	/* TSTATE_WAIT_MQNOTEMPTY */
	{&g_waitingformqnotfull,  true }	/* TSTATE_WAIT_MQNOTFULL */
#endif
#ifdef CONFIG_PAGING
	,
	{&g_waitingforfill,       true }	/* TSTATE_WAIT_PAGEFILL */
#endif
};

/****************************************************************************
 * Private Variables
 ****************************************************************************/
/* This is the task control block for this thread of execution. This thread
 * of execution is the IDLE task.  NOTE:  the system boots into the IDLE
 * task.  The IDLE task spawns the user initialization task and that user
 * initialization task is responsible for bringing up the rest of the system.
 */

static FAR struct task_tcb_s g_idletcb;

/* This is the name of the idle task */

static FAR const char g_idlename[] = "Idle Task";

/* This the IDLE idle threads argument list. */

static FAR char *g_idleargv[2];

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

struct task_tcb_s tcb_runner;
struct task_tcb_s tcb_t1;
struct task_tcb_s tcb_t2;
char stack_runner[4096];
char stack_t1[4096];
char stack_t2[4096];
sem_t *sem;
void test_func_1(void);
void test_func_2(void);
void task_runner(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: os_start
 *
 * Description:
 *   This function is called to initialize the operating system and to spawn
 *   the user initialization thread of execution.  This is the initial entry
 *   point into TinyAra
 *
 * Input Parameters:
 *   None
 *
 * Returned value:
 *   Does not return.
 *
 ****************************************************************************/

void os_start(void)
{
	int i;

	/* Initialize RTOS Data ************************************************** */
	/* Initialize all task lists */

	dq_init(&g_readytorun);
	dq_init(&g_pendingtasks);
	dq_init(&g_waitingforsemaphore);
	dq_init(&g_inactivetasks);
	sq_init(&g_delayed_kufree);

	/* Initialize the logic that determine unique process IDs. */

	g_lastpid = 0;
	for (i = 0; i < CONFIG_MAX_TASKS; i++) {
		g_pidhash[i].tcb = NULL;
		g_pidhash[i].pid = INVALID_PROCESS_ID;
	}

	/* Assign the process ID of ZERO to the idle task */

	g_pidhash[PIDHASH(0)].tcb = &g_idletcb.cmn;
	g_pidhash[PIDHASH(0)].pid = 0;

	/* Increment the g_alive_taskcount as first task "idle task" is created */
	g_alive_taskcount++;

	/* Initialize the IDLE task TCB *******************************************/
	/* Initialize a TCB for this thread of execution.  NOTE:  The default
	 * value for most components of the g_idletcb are zero.  The entire
	 * structure is set to zero.  Then only the (potentially) non-zero
	 * elements are initialized. NOTE:  The idle task is the only task in
	 * that has pid == 0 and sched_priority == 0.
	 */

	bzero((void *)&g_idletcb, sizeof(struct task_tcb_s));
	g_idletcb.cmn.task_state = TSTATE_TASK_RUNNING;
	g_idletcb.cmn.entry.main = (main_t)os_start;
	g_idletcb.cmn.flags = TCB_FLAG_TTYPE_KERNEL;

	/* Configure the task name in the argument list.  The IDLE task does
	 * not really have an argument list, but this name is still useful
	 * for things like the TASH PS command.
	 *
	 * In the kernel mode build, the arguments are saved on the task's stack
	 * and there is no support that yet.
	 */

	g_idleargv[0]  = (FAR char *)g_idlename;
	g_idleargv[1]  = NULL;
	g_idletcb.argv = g_idleargv;

	/* Then add the idle task's TCB to the head of the ready to run list */

	dq_addfirst((FAR dq_entry_t *)&g_idletcb, (FAR dq_queue_t *)&g_readytorun);

	/* Initialize the processor-specific portion of the TCB */

	up_initial_state(&g_idletcb.cmn);

	/* Initialize RTOS facilities *********************************************
	 * Initialize the semaphore facility.  This has to be done very early
	 * because many subsystems depend upon fully functional semaphores.
	 */

	sem_initialize();
	//irq_initialize();
	wd_initialize();
  clock_initialize();

	/* The processor specific details of running the operating system
	 * will be handled here.  Such things as setting up interrupt
	 * service routines and starting the clock are some of the things
	 * that are different for each  processor and hardware platform.
	 */

	up_initialize();
  sem_init(&sem, 0, 1);

  int ret = task_init(&tcb_runner, "task runner", 250, stack_runner,
                      4096, task_runner, NULL);
  if (ret == OK) {
      (void)task_activate(&tcb_runner);
  }

	/* The IDLE Loop **********************************************************/
	/* When control is return to this point, the system is idle. */

	for (;;) {
		/* Perform garbage collection (if it is not being done by the worker
		 * thread).  This cleans-up memory de-allocations that were queued
		 * because they could not be freed in that execution context (for
		 * example, if the memory was freed from an interrupt handler).
		 */

		/* Perform any processor-specific idle state operations */

		up_idle();
	}
}

void task_runner(void) {
    int ret = task_init(&tcb_t2, "task_2", 120, stack_t2, 4096, test_func_2, NULL);
    if (ret == OK) {
        (void)task_activate(&tcb_t2);
    }

    ret = task_init(&tcb_t1, "task_1", 110, stack_t1, 4096, test_func_1, NULL);
    if (ret == OK) {
        (void)task_activate(&tcb_t1);
    }
}

void test_func_2(void) {

    while(1) {
        sem_wait(&sem);
    }

    struct timespec timeout;
    struct timespec current;

    while(1) {
        clock_gettime(CLOCK_REALTIME, &current);
        timeout.tv_sec = 2 + current.tv_sec;
        timeout.tv_nsec = current.tv_nsec;
        int ret = sem_timedwait(&sem, &timeout);
        if (ret == OK) {
            sem_post(&sem);
        }
    }
}

void test_func_1(void) {

    while(1) {
        sem_post(&sem);
    }

    int x = 10000;
    while(1) {
        sem_wait(&sem);
        sched_yield();

        while(x > 0) {x--;};
        x = 1000;

        sem_post(&sem);
        sched_yield();
        while(x > 0) {x--;};
        x = 1000;
    }
}
