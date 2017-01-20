//----------------------------------------------------------------------
//
// Module Name:  memsep.c
//
// Include File:  none
//
// Description:   Code for Abstract Machine Test Utility - Memory
//                Separation Test.
//
// Notes:  This module performs the memory separation test for the 
//         Abstract Machine Test Utility (AMTU).  The return codes are
//         as follows:
//         -1 = failure occurred
//          0 = success
// -----------------------------------------------------------------
// LANGUAGE:     C
//
// (C) Copyright International Businesses Machine Corp. 2003-2009
// Licensed under the Common Public License v. 1.0
// -----------------------------------------------------------------
//
// Change Activity:
// DATE     PGMR      COMMENTS
// ------- --------- ----------------------
// 07/29/03 K.Simon  Added debug messages 
//                   and laus logging
// 8/19/03  K.Simon  Version # on CPL + comment stanzas for functions,
//		     changed /proc/iomem to /proc/ksyms, aligned code
//		     with tabs, aligned code to 80 chars per line,
//		     changed printfs to fprintfs, changed *first to
//                   *mem_ptr, shorten comments
// 08/25/03 K.Simon  Added NO_TAG for LAUS_LOG,added LAUS_LOG success
//		     message
// 08/26/03 K.Simon  Added printf to display test name, moved success
//		     messages to end of file
// 08/27/03 K.Simon  Added ifdefs for architecture differences
// 10/16/03 K.Simon, Code revised per AMTU evaluation, removed NO_TAG
//	    J.Latten
// 10/21/03 K.Simon  For get_num_in range: Changed variable k to double, 
//		     removed "+ 1" for k assignment, revised comment 
//		     block. For write_read_mem: removed ifdefs, added
//		     cast to long for variable ptr, added fflush, 
//		     changed flag to write_flag. Changed all references
//		     to I/O Memory
//11/12/03 K.Weider  Patch for checking memory addresses reserved for 
//	             stack.
//03/15/05 D.Velarde Commented out switch to test user in memsep.
//                   On RHEL4 only root has read access for /proc/self/maps
//03/15/05 D.Velarde Added AUDIT_LOG statements to be used if we're
//                   using libaudit instead of liblaus
//03/16/05 D.Velarde Rather than commenting out switch, use #ifndef
//                   don't switch to user nobody if running on RHEL4
//12/08/05 D.Velarde Updated with memsep-random patch from RedHat
//12/08/05 D.Velarde Updated with new audit records patch from RedHat
//6/12/09  EJR	     Now also catch SIGBUS
//
//----------------------------------------------------------------------

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <syslog.h>
#include <sys/wait.h>
#include <pwd.h>
#include <time.h>
#include "amtu.h"

/************************************************************************/
/*                                                                      */
/* FUNCTION: sig_handler                                                */
/*                                                                      */
/* PURPOSE: Signal handler to catch the segmentation violation which is */
/*          expected when trying to read from or write to restricted    */
/*          memory (i.e. kernel memory).                                */
/*                                                                      */
/************************************************************************/
void sig_handler(int sig)
{
	if (debug) {
		fprintf(stderr, "caught the fault %d\n", sig);
	}
	exit(0);
}

/************************************************************************/
/*                                                                      */
/* FUNCTION: get_pointer_in_range                                       */
/*                                                                      */
/* PURPOSE: Returns a pointer to a random memory address in memory      */
/*          range start <= j < end, aligned to sizeof(int)              */
/*                                                                      */
/************************************************************************/
#if __LP64__
#define RANDNUM ((uint64_t)random() << 32 | random())
#else
#define RANDNUM random()
#endif
int *get_pointer_in_range(int *start, int *end)
{
        return (int *)((char *)start + (RANDNUM % ((char *)end - (char *)start)));
}

/************************************************************************/
/*                                                                      */
/* FUNCTION: write_read_mem                                             */
/*                                                                      */
/* PURPOSE: Attempts to read or write to memory addresses               */
/*                                                                      */
/************************************************************************/
void write_read_mem(int *ptr, int write_flag)
{
	int stat;
	struct sigaction sig;             
	pid_t pid, wpid;		 

	// Signal handling
	sig.sa_handler = sig_handler;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGSEGV, &sig, NULL);
	sigaction(SIGILL, &sig, NULL);
	sigaction(SIGBUS, &sig, NULL);

	// Try reading/writing to the memory addresses.
	fflush(stdout);
	pid = fork();   // Create child process
	if (pid == 0) {
		// Read or write to memory addresses
		if (!write_flag) {
			if (debug) {
				fprintf(stderr, "Reading Memory Address"
					" %p\n", ptr);
			}
			fprintf(stderr, "value of address: %d\n", *ptr);
		}
		else {
			if (debug) {
				fprintf(stderr, "Writing to Memory Address"
					" %p\n", ptr);
			}
			*ptr = rand();
		}
		exit(-1);
	}
	else if (pid == -1) {
		// Error condition 
		fprintf(stderr, "Memory Separation Test FAILED!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed memory separation test"))
#else
		AUDIT_LOG("amtu failed memory separation test", 0)
#endif
		exit(-1);
	}

	// Parent process
	wpid = wait(&stat);

	// Check if child process exited cleanly.
	// If not, report failure.
	if (!(WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))) {
		fprintf(stderr, "Memory Separation Test FAILED!\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed memory separation test"))
#else
		AUDIT_LOG("amtu failed memory separation test", 0)
#endif
		exit(-1);
	}
}

/************************************************************************/
/*                                                                      */
/* FUNCTION: memsep                                                     */
/*                                                                      */
/* PURPOSE: Execute Memory Separation Test that randomly writes to      */
//          areas of memory, then reads the memory back to ensure       */
/*          the values written remain unchanged.                        */
/*                                                                      */
/************************************************************************/
int memsep(int argc, char *argv[])
{
	struct passwd *pwd;          
	uid_t id;                    
	FILE *fp;
	char line[200];
	char flags[10];
	int *start, *end, *inrange, *last_end;
	int is_stack_area;

	printf("Executing Memory Separation Test...\n");

	// First, get the UID of the unprivileged user nobody.
	pwd = getpwnam("nobody");

	if (pwd == NULL) {   // Error
		fprintf(stderr, "Could not obtain info for user nobody");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu memory separation test: could not"
			" obtain info for user nobody"))
#else
		AUDIT_LOG("amtu memory separation test: could not"
			" obtain info for user nobody", 0)
#endif
		return -1;
	}
	else {
		id = pwd->pw_uid;
	}

#ifndef HAVE_LIBAUDIT
/* On RHEL4 must be root in order to read /proc/self/maps */
/* Don't switch to user nobody if running on RHEL4 */

	// Now set the effective UID to the unprivileged user nobody.
	if (debug) {
		fprintf(stderr, "Setting effective UID of user nobody to:"
			" %d\n", id);
	}
	seteuid(id);
	if (debug) {
		fprintf(stderr, "Effective UID is now: %d\n", geteuid());
	}
#endif

	// Check that reading and writing to memory addresses is not allowed.
	fp = fopen("/proc/self/maps", "r");
	if (fp == NULL) {
		fprintf(stderr, "File /proc/self/maps could not be opened");
#ifdef HAVE_LIBLAUS
                LAUS_LOG(("amtu memory separation test: file"
			" /proc/meminfo could not be opened"))
#else
                AUDIT_LOG("amtu memory separation test: file"
			" /proc/meminfo could not be opened", 0)
#endif
		return -1;
	}

	srand((unsigned) time(NULL));

	// Get memory ranges from /proc/self/maps, then
	// attempt to read/write to memory
	last_end = 0;
	while (!feof(fp)) {
		is_stack_area = 0;
		if (!fgets(line, sizeof line, fp)) break;

		if (debug) {
			printf("Line from /proc/self/maps: %s", line);
		}

		/* sample /proc/self/maps lines:
		 * 40028000-4014f000 r-xp 00000000 03:05 283345   /lib/libc-2.3.2.so
		 * 4014f000-40154000 rw-p 00127000 03:05 283345   /lib/libc-2.3.2.so
		 * bfffc000-c0000000 rwxp ffffd000 00:00 0
		 * or 64-bit
		 * 00000000ffff9000-00000000fffff000 rwxp ffffffffffffb000 00:00 0
		 */

		sscanf(line, "%p-%p %s %*s", &start, &end, flags);
		if (debug) {
			printf("start %p, end %p, flags %s\n", start, end, flags);
		}

		if (start < &is_stack_area && &is_stack_area < end) {
			if (debug) printf("This is the stack area.\n");
			is_stack_area = 1;
		}

		if (strchr(flags, 'w') == NULL) {
			// This area is marked read-only. Try writing to it.
			inrange = get_pointer_in_range(start, end);
			write_read_mem(inrange, 1);
		}

		if (start > last_end && !is_stack_area)  {
			/* There is an unmapped area between the end of
			 * the previous one and the start of the current
			 * area.  Try reading a value from there, which
			 * should fail.  Exception: don't try to read
			 * the area reserved for the stack, since that
			 * will auto-extend on some platforms.
			 */
			inrange = get_pointer_in_range(last_end, start);
			write_read_mem(inrange, 0);
		}
		last_end = end;
	}

#ifndef HAVE_LIBAUDIT
/* On RHEL4 don't need to change back, still root */

	// Reset the UID to root.
	pwd = getpwnam("root");

	if (pwd == NULL) {   // Error
		fprintf(stderr, "Could not reset UID to root");
		LAUS_LOG(("amtu memory separation test: could not"
			" reset UID to root"))
		return -1;
	}
	else {
		id = pwd->pw_uid;
	}

	seteuid(id);
	if (debug) {
		fprintf(stderr, "Reset Effective UID to root: %d\n",
			geteuid());
	}
#endif

	fprintf(stderr, "Memory Separation Test SUCCESS!\n");
#ifdef HAVE_LIBLAUS
	LAUS_LOG(("amtu - Memory Separation Test succeeded"))
#else
	AUDIT_LOG("amtu - Memory Separation Test succeeded", 1)
#endif
	return 0;
}
