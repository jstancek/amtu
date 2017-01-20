//----------------------------------------------------------------------
//
// Module Name:  amtu-i86.c
//
// Include File:  none
//
// Description:   Code for Abstract Machine Test i386 Privilege test.
//
// Notes:  This module performs the machine specific privilege tests
// 		to ensure that the underlying hardware is still enforcing
// 		the appropriate control mechanisms.
// -----------------------------------------------------------------
// LANGUAGE:     C
//
// (C) Copyright International Businesses Machine Corp. 2003,2005
// Licensed under the Common Public License v. 1.0
// -----------------------------------------------------------------
//
// Change Activity:
// DATE     PGMR      COMMENTS
// -------- --------- ----------------------
// 2/05/03  J.Young   Add new X86-64 instructions
// 7/20/03  EJR       Added prolog, comments
// 8/19/03  EJR       Version # on CPL + comment stanzas for functions
// 8/25/03  K.Simon   Added NO_TAG to LAUS_LOG
// 8/26/03  K.Simon   Added printf to display test name
// 10/17/03 K.Simon   Removed NO_TAG
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
#include <sys/wait.h>
#include <syslog.h>
#include "amtu.h"

#if defined(HAVE_I86) || defined(HAVE_X86_64)

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
	pid_t pid, wpid;
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
		__asm__ ("HLT\n\t");
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
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on HLT!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on HLT"))
#else
		AUDIT_LOG("amtu failed privilege separation on HLT", 0)
#endif
		return(-1);
	}
	/*---------------------------------------------------------*/
	/* Test Two                                                */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		/* child */
		if (debug) {
			printf("RDPMC test: ");
		}
		__asm__ ("RDPMC\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (RDPMC)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on RDPMC"))
#else
		AUDIT_LOG("amtu failed privilege separation on RDPMC", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on RDPMC!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on RDPMC"))
#else
		AUDIT_LOG("amtu failed privilege separation on RDPMC", 0)
#endif
		return(-1);
	}
	/*---------------------------------------------------------*/
	/* Test Three                                              */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("CLTS test: ");
		}
		__asm__ ("CLTS\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (CLTS)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on CLTS"))
#else
		AUDIT_LOG("amtu failed privilege separation on CLTS", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on CLTS!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on CLTS"))
#else
		AUDIT_LOG("amtu failed privilege separation on CLTS", 0)
#endif
		return(-1);
	}
	/*---------------------------------------------------------*/
	/* Test Four                                               */
	/* Load Global Descriptor Table Test                       */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("LGDT test: ");
		}
		__asm__ ("SGDT 4\n\t");
		__asm__ ("LGDT 4\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege SeparationTest FAILED (LGDT)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LGDT"))
#else
		AUDIT_LOG("amtu failed privilege separation on LGDT", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on LGDT!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LGDT"))
#else
		AUDIT_LOG("amtu failed privilege separation on LGDT", 0)
#endif
		return(-1);
	}
	/*---------------------------------------------------------*/
	/* Test Five                                               */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("LIDT test: ");
		}
		__asm__ ("SIDT 4\n\t");
		__asm__ ("LIDT 4\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (LIDT)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LIDT"))
#else
		AUDIT_LOG("amtu failed privilege separation on LIDT", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on LIDT!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LIDT"))
#else
		AUDIT_LOG("amtu failed privilege separation on LIDT", 0)
#endif
		return(-1);
	}
	/*---------------------------------------------------------*/
	/* Test Six                                                */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("LTR test: ");
		}
		__asm__ ("STR 4\n\t");
		__asm__ ("LTR 4\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (LTR)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LTR"))
#else
		AUDIT_LOG("amtu failed privilege separation on LTR", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on LTR!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LTR"))
#else
		AUDIT_LOG("amtu failed privilege separation on LTR", 0)
#endif
		return(-1);
	}

	/*---------------------------------------------------------*/
	/* Test Seven                                              */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("LLDT test: ");
		}
		__asm__ ("SLDT 4\n\t");
		__asm__ ("LLDT 4\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (LLDT)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LLDT"))
#else
		AUDIT_LOG("amtu failed privilege separation on LLDT", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on LLDT!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LLDT"))
#else
		AUDIT_LOG("amtu failed privilege separation on LLDT", 0)
#endif
		return(-1);
	}

#if 0
	/*---------------------------------------------------------*/
	/* Test Eight                                              */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("CPU Instruction test: ");
		}
		__asm__ ("MOVL %cs,28(%esp)\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf("Privilege Separation Test FAILED (CPU)!\n");
		LAUS_LOG(("amtu failed privilege separation on CPU Instruction test"))
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on CPU! %d\n", WEXITSTATUS(stat));
		LAUS_LOG(("amtu failed privilege separation on CPU Instruction test"))
		return(-1);
	}
#endif


/* X86-64 only instructions */
#if defined(HAVE_X86_64)
	/*---------------------------------------------------------*/
	/* Test Nine						   */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("LMSW test: ");
		}
		__asm__("LMSW 4\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED on (LMSW)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LMSW"))
#else
		AUDIT_LOG("amtu failed privilege separation on LMSW", 0)
#endif
		return(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on LMSW!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on LMSW"))
#else
		AUDIT_LOG("amtu failed privilege separation on LMSW", 0)
#endif
		return(-1);
	}

	/*---------------------------------------------------------*/
	/* Test Ten                                                */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("RDMSR test");
		}
		__asm__("RDMSR\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED on (RDMSR)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on RDMSR"))
#else
		AUDIT_LOG("amtu failed privilege separation on RDMSR", 0)
#endif
		return(-1);
	}
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on RDMSR!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on RDMSR"))
#else
		AUDIT_LOG("amtu failed privilege separation on RDMSR", 0)
#endif
		return(-1);
	}


	/*---------------------------------------------------------*/
	/* Test Elven                                              */
	/*---------------------------------------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("WRMSR test");
		}
		__asm__("WRMSR\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED on (WRMSR)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on WRMSR"))
#else
		AUDIT_LOG("amtu failed privilege separation on WRMSR", 0)
#endif
		return(-1);
	}
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on WRMSR!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on WRMSR"))
#else
		AUDIT_LOG("amtu failed privilege separation on WRMSR", 0)
#endif
		return(-1);
	}
#endif 

#ifdef HAVE_LIBLAUS
	LAUS_LOG(("amtu - Privileged Instruction Test succeeded"))
#else
	AUDIT_LOG("amtu - Privileged Instruction Test succeeded", 1)
#endif
	printf("Privileged Instruction Test SUCCESS!\n");
	return(0);
}
#endif
