/*
  ptyopen - runs a program on a fake terminal
  ptyopen.c - main program
  Copyright (C) 1999-2000 Phlippe Troin <phil@fifi.org>

  $Id$
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <grp.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

static char CVSID[] __attribute__((unused))="$Header$";

#ifndef NDEBUG
#include <assert.h>
#define RING_CHECK(ring) ring_check(ring)
#else
#define RING_CHECK(ring) do ; while(0)
#endif

/* Structures */
typedef struct {
  uid_t  uid;
  gid_t  gid;
  mode_t mode;
} savedperms_t;

struct ring_st;
typedef struct ring_st ring_t;

/* Prototypes */
#ifdef __STDC__
# define _P_(a) a
#else
# define _P_(a) ()
#endif

int 	    main           _P_((int, char*[]));
#ifdef HAVE_GRANTPT
#  define getprivs()
#  define dropprivs()
#  define ispriv() 1
#else /* ! HAVE_GRANTPT */
void 	    getprivs       _P_((void));
void 	    dropprivs      _P_((void));
int  	    ispriv         _P_((void));
#endif /* HAVE_GRANTPT */
const char* getpttypair    _P_((int[2]));
void*       xmalloc        _P_((size_t));
char*       xstrdup        _P_((const char*));
#ifdef HAVE_GRANTPT
#  define saveperms(a,b) 0
#  define restoreperms(a,b) 0
#  define setperms(a,b) 0
#else /* ! HAVE_GRANTPT */
int         saveperms      _P_((int,savedperms_t*));
int         restoreperms   _P_((const char*, savedperms_t*));
int         setperms       _P_((int, mode_t));
#endif /* HAVE_GRANTPT */
void        loop_on        _P_((int, size_t, const char));
ring_t*     ring_construct _P_((size_t));
void        ring_delete    _P_((ring_t*));
size_t      ring_space     _P_((ring_t*));
int         ring_read      _P_((ring_t*, int));
int         ring_write     _P_((ring_t*, int));
int         ring_push_char _P_((ring_t*, const char));
int         term_raw       _P_((int));
int         term_restore   _P_((int));
int         term_winsize   _P_((int));
void        cleanup        _P_((void));
void        sig_chld_h     _P_((int));
void        sig_fatal_h    _P_((int));
void        sig_tstp_h     _P_((int));
void        sig_cont_h     _P_((int));
void        sig_winch_h    _P_((int));
#undef _P_

/* Configurable options */
#ifndef HAVE_GRANTPT
const char *tty_prefix    = "/dev/tty";
const char *pty_prefix    = "/dev/pty";
const char *ptty_1st_char = "pqrstuvwxyz";
const char *ptty_2nd_char = "0123456789abcdef";
const char *tty_group     = "tty";
#endif /* HAVE_GRANTPT */
#ifndef HAVE_GETPT
const char *devptmx       = "/dev/ptmx";
#endif /* HAVE_PGETPT */
#define DEFAULT_RING_SIZE 10240
#define MAX_RING_SIZE (1<<30)

/* Global variables */
char* 	       	progname = "<unset>";
int   	       	opt_verbose;
unsigned long   opt_geometry_width;
unsigned long   opt_geometry_height;
#ifndef HAVE_GRANTPT
uid_t 	       	unsecure_uid;
gid_t 	       	unsecure_gid;
uid_t 	       	secure_uid;
gid_t 	       	secure_gid;
#endif
volatile int    state_child_exited;
const char*    	state_tty_name;
savedperms_t   	state_tty_perms;
int 	       	state_saved_stdin_flags  = -1;
int 	        state_saved_stdout_flags = -1;
struct termios* state_orig_termios; 
pid_t           state_child_pid;
int             state_pty_fd;

/* Main function */
int 
main(argc, argv) 
     int argc;
     char *argv[];
{
  char *cptr;
  int pty_pair[2];
  int opt_unsecure;
  int opt_write;
  size_t opt_ring_size;
  int child_status;
  int parent_exit=0;
  struct sigaction sa;
  struct termios tios;
  /**/

#ifndef HAVE_GRANTPT  
  /* Remember our initial ids */
  unsecure_uid = geteuid();
  unsecure_gid = getegid();
  secure_uid   = getuid();
  secure_gid   = getgid();
#endif

  /* Drop privileges at startup */
  dropprivs();

  /* Get our name */
  cptr=progname=argv[0];
  while (*cptr) 
    {
      if (*cptr=='/') 
	{
	  progname = ++cptr;
	}
      else 
	{
	  ++cptr;
	}
    }

  /* Set default options */
  opt_verbose=0;
  opt_unsecure=0;
  opt_write=0;
  opt_ring_size = DEFAULT_RING_SIZE;
  opt_geometry_width = 0;
  opt_geometry_height = 0;

  /* Check options */
  ++argv; --argc;
  while (argc && argv[0][0]=='-') 
    {
      if (strcmp("-v", argv[0])==0 || strcmp("--verbose", argv[0])==0)
	{
	  opt_verbose=1;
	}
#ifndef HAVE_GRANTPT
      else if (strcmp("-u", argv[0])==0 || strcmp("--unsecure", argv[0])==0)
	{
	  opt_unsecure=1;
	}
#endif
      else if (strcmp("-w", argv[0])==0 || strcmp("--write", argv[0])==0)
	{
	  opt_write=1;
	}
      else if (strcmp("-r", argv[0])==0 
	       || strcmp ("--ringsize",  argv[0])==0) 
	{
	  char *end;
	  /**/

	  if (argc<2) 
	    {
	      fprintf(stderr, "%s: option %s requires an argument\n",
		      progname, argv[0]);
	      exit(255);
	    }
	  opt_ring_size=strtoul(argv[1], &end, 10);
	  if (*end) 
	    {
	      fprintf(stderr, "%s: %s is not a number\n",
		      progname, argv[1]);
	      exit(255);
	    }
	  if (opt_ring_size==0 || opt_ring_size>MAX_RING_SIZE)
	    {
	      fprintf(stderr, "%s: ring size out of range\n", progname);
	      exit(255);
	    }
	  ++argv; --argc;
	}
      else if (strncmp("--ringsize=", argv[0], 11)==0)
	{
	  char *start;
	  char *end;
	  /**/

	  start = argv[0]+11;
	  if ( !*start) 
	    {
	      fprintf(stderr, "%s: option %s requires an argument\n",
		      progname, argv[0]);
	      exit(255);
	    }
	  opt_ring_size=strtoul(start, &end, 10);
	  if (*end) 
	    {
	      fprintf(stderr, "%s: %s is not a number\n",
		      progname, start);
	      exit(255);
	    }	  
	  if (opt_ring_size==0 || opt_ring_size>MAX_RING_SIZE)
	    {
	      fprintf(stderr, "%s: ring size out of range\n", progname);
	      exit(255);
	    }
	}
      else if (strcmp("-g", argv[0])==0 || strcmp("--geometry", argv[0])==0)
	{
	  char *start;
	  char *end;
	  /**/

	  if (argc<2) 
	    {
	      fprintf(stderr, "%s: option %s requires an argument\n",
		      progname, argv[0]);
	      exit(255);
	    }
	  opt_geometry_width = strtoul(argv[1], &end, 10);
	  if (*end != 'x')
	    {
	    bad_geom:
	      fprintf(stderr, "%s: bad geometry specification \"%s\"\n",
		      progname, argv[1]);
	      exit(255);
	    }
	  start = end+1;
	  opt_geometry_height = strtoul(start, &end, 10);
	  if (*end) goto bad_geom;

	  if (opt_geometry_width == 0 || opt_geometry_width >= 65536
	      || opt_geometry_height == 0 || opt_geometry_height >= 65536)
	    goto bad_geom;
	  --argc, ++argv;
	}
      else if (strcmp("-h", argv[0])==0 || strcmp("--help", argv[0])==0)
	{
	  printf("usage: %s <options> <program> <args>...\n"
		 "  where options can be any of:\n"
		 "  -v, --verbose     : prints warnings, problems, etc...\n"
		 "  -V, --version     : reports version of %s\n"
#ifndef HAVE_GRANTPT
		 "  -u, --unsecure    : runs unsecurely\n"
#endif
		 "  -w, --write:      : allows tty to be write(1)n to\n"
		 "  -r, --ringsize <s>: use a ring of <s> bytes\n"
		 "  -g, --geometry <g>: pretends the terminal size is <g> "
		 " (eg. 123x50)\n"
		 "  -h, --help        : prints this help\n",
		 progname,
		 progname);
	  exit(0);
	}
      else if (strcmp("-V", argv[0])==0 || strcmp("--version", argv[0])==0)
	{
	  printf("%s version " VERSION "\n", progname);
	  exit(0);
	}
      else 
	{
	  fprintf(stderr, "%s: bad option \"%s\", try \"%s --help\"\n", 
		  progname, argv[0], progname);
	  exit(255);
	}
      ++argv; --argc;
    }

  /* Check number of arguments */
  if (argc==0) 
    {
      fprintf(stderr, "%s: not enough arguments, try \"%s --help\"\n",
	      progname, progname);
      exit(255);
    }

  /* Set up traps for all deadly signals to clean up */
  sa.sa_handler = sig_fatal_h;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGHUP, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGHUP): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGINT, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGINT): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGQUIT, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGQUIT): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGABRT, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGABRT): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGILL, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGILL): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGSEGV, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGSEGV): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGPIPE, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGPIPE): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGTERM, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGTERM): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGTRAP, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGTRAP): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  if (sigaction(SIGBUS, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGBUS): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }    

  /* Special handling for SIGSTOP and SIGCONT */
  sa.sa_handler = sig_tstp_h;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART|SA_NODEFER;
  if (sigaction(SIGTSTP, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGTSTP): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  
  sa.sa_handler = sig_cont_h;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCONT, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGCONT): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  

  /* Handle window size changes */
  sa.sa_handler = sig_winch_h;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGWINCH, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGWINCH): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  

  /* Don't forget to cleanup at exit too ! */
  atexit(cleanup);
  
  /* Ok, we're rolling: get the ptys */
  state_tty_name = getpttypair(pty_pair);
  state_pty_fd = pty_pair[0];

  /* Check/change perms */
  if (!opt_unsecure) 
    {
      if (saveperms(pty_pair[1],&state_tty_perms)==-1) 
	{
	  fprintf(stderr, "%s: saving permissions on tty: %s\n",
		  progname, strerror(errno));
	  exit(255);
	}
      if (setperms(pty_pair[1], 
		   S_IRUSR | S_IWUSR | (opt_write ? S_IWGRP : 0))==-1) {
	  fprintf(stderr, "%s: setting permissions on tty: %s\n",
		  progname, strerror(errno));
	  exit(255);
	}
    }

  /* Get the termios EOF char on the slave */
  if (tcgetattr(pty_pair[1], &tios)==-1) 
    {
      fprintf(stderr, "%s: tcgetattr on pty slave: %s\n",
	      progname, strerror(errno));
      exit(255);
    }

  /* Are we on a tty ? If yes, then go into raw mode  on this term */
  if (isatty(0)) 
    term_raw(1);
  /* If we're on a tty OR we forced a geometry, set the window size */
  if (isatty(0) || opt_geometry_height > 0)
    term_winsize(1);

  /* Prepare to fork: add signal handler and clear the state */
  state_child_exited = 0;
  sa.sa_handler = sig_chld_h;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_NOCLDSTOP|SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL)==-1) 
    {
      fprintf(stderr, "%s: sigaction(SIGCHLD): %s\n", 
	      progname, strerror(errno));
      exit(255);
    }  

  /* Fork it ! */
  fflush(stdin);
  fflush(stdout);
  fflush(stderr);
  switch (state_child_pid = fork()) 
    {
    case -1:
      /* Error */
      fprintf(stderr, "%s: fork(): %s", progname, strerror(errno));
      exit(255);
    case 0:
      /* Child */
      {
	int i;
	int fdflags;
	/**/

	/* Close the pty master */
	close(pty_pair[0]);
	
	/* Create a new session */
	if (setsid()==-1) 
	  {
	    fprintf(stderr, "%s: cannot create a new session: %s\n",
		    progname, strerror(errno));
	    exit(255);
	  }	

	/* Dup the pty slave to fds 0,1 and 2 */
	for (i=0; i<=2; i++) 
	  {
	    if (dup2(pty_pair[1], i)==-1) 
	      {
		fprintf(i==2 ? stdout : stderr, 
			"%s[child]: cannot dup2 tty to fd %d: %s\n",
			progname, i, strerror(errno));
		exit(255);
	      }
	  }
	
	/* Close the tty original fd */
	close(pty_pair[1]);

	/* Clear CLOEXEC on these fds */
	for (i=0; i<=2; i++)
	  {
	    if ((fdflags=fcntl(i, F_GETFD))==-1) 
	      {
		fprintf(stderr, "%s[child]: cannot F_GETFL on fd %d: %s\n",
			progname, i, strerror(errno));
		exit(255);
	      }
	    fdflags &= ~FD_CLOEXEC;
	    if (fcntl(i, F_SETFD, fdflags)==-1) {
		fprintf(stderr, "%s[child]: cannot F_SETFL on fd %d: %s\n",
			progname, i, strerror(errno));
		exit(255);
	    }
	  }

	/* Set the controlling terminal */
#ifdef TIOCSCTTY
	if (ioctl(0, TIOCSCTTY)==-1) 
	  {
	    fprintf(stderr, "%s[child]: TIOCSCTTY: %s\n",
		    progname, strerror(errno));
	    exit(255);
	  }
#endif /* def TIOCSCTTY */

	/* Ready to exec */
	if (execvp(argv[0], argv)==-1) 
	  {
	    fprintf(stderr, "%s[child]: cannot exec %s: %s\n",
		    progname, argv[0], strerror(errno));
	    exit(255);
	  }
	/* Never reached */
      }
    default:
      /* Parent */
      break;
    }

  /* Close the slave */
  close(pty_pair[1]);

  /* Run the loop on the master */
  loop_on(pty_pair[0], opt_ring_size, tios.c_cc[VEOF]);

  /* Close the master now */
  close(pty_pair[0]);
  state_pty_fd = 0;

  /* Wait for the child to complete */
  if (waitpid(state_child_pid, &child_status, 0)==-1) 
    {
      fprintf(stderr, "%s: waitpid() for child: %s\n", 
	      progname, strerror(errno));
      exit(255);
    }
  if (WIFEXITED(child_status)) 
    {
      parent_exit = WEXITSTATUS(child_status);
      if (opt_verbose) 
	{
	  fprintf(stderr, "%s: child exited with code %d\n",
		  progname, parent_exit);
	}
    }
  else if (WIFSIGNALED(child_status)) 
    {
      parent_exit = 254;
      if (opt_verbose) 
	{
	  fprintf(stderr, "%s: child got signal %d: %s%s\n",
		  progname, WTERMSIG(child_status),
		  strsignal(WTERMSIG(child_status)),
		  WCOREDUMP(child_status) ? " (core dumped)" : "");
	}
    }
  else 
    {
      fprintf(stderr, "%s: exotic exit status from child %08X\n",
	      progname, child_status);
      exit(255);
    }
    
  /* Restore perms */
  if (!opt_unsecure) 
    {
      if (restoreperms(state_tty_name, &state_tty_perms)==-1) 
	{
	  fprintf(stderr, "%s: restoring permissions on tty: %s\n",
		  progname, strerror(errno));
	  exit(255);
	}
    }
  free((void*)state_tty_name);
  state_tty_name=0;

  /* Restore terminal */
  term_restore(1);

  return parent_exit;
}

#ifdef HAVE_GRANTPT
const char*
getpttypair(fds)
     int fds[2];
{
  int ptmx;
  int slave;
  const char* name;
  /**/

#ifdef HAVE_GETPT
  if ((ptmx = getpt())<0)
    {
      fprintf(stderr, "%s: getpt(): %s\n", progname, sterror(errno));
      exit(255);
    }
#else /* ! HAVE_GETPT */
  if ((ptmx = open(devptmx, O_RDWR))<0)
    {
      fprintf(stderr, "%s: open(\"%s\"): %s\n", progname, 
	      devptmx, strerror(errno));
      exit(255);
    }
#endif /* HAVE_GETPT */

  if ((slave = grantpt(ptmx))<0)
    {
      fprintf(stderr, "%s: grantpt(): %s\n", progname, strerror(errno));
      exit(255);
    }

#ifdef HAVE_UNLOCKPT
  if (unlockpt(ptmx)<0)
    {
      fprintf(stderr, "%s: unlockpt(): %s\n", progname, strerror(errno));
      exit(255);
    }
#endif

  if (!(name=ptsname(ptmx)))
    {
      fprintf(stderr, "%s: ptsname(): %s\n", progname, strerror(errno));
      exit(255);
    }

  fds[0] = ptmx;
  fds[1] = slave;

  return xstrdup(name);
}
#else /* ! HAVE_GRANTPT */
/* Get a pty/tty pair, return open file descriptors in the arguments
   (a la pipe)
   */
const char*
getpttypair(fds)
     int fds[2];
{
  char *pty_string;
  char *tty_string;
  size_t pty_size;
  size_t tty_size;
  const char* last_1st_char;
  const char* last_2nd_char;
  const char* p_1st_char;
  const char* p_2nd_char;
  /**/

  /* Allocate and copy prefixes */
  pty_size = strlen(pty_prefix);
  tty_size = strlen(tty_prefix);
  pty_string = xmalloc(pty_size+3);
  tty_string = xmalloc(tty_size+3);
  strcpy(pty_string, pty_prefix);
  strcpy(tty_string, tty_prefix);
  pty_string[pty_size+2]=0;
  tty_string[tty_size+2]=0;
  
  /* How many chars in the arrays ? */
  last_1st_char = ptty_1st_char+strlen(ptty_1st_char);
  last_2nd_char = ptty_2nd_char+strlen(ptty_2nd_char);

  /* Loop, and we need privileges to do the open() */
  for ( p_1st_char=ptty_1st_char; p_1st_char < last_1st_char; ++p_1st_char)
    {
      for ( p_2nd_char=ptty_2nd_char; p_2nd_char < last_2nd_char; ++p_2nd_char)
	{
	  pty_string[pty_size]   = *p_1st_char;
	  pty_string[pty_size+1] = *p_2nd_char;
	  getprivs();
	  fds[0]=open(pty_string, O_RDWR|O_NOCTTY);
	  dropprivs();
	  if (fds[0]==-1) 
	    {
	      switch(errno) 
		{
		case ENOENT:
		  /* File not found */
		  goto no_more_ptys;
		case EIO:
		  /* Pty busy */
		  break;
		case EACCES:
		  /* File unreadable */
		  if (!ispriv()) 
		    {
		      break;
		    }
		  /* If we're privileged *AND* we get EACCESS, fall
		     back on default: */
		default:
		  fprintf(stderr, "%s: open %s: %s\n",
			  progname, pty_string, strerror(errno));
		  exit(255);
		}
	    }
	  else 
	    {
	      /* Found the pty, try open the tty */
	      tty_string[tty_size]   = *p_1st_char;
	      tty_string[tty_size+1] = *p_2nd_char;
	      getprivs();
	      fds[1]=open(tty_string, O_RDWR|O_NOCTTY);
	      dropprivs();
	      if (fds[1]==-1) 
		{
		  switch(errno) {
		  case ENOENT:
		    /* File not found */
		    if (opt_verbose) 
		      {
			fprintf(stderr, "%s: found %s without matching %s\n",
				progname, pty_string, tty_string);
		      }
		    break;
		  case EACCES:
		    if (!ispriv()) 
		      { 
			break;
		      }
		    /* If we're privileged *AND* we get EACCESS, fall
		     back on default: */
		  default:
		    fprintf(stderr, "%s: open %s: %s\n",
			    progname, tty_string, strerror(errno));
		    exit(255);
		  }
		  /* Close the pty then, and start over */
		  if (close(fds[0])==-1)
		      {
			fprintf(stderr, "%s: close %s: %s\n",
				progname, tty_string, strerror(errno));
			exit(255);
		      }
		}
	      else 
		{
		  /* Okie */
		  free(pty_string);
		  return tty_string;
		}
	    }
	}
    }
no_more_ptys:
  /* Ran out of ptys ! */
  fprintf(stderr, "%s: no more ptys available\n", progname);
  exit(255);
}
#endif /* HAVE_GRANTPT */

#ifndef HAVE_GRANTPT
/* Save permissions on a fd for future restoration eventually */  
int
saveperms(fd,perms) 
     int fd;
     savedperms_t* perms;
{
  struct stat statbuf;

  if (fstat(fd, &statbuf)==1) 
    {
      return -1;
    }
  else 
    {
      perms->uid  = statbuf.st_uid;
      perms->gid  = statbuf.st_gid;
      perms->mode = statbuf.st_mode 
	& (S_IRWXU|S_IRWXG|S_IRWXO|S_ISGID|S_ISUID|S_ISVTX);
      return 0;
    }
}

/* Restore permissions on a fd */
int 
restoreperms(file, perms)
     const char* file;
     savedperms_t* perms;
{
  int rv;

  if (chmod(file, perms->mode)==-1) return -1;

  /* Need privileges to chown it */
  getprivs();
  rv=chown(file, perms->uid, perms->gid);
  dropprivs();

  return rv;
}

/* Set tty-permissions on a fd */
int 
setperms(fd, mode)
     int fd;
     mode_t mode;
{
  struct group *grbuf;
  gid_t group;
  int rv;
  /**/

  /* Retrieve the tty group */
  grbuf = getgrnam(tty_group);
  if (grbuf==NULL) 
    {
      if (opt_verbose) 
	{
	  fprintf(stderr, "%s: cannot find a group named %s\n", 
		  progname, tty_group);
	}
      /* Use the user group, and remove permissions on group */
      group = secure_gid;
      mode &= ~S_IRWXG;
    }
  else 
    {
      group = grbuf->gr_gid;
    }
  
  /* Chown it with privs */
  getprivs();
  rv = fchown(fd, secure_uid, group);
  dropprivs();
  if (rv==-1) return -1;

  /* Chmod it */
  return fchmod(fd, mode);
}
#endif /* ndef HAVE_GRANTPT */

int  xselect(int  n,  fd_set  *readfds,  fd_set  *writefds,
	     fd_set *exceptfds, struct timeval *timeout)
{
  return select(n, readfds, writefds, exceptfds, timeout);
}

/* Master loop */
void 
loop_on(fd, ring_size, eof_char)
     int fd;
     size_t ring_size;
     const char eof_char;
{
  ring_t* in_ring;
  ring_t* out_ring;
  fd_set readable_set;
  fd_set writable_set;
  int maxfd;
  int activefds;
  size_t space;
  int pty_flags;
  int eof_pty;
  int eof_stdin;
  /**/

  /* Prepare the rings */
  in_ring       = ring_construct(ring_size);
  out_ring      = ring_construct(ring_size);

  /* Biggest fd */
  maxfd = fd > 2 ? fd+1 : 3;

  /* Save the file descriptor flags for all the fds we'll manipulate */
  if ((state_saved_stdin_flags=fcntl(0, F_GETFL))==-1)
    {
      fprintf(stderr, "%s: F_GETFL on stdin: %s\n", 
	      progname, strerror(errno));
      exit(255);
    }
  if ((state_saved_stdout_flags=fcntl(1, F_GETFL))==-1)
    {
      fprintf(stderr, "%s: F_GETFL on stdout: %s\n", 
	      progname, strerror(errno));
      exit(255);
    }
  if ((pty_flags=fcntl(fd, F_GETFL))==-1)
    {
      fprintf(stderr, "%s: F_GETFL on pty master: %s\n", 
	      progname, strerror(errno));
      exit(255);
    }

  /* Set these descriptors to non blocking mode */
  if (fcntl(0, F_SETFL, state_saved_stdin_flags | O_NONBLOCK)==-1) {
    fprintf(stderr, "%s: F_SETFL on stdin: %s\n", 
	    progname, strerror(errno));
    exit(255);
  }
  if (fcntl(1, F_SETFL, state_saved_stdout_flags | O_NONBLOCK)==-1) {
    fprintf(stderr, "%s: F_SETFL on stdout: %s\n", 
	    progname, strerror(errno));
    exit(255);
  }
  if (fcntl(fd, F_SETFL, pty_flags | O_NONBLOCK)==-1) {
    fprintf(stderr, "%s: F_SETFL on pty master: %s\n", 
	    progname, strerror(errno));
    exit(255);
  }

  /* Initially, all our fds are open */
  eof_pty   = 0;
  eof_stdin = 0;
  
  /* Master loop */
  while(1) 
    {	 
      /* Determine the ring states and set the fds to look at*/
      FD_ZERO(&readable_set);
      FD_ZERO(&writable_set);
      space = ring_space(in_ring);
      if (space>0 && !eof_stdin)                  
	FD_SET(0,  &readable_set);
      if (space<ring_size && ! state_child_exited) 
	FD_SET(fd, &writable_set);
      space = ring_space(out_ring);
      if (space>0 && !eof_pty) 
	FD_SET(fd, &readable_set);
      if (space<ring_size)     
	FD_SET(1,  &writable_set);

      /* If child has exited and the output ring is empty, bye bye */
      if (state_child_exited && eof_pty && space==ring_size) break;
      
      activefds = xselect(maxfd, &readable_set, &writable_set, NULL, NULL);
      switch (activefds) 
	{
	case -1:
	  switch(errno) 
	    {
	    case EINTR:
	      /* Signal occured, ok */
	      break;
	    default:
	      fprintf(stderr, "%s: select(): %s\n", progname, strerror(errno));
	      exit(255);
	    }
	  break;
	case 0:
	  fprintf(stderr, "%s: select() returned 0 fds ready\n", progname);
	  exit(255);
	default:
	  /* Some descriptors active */
	  /* Try to write stuff outside first to make room */
	  if (FD_ISSET(1,  &writable_set))
	    {
	      switch(ring_write(out_ring, 1))
		{
		case 0:
		  fprintf(stderr, "%s: write() on stdout returned 0\n",
			  progname);
		  exit(255);
		case -1:
		  fprintf(stderr, "%s: write() on stdout: %s\n",
			  progname, strerror(errno));
		  exit(255);
		}
	    }
	  if (FD_ISSET(fd, &writable_set))
	    {
	      switch(ring_write(in_ring, fd))
		{
		case 0:
		  fprintf(stderr, "%s: write() on pty master returned 0\n",
			  progname);
		  exit(255);
		case -1:
		  fprintf(stderr, "%s: write() on pty master: %s\n",
			  progname, strerror(errno));
		  exit(255);
		}
	    }
	  
	  /* Try to read more stuff */
	  if (FD_ISSET(0,  &readable_set))
	    {
	      switch (ring_read(in_ring,  0))
		{
		case 0:
		  ring_push_char(in_ring, eof_char);
		  eof_stdin = 1;
		  break;
		case -1:
		  fprintf(stderr, "%s: read() on stdin: %s\n",
			  progname, strerror(errno));
		  exit(255);
		}
	    }
	  if (FD_ISSET(fd, &readable_set)) 
	    {
	      switch(ring_read(out_ring, fd))
		{
		case 0:
		  eof_pty = 1;
		  break;
		case -1:
		  switch (errno) 
		    {
		    case EIO:
		      /* The tty was closed */
		      eof_pty=1;
		      break;
		    default:
		      fprintf(stderr, "%s: read() on pty master: %s\n",
			      progname, strerror(errno));
		      exit(255);
		    }
		  break;
		}
	    }
	}
    }

  /* Restore the descriptors modes */
  if (fcntl(0, F_SETFL, state_saved_stdin_flags)==-1) {
    fprintf(stderr, "%s: F_SETFL on stdin: %s\n", 
	    progname, strerror(errno));
    exit(255);
  }
  state_saved_stdin_flags=-1;
  if (fcntl(1, F_SETFL, state_saved_stdout_flags)==-1) {
    fprintf(stderr, "%s: F_SETFL on stdout: %s\n", 
	    progname, strerror(errno));
    exit(255);
  }
  state_saved_stdout_flags=-1;
}

#ifndef HAVE_GRANTPT
/* Drop setuid privileges */
void
dropprivs() 
{
  if (ispriv()) {
    if (seteuid(secure_uid)==-1) 
      {
	fprintf(stderr, "%s: while dropping privileges: seteuid(%d): %s",
		progname, (int)secure_uid, strerror(errno));
	exit(255);
      }
    if (setegid(secure_gid)==-1) 
      {
	fprintf(stderr, "%s: while dropping privileges: setegid(%d): %s",
		progname, (int)secure_gid, strerror(errno));
	exit(255);
      }
  }
}

/* Get setuid privileges */
void
getprivs() 
{
  if (ispriv()) {
    if (seteuid(unsecure_uid)==-1) 
      {
	fprintf(stderr, "%s: while getting privileges: seteuid(%d): %s",
		progname, (int)unsecure_uid, strerror(errno));
	exit(255);
      }
    if (setegid(unsecure_gid)==-1) 
      {
	fprintf(stderr, "%s: while setting privileges: setegid(%d): %s",
		progname, (int)unsecure_gid, strerror(errno));
	exit(255);
      }
  }
}

/* Are we a privileged (setuid) process */
int
ispriv() 
{
  return secure_uid != unsecure_uid || secure_gid != unsecure_gid;
}
#endif /* ndef HAVE_GRANTPT */

/* Xmalloc, allocates memory and abort if something goes bad */
void*
xmalloc(size) 
     size_t size;
{
  void* ptr;
  /**/
  
  ptr = malloc(size);
  if (ptr==NULL) {
    fprintf(stderr, "%s: not enough memory\n", progname);
    exit(255);
  }
  return ptr;
}

/* Xstrdup, makes a copy of a string */
char*
xstrdup(const char* s)
{
  char* ptr;
  /**/

  ptr = xmalloc(strlen(s)+1);
  strcpy(ptr, s);
  return ptr;
}

/* "Private" ring structure */
struct ring_st {
  void* start;
  void* end;
  void* head;
  void* tail;
  size_t space;
};


/* Ring check */
#ifndef NDEBUG
void
ring_check(self)
     ring_t *self;
{
  int size = self->end - self->start;
  assert(self->start <= self->head && self->head < self->end);
  assert(self->start <= self->tail && self->tail < self->end);
  if (self->tail==self->head)
    assert(self->space == 0 || self->space == size);
  else if (self->tail > self->head)
    assert(self->space == size - (self->tail - self->head));
  else
    assert(self->space == self->head - self->tail);
}
#endif

/* Ring allocation */
ring_t*
ring_construct(size)
     size_t size;
{
  ring_t* self;
  self  = xmalloc(sizeof(ring_t));
  self->start = xmalloc(size);
  self->end   = self->start + size;
  self->head  = self->tail = self->start;
  self->space  = size;

  RING_CHECK(self);
  return self;
}

/* Ring deallocation */
void
ring_delete(self)
     ring_t *self;
{
  RING_CHECK(self);
  free(self->start);
  free(self);
}

/* Return free space in ring */
size_t
ring_space(self)
     ring_t *self;
{
  RING_CHECK(self);
  return self->space;
}

/* Read the biggest possible block in a ring from a fd */
int
ring_read(self, fd)
     ring_t *self;
     int fd;
{
  int readcount=0;
  /**/

  if (self->space) 
    {
      size_t chunk;
      /**/

      /* Determine largest readable chunk */
      if (self->tail >= self->head) 
	{
	  chunk = self->end - self->tail;
	}
      else 
	{
	  chunk = self->head - self->tail;
	}

      /* Read the chunk */
      readcount = read(fd, self->tail, chunk);
      if (readcount>0)
	{
	  self->tail  += readcount;
	  self->space -= readcount;
	  if (self->tail == self->end) 
	    {
	      self->tail = self->start;
	    }
	}

    }

  RING_CHECK(self);
  return readcount;
}

/* Write the biggest possible block in a ring to a fd */
int
ring_write(self, fd)
     ring_t *self;
     int fd;
{
  int writtencount=0;
  /**/

  if (self->space != self->end - self->start) 
    {
      size_t chunk;
      /**/

      /* Determine largest writable chunk */
      if (self->head >= self->tail)
	{
	  chunk = self->end - self->head;
	}
      else 
	{
	  chunk = self->tail - self->head;
	}

      /* Write the chunk */
      writtencount = write(fd, self->head, chunk);
      if (writtencount>0) 
	{
	  self->head  += writtencount;
	  self->space += writtencount;
	  if (self->head == self->end)
	    {
	      self->head = self->start;
	    }
	}
    }

  RING_CHECK(self);
  return writtencount;
}

/* Push a characted in a ring */
int
ring_push_char(self, c)
     ring_t* self;
     const char c;
{
  int writtencount=0;
  /**/

  if (self->space != 0) 
    {
      /* Write the byte for output */
      writtencount = 1;
      *(char*)self->tail = c;
      self->tail  += 1;
      self->space -= 1;
      if (self->tail == self->end)
	{
	  self->tail = self->start;
	}
    }

  RING_CHECK(self);
  return writtencount;
}

/* Cleanup everything */
void 
cleanup()
{
  /* Restore our state */
  if (state_saved_stdin_flags!=-1) 
    {
      fcntl(0, F_SETFL, state_saved_stdin_flags);
    }
  if (state_saved_stdout_flags!=-1) 
    {
      fcntl(1, F_SETFL, state_saved_stdout_flags); 
    }
  if (state_tty_name)
    {
      restoreperms(state_tty_name, &state_tty_perms);
    }

  /* Restore terminal */
  term_restore(0);
}

/* Pass into terminal raw mode and save the old mode */
int
term_raw(verbose)
     int verbose;
{
  struct termios tios;
  sigset_t blockedset;
  sigset_t oldset;
  /**/

  /* Block all signals */
  sigfillset(&blockedset);
  sigprocmask(SIG_SETMASK, &blockedset, &oldset);

  /* Change the terminal */
  if (tcgetattr(0, &tios)==-1) 
    {
      if (verbose) 
	{
	  fprintf(stderr, "%s: tcgetattr on current terminal: %s\n",
		  progname, strerror(errno));
	  exit(255);
	}
      return -1;
    }
  if (state_orig_termios==NULL) 
    {
      state_orig_termios  = xmalloc(sizeof(struct termios));
      *state_orig_termios = tios;
    }
  tios.c_lflag &= ~(ICANON|ECHO);
  if (tcsetattr(0, TCSAFLUSH, &tios)==-1) 
    {
      if  (verbose) 
	{
	  fprintf(stderr, "%s: tcsetattr on current terminal: %s\n",
		  progname, strerror(errno));
	  exit(255);
	}
      return -1;
    }

  /* Restore signals */
  sigprocmask(SIG_SETMASK, &oldset, NULL);
  return 0;
}

/* Restore terminal state */
int
term_restore(verbose)
     int verbose;
{
  if (state_orig_termios!=NULL)
    {
      sigset_t blockedset;
      sigset_t oldset;
      /**/
      
      /* Block all signals */
      sigfillset(&blockedset);
      sigprocmask(SIG_SETMASK, &blockedset, &oldset);

      /* Restore the terminal */
      if (tcsetattr(0, TCSAFLUSH, state_orig_termios)==-1) 
	{
	  if  (verbose) 
	    {
	      fprintf(stderr, "%s: tcsetattr on current terminal: %s\n",
		      progname, strerror(errno));
	      exit(255);
	    }
	  return -1;
	}
      free(state_orig_termios);
      state_orig_termios=0;

      /* Restore signals */
      sigprocmask(SIG_SETMASK, &oldset, NULL);
    }
  return 0;
}

/* Communicate window size to child */
int
term_winsize(verbose)
     int verbose;
{
  if (state_pty_fd) 
    {
      struct winsize ws;
      if (opt_geometry_height == 0 && opt_geometry_width == 0)
	{
	  if (ioctl(0, TIOCGWINSZ, &ws)==-1)
	    {
	      if (verbose)
		{
		  fprintf(stderr, "%s: TIOCGWINSZ on stdin: %s\n",
			  progname, strerror(errno));
		  exit(255);
		}
	      return -1;
	    }
	}
      else
	{
	  ws.ws_row = opt_geometry_height;
	  ws.ws_col = opt_geometry_width;
	  ws.ws_xpixel = 0;
	  ws.ws_ypixel = 0;
	}
      if (ioctl(state_pty_fd, TIOCSWINSZ, &ws)==-1)
	{
	  if (verbose)
	    {
	      fprintf(stderr, "%s: TIOCSWINSZ on pty slave: %s\n",
		      progname, strerror(errno));
	      exit(255);
	    }
	  return -1;
	}
    }
  return 0;
}

/* SIGCHLD handler */
void
sig_chld_h(sig)
     int sig;
{
  sig=0;
  state_child_exited = 1;
}

/* all fatal signals handler */
void
sig_fatal_h(sig)
     int sig;
{
  struct sigaction sa;
  /**/

  /* Do the cleanup */
  cleanup();

  /* Disarm this signal handler */
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_NODEFER;
  sigaction(sig, &sa, NULL);

  /* Raise the signal */
  raise(sig);
}

/* Terminal stop */
void
sig_tstp_h(sig)
     int sig;
{
  struct sigaction sa;
  struct sigaction oldsa;
  /**/

  /* Restore terminal */
  if (term_restore(0)==-1) _exit(255);
  
  /* Warn child */
  kill(state_child_pid, sig);

  /* Restore default signal bindings */
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &sa, &oldsa);

  /* Raise a new TSTP */
  raise(SIGTSTP);

  /* Restore old action */
  sigaction(SIGTSTP, &oldsa, NULL);
  
}

/* Continuation */
void
sig_cont_h(sig)
     int sig;
{
  if (term_raw(0)==-1) _exit(255);
  kill(state_child_pid, SIGWINCH);
}

/* Window size change */
void
sig_winch_h(sig)
     int sig;
{
  if (term_winsize(0)!=-1) 
    {
      kill(state_child_pid, sig);
    }
}
