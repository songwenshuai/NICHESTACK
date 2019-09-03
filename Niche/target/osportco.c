/*
 * FILENAME: osportco.c
 *
 * Copyright  2004 By InterNiche Technologies Inc. All rights reserved
 *
 * Code to map NicheTask "TK_" macros and other port routines
 * to uCOS-II RTOS on ALTERA Cyclone board with the ALTERA Nios2 Core.
 *
 * This file for:
 *   ALTERA Cyclone Dev board with the ALTERA Nios2 Core.
 *   SMSC91C11 10/100 Ethernet Controller
 *   GNU C Compiler provided by ALTERA Quartus Toolset.
 *   Quartus HAL BSP
 *   uCOS-II RTOS Rel 2.76 as distributed by Altera/NIOS2
 *
 * MODULE  : Nios II
 * PORTABLE: no
 *
 * 06/21/2004
 * 10/21/2008 (skh)
 *
 */

/*
 * Altera Niche Stack Nios port modification:
 * Rearranged ipport.h and includes.h, using UCOS_II to not compile the file
 */
#include "ipport.h"        /* from Interniche directory */
#ifdef UCOS_II             /* Whole file is #ifdef'ed away if no uCOS-II */
#if 0
#include "includes.h"      /* from Nios2 uCOS-II directory */
#endif
#include "os_cpu.h"

#include "osport.h"

#include "in_utils.h"
#include "memwrap.h"

extern OsSemaphore mheap_sem_ptr;
extern OsSemaphore rcvdq_sem_ptr;
#ifdef OS_PREEMPTIVE
extern OsSemaphore resid_semaphore[MAX_RESID+1];
extern OsSemaphore app_semaphore[MAX_SEMID+1];
#endif
#if OS_TASK_CREATE_EXT_EN > 0
struct wake_event global_tcb_ext[OS_LOWEST_PRIO+1];
#endif

/* state of ints, saved by XXlock_net_res() */
int   netq_intmask;

extern int netmain(void);

int old_mode;

unsigned long cticks;

#ifndef TCPWAKE_RTOS
/*
 * Q and Mutex used by tcp_sleep/wakeup
 */
struct TCP_PendPost global_TCPwakeup_set[GLOBWAKE_SZ];
int global_TCPwakeup_setIndx;
u_long tcp_sleep_count = 0;
u_long tcp_wakeup_count = 0;
#endif


/*
 * Altera Niche Stack Nios port modification:
 * This seems to be a sample main() program. We provide these
 * in our (web server, simple socket serve) software examples,
 * and generally allow you to write your own main. Disabling
 */
#ifndef ALT_INICHE

INT8U app_priority = 13;    /* first/next priority to try for application */



/* Define C code main entry point.  */

int
main(void)
{
   INT8U mute_err;
   int priort;
   int i;

   iniche_net_ready = 0;
   
/* OSInit(); */
   OSTimeSet(0);

   /* create all of the various semaphores */
   mheap_sem_ptr = OSSemCreate(1);      /* npalloc() heap */
   if (!mheap_sem_ptr)
      panic("mheap_sem_ptr create err");

   rcvdq_sem_ptr = OSSemCreate(0);      /* RCVD queue "ready" */
   if (!rcvdq_sem_ptr)
      panic("rcvdq_sem_ptr create err"); 

#ifdef OS_PREEMPTIVE
   for (i = 0; i <= MAX_RESID; i++)
   {
      resid_semaphore[i] = OSSemCreate(1);
      if (!resid_semaphore[i])
         panic("resid_semaphore create err");  
   }
   for (i = 0; i <= MAX_SEMID; i++)
   {
      app_semaphore[i] = OSSemCreate(1);
      if (!app_semaphore[i])
         panic("app_semaphore create err");  
   }
#endif  /* OS_PREEMPTIVE */

#ifndef TCPWAKE_RTOS
   /* 
    * clear global_TCPwakeup_set
    */
   for (i = 0; i < GLOBWAKE_SZ; i++)
   {
      global_TCPwakeup_set[i].ctick = 0;
      global_TCPwakeup_set[i].soc_event = NULL;
      global_TCPwakeup_set[i].semaphore = OSSemCreate(0);
      if (!global_TCPwakeup_set[i].semaphore)
         panic("globwake_semaphore create err");  
   }
   global_TCPwakeup_setIndx = 0;

#endif /* TCPWAKE_RTOS */

   /* We have to lock scheduler while creating net tasks. Even though the RTOS
    * technically running yet, the ISR hooks to change tasks don't know this.
    * Since the UART uses interrupts heavly, we have to resort to this to
    * get console output during net_init.
    */
   OSLockNesting++;
   netmain();        /* Create net tasks */
   OSLockNesting--;

   dprintf("+++ uCOS init, app_priority = %d\n", app_priority);

   OSStart();        /* Jump to uCOS-II - Start system; never returns */
   panic("uCOS-II returned");

   return 0;
}


u_char
uCOS_self(void)
{
   /* Return uCOS currently running task ID */
   return(OSTCBCur->OSTCBPrio);
}
#endif /* if not defined: ALT_INICHE */


extern void irq_Mask(void);
extern void irq_Unmask(void);

void
LOCK_NET_RESOURCE(int resid)
{
   int status;
   int   errct = 0;

   if ((0 <= resid) && (resid <= MAX_RESID))
   {
      do
      {
         status = osWaitForSemaphore(&resid_semaphore[resid], INFINITE_DELAY);
         /* 
          * Sometimes we get a "timeout" error even though we passed a zero
          * to indicate we'll wait forever. When this happens, try again:
          */
         if ((!status) && (++errct > 1000))
         {
            panic("lock NET");   /* fatal */
            return;
         }
      } while (!status);
   }
   else
      dtrap();
}

void
UNLOCK_NET_RESOURCE(int resid)
{
   INT8U error = 0;

   if ((0 <= resid) && (resid <= MAX_RESID))
   {
      error = osReleaseSemaphore(&resid_semaphore[resid]);
      if (!error)
      {
         panic("unlock NET");
      }
   }
   else
      dtrap();
}


/* 
 * Altera Niche Stack Nios port modification:
 * TK_NEWTASK was significantly modified to remove the automatic
 * priority assignment scheme. A task created with TK_NEWTASK
 * should have its priority assigned in the inet_taskinfo
 * pointer passed in to TK_NEWTASK. Task priority and stack
 * size assignments for various Interniche features are specified
 * in ipport.h. For application tasks, you must assign your own.
 */

/*
 * Here's the workaround to an unusual quirk of uCOS-II - It
 * cannot have multiple tasks of the same priority. Networking systems
 * are best done with a few threads which have equal priority and thus 
 * ensure fair CPU sharing without convoluted priority balancing. This 
 * means we have to guess a priority for each of our net tasks and then
 * code tk_yield() to force a non-zero delay (one tick), to ensure that all
 * the network tasks get a round-robin slot to do their thing. Of course
 * we could alter the code's design to be optimized for a strictly
 * prioritized RTOS, but that would penalize all the non-preemptive and
 * non-prioritized systems we also support.
 */

extern TK_ENTRY(tk_netmain);        /* in netmain.c */
extern long     netmain_wakes;

#ifdef TK_STDIN_DEVICE
extern TK_ENTRY(tk_keyboard);       /* in netmain.c */
extern long     keyboard_wakes;
#endif

extern TK_ENTRY(tk_nettick);        /* in netmain.c */
extern long     nettick_wakes;


int 
TK_NEWTASK(struct inet_taskinfo * nettask)
{
   OsTask *task_ptr;


   task_ptr = osCreateTask( nettask->name,       /* 任务名称                                 */
                            nettask->entry,      /* 函数指针，void *pd为函数的参数           */
                            NULL,                /* 建立任务时，传递的参数                   */
                            nettask->stacksize,  /* 指定任务堆栈的大小，由OS_STK类型决定     */
                            nettask->priority    /* 任务优先级                               */
                           );
   if (task_ptr == NULL)
   {
      /* All other errors are fatal */
      TRACE_ERROR("Task create error on %s\n", nettask->name);
      return (-1);
   }

   nettask->tk_ptr = task_ptr;

  return (0);
}



#ifdef OS_PREEMPTIVE

/* FUNCTION: wait_app_sem()
 *
 * Wait on an application semaphore
 *
 * PARAM1: unsigned long      semaphore ID
 *
 * RETURN: none
 *
 * This function is invoked by the PING and FTP client tasks to block while
 * they wait for the corresponding application event to be signaled.
 * The application event is signaled by the console task, or the 
 * Telnet server task, or the timer task upon the occurance of a relevant
 * event (e.g., configuration inputs from user, initiation of a new session,
 * or a periodic timeout notification.
 */
void
wait_app_sem(long int semid)
{
   int   status;
   int   errct = 0;

   if ((0 <= semid) && (semid <= MAX_SEMID))
   {
      do
      {
         status = osWaitForSemaphore(&app_semaphore[semid], INFINITE_DELAY);
         /* 
          * Sometimes we get a "timeout" error even though we passed a zero
          * to indicate we'll wait forever. When this happens, try again:
          */
         if ((!status) && (++errct > 1000))
         {
            panic("lock NET");   /* fatal */
            return;
         }
      } while (!status);
   }
   else
      dtrap();
}



/* FUNCTION: post_app_sem()
 *
 * Post an application semaphore
 *
 * PARAM1: unsigned long      semaphore ID
 *
 * RETURN: none
 *
 * This function is invoked by the console task, or the Telnet server task,
 * or the timer task upon the occurence of an event such as a configuration
 * input from user, initiation of a new session, or a periodic timeout
 * notification. It signals the corresponding application event.
 */

void
post_app_sem(long int semid)
{
   INT8U error;

   if ((0 <= semid) && (semid <= MAX_SEMID))
   {
      error = osReleaseSemaphore(&app_semaphore[semid]);
      if (!error)
      {
         panic("unlock NET");
      }
   }
   else
      dtrap();
}

#endif   /* OS_PREEMPTIVE */



/* Rest of file is for TEST Purposes Only and is excluded unless we're
 * doing lock_net checking.
 */

#ifdef LOCKNET_CHECKING

#include "q.h"       /* InterNiche queue defines */

/* locally define external items involved in checking locks */
extern queue rcvdq;
extern queue bigfreeq;
extern queue lilfreeq;
extern int iniche_net_ready;
extern queue mfreeq;

extern int irq_level;  /* level of nesting in irq_Mask() and irq_Unmask() */

/* FUNCTION: LOCKNET_CHECK()
 *
 * If the passed queue is one of the sensitive ones then make sure
 * the related resource is locked.
 *
 * PARAM1: queue * q
 *
 * RETURNS: nothing, panics if test fails
 */

void
LOCKNET_CHECK(struct queue * q)
{

   /* Don't start checking until the net is up */
   if(!iniche_net_ready)
      return;

#ifdef INCLUDE_TCP
#ifndef MINI_TCP
   /* On uCOS/Nichestack/TCP builds, check the mbuf free queue is locked
    * by the NET_RESID semaphore. If it's not we panic().
    */
   if(q == &mfreeq)
   {
      /* A non-zero from OSSemAccept() means the semaphore was NOT locked */
      if (OSSemAccept(resid_semaphore[NET_RESID].p) != 0)
      {
         panic("locknet_check1");
      }
   }
#endif /* not MINI_TCP */
#endif /* INCLUDE_TCP */

   /* quick return if it's not one of the other sensitive queues */
   if((q != &rcvdq) &&
      (q != &bigfreeq) &&
      (q != &lilfreeq))
   {
      return;
   }

   /* Since the Nios2 build uses interrupt disabling to protect these
    * queues, we just need to check the Interrupt state. We use look at
    * the irq_level to get the current interrupt state, if interrupts
    * were not then we are going to panic().
    */

   if(irq_level != 1)    /* Get current interupt state */
   {
      panic("locknet_check2");
   }
   return;
}

#endif /* LOCKNET_CHECKING */

#endif /* UCOS_II */

