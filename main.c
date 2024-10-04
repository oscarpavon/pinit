#include <pthread.h>
#define _XOPEN_SOURCE 200809L
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mount.h>

#define TIMEO	30

sigset_t set_of_signals;

static char * const mount_proc_command[] = {"/bin/mount",
  "-o", "nosuid,noexec,nodev", "-t", "proc", "proc", "/proc",NULL};
static char * const mount_sys_command[] = {"/bin/mount",
  "-o", "nosuid,noexec,nodev", "-t", "sysfs", "sys", "/sys",NULL};
static char * const mount_dev_command[] = {"/bin/mount",
  "-o", "mode=0775,nosuid", "-t", "devtmpfs", "dev", "/dev",NULL};

static char* const ln_stdin_command[] = {"/bin/ln","-sf", "/proc/self/fd/0","/dev/stdin",NULL};
static char* const ln_stdout_command[] = {"/bin/ln","-sf", "/proc/self/fd/1","/dev/stdout",NULL};
static char* const ln_stderr_command[] = {"/bin/ln","-sf", "/proc/self/fd/2","/dev/stderr",NULL};
static char* const ln_fd_command[] = {"/bin/ln","-sf", "/proc/self/fd","/dev/fd",NULL};


static char * const agetty_command[] = {"/sbin/agetty","--noclear", "--autologin", "root",
  "tty1", "9600",NULL};

static char * const pts_command[] = {"/bin/mount","-n", "-t" , "devpts", 
  "-o", "gid=5,mode=0620", "devpts", "/dev/pts", NULL};



void wait_signal_for_close(int pid){
    
	  waitpid(pid, NULL, WNOHANG);
    pthread_exit(0);
    
}

void* execute_thread_command(void*command_line){
  char* const* command = (char* const *)(command_line);
  int pid; 
  if((pid = fork())){
    wait_signal_for_close(pid);
  }else{
		setsid();
		execvp(*command,command_line);
  } 
  return NULL;
}

static void launch_agetty(){
  int result;

  if(fork() == 0){
		sigprocmask(SIG_UNBLOCK, &set_of_signals, NULL);
		setsid();
		result = execvp(agetty_command[0], agetty_command);
    if(result == -1){
      printf("Can't execvp\n");
    }
		perror("execvp");
  }
}


void* mount_proc(void*){
  int pid; 
  if((pid = fork())){
    wait_signal_for_close(pid);
  }else{
		setsid();
		execvp(mount_proc_command[0],mount_proc_command );
  } 
  return NULL;
}

void* mount_sys(void*){
  int pid; 
  if((pid = fork())){
    wait_signal_for_close(pid);
  }else{
		setsid();
		execvp(mount_sys_command[0],mount_sys_command );
  }  
  return NULL;
}

void* mount_dev(void*){
  
  
  mount("dev", "/dev", "devtmpfs", MS_NOSUID,NULL);
  printf("/dev mounted\n");
  return NULL;
}


static void signal_reap(void)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
	alarm(TIMEO);
}

int main(){
  printf("Pavon Init\n");
  
  if(getpid() != 1){
    printf("Need to be PID 1\n");
    _exit(1);
  }

  chdir("/");

  sigfillset(&set_of_signals);
  sigprocmask(SIG_BLOCK, &set_of_signals, NULL);
  
  pthread_t mount_thread;
  pthread_t mount_dev_thread;

  pthread_create(&mount_thread, NULL , mount_proc, NULL) ;

  pthread_create(&mount_thread, NULL , mount_sys, NULL) ;

  pthread_create(&mount_dev_thread, NULL , mount_dev, NULL) ;
 // mount_dev(NULL);
  pthread_join(mount_dev_thread,NULL);
  

  int mkdir_status;
  mkdir_status = mkdir("/dev/pts", S_IRWXU | S_IRWXG | S_IRWXO);
  if(mkdir_status == -1){
    printf("Error creating /dev/pts\n");
  }

  pthread_create(&mount_thread, NULL , execute_thread_command, ln_stdin_command) ;
  pthread_create(&mount_thread, NULL , execute_thread_command, ln_stdout_command) ;
  pthread_create(&mount_thread, NULL , execute_thread_command, ln_stderr_command) ;
  
  pthread_create(&mount_thread, NULL , execute_thread_command, ln_fd_command) ;

  pthread_create(&mount_thread, NULL , execute_thread_command, pts_command) ;

  launch_agetty();
  
  int signal;
  while(1){
    alarm(30) ;
    sigwait(&set_of_signals,&signal);
    
    if(signal == SIGCHLD || signal == SIGALRM){
      signal_reap();
    }
    
  }

  return 0;
}
