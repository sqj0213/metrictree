#ifndef DAEMON_HEADER
#define DAEMON_HEADER

#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "inc.hpp"
static void signalFunc( int sigNum )
{
	if ( sigNum == SIGUSR1 )
	{
		BOOST_LOG_TRIVIAL(info)<< " pid:"<<getpid()<<" recive signal:SIGUSR1 and reload programe...";
	}
	else if ( sigNum == SIGUSR2 )
	{
		BOOST_LOG_TRIVIAL(info)<< " pid:"<<getpid()<<" recive signal:SIGUSR2 and restart programe...";
	}
	else if ( sigNum == SIGHUP )
	{
		BOOST_LOG_TRIVIAL(info)<< " pid:"<<getpid()<<" recive signal:SIGHUP and reload programe...";
	}
	else if ( sigNum == SIGTERM )
	{
		BOOST_LOG_TRIVIAL(info)<< " pid:"<<getpid()<<" recive signal:SIGTERM and exit programe...";
		stopApp();
	}
	else if ( sigNum == SIGKILL )
	{
		BOOST_LOG_TRIVIAL(info)<< " pid:"<<getpid()<<" recive signal:SIGKILL force exit...";
		stopApp();
	}
	else
	{
		BOOST_LOG_TRIVIAL(info)<< " pid:"<<getpid()<<" recive signal:"<<sigNum<<" igore signal...";
	}
}
 
void init_daemon()
{
    unsigned i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
 
    /*
     * Clear file creation mask.
     */
     umask(0);
 
     /*
      * Get maximum number of file descriptors.
      */
     if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        fprintf(stderr, "can't get file limit" );
        exit(1);
      }
 
     /*
      * Become a session leader to lose controllong TTY.
      */
     if ((pid = fork()) < 0) {
        fprintf(stderr, "can't fork");
        exit(1);
     } else if (pid > 0) {
        exit(0);
     }
 
     setsid();
 
    /*
     * Ensure future opens won't allocate controlling TTYs.
     */
    sa.sa_handler = signalFunc;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);  
    sigaction(SIGKILL, &sa, NULL);  
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        fprintf(stderr, "can't ignore SIGHUP");
        exit(1);
    }
 
    if ((pid = fork()) < 0) {
        fprintf(stderr, "can't fork" );
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }
 
    /*
     * Change the current working directory to the root so
     * we won't prevent file systems from being unmounted.
     */
    if (chdir("/") < 0) {
        fprintf(stderr, "can't change directoryto /");
        exit(1);
    }
 
    /*
     * Close all open file descriptors.
     */
     
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);
 
    /*
     * Attach file descriprots 0, 1 and 2 to /dev/null
     */
     
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
}
#endif //END DAEMON_HEADER
