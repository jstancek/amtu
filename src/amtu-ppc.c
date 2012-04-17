//----------------------------------------------------------------------
//
// Module Name:  amtu-ppc.c
//
// Include File:  none
//
// Description:   Code for Abstract Machine Test POWER/PowerPC Privilege test.
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
// 8/25/03  K.Simon   Added NO_TAG to LAUS_LOG
// 8/26/03  K.Simon   Added printf to display test name
// 8/27/03  K.Simon   Modified ifdef statement
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

#if defined(HAVE_PPC) || defined(HAVE_PPC64)

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
/*          legitimately be run in user mode.                            */
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
			printf("MFMSR test: ");
		}
		/* move from machine state register to general */
		/* purpose register 4                          */
		__asm__ ("MFMSR 4\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (MFMSR)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on MFMSR"))
#else
		AUDIT_LOG("amtu failed privilege separation on MFMSR", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on MFMSR!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on MFMSR"))
#else
		AUDIT_LOG("amtu failed privilege separation on MFMSR", 0)
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
			printf("TLBSYNC test: ");
		}
		__asm__ ("TLBSYNC\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (TLBSYNC)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on TLBSYNC"))
#else
		AUDIT_LOG("amtu failed privilege separation on TLBSYNC", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on TLBSYNC!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on TLBSYNC"))
#else
		AUDIT_LOG("amtu failed privilege separation on TLBSYNC", 0)
#endif
		return(-1);
	}

	/*------------------------*/
	/* Third Instruction Test */
	/*------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("MFSPR test: ");
		}
		/* Move from special purpose register TID to GPR 6 */
		__asm__ ("MFSPR 6,17\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on MFSPR"))
#else
		AUDIT_LOG("amtu failed privilege separation on MFSPR", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
        /*
         * ignore this test, see:
         * Bug 797123 - ppc: Privilege Separation Test FAILED on MFSPR!
         *
         * Following commit introduced emulation of mfspr rD, DSCR:
         * commit efcac6589a277c10060e4be44b9455cf43838dc1
         * Author: Alexey Kardashevskiy <aik@au1.ibm.com>
         * Date:   Wed Mar 2 15:18:48 2011 +0000
         *     powerpc: Per process DSCR + some fixes (try#4)
         */

	/*------------------------*/
	/* Forth Instruction Test */
	/*------------------------*/
	pid = fork();
	if (pid == 0) {
		if (debug) {
			printf("MFSPR (Interrupt) test: ");
		}
		/* Move from special purpose register DSISR to GPR 6 */
		/* DSISR is the data storage interrupt status register */
		__asm__ ("MFSPR 6,18\n\t");
		exit(-1);
	} else if (pid == -1) {
		/* error condition */
		fprintf(stderr, "Privilege Separation Test FAILED (MFSPR-DSISR)!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on MFSPR (interrupt)"))
#else
		AUDIT_LOG("amtu failed privilege separation on MFSPR (interrupt)", 0)
#endif
		exit(-1);
	}
	/* parent */
	wpid = wait(&stat);
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Privilege Separation Test FAILED on MFSPR-DSISR!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed privilege separation on MFSPR (interrupt)"))
#else
		AUDIT_LOG("amtu failed privilege separation on MFSPR (interrupt)", 0)
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
