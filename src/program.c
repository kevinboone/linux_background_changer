/*============================================================================
  
  lbc 
  
  program.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <stdio.h> 
#include <time.h> 
#include <stdlib.h> 
#include <string.h> 
#include <errno.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <assert.h> 
#include <sys/file.h> 
#include <signal.h> 
#include <klib/klib.h> 
#include "program_context.h" 
#include "program.h" 
#include "changer.h" 

/*============================================================================
  
  Global wossnames

  ==========================================================================*/
int lock_fd = -1; // Handle of lock file

#define KLOG_CLASS "lbc.program"

#define HAS_OPTION(x) program_context_get_boolean(context,x,FALSE)
#define GET_INTEGER(x,y) program_context_get_integer(context,x,y)
#define GET(x) program_context_get(context,x)

void program_log_handler (KLogLevel level, const char *cls, 
                  void *user_data, const char *msg); //FWD
static BOOL program_consider_path (const ProgramContext *context, 
         const KPath *path, KList *file_list); // FWD
static BOOL program_remove_lock (void); // FWD

#define DEFAULT_MAX_FILES 1000 
#define DEFAULT_INTERVAL  120

/*============================================================================
  
  program_build_file_list

  ==========================================================================*/
static BOOL program_build_file_list (const ProgramContext *context,
         KList *file_list)
  {
  KLOG_IN
  int ret = TRUE;
  int max_files = GET_INTEGER ("max-files", DEFAULT_MAX_FILES);
  klog_set_handler (program_log_handler);

  // First check specific entries in --dirs
  char *c_dirs = GET ("dirs");
  if (c_dirs)
    {
    klog_debug (KLOG_CLASS, "Processing --dirs: %s", c_dirs);
    KString *dirs = kstring_new_from_utf8 ((UTF8 *)c_dirs);

    KList *dirlist = kstring_tokenize_utf8 (dirs, (UTF8 *)":");

    int l = klist_length (dirlist);
    for (int i = 0; i < l; i++)
      {
      KString *s = klist_get (dirlist, i);
      klog_debug (KLOG_CLASS, "Processing entry in --dirs: %S", 
        kstring_cstr(s));
      KPath *path = kpath_new_from_kstring (s);
      program_consider_path (context, path, file_list);
      kpath_destroy (path);
      }

    klist_destroy (dirlist);

    kstring_destroy (dirs);
    free (c_dirs);
    }

  // Don't carry on checking if the file list is already full
  int l = klist_length (file_list);
  if (l < max_files - 1)
    {
    int argc = program_context_get_nonswitch_argc (context);
    char **argv = program_context_get_nonswitch_argv (context);
    for (int i = 1; i < argc; i++)
      {
      KPath *path = kpath_new_from_utf8 ((UTF8 *)argv[i]);
      program_consider_path (context, path, file_list);
      kpath_destroy (path);
      }
    }
  else
    klog_debug (KLOG_CLASS, 
      "Not checking command-line paths because file list is already full");

  srand (time (NULL));
  klist_shuffle (file_list); // TODO

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_consider_file

  ==========================================================================*/
static BOOL program_consider_file (const ProgramContext *context, 
        const KPath *path)
  {
  KLOG_IN
  BOOL ret = FALSE;
  char *filename = (char *)kstring_to_utf8 ((KString *)path);
  if (strstr (filename, "thumbnail") == NULL)
    {
    static UTF32 jpg[] = {'j','p','g',0};
    static UTF32 jpeg[] = {'j','p','e','g',0};
    static UTF32 JPG[] = {'J','P','G',0};
    static UTF32 JPEG[] = {'J','P','E','G',0};
    static UTF32 png[] = {'p','n','g',0};
    static UTF32 PNG[] = {'P','N','G',0};
    static UTF32 gif[] = {'g','i','f',0};
    static UTF32 GIF[] = {'G','I','F',0};

    int min_width = GET_INTEGER ("width", -1);
    int min_height = GET_INTEGER ("height", -1);
    int aspect_mode = GET_INTEGER ("aspect-mode", ASPECT_ANY);

    klog_debug (KLOG_CLASS, "Considering file %s", filename); 

    int width = -1;
    int height = -1;

    BOOL is_image = FALSE;

    KString *ext = kpath_get_ext (path);
    if (kstring_strcmp_utf32 (kstring_cstr(ext), jpg) == 0
	|| kstring_strcmp_utf32 (kstring_cstr(ext), JPG) == 0)
      {
      int components;
      if (jpegreader_get_image_size (filename, &height,
	   &width, &components))
	 {
	 is_image = TRUE;
	 klog_debug (KLOG_CLASS, "width=%d", width);
	 klog_debug (KLOG_CLASS, "height=%d", height);
	 }
      }
    else if (kstring_strcmp_utf32 (kstring_cstr(ext), jpeg) == 0
	|| kstring_strcmp_utf32 (kstring_cstr(ext), JPEG) == 0)
      {
      is_image = TRUE;
      }
    else if (kstring_strcmp_utf32 (kstring_cstr(ext), png) == 0
	|| kstring_strcmp_utf32 (kstring_cstr(ext), PNG) == 0)
      {
      is_image = TRUE;
      }
    else if (kstring_strcmp_utf32 (kstring_cstr(ext), gif) == 0
	|| kstring_strcmp_utf32 (kstring_cstr(ext), GIF) == 0)
      {
      is_image = TRUE;
      }

    if (is_image)
      {
      if (width >= min_width || min_width == -1 || width == -1)
	{
	if (height >= min_height || min_height == -1 || height == -1)
	  {
	  if (height == 0) height = 1; // Should never happen, but avoid / by 0
	  double aspect = (double)width / (double)height;
	  if ((aspect_mode == ASPECT_LANDSCAPE && aspect >= 1)
	       || (aspect_mode == ASPECT_PORTRAIT && aspect < 1)
	       || (aspect_mode == ASPECT_ANY))
	    {
	    ret = TRUE;
	    }
	  else
	    {
	    klog_debug (KLOG_CLASS, "Image %s has wrong aspect ratio", filename); 
	    }
	  }
	else
	  {
	  klog_debug (KLOG_CLASS, "Image %s is not tall enough", filename); 
	  }
	}
      else
	{
	klog_debug (KLOG_CLASS, "Image %s is not wide enough", filename); 
	}
      }

    kstring_destroy (ext);
    }
  else
    klog_debug (KLOG_CLASS, "Image %s is a thumbnail", filename); 
    
  free (filename);
  KLOG_OUT
  return ret;
  }


/*============================================================================
  
  program_consider_path

  ==========================================================================*/
static BOOL program_consider_path (const ProgramContext *context, 
         const KPath *path, KList *file_list)
  {
  KLOG_IN

  int max_files = GET_INTEGER ("max-files", DEFAULT_MAX_FILES);
  BOOL ret = TRUE;
  int l = klist_length (file_list);
  if (l < max_files) 
    {
    klog_debug (KLOG_CLASS, "Considering path: %S", 
    kstring_cstr ((KString *)path));
  
    KPathType t = kpath_get_type (path);
    if (t == KPT_REG)
      {
      if (program_consider_file (context, path))
        klist_append (file_list, kpath_clone (path));
      }
    else if (t == KPT_DIR)
      {
      KList *list = kpath_expand (path, 0);
      if (list)
	{
	BOOL stop = FALSE;
	int l = klist_length (list);
	for (int i = 0; i < l && !stop; i++)
	  {
	  KPath *file = klist_get (list, i);
	  if (!program_consider_path (context, file, file_list))
	    stop = TRUE;
	  }
	klist_destroy (list);
	if (stop) ret = FALSE;
	}
      else
	{
	klog_error (KLOG_CLASS, "Can't expand directory: %S",
	  kstring_cstr ((KString *)path));
	}
      }
    else
      {
      klog_error (KLOG_CLASS, "Path is neither a file nor a directory: %S",
	kstring_cstr ((KString *)path));
      }
    }
  else
    {
    ret = FALSE; // file_list too big
    }

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_get_lock_filename

  ==========================================================================*/
static char *program_get_lock_filename (void)
  {
  KLOG_IN
  KPath *path = kpath_new_home();
  kpath_append_utf8 (path, (UTF8 *)".lbc.pid");
  char *ret = (char *)kpath_to_utf8 (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_get_lock

  ==========================================================================*/
static BOOL program_get_lock (void)
  {
  KLOG_IN
  BOOL ret = FALSE;
  char *fname = program_get_lock_filename ();

  if ((lock_fd = open(fname, O_WRONLY|O_CREAT, 0666)) == -1)
    return FALSE;

  //if(fcntl(fdlock, F_SETLK, &fl) == -1)
  //  return FALSE;

  if (flock (lock_fd, LOCK_EX | LOCK_NB) == 0) 
    {
    // Write our PID to the lock file
    int pid = getpid();
    char s[50];
    sprintf (s, "%d\n", pid);
    write (lock_fd, s, strlen(s));
    ret = TRUE;
    }
  else
    ret = FALSE;

  free (fname);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_get_pid

  ==========================================================================*/
static pid_t program_get_pid (void)
  {
  KLOG_IN
  BOOL pid = 0;

  if (program_get_lock())
    {
    // program_get_lock _should_ fail at this point. If we can get a lock,
    //  then there isn't a running program to signal
    program_remove_lock();
    }
  else
    {
    char *fname = program_get_lock_filename ();
    FILE *f = fopen (fname, "r");
    if (f)
      {
      fscanf (f, "%d", &pid);
      fclose(f);
      klog_debug (KLOG_CLASS, "PID from lockfile %s is %d",
       fname, pid);
      }
    else
      klog_warn (KLOG_CLASS, "Could not get PID from lockfile %s. Strange.",
       fname);
    free (fname);
    }

  KLOG_OUT
  return pid;
  }


/*============================================================================
  
  program_log_handler

  ==========================================================================*/
void program_log_handler (KLogLevel level, const char *cls, 
                  void *user_data, const char *msg)
  {
  if (level == KLOG_ERROR)
    fprintf (stderr, "%s: %s\n", NAME, msg);
  else
    fprintf (stderr, "%s %s: %s\n", klog_level_to_utf8 (level), 
      cls, msg);
  }

/*============================================================================
  
  program_next

  ==========================================================================*/
static void program_next (const ProgramContext *context)
  {
  KLOG_IN

  int pid = program_get_pid();
  if (pid != 0)
    {
    kill (pid, SIGUSR1);
    }
  else
    {
    klog_error (KLOG_CLASS, "Could not git PID of lbc program. Is it running?");
    }

  KLOG_OUT
  }

/*============================================================================
  
  program_prev

  ==========================================================================*/
static void program_prev (const ProgramContext *context)
  {
  KLOG_IN

  int pid = program_get_pid();
  if (pid != 0)
    {
    kill (pid, SIGUSR2);
    }
  else
    {
    klog_error (KLOG_CLASS, "Could not git PID of lbc program. Is it running?");
    }

  KLOG_OUT
  }

/*============================================================================
  
  program_stop

  ==========================================================================*/
static void program_stop (const ProgramContext *context)
  {
  KLOG_IN

  int pid = program_get_pid();
  if (pid != 0)
    {
    kill (pid, SIGINT);
    }
  else
    {
    klog_error (KLOG_CLASS, "Could not git PID of lbc program. Is it running?");
    }

  KLOG_OUT
  }

/*============================================================================
  
  program_remove_lock

  ==========================================================================*/
static BOOL program_remove_lock (void)
  {
  KLOG_IN
  BOOL ret = FALSE;
  if (lock_fd != -1) close (lock_fd);
  char *fname = program_get_lock_filename ();
  unlink (fname);
  free (fname);
  KLOG_OUT
  return ret;
  }



/*============================================================================
  
  program_run 

  ==========================================================================*/
int program_run (const ProgramContext *context)
  {
  KLOG_IN
  int ret = 0;
  klog_set_handler (program_log_handler);

  BOOL cont = TRUE;

  if (HAS_OPTION ("next"))
    {
    program_next (context); 
    cont = FALSE;
    }

  if (HAS_OPTION ("prev"))
    {
    program_prev (context); 
    cont = FALSE;
    }

  if (HAS_OPTION ("stop"))
    {
    program_stop (context); 
    cont = FALSE;
    }

  if (cont)
    {
    if (program_get_lock())
      {
      int max_files = GET_INTEGER ("max-files", DEFAULT_MAX_FILES);
      KList *file_list = klist_new_empty ((KListFreeFn) kpath_destroy);
      int interval = GET_INTEGER ("interval", DEFAULT_INTERVAL);
      SetBackgroundMethod method = GET_INTEGER ("method-i", SBM_GNOMESHELL);

      if (program_build_file_list (context, file_list))
	{
	int l = klist_length (file_list);
	if (l >= max_files - 1)
	  klog_warn (KLOG_CLASS, "File count reached limit of %d", max_files);
	if (l > 0)
	  {
	  klog_info (KLOG_CLASS, "Found %d suitable file(s)", l);
          if (!HAS_OPTION ("foreground"))
            {
            // Note that we need to remove the lock and reacquire it.
            // The lock is not acquired by the spawned child process.
            // I'm not going to worry about the infinitessimal risk of
            //   somebody starting a second instances between these
            //   two lines of code.
            program_remove_lock();
            daemon (0, 0);
            program_get_lock();
            }
	  BOOL dual = HAS_OPTION ("dual");
          char *cmd = GET ("cmd");
	  Changer *changer = changer_new (file_list, interval, method, dual, cmd);
	  changer_run (changer);
          if (cmd) free (cmd);
	  changer_destroy (changer);
	  }
	else
	  klog_error (KLOG_CLASS, 
	    "No matching files found (check your directories and inclusion criteria)");
	}
      else
	{
	// Error will already have been displayed
	ret = EINVAL;
	}

      klist_destroy (file_list);
      program_remove_lock();
      }
    else
      {
      char *fname = program_get_lock_filename ();
      
      klog_error (KLOG_CLASS, "Can't lock '%s'", fname); 
      klog_error (KLOG_CLASS, 
	"If you're sure the program isn't already running, delete this file."); 

      free (fname);
      }
    }

  KLOG_OUT
  return ret;
  }



