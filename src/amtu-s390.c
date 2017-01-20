//----------------------------------------------------------------------
//
// Module Name:  amtu-s390.c
//
// Include File:  none
//
// Description:   Code for Abstract Machine Test S390 Privilege test.
//
// Notes:  This module performs the machine specific privilege tests
//              to ensure that the underlying hardware is still enforcing
//              the appropriate control mechanisms.
// -----------------------------------------------------------------
// LANGUAGE:     C
//
// (C) Copyright International Business Machines Corp. 2003,2005
// Licensed under the Common Public License v. 1.0
// -----------------------------------------------------------------
//
// Change Activity:
// DATE     PGMR      COMMENTS
// -------- --------- ----------------------
// 7/20/03  EJR       Added prolog, comments
// 8/11/03  KSimon    Added #include "amtu.h" and #include <syslog.h>
// 8/19/03  EJR       Version # on CPL + comment stanzas for functions
// 8/25/03  KSimon    Added NO_TAG to LAUS_LOG
// 8/26/03  KSimon    Added printf to display test name
// 10/17/03 KSimon    Removed NO_TAG
// 03/15/05 D.Velarde Added AUDIT_LOG statements to be used if we're
//                    using libaudit instead of liblaus
//----------------------------------------------------------------------

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <syslog.h>
#include <sys/wait.h>
#include "amtu.h"

#ifdef HAVE_S390

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
/*          legitimately be run in user mode.                           */
/*                                                                      */
/************************************************************************/
int amtu_priv(int argc, char *argv[])
{
	struct sigaction sig;
	pid_t pid, wpid;
	int stat;
	
	printf("Executing Supervisor Mode Instructions Test...\n");

	sig.sa_handler = catchfault;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGSEGV, &sig, NULL);
	sigaction(SIGILL, &sig, NULL);

	/* Each assembly directive should seg fault since they are */
	/* privileged instructions.                                */
	/* RDPMC can be used in user space if correct flags are    */
	/* set but Linux does not set those flags.                 */
	/* See Chapter 4 of Intel's Systems Programming Guide      */

	/*------------------------*/
	/* First Instruction Test */
	/*------------------------*/
	pid = fork();
	if (pid == 0) {
		/* child */
		if (debug) {
			printf("PTLB test: ");
		}
		__asm__ ("PTLB\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (PTLB)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on PTLB"))
#else
		AUDIT_LOG("amtu failed privilege separation on PTLB", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on PTLB!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on PTLB"))
#else
		AUDIT_LOG("amtu failed privilege separation on PTLB", 0)
#endif
		return(-1);
	}

	/*-------------------------*/
	/* Second Instruction Test */
	/*-------------------------*/
	pid = fork();
	if (pid == 0) {
		/* child */
		if (debug) {
			printf("HSCH test: ");
		}
		__asm__ ("HSCH\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (HSCH)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on HSCH"))
#else
		AUDIT_LOG("amtu failed privilege separation on HSCH", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on HSCH!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on HSCH"))
#else
		AUDIT_LOG("amtu failed privilege separation on HSCH", 0)
#endif
		return(-1);
	}

	/*------------------------*/
	/* Third Instruction Test */
	/*------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("PALB test: ");
		}
		__asm__ ("PALB\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (PALB)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on PALB"))
#else
		AUDIT_LOG("amtu failed privilege separation on PALB", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on PALB!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on PALB"))
#else
		AUDIT_LOG("amtu failed privilege separation on PALB", 0)
#endif
		return(-1);
	}

	/*------------------------*/
	/* Fourth Instruction Test */
	/*------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("RRBE test: ");
		}
		__asm__ ("RRBE 4,8\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (RRBE)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on RRBE"))
#else
		AUDIT_LOG("amtu failed privilege separation on RRBE", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on RRBE!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on RRBE"))
#else
		AUDIT_LOG("amtu failed privilege separation on RRBE", 0)
#endif
		return(-1);
	}

	/*------------------------*/
	/* Fifth Instruction Test */
	/*------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("EPAR test: ");
		}
		__asm__ ("EPAR 1\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (EPAR)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on EPAR"))
#else
		AUDIT_LOG("amtu failed privilege separation on EPAR", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on EPAR!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on EPAR"))
#else
		AUDIT_LOG("amtu failed privilege separation on EPAR", 0)
#endif
		return(-1);
	}


#ifdef HAVE_LIBLAUS
	LAUS_LOG(("amtu - Privileged Instruction Test succeeded"))
#else
	AUDIT_LOG("amtu - Privileged Instruction Test succeeded", 1)
#endif
	printf("Privileged Instruction Test SUCCESS!\n");
	return(0);
}
#endif
