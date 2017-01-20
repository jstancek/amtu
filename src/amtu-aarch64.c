#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include "amtu.h"

#if defined(HAVE_AARCH64)

/************************************************************************/
/*                                                                      */
/* FUNCTION: catchfault                                                 */
/*                                                                      */
/* PURPOSE: Signal handler to catch the segmentation violation which is */
/*          expected when trying to execute privileged instructions     */
/*          without privilege.                                          */
/*                                                                      */
/************************************************************************/
void catchfault(int sig)
{
	if (debug) {
		printf("caught the fault %d\n", sig);
	}
	exit(0);
}


/************************************************************************/
/*                                                                      */
/* FUNCTION: amtu_priv                                                  */
/*                                                                      */
/* PURPOSE: Execute privileged instructions to ensure that they cannot  */
/*	    legitimately be run in user mode.                           */
/*                                                                      */
/************************************************************************/
int amtu_priv(int argc, char *argv[])
{
	struct sigaction sig;
	pid_t pid;
	int stat;

	printf("Executing Supervisor Mode Instructions Test...\n");

	/* Set up signal handler */
	sig.sa_handler = catchfault;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGSEGV, &sig, NULL);
	sigaction(SIGILL, &sig, NULL);
	sigaction(SIGIOT, &sig, NULL);
	sigaction(SIGIO, &sig, NULL);
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGABRT, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL);
	sigaction(SIGQUIT, &sig, NULL);
	sigaction(SIGBUS, &sig, NULL);

	/* Each assembly directive should seg fault since they are */
	/* privileged instructions.                                */
	/* RDPMC can be used in user space if correct flags are    */
	/* set but Linux does not set those flags.                 */
	/* See Chapter 4 of Intel's Systems Programming Guide      */

	/*---------------------------------------------------------*/
	/* Test One                                                */
	/* Try to halt the CPU.                                    */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		/* child */
		if (debug) {
			printf("HLT test: ");
		}
		__asm__ ("HLT 1\n\t");
		return(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (HLT)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on HLT"))
#else
		AUDIT_LOG("amtu failed privilege separation on HLT", 0)
#endif
		exit(-1);
	}
	/* parent */
	(void) wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on HLT!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on HLT"))
#else
		AUDIT_LOG("amtu failed privilege separation on HLT", 0)
#endif
		return(-1);
	}

	printf("Privileged Instruction Test SUCCESS!\n");
	return(0);
}
#endif
