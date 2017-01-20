//----------------------------------------------------------------------
//
// Module Name:  iodisktest.c
//
// Include File:  none
//
// Description:   Code for Abstract Machine Test Utility - I/O Controller
//		  - Disk Test
//
// Notes:  This module performs the I/O Controller - Disk Test for the Abstract
//         Machine Test Utility (AMTU).  The return codes are 
//         as follows:
//         -1 = failure 
//          0 = success
// -----------------------------------------------------------------
// LANGUAGE:     C
//
// (C) Copyright International Businesses Machine Corp. 2003,2005
// Licensed under the Common Public License v. 1.0
// -----------------------------------------------------------------
//
// Change Activity:
// DATE     PGMR      COMMENTS
// ------- --------- ----------------------
// 7/29/03  K.Simon  Added debug messages
//	             and laus logging
// 8/04/03  K.Simon  Changes per 8/4 code review
// 8/12/03  K.Simon  Added directory descriptor
//	    	     variable dd, updated 
//		     debug messages, code to 
//		     ignore I/O controllers
//		     dedicated to floppy and
//		     cdrom drives
// 8/19/03 K.Simon   Version # on CPL + comment stanzas for functions,
//                   Aligned code with tabs, corrected 10MB amount, 
//		     aligned code lines to 80 chars, changed names of
//		     memory clean up functions, added #define MAXMEMSIZE,
//		     close file before syncing its directory, changed
//		     printfs to fprintfs, corrected possible strncat
//		     overflow of new_file
// 8/25/03 K.Simon   Added NO_TAG to LAUG_LOG
// 8/26/03 K.Simon   Added printf to display test name
//10/17/03 K.Simon   Removed NO_TAG
//10/17/03 K.Simon   Code clean-up per AMTU evaluation
//10/23/03 K.Simon   Corrected reference to Memory Separation Test in
//		     error message
//10/24/03 K.Simon   Removed debug message that prints 10MB string
//10/27/03 K.Simon   Added check for return code of fputs, created
//		     remove_file function
//10/29/03 K.Simon   strncat a "/" to file name for mkstemp, removed
//		     extra parentheses
//10/30/03 K.Simon   Reinitialized n and num_of_chars to zero
//03/15/05 D.Velarde Added AUDIT_LOG statements to be used if we're
//                   using libaudit instead of liblaus
//----------------------------------------------------------------------

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <mntent.h>
#include "amtu.h"

#define MAXLINE 500
#define MAXINDEX 100
#define MAXMEMSIZE 10485670
#define BDEVNAME_SIZE 32 

// Structure to track info about partitions
typedef struct {
	char *part_name;   // Partition name
	int rw_count;      // Number of rw filesystems
	int ro_count;      // Number of ro filesystems
	int assoc_part;    // Indicates if filesystem is associated with partition
} part_rec;

typedef part_rec *part_rec_ptr;

// Structure to track info about filesystems
typedef struct {
	char *fs_name;    // mounted filesystem name
	char *mnt_pt;	  // mount point
	char *fs_opt;     // filesystems options (rw, ro)
} fs_rec;

typedef fs_rec *fs_rec_ptr;

/************************************************************************/
/*                                                                      */
/* FUNCTION: free_part_rec                                              */
/*                                                                      */
/* PURPOSE: Function to clean up memory for part_rec variable           */
/*                                                                      */
/************************************************************************/
void free_part_rec(part_rec *x[MAXINDEX])
{
	int i = 0;

	// Clean-up
	while (x[i] != NULL) {
		free(x[i]->part_name);
		free(x[i]);
		i++;
	}
}

/************************************************************************/
/*                                                                      */
/* FUNCTION: free_fs_rec                                                */
/*                                                                      */
/* PURPOSE: Function to clean up memory for fs_rec structure            */
/*                                                                      */
/************************************************************************/
void free_fs_rec(fs_rec *x[MAXINDEX])
{
	int i = 0;

	// Clean-up
	while (x[i] != NULL) {
		free(x[i]->fs_name);
		free(x[i]->mnt_pt);
		free(x[i]->fs_opt);
		free(x[i]);
		i++;
	}
}

/************************************************************************/
/*                                                                      */
/* FUNCTION: get_io_ctrl                                                */
/*                                                                      */
/* PURPOSE: Determine the I/O IDE Controller based on a partition name. */
/*          For example, the 'a' in partition name 'hda' means that     */
/*          the partition is associated with lst drive of the IDE       */
/*          Controller #1, 'b'is the 2nd drive of IDE Controller #1,    */
/*          'c' is associated with the 1st drive of IDE Controller #2,  */
/*          and so on.  This is needed to determine if an IDE           */
/*          Controller is associated with read-only filesystems.  IDE   */
/*          of this type cannot be checked.                             */
/*                                                                      */
/************************************************************************/
int get_io_ctrl(char *name) 
{
	int io_ctrl = 0;

	// Based on third letter of partition name,
	// determine the I/O Controller
	switch (name[2]) {
		case 'a':
		case 'b':
			io_ctrl = 1;
			break;
		case 'c':
		case 'd':
			io_ctrl = 2;
			break;
		case 'e':
		case 'f':
			io_ctrl = 3;
			break;
		case 'g':
		case 'h':
			io_ctrl = 4;
			break;
		case 'i':
		case 'j':
			io_ctrl = 5;
			break;
		case 'k':
		case 'l':
			io_ctrl = 6;
			break;
		case 'm':
		case 'n':
			io_ctrl = 7;
			break;
		case 'o':
		case 'p':
			io_ctrl = 8;
			break;
		case 'q':
		case 'r':
			io_ctrl = 9;
			break;
		case 's':
		case 't':
			io_ctrl = 10;
			break;
		case 'u':
		case 'v':
			io_ctrl = 11;
			break;
		case 'w':
		case 'x':
			io_ctrl = 12;
			break;
		case 'y':
		case 'z':
			io_ctrl = 13;
			break;
		default:
			if (debug) {
				fprintf(stderr, "Could not determine I/O Controller\n");	
			}

	}
	
	return io_ctrl;
}

/************************************************************************/
/*                                                                      */
/* FUNCTION: remove_file                                                */
/*                                                                      */
/* PURPOSE: Removes a file in a directory path                          */ 
/*                                                                      */
/************************************************************************/
void remove_file(char * file)
{
	int rc;

	// Remove the file
	rc = unlink(file);

	if (rc == -1) {
		// Do not terminate if a file cannot be removed
		if (debug) {
			fprintf(stderr, "Could not remove file: %s. Must"
				" be manually removed\n",
				file);
#ifdef HAVE_LIBLAUS
			LAUS_LOG(("amtu i/o controller - disk"
				" test: could not remove file"))
#else
			AUDIT_LOG("amtu i/o controller - disk"
				" test: could not remove file", 1)
#endif
		}
	}
	else {
		if (debug) {
			fprintf(stderr, "Removed file: %s\n", file);
		}
	}
}
/************************************************************************/
/*                                                                      */
/* FUNCTION: iodisktest                                                 */
/*                                                                      */
/* PURPOSE: Execute I/O Controller - Disk Test to verify random data    */ 
/*          written to disks remains unchanged.                         */
/*                                                                      */
/************************************************************************/
int iodisktest(int argc, char *argv[])
{
	int rc = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	int m = 0;
	int n = 0;
	int found = 0;
	int io_ctrl = 0;
	int inchar = 0; 
	int num_of_chars = 0;
	int new_file_size = 0;
	int fd = 0;
	int dd = 0;
	FILE *fname;  
	FILE *fs;                
	FILE *fs1;           	
	char token[BDEVNAME_SIZE]; 
	char line[MAXLINE]; 
	int num = 32;    
	int num_of_rands = 0;
	char c = 'a';    
	struct mntent *entry;
	char *rand_str;
	char *new_file;
	DIR *dirp;
	mode_t file_mode;
	part_rec *partitions[MAXINDEX];
	fs_rec *fs_info[MAXINDEX];     

	printf("Executing I/O Controller - Disk Test...\n");

        // Get partition names by parsing /proc/partitions file
	fs = fopen("/proc/partitions", "r");
	bzero(partitions, sizeof(partitions));
	if (fs == NULL) {
		fprintf(stderr, "File /proc/partitions could not be opened\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu i/o controller - disk test: file"
			" /proc/partitions could not be opened"))
#else
		AUDIT_LOG("amtu i/o controller - disk test: file"
			" /proc/partitions could not be opened", 0)
#endif
		return -1;
	}

	while(!feof(fs)) {
		fgets(line, sizeof line, fs);
	
		if (line[0] == '\n')  // Skip blank line
			continue;

		if (strstr(line, "major") == NULL) {   // Skip first line
			if (debug) {
                		fprintf(stderr, "Line retrieved from /proc/partitions:"
						" %s\n", line);
			}

			sscanf(line, "%*s %*s %*s %s %*s", token);
			partitions[m] = (part_rec_ptr) malloc (sizeof (part_rec));

			if (partitions[m] == NULL) {
				fprintf(stderr, "Could not allocate memory to"
					" obtain partition info\n");
#ifdef HAVE_LIBLAUS
				LAUS_LOG(("amtu i/o controller - disk test: "
					"could not allocate memory to obtain"
					" partition info"))
#else
				AUDIT_LOG("amtu i/o controller - disk test: "
					"could not allocate memory to obtain"
					" partition info", 0)
#endif
				return -1;
			}

			partitions[m]->part_name = (char *) malloc (strlen(token) + 1);
			if (partitions[m] == NULL) {
				fprintf(stderr, "Could not allocate memory to"
					" obtain partition info\n");
#ifdef HAVE_LIBLAUS
				LAUS_LOG(("amtu i/o controller - disk test:"
					" could not allocate memory to obtain"
					" partition info"))
#else
				AUDIT_LOG("amtu i/o controller - disk test:"
					" could not allocate memory to obtain"
					" partition info", 0)
#endif
				return -1;
			}

			partitions[m]->rw_count = 0;
			partitions[m]->ro_count = 0;
			strncpy(partitions[m]->part_name, token, strlen(token) + 1);
			if (debug) {
				fprintf(stderr, "Stored partition name = %s\n",
				partitions[m]->part_name);
			}
			m++;
		}
	}
	fclose(fs);

	// Open /etc/mtab to process mounted filesystems
	fs1 = setmntent("/etc/mtab", "r");
	bzero(fs_info, sizeof(fs_info));

	if (fs1 != NULL) {
		entry = getmntent(fs1);	
		while (entry != NULL) {
			if (entry != NULL) {
				fs_info[j] = (fs_rec_ptr) malloc (sizeof (fs_rec));
				if (fs_info[i] == NULL) {
					fprintf(stderr, "Could not allocate memory to obtain"
						" filesystem info\n");
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info", 0)
#endif
					return -1;
				}
				fs_info[j]->fs_name = (char *) malloc (strlen(entry->mnt_fsname) + 1);
				if (fs_info[i] == NULL) {
					fprintf(stderr, "Could not allocate memory to obtain"
						" filesystem info\n");
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info", 0)
#endif
					return -1;
				}
				fs_info[j]->mnt_pt = (char *) malloc (strlen(entry->mnt_dir) + 1);
				if (fs_info[i] == NULL) {
					fprintf(stderr, "Could not allocate memory to obtain"
						" filesystem info\n");
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info", 0)
#endif
					return -1;
				}
				fs_info[j]->fs_opt = (char *) malloc (strlen(entry->mnt_opts) + 1);
				if (fs_info[i] == NULL) {
					fprintf(stderr, "Could not allocate memory to obtain"
						" filesystem info\n");
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not allocate memory to obtain"
						" filesystem info", 0)
#endif
					return -1;
				}
				strncpy(fs_info[j]->fs_name, entry->mnt_fsname, 
					strlen(entry->mnt_fsname) + 1);
				strncpy(fs_info[j]->mnt_pt, entry->mnt_dir,
					strlen(entry->mnt_dir) + 1);
				strncpy(fs_info[j]->fs_opt, entry->mnt_opts,
					strlen(entry->mnt_opts) + 1);
				if (debug) {
					fprintf(stderr, "Stored mounted filesystem info = %s %s %s\n",
						fs_info[j]->fs_name,
						fs_info[j]->mnt_pt,
					 	fs_info[j]->fs_opt);
				}
				entry = getmntent(fs1);	
			}
			j++;
		}
		endmntent(fs1);
	}	
	else {
		fprintf(stderr, "Could not open /etc/mtab\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu i/o controller - disk test: could not open /etc/mtab"))
#else
		AUDIT_LOG("amtu i/o controller - disk test: could not open /etc/mtab", 0)
#endif
		return -1;
	}
		
	// Malloc 10MB of memory (1MB = 1,048,576 bytes)
	rand_str = (char *) malloc (MAXMEMSIZE); 

	if (rand_str == NULL) {
		fprintf(stderr, "Could not allocate 10MB of memory\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu i/o controller - disk test: could not allocate 10MB of memory"))
#else
		AUDIT_LOG("amtu i/o controller - disk test: could not allocate 10MB of memory", 0)
#endif
		return -1;
	}
	bzero(rand_str, MAXMEMSIZE);

	// Generate 10MB string to force write to disk
	srand((unsigned) time(NULL));

	// Generate random string
	if (debug) {
		fprintf(stderr, "Generating random numbers...\n"); 
	}
	while (num_of_rands < MAXMEMSIZE) {
		num = rand();
		c = (char) num;

		// Based on the type, accept certain characters
		if (isprint(c)) {
			rand_str[num_of_rands++] = (char) c;
		}
	}

	for (l = 0; fs_info[l] != NULL; l++) {
		if (debug) {
			fprintf(stderr, "Filesystem option is %s for %s\n",fs_info[l]->fs_opt,
				fs_info[l]->fs_name);
		}
		
		// Only check mounted filesystems that are associated with a partition.
		// Partition names with sd* are linked to SCSI controllers and
		// those with hd* are linked to IDE controllers.
		for (k = 0; partitions[k] != NULL; k++) {
			if (strlen(partitions[k]->part_name) == 3) {
				if (strstr(fs_info[l]->fs_name,
					partitions[k]->part_name) != NULL) {
					found = 1;
					partitions[k]->assoc_part = 1;
					if (debug) {
						fprintf(stderr, "Detected filesystem associated"
							" with SCSI/IDE Controller\n");
					}
					if (strstr(fs_info[l]->fs_opt, "rw") != NULL) {
						partitions[k]->rw_count++;
						if (debug) {
							fprintf(stderr, "Incremented rw count to: %d\n",
								 partitions[k]->rw_count);
						}
					}
					if (strstr(fs_info[l]->fs_opt, "ro") != NULL) {
						partitions[k]->ro_count++;
						if (debug) {
							fprintf(stderr, "Incremented ro count to: %d\n",
								 partitions[k]->ro_count);
						}
					}
					break;
				}
			}
		}
					
		if (strstr(fs_info[l]->fs_opt, "ro") == NULL) {
			if (found == 1) {	
				// Malloc memory for a new file name
				new_file_size = strlen(fs_info[l]->mnt_pt) + 9;
				new_file = (char *) malloc(new_file_size);	
				new_file[new_file_size - 1] = 0;
				--new_file_size;

				if (new_file == NULL) {
					fprintf(stderr, "Could not allocate memory" 
						" to create new file\n");
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk"
						" test: could not allocate memory"
						" to create new file"))
#else
					AUDIT_LOG("amtu i/o controller - disk"
						" test: could not allocate memory"
						" to create new file", 0)
#endif
					return -1;
				}

				// Create unique file on mounted filesystems
				// with restricted permissions
				strncpy(new_file,fs_info[l]->mnt_pt, 
					new_file_size);
				strncat(new_file, "/",
					new_file_size - strlen(new_file));
				strncat(new_file, "XXXXXX",
					new_file_size - strlen(new_file));
				file_mode = umask(077);
				fd = mkstemp(new_file);
				(void) umask(file_mode);
				if (fd == -1) {
					fprintf(stderr, "Could not create a new file: %d\n", errno);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not create new file"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not create new file", 0)
#endif
					return -1;
				}
				if (debug) {
					fprintf(stderr, "New file created %s\n", new_file);
				}

				// Now, open the newly created file
				fname = fdopen(fd, "w+");

				if (fname == NULL) {
					fprintf(stderr, "File %s could not be opened\n", new_file);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not open file"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not open file", 0)
#endif
					remove_file(new_file);
					return -1;
				}

				// Write the 10MB string to the file
				if (debug) {
					fprintf(stderr, "Writing 10MB string to file %s...\n",
						new_file);
				}
				rc = fputs(rand_str, fname);
				if (rc == EOF) { 
					fprintf(stderr, "Could not write 10MB string to"
						" file: %s\n", new_file);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not write 10MB string file"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not write 10MB string file", 0)
#endif
					remove_file(new_file);					
					return -1;
				}

				// Sync the new file to make sure its contents
				// are written to disk. 
				rc = fsync(fd);
				if (rc == -1) {
					fprintf(stderr, "Could not sync file: %s\n", new_file);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not sync file"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not sync file", 0)
#endif
					remove_file(new_file);
					return -1;
				}

				// Open the new file's directory.
				if (debug) {
					fprintf(stderr, "Opening directory %s ...\n",
						fs_info[l]->mnt_pt);
				}
				dirp = opendir(fs_info[l]->mnt_pt);
				if (dirp == NULL) {
					fprintf(stderr, "Could not open directory: %s\n", 
						fs_info[l]->mnt_pt);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not open directory"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not open directory", 0)
#endif
					remove_file(new_file);
					return -1;
				}
	
				// Get file descriptor of new file's directory
				if (debug) {
					fprintf(stderr, "Getting file descriptor"
						" of directory %s ...\n",
						fs_info[l]->mnt_pt);
				}
				dd = dirfd(dirp);	
				if (dd == -1) {
					fprintf(stderr, "Could not get file"
						" descriptor for directory:"
						" %s\n", fs_info[l]->mnt_pt);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk"
						" test: could not get file"
						" descriptor for directory"))
#else
					AUDIT_LOG("amtu i/o controller - disk"
						" test: could not get file"
						" descriptor for directory", 0)
#endif
					remove_file(new_file);
					return -1;
				}

				// Close file
				if (debug) {
					fprintf(stderr, "Closing file %s ...\n", new_file);
				}
				fclose(fname);

				// Sync the new file's directory.  Must do an
				// explicit sync to ensure that entry in the
				// directory containing the new file has also
				// reached the disk.
				rc = fsync(dd);
				if (rc == -1) {
					fprintf(stderr, "Could not sync directory: %s\n", 
						fs_info[l]->mnt_pt);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						"could not sync directory"))
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						"could not sync directory", 0)
#endif
					remove_file(new_file);
					return -1;
				}

				// Close directory
				if (debug) {
					fprintf(stderr, "Closing directory %s ...\n", 
						fs_info[l]->mnt_pt);
				}
				closedir(dirp);

				// Re-open file
				if (debug) {
					fprintf(stderr, "Reopening file %s ...\n", new_file);
				}

				fname = fopen(new_file, "r");
				if (fname == NULL) {
					fprintf(stderr, "Could not re-open file: %s\n", new_file);
					remove_file(new_file);
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu i/o controller - disk test:"
						" could not re-open file")) 
#else
					AUDIT_LOG("amtu i/o controller - disk test:"
						" could not re-open file", 0) 
#endif
					return -1;
				}
	
				// Verify that the string in the file is actually what
				// was written to it.
				if (debug) {
					fprintf(stderr, "Verifying string in file %s matches what"
						" was written...\n", new_file);
				}

				n = num_of_chars = 0;
				while ((inchar = getc(fname)) != EOF) {
					if ((char) inchar != rand_str[n++]) {
						if (debug) {
							fprintf(stderr, "String in file %s does not"
								" match what was written\n",
								new_file);
						}
						fprintf(stderr, "I/O Controller - Disk Test FAILED!\n");
#ifdef HAVE_LIBLAUS
						LAUS_LOG(("amtu failed i/o controller - disk test"))
#else
						AUDIT_LOG("amtu failed i/o controller - disk test", 0)
#endif
						remove_file(new_file);
						return -1;
					}
					num_of_chars++;
				}
		
				// Make sure file contains the entire random string written to it.
				if (debug) {
					fprintf(stderr, "Number of file characters checked: %d\n",
						num_of_chars);
					fprintf(stderr, "Number of characters in random string: %d\n",
						num_of_rands);
				}
				if (num_of_chars != num_of_rands) {
					fprintf(stderr, "File %s does not contain the entire string"
						" written\n", new_file); 
					fprintf(stderr, "I/O Controller - Disk Test FAILED!\n");
#ifdef HAVE_LIBLAUS
					LAUS_LOG(("amtu failed i/o controller - disk test"))
#else
					AUDIT_LOG("amtu failed i/o controller - disk test", 0)
#endif
					return -1;
				}

				// Close file
				if (debug) {
					fprintf(stderr, "Closing file %s ...\n", new_file);
				}
				fclose(fname);

				// Remove the file
				remove_file(new_file);

				// Clean up memory
				free(new_file);
			}
		}
		found = 0;
	}

	// Check for I/O IDE Controllers with ro mounted filesystems only.
	// Print Warning if found since we are unable to check
	// read-only mounted filesystems.  Do not print a warning for
	// mounted filesystems that are dedicated to floppy or cdrom devices.
	// Currently, we are not able to print warnings for SCSI Controllers.
	for (k = 0; partitions[k] != NULL; k++) {
		if ((partitions[k]->assoc_part == 1) 
			&& (partitions[k]->rw_count == 0) 
			&& (partitions[k]->ro_count > 0 ) 
			&& (strstr(partitions[k]->part_name, "fd") == NULL)
			&& (strstr(partitions[k]->part_name, "sd") == NULL)
			&& (strstr(partitions[k]->part_name, "cd")  == NULL)) {

			// Obtain the I/O Controller 
			io_ctrl = get_io_ctrl(partitions[k]->part_name);
			if (io_ctrl != 0) {
				fprintf(stderr, "WARNING: Unable to check I/O IDE Controller %d"
					"associated with partition %s!  This controller" 
					"controls read-only filesystems only.\n",
					io_ctrl, partitions[k]->part_name);
#ifdef HAVE_LIBLAUS
				LAUS_LOG(("WARNING: Unable to check I/O Controller"
					" associated with partition!"))
#else
				AUDIT_LOG("WARNING: Unable to check I/O Controller"
					" associated with partition!", 0)
#endif
			}
		}
	}

	// Clean up memory
	free(rand_str);
	free_part_rec(partitions);
	free_fs_rec(fs_info);

	fprintf(stderr, "I/O Controller - Disk Test SUCCESS!\n");
#ifdef HAVE_LIBLAUS
	LAUS_LOG(("amtu - I/O Controller - Disk Test succeeded"))
#else
	AUDIT_LOG("amtu - I/O Controller - Disk Test succeeded", 1)
#endif

	return 0;
}
