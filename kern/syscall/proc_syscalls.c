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
#include "opt-A2.h"
#include <synch.h>
#include <machine/trapframe.h>
#include <copyinout.h>

//A2b

#include <kern/fcntl.h>
#include <vfs.h>
#include <test.h>
#include <limits.h>

//A2b

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

    pid_setexitstatus(curpid, exitcode);
    pid_setisexited(curpid, true);
    
    struct semaphore *pid_sem = pid_getsem(curpid);

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

  #if OPT_A2

      V(pid_sem);

  #endif

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

  return 0;
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
    return EINVAL;
  }

  #if OPT_A2

  if(!pid_checkexists(pid)) {
    return ESRCH;
  }
  
  if(pid_getparentpid(pid) != curpid) {
    return ECHILD;
  }

    struct semaphore *pid_sem = pid_getsem(pid);

    P(pid_sem);

    exitstatus = _MKWAIT_EXIT(pid_getexitstatus(pid));

  if(status == NULL) {
    return EFAULT;
  }

  #else

    /* for now, just pretend the exitstatus is 0 */
    exitstatus = 0;

  #endif
 
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return result;
  }
  *retval = pid;
  return 0;
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

void thread_fork_init(void * ptr, unsigned long val);
void thread_fork_init(void * ptr, unsigned long val){
  //Stop the compiler from complaining
  (void)val;

  enter_forked_process((struct trapframe *) ptr);

}

pid_t sys_fork(struct trapframe *tf, pid_t *retval){

  //Used for error checking
  int result;

  struct trapframe *temporaryTrapFrame = kmalloc(sizeof(struct trapframe));

  if (temporaryTrapFrame == NULL){
    return ENOMEM;
  }

  //copied to the heap, we will need to copy it to the thread's stack later
  *temporaryTrapFrame = *tf;

  struct proc * child = proc_create_runprogram(curproc->p_name);

  if(child == NULL){
    kfree(temporaryTrapFrame);
    return ENOMEM;
  }

  //struct addrspace *curproc_curas = curproc_getas();

  result = as_copy(curproc->p_addrspace, &child->p_addrspace);

  if (result) {
    kfree(temporaryTrapFrame);
    proc_destroy(child);
    return ENOMEM;
  }

  //Assign parent to child

  pid_setparentpid(child->p_pid, curproc->p_pid);

  result = thread_fork(curthread->t_name, child, thread_fork_init,
    temporaryTrapFrame, 0);

  if (result) {
    as_destroy(child->p_addrspace);
    proc_destroy(child);
    kfree(temporaryTrapFrame);
    return result;
  }

  //Modify retval to return the child's pid
  *retval = child->p_pid;

  //If we reach here, the child process returns 0 on success

  return 0;
}

/*

Description

execv replaces the currently executing program with a newly loaded program image. This occurs within one process; the process id is unchanged.
The pathname of the program to run is passed as program. The args argument is an array of 0-terminated strings. The array itself should be terminated by a NULL pointer.

The argument strings should be copied into the new process as the new process's argv[] array. In the new process, argv[argc] must be NULL.

By convention, argv[0] in new processes contains the name that was used to invoke the program. This is not necessarily the same as program, and furthermore is only a convention and should not be enforced by the kernel.

The process file table and current working directory are not modified by execve.

Return Values

On success, execv does not return; instead, the new program begins executing. On failure, execv returns -1, and sets errno to a suitable error code for the error condition encountered.
Errors

The following error codes should be returned under the conditions given. Other error codes may be returned for other errors not mentioned here.
   
ENODEV  The device prefix of program did not exist.
ENOTDIR A non-final component of program was not a directory.
ENOENT  program did not exist.
EISDIR  program is a directory.
ENOEXEC program is not in a recognizable executable file format, was for the wrong platform, or contained invalid fields.
ENOMEM  Insufficient virtual memory is available.
E2BIG The total size of the argument strings is too large.
EIO A hard I/O error occurred.
EFAULT  One of the args is an invalid pointer.

*/

int execv(userptr_t program, userptr_t args){

(void)args;


//  char * path = kmalloc(PATH_MAX);

  //copyinstr(&program, path, PATH_MAX, NULL);
  
  //• Count the number of arguments and copy them into the kernel

  /*
 int argNum=0;
  while (args[argNum]!=NULL){
    argNum++;
  }

  */

  //• Copy the program path into the kernel

  struct addrspace *as;
  vaddr_t entrypoint, stackptr;
  int result;
  struct vnode *v;
 

  // • Open the program file using vfs_open(prog_name, …)

  result = vfs_open((char *)program, O_RDONLY, 0, &v);
  if (result) {
   return result;
  }

        // • Create new address space, set process to the new address space, and activate it
        as = as_create();
        if (as ==NULL) {
                vfs_close(v);
                return ENOMEM;
        }

        struct addrspace *oldAs=curproc_setas(as);
        as_activate();

        //destroy the old address space
        as_destroy(oldAs);
        
        //  • Using the opened program file, load the program image using load_elf
        result = load_elf(v, &entrypoint);
        if (result) {
                /* p_addrspace will go away when curproc is destroyed */
                vfs_close(v);
                return result;
        }

        /* Done with the file now. */
        vfs_close(v);

        // • Need to copy the arguments into the new address space. Consider copying the 
        //arguments (both the array and the strings) onto the user stack as part of 
        //as_define_stack.
/* Define the user stack in the address space */
        result = as_define_stack(as, &stackptr);
        if (result) {
                /* p_addrspace will go away when curproc is destroyed */
                return result;
        }
       
        //  • Delete old address space
        //as_destroy(as);  
 
        //• Call enter_new_process with address to the arguments on the stack, the stack 
        //pointer (from as_define_stack), and the program entry point (from vfs_open)
        /* Warp to user mode. */
        enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
                          stackptr, entrypoint);

        /* enter_new_process does not return. */
        panic("enter_new_process returned\n");
        return EINVAL;
  
 }
