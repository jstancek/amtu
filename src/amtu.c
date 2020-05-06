//----------------------------------------------------------------------
//
// Module Name:  amtu.c
//
// Include File:  amtu.h
//
// Description:   Code for Abstract Machine Test Utility.
//
// Notes:  This module is the framework for the Abstract Machine
//         Test Utility (AMTU). The following tests will be invoked
//         from this module:
//         - Memory
//         - Memory Separation
//         - I/O Controller - Network
//         - I/O Controller - Disk
//         - Supervisor Mode Instructions
// -----------------------------------------------------------------
// LANGUAGE:     C
//
// (C) Copyright IBM Corp. 2003,2005
// Licensed under the Common Public License
// -----------------------------------------------------------------
//
// Change Activity:
// DATE     PGMR      COMMENTS
// ------- --------- ----------------------
// 07/29/03 K.Simon  Added iodisktest function 
// 08/25/03 K.Simon  Added audit tag for LAUS_LOG  		      
// 08/26/03 K.Simon  Improved usage statement
// 10/17/03 K.Simon  Removed NO_TAG
// 10/20/03 K.Simon  Added msg variable to allow LAUS_LOG
//		     to print variables
// 03/15/05 D.Velarde Added AUDIT_LOG and AUDIT_OPEN statements to be
//                    used if LIBLAUS isn't being used
// 03/16/05 D.Velarde Stop auditing when test is done.
// 07/12/05 D.Velarde Do NOT start or stop audit daemon.  Remove all
//                    AUDIT_OPEN and AUDIT_CLOSE calls.
// 12/08/05 D.Velarde Updated with new records patch from RedHat
//
//----------------------------------------------------------------------

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "amtu.h"

int debug;

void usage()
{
	printf("Usage: amtu [-dmsinph]\n");
	printf("d      Display debug messages\n");
	printf("m      Execute Memory Test\n");
	printf("s      Execute Memory Separation Test\n");
	printf("i      Execute I/O Controller - Disk Test\n");
	printf("n      Execute I/O Controller - Network Test\n");
	printf("p      Execute Supervisor Mode Instructions Test\n");
	printf("h      Display help message\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int c;
	int memtest = 0, memseptest = 0, disktest = 0;
	int nettest = 0, privtest = 0;
	int testspecified = 1;
	char msg[50];

#ifdef HAVE_LIBLAUS
	LAUS_OPEN
#endif
	
	while ((c = getopt(argc, argv, "dmsinph")) != -1) { 
		switch (c) {
			case 'd':
				debug = 1;
				break;
			case 'm':
				memtest++;
				testspecified = 0;
				break;
			case 's':
				memseptest++;
				testspecified = 0;
				break;
			case 'i':
				disktest++;
				testspecified = 0;
				break;
			case 'n':
				nettest++;
				testspecified = 0;
				break;
			case 'p':
				privtest++;
				testspecified = 0;
				break;
			case 'h':
				usage();
				break;
			default:
				usage();
				break;
		}
	}

	// Invoke Memory Test
	if (testspecified || memtest) {
		rc = memory(argc, argv); 
	}

	// Invoke Memory Separation Test
	if (testspecified || memseptest) {
		rc |= memsep(argc, argv);
	}

	// Invoke I/O Controller - Network Test
	if (testspecified || nettest) {
		rc |= networkio(argc, argv);
	}	

	// Invoke I/O Controller - Disk Test
	if (testspecified || disktest) {
		rc |= iodisktest(argc, argv);
	}

	// Invoke Supervisor Mode Instructions Test
	if (testspecified || privtest) {
		rc |= amtu_priv(argc, argv);
	}

	if (rc != 0) {
		sprintf(msg, "amtu failed: %d", rc);
#ifdef HAVE_LIBLAUS
		LAUS_LOG((msg))
#else
		AUDIT_LOG(msg, 0)
#endif
	} else {
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu completed successfully."))
#else
		AUDIT_LOG("amtu completed successfully", 1)
#endif
	}
	return(rc);
}
