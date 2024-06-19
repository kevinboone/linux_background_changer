/*============================================================================
  
  lbc 
  
  programcontext.c

  Copyright (c)1990-2024 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <assert.h> 
#include <stdio.h> 
#include <time.h> 
#include <stdlib.h> 
#include <klib/klib.h> 
#include <string.h> 
#include <getopt.h> 
#include "program_context.h" 
#include "changer.h" 

#define KLOG_CLASS "lbc.program_context"

void program_context_show_usage (FILE *f, const char *argv0); // FWD

#define PCP program_context_put
#define PCG program_context_get
#define PCPB program_context_put_boolean
#define PCGB program_context_get_boolean
#define PCPI program_context_put_integer
#define PCGI program_context_get_integer

/*============================================================================
  
  ProgramContext  

  ==========================================================================*/
struct _ProgramContext
  {
  KProps *props;
  int nonswitch_argc;
  char **nonswitch_argv;
  };

/*============================================================================
  
  program_context_create

  ==========================================================================*/
ProgramContext *program_context_new (void)
  {
  KLOG_IN
  ProgramContext *self = malloc (sizeof (ProgramContext));
  memset (self, 0, sizeof (ProgramContext));
  self->props = kprops_new_empty();
  KLOG_OUT
  return self;
  }

/*============================================================================
  
  program_context_destroy

  ==========================================================================*/
void program_context_destroy (ProgramContext *self)
  {
  KLOG_IN
  if (self)
    {
    if (self->props) kprops_destroy (self->props);
    for (int i = 0; i < self->nonswitch_argc; i++)
      free (self->nonswitch_argv[i]);
    free (self->nonswitch_argv);
    free (self);
    }
  KLOG_OUT
  }

/*============================================================================
  
  program_context_check_and_resolve

  ==========================================================================*/
BOOL program_context_check_and_resolve (ProgramContext *context)
  {
  KLOG_IN
  BOOL ret = TRUE;

  char *method = PCG (context, "method");
  if (method)
    {
    if (strcmp (method, "help") == 0)
      {
      printf ("Supported methods: \n");
      changer_dump_methods (stdout); 
      ret = FALSE;
      }
    else
      {
      SetBackgroundMethod m = changer_get_method (method);
      if (m == -1)
         {
         klog_error (KLOG_CLASS, "Unknown method '%s'. '--method help' for a list",
           method);
         ret = FALSE;
         } 
      else
        PCPI (context, "method-i", m);
      if (m == SBM_CMD)
        {
        char *cmd = PCG (context, "cmd");
        if (cmd)
          {
          free (cmd);
          }
        else
          {
          klog_error (KLOG_CLASS, "'--method cmd' requires '--cmd {program}'");
          ret = FALSE;
          }
        }
      }
    free (method);
    }

  if (ret)
    {
    char *aspect = PCG (context, "aspect");
    if (aspect)
      {
      if (strcmp (aspect, "landscape") == 0)
        PCPI (context, "aspect-mode", ASPECT_LANDSCAPE);
      else if (strcmp (aspect, "portrait") == 0)
        PCPI (context, "aspect-mode", ASPECT_PORTRAIT);
      else if (strcmp (aspect, "any") == 0)
        PCPI (context, "aspect-mode", ASPECT_ANY);
      else
        {
	klog_error (KLOG_CLASS, 
	   "'aspect' must be 'landscape', 'portrait', or 'any'");
        ret = FALSE;
	}
      free (aspect);
      }
    else
      PCPI (context, "aspect-mode", ASPECT_ANY);
    }

  if (PCGB (context, "dual", FALSE))
    {
    char *method = PCG (context, "method");
    if (method)
      {
      SetBackgroundMethod m = changer_get_method (method);
      if (m != SBM_XFCE4 && m != SBM_CMD)
        klog_warn (KLOG_CLASS, 
          "LBC dual-monitor mode is not comaptible with chosen changer method");
      }
    else
      {
      klog_warn (KLOG_CLASS, 
        "LBC dual-monitor mode is not comaptible with chosen changer method");
      }
    }

  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get

  Caller must free the string if it is non-null

  ========================================================================*/
char *program_context_get (const ProgramContext *self, 
                   const char *key)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  const KString *val = kprops_get_utf8 (self->props, (UTF8 *)key);
  char *ret = NULL;
  if (val) ret = (char *)kstring_to_utf8 (val);
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get_boolean

  ========================================================================*/
BOOL program_context_get_boolean (const ProgramContext *self, 
    const char *key, BOOL deflt)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  BOOL ret = kprops_get_boolean_utf8 (self->props, (UTF8 *)key, deflt);
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get_integer

  ========================================================================*/
BOOL program_context_get_integer (const ProgramContext *self, 
    const char *key, BOOL deflt)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  BOOL ret = kprops_get_integer_utf8 (self->props, (UTF8 *)key, deflt);
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get_nonswitch_argc

  ========================================================================*/
int program_context_get_nonswitch_argc (const ProgramContext *self)
  {
  KLOG_IN
  assert (self != NULL);
  int ret = self->nonswitch_argc; 
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get_nonswitch_argv

  ========================================================================*/
char **program_context_get_nonswitch_argv (const ProgramContext *self)
  {
  KLOG_IN
  assert (self != NULL);
  char **ret = self->nonswitch_argv; 
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_context_parse_command_line

  ==========================================================================*/
BOOL program_context_parse_command_line (ProgramContext *self, 
        int argc, char **argv)
  {
  KLOG_IN
  BOOL ret = TRUE;
  static struct option long_options[] =
    {
      {"aspect", required_argument, NULL, 'a'},
      {"dirs", required_argument, NULL, 'd'},
      {"cmd", required_argument, NULL, 'c'},
      {"dual", no_argument, NULL, 0},
      {"foreground", no_argument, NULL, 'f'},
      {"help", no_argument, NULL, 0},
      {"log-level", required_argument, NULL, 0},
      {"max-files", required_argument, NULL, 0},
      {"method", required_argument, NULL, 'm'},
      {"interval", required_argument, NULL, 'i'},
      {"version", no_argument, NULL, 'v'},
      {"prev", no_argument, NULL, 'p'},
      {"next", no_argument, NULL, 'n'},
      {"stop", no_argument, NULL, 's'},
      {"width", required_argument, NULL, 'w'},
      {"height", required_argument, NULL, 'h'},
      {0, 0, 0, 0}
    };

   int opt;
   while (ret)
     {
     int option_index = 0;
     opt = getopt_long (argc, argv, "a:vd:fm:npw:h:i:s", long_options, 
            &option_index);

     if (opt == -1) break;

     switch (opt)
       {
       case 0:
         if (strcmp (long_options[option_index].name, "log-level") == 0)
           PCPI (self, "log-level", atoi (optarg));
         else if (strcmp (long_options[option_index].name, "help") == 0)
          PCPB (self, "show-usage", TRUE); 
         else if (strcmp (long_options[option_index].name, "dual") == 0)
          PCPB (self, "dual", TRUE); 
         else if (strcmp (long_options[option_index].name, "max-files") == 0)
          PCPI (self, "max-files", atoi(optarg)); 
         else
           exit (-1);
         break;
       case '?': 
         PCPB (self, "show-usage", TRUE); break;
       case 'a': PCP (self, "aspect", optarg); break;
       case 'c': PCP (self, "cmd", optarg); break;
       case 'd': PCP (self, "dirs", optarg); break;
       case 'f': PCPB (self, "foreground", TRUE); break;
       case 'w': PCP (self, "width", optarg); break;
       case 'h': PCP (self, "height", optarg); break;
       case 'i': PCPI (self, "interval", atoi(optarg)); break;
       case 'n': PCPB (self, "next", TRUE); break;
       case 'p': PCPB (self, "prev", TRUE); break;
       case 'm': PCP (self, "method", optarg); break;
       case 's': PCPB (self, "stop", TRUE); break;
       case 'v': PCPB (self, "show-version", TRUE); break;
       default:
         ret = FALSE; 
       }
    }

  if (ret)
    {
    self->nonswitch_argc = argc - optind + 1;
    self->nonswitch_argv = malloc (self->nonswitch_argc * sizeof (char *));
    self->nonswitch_argv[0] = strdup (argv[0]);
    int j = 1;
    for (int i = optind; i < argc; i++)
      {
      self->nonswitch_argv[j] = strdup (argv[i]);
      j++;
      }
    }

  if (PCGB (self, "show-version", FALSE))
    {
    printf ("%s: %s version %s\n", argv[0], NAME, VERSION);
    printf ("Copyright (c)2024 Kevin Boone\n");
    printf ("Distributed under the terms of the GPL v3.0\n");
    ret = FALSE;
    }

   if (PCGB (self, "show-usage", FALSE))
    {
    program_context_show_usage (stdout, argv[0]);
    ret = FALSE;
    }

  KLOG_OUT
  return ret;  
  }

/*============================================================================
  
  program_context_put

  ==========================================================================*/
void program_context_put (ProgramContext *self, const char *name, 
       const char *value)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  KString *temp = kstring_new_from_utf8 ((UTF8 *)value);
  kprops_add_utf8 (self->props, (UTF8 *)name, temp);
  kstring_destroy (temp);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_put_boolean

  ==========================================================================*/
void program_context_put_boolean (ProgramContext *self, const char *name, 
       BOOL value)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  kprops_put_boolean_utf8 (self->props, (UTF8 *)name, value);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_put_integer

  ==========================================================================*/
void program_context_put_integer (ProgramContext *self, const char *name, 
       int value)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  kprops_put_integer_utf8 (self->props, (UTF8 *)name, value);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_rc_file

  ==========================================================================*/
void program_context_read_rc_file (ProgramContext *self, const KPath *path)
  {
  KLOG_IN
  klog_debug (KLOG_CLASS, "Reading RC file %S", 
     kstring_cstr ((KString *)path));

  kprops_from_file (self->props, path);

  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_rc_files

  ==========================================================================*/
void program_context_read_rc_files (ProgramContext *self)
  {
  KLOG_IN
  program_context_read_system_rc_file (self);
  program_context_read_user_rc_file (self);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_system_rc_file

  // TODO Windows. 

  ==========================================================================*/
void program_context_read_system_rc_file (ProgramContext *self)
  {
  KLOG_IN
  KPath *path = kpath_new_from_utf8 ((UTF8*) ("/etc/" NAME ".rc"));
  klog_debug (KLOG_CLASS, 
     "System RC file is %S", kstring_cstr ((KString *)path));
  program_context_read_rc_file (self, path);
  kpath_destroy (path);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_user_rc_file
  
  ==========================================================================*/
void program_context_read_user_rc_file (ProgramContext *self)
  {
  KLOG_IN
  KPath *path = kpath_new_home();
  kpath_append_utf8 (path, (UTF8 *) ("." NAME ".rc"));
  klog_debug (KLOG_CLASS, 
     "User RC file is %S", kstring_cstr ((KString *)path));
  program_context_read_rc_file (self, path);
  kpath_destroy (path);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_show_usage
  
  ==========================================================================*/
void program_context_show_usage (FILE *fout, const char *argv0)
  {
  KLOG_IN
  fprintf (fout, "Usage: %s [options]\n", argv0);
  fprintf (fout, "  -a,--aspect=landscape|portrait|any\n" 
                 "                           aspect ratio filter (any)\n");
  fprintf (fout, 
      "     --dual                different images on each screen\n");
  fprintf (fout, "  -c,--command             command to run; use with '-m cmd'\n");
  fprintf (fout, "  -d,--dirs                colon-separated directory list\n");
  fprintf (fout, "     --help                show this message\n");
  fprintf (fout, "  -f,--foregound           run in foreground\n");
  fprintf (fout, "  -h,--height=[N]          minimum height (none)\n");
  fprintf (fout, 
                 "  -i,--interval=[N]        seconds between changes (120)\n");
  fprintf (fout, "     --log-level=[0..4]    log level (1)\n");
  fprintf (fout, "     --max-files=[N]       maxium files (1000)\n");
  fprintf (fout, "  -m,--method=[name,help]  set changing method\n");
  fprintf (fout, "  -n,--next                next background\n");
  fprintf (fout, "  -p,--prev                previous background\n");
  fprintf (fout, "  -s,--stop                stop the program\n");
  fprintf (fout, "  -v,--version             show version\n");
  fprintf (fout, "  -w,--width=[N]           minimum width (none)\n");
  KLOG_OUT
  }



