#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include "opt-A2.h"
#include <machine/trapframe.h>
#include <synch.h>

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

/* Cause the current process to exit. The exit code exitcode is reported back to other process(es) via the waitpid() call. 
  The process id of the exiting process should not be reused until all processes interested in collecting the exit code with waitpid have done so. 
  (What "interested" means is intentionally left vague; you should design this.)

  _exit does not return.

*/

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;

  #if OPT_A2

   pid_t curpid = p->p_pid;
   // KASSERT(curpid != 0);

  lock_acquire(process_Pids[curpid]->p_exit_lock);

    process_Pids[curpid]->p_exitStatus = exitcode;
    process_Pids[curpid]->p_isExited = true;

    cv_broadcast(process_Pids[curpid]->p_exit_cv, process_Pids[curpid]->p_exit_lock);

  lock_release(process_Pids[curpid]->p_exit_lock);


  #else

    /* for now, just include this to keep the compiler from complaining about
    an unused variable */
    
    (void)exitcode;

  #endif



  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */

/* getpid returns the process id of the current process.
   getpid does not fail.
*/
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */

  #if OPT_A2

    *retval = curproc->p_pid;

  #else

    *retval = 1;

  #endif

  return(0);
}

/* stub handler for waitpid() system call                */

/*
  Wait for the process specified by pid to exit, and return an encoded exit status in the integer pointed to by status. 
  If that process has exited already, waitpid returns immediately. If that process does not exist, waitpid fails.
What it means for a process to move from "has exited already" to "does not exist", and when this occurs, is something you must decide.

If process P is "interested" in the exit code of process Q, process P should be able to find out that exit code by calling waitpid, 
even if Q exits somewhat before the time P calls waitpid. As described under _exit(), precisely what is meant by "interested" is up to you.

You might implement restrictions or requirements on who may wait for which processes, like Unix does. 
You might also add a system call for one process to express interest in another process's exit code. 
If you do this, be sure to write a man page for the system call, and discuss the rationale for your choices therein in your design document.

Note that in the absence of restrictions on who may wait for what, it is possible to set up situations that may result in deadlock. 
Your system must (of course) in some manner protect itself from these situations, either by prohibiting them or by detecting and resolving them.

In order to make the userlevel code that ships with OS/161 work, 
assume that a parent process is always interested in the exit codes of its child processes generated with fork(), 
unless it does something special to indicate otherwise.

The options argument should be 0. You are not required to implement any options. 
(However, your system should check to make sure that options you do not support are not requested.)

If you desire, you may implement the Unix option WNOHANG; this causes waitpid, 
when called for a process that has not yet exited, to return 0 immediately instead of waiting.

The Unix option WUNTRACED, to ask for reporting of processes that stop as well as exit, 
is also defined in the header files, but implementing this feature is not required or necessary unless you are implementing job control.

You may also make up your own options if you find them helpful. However, please, document anything you make up.

The encoding of the exit status is comparable to Unix and is defined by the flags found in <kern/wait.h>. 
(Userlevel code should include <sys/wait.h> to get these definitions.) 
A process can exit by calling _exit() or it can exit by receiving a fatal signal. 
In the former case the _MKWAIT_EXIT() macro should be used with the user-supplied exit code to prepare the exit status; 
in the latter, the _MKWAIT_SIG() macro (or _MKWAIT_CORE() if a core file was generated) should be used with the signal number. 
The result encoding also allows notification of processes that have stopped; 
this would be used in connection with job control and with ptrace-based debugging if you were to implement those things.

To read the wait status, use the macros WIFEXITED(), WIFSIGNALED(), and/or WIFSTOPPED() to find out what happened, 
and then WEXITSTATUS(), WTERMSIG(), or WSTOPSIG() respectively to get the exit code or signal number. 
If WIFSIGNALED() is true, WCOREDUMP() can be used to check if a core file was generated. 
This is the same as Unix, although the value encoding is different from the historic Unix format.


waitpid returns the process id whose exit status is reported in status. In OS/161, this is always the value of pid.
If you implement WNOHANG, and WNOHANG is given, and the process specified by pid has not yet exited, waitpid returns 0.

(In Unix, but not by default OS/161, you can wait for any of several processes by passing magic values of pid, so this return value can actually be useful.)

On error, -1 is returned, and errno is set to a suitable error code for the error condition encountered.


The following error codes should be returned under the conditions given. Other error codes may be returned for other errors not mentioned here.
   
EINVAL  The options argument requested invalid or unsupported options.
ECHILD  The pid argument named a process that the current process was not interested in or that has not yet exited.
ESRCH The pid argument named a nonexistent process.
EFAULT  The status argument was an invalid pointer.

*/

/** * you should ensure that a process can use waitpid to obtain the exit status of any of its children, 
      and that a process may not use waitpid to obtain the exit status of any other processes.
* **/

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  //This is the pid of the parent
  pid_t curpid = curproc->p_pid;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }

  #if OPT_A2

  if(!pid_checkexists(pid)) {
    return ESRCH;
  }

  if(process_Pids[pid]->p_parentPid != curpid) {
    return ECHILD;
  }

  
  lock_acquire(process_Pids[pid]->p_exit_lock);

    exitstatus = _MKWAIT_EXIT(process_Pids[pid]->p_exitStatus);

    if(!process_Pids[pid]->p_isExited){

      cv_wait(process_Pids[pid]->p_exit_cv, process_Pids[pid]->p_exit_lock);

    }

  lock_release(process_Pids[pid]->p_exit_lock);


  if(status == NULL) {
    return EFAULT;
  }

  #else

    /* for now, just pretend the exitstatus is 0 */
    exitstatus = 0;

  #endif
 
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}


/** * Need to implement this *  **/

/*
fork duplicates the currently running process. The two copies are identical, except that one (the "new" one, or "child"), has a new, unique process id, 
and in the other (the "parent") the process id is unchanged.
The process id must be greater than 0.

The two processes do not share memory or open file tables; this state is copied into the new process, and subsequent modification in one process does not affect the other.

However, the file handle objects the file tables point to are shared, so, for instance, calls to lseek in one process can affect the other.

On success, fork returns twice, once in the parent process and once in the child process. In the child process, 0 is returned. 
In the parent process, the process id of the new child process is returned.
On error, no new process is created, fork only returns once, returning -1, and errno is set according to the error encountered.

The following error codes should be returned under the conditions given. Other error codes may be returned for other errors not mentioned here.
   
EMPROC  The current user already has too many processes.
ENPROC  There are already too many processes on the system.
ENOMEM  Sufficient virtual memory for the new process was not available.

*/

// define the function to have 2 parameters a trap fram pointer and a pid_t* retval
//kmalloc the size of the trap frame
//copy the data of the trap frame pointer to some variable so that it can be passed to the child
//then create the process child, which is done with proc_create_runprogram
//then create an adress space pointer pointer
//then use as_copy which copies the parents address to some other address
//this as_copy returns an int which is possibly an error value
//then call thread_fork - first arg is function ptr (that includes the trap frame stucture which you declare)
//the second paramater is an address space pointer which you declared above
//change the register values of the trap frame to 0 because its a child
//also activate mips_usermode in this function


//Opted to use this function because of its code locality to sys_fork
//rather than use enter_forked_process()
void thread_fork_init(void * ptr, unsigned long val);
void thread_fork_init(void * ptr, unsigned long val) {
  (void)val;

  //Do I need to call curproc_setas somewhere?

  as_activate();

  KASSERT(ptr != NULL);
  struct trapframe newTf = *ptr; // this should be copied to our stack now.

  //free the trap frame pointer we allocated in sys_fork, it's our last chance to do so
  kfree(ptr);
 
  // this will set our return parameters (e.g. mips values for the tf).

  //Return 0 for success (see syscall)
  newTf.tf_v0 = 0;
  newTf.tf_a3 = 0;
  // Increment the PC
  newTf.tf_epc += 4; 


  /** * you will want to call mips usermode() (from locore/trap.c) to actually cause the switch from kernel mode to user mode.  * **/
  //Switch from kernel mode to user mode
  mips_usermode(&newTf);

  //mips_usermode doesn't return

}

pid_t sys_fork(struct trapframe *tf, pid_t *retval){

  struct trapframe *temporaryTrapFrame = kmalloc(sizeof(struct trapframe));

  if (temporaryTrapFrame == NULL){
    return ENOMEM;
  }

  //copied to the heap, we will need to copy it to the thread's stack later
  *temporaryTrapFrame = *tf;

  struct proc * child = proc_create_runprogram(curproc);

  if(child == NULL){
    kfree(temporaryTrapFrame);
    splx(spl);
    return ENOMEM;
  }

  int addressError = as_copy(curproc->p_addrspace, &child->p_addrspace);

    if (addressError) {
    // destroy proc
    proc_destroy(child);
    kfree(temporaryTrapFrame);
    return addressError;
  }

  int threadError = thread_fork(curthread->t_name, child, &thread_fork_init,
    (void*)temporaryTrapFrame, void);


  if (threadError) {
    // destroy proc
    proc_destroy(child);
    kfree(temporaryTrapFrame);
    return threadError;
  }

  //Modify retval to return the child's pid
  *retval = child->p_pid;

  //If we reach here, the child process returns 0 on success

  return(0);
}
