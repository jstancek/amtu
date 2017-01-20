//----------------------------------------------------------------------
//
// Header Name:  amtu.h
//
//
// Description:   Header File for Abstract Machine Test Utility.
//
// -----------------------------------------------------------------
// LANGUAGE:     C
//
// (C) Copyright IBM Corp. 2003-2007
// Licensed under the Common Public License
// -----------------------------------------------------------------
//
// Change Activity:
// DATE     PGMR      COMMENTS
// -------- --------- ----------------------
// 07/20/03 EJR       Created for function prototypes and debug
// 08/25/03 KSimon    Added #include <laus_tags.h>
// 03/15/05 D.Velarde Added AUDIT_OPEN and AUDIT_LOG macros
// 03/15/05 D.Velarde Added #define HAVE_LIBAUDIT 1 - may need to be moved
// 03/16/05 D.Velarde Modified AUDIT_OPEN and AUDIT_LOG macros
// 03/16/05 D.Velarde Added AUDIT_CLOSE macro
// 03/17/05 D.Velarde HAVE_LIBAUDIT now set correctly in configure.in
// 11/28/07 EJR	      Fixed bug in AUDIT_LOG #define when libaudit.h not avail
//----------------------------------------------------------------------
#ifndef _AMTU_H_
#define _AMTU_H_
int debug;

/* Function Prototypes */
int memory(int, char **);
int memsep(int, char **);
int iodisktest(int, char **);
int amtu_priv(int, char **);
int networkio(int, char **);

/* LAuS defines from Tom Lendacky */
#ifdef HAVE_LIBLAUS
#include <sys/param.h>
#include <laus.h>
#include <laus_tags.h>
#define LAUS_ERRMSG(f, x)                                               \
{                                                                       \
        syslog(LOG_WARNING,                                             \
                "LAuS error - %s:%i - %s: (%i) %s\n",                   \
                __FILE__, __LINE__,                                     \
                f, x, laus_strerror(x));                                \
}
#define LAUS_OPEN                                                       \
{                                                                       \
        int rc = laus_open(NULL);                                       \
        if (rc < 0)                                                     \
                LAUS_ERRMSG("laus_open", rc)                            \
}
#define LAUS_LOG(msg)                                                   \
{                                                                       \
        int rc = laus_log("ADMIN_amtu", msg);                           \
        if (rc < 0)                                                     \
                LAUS_ERRMSG("laus_log", rc)                             \
}
#else
#define LAUS_ERRMSG(f, x)
#define LAUS_OPEN
#define LAUS_LOG(msg)
#endif

/* If using libaudit instead of laus, res 0 failed, 1 success */
#ifdef HAVE_LIBAUDIT
#include <libaudit.h>
#define AUDIT_LOG(msg, res)						\
{									\
	int fd = audit_open();						\
	if (res)							\
		audit_log_user_message(fd, AUDIT_TEST, msg,		\
			NULL, NULL, NULL, res);				\
	else								\
		audit_log_user_message(fd, AUDIT_ANOM_AMTU_FAIL, msg,	\
			NULL, NULL, NULL, res);				\
	close(fd);							\
}
#else
#define AUDIT_LOG(msg, res)
#endif


#endif

