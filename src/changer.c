/*============================================================================
  
  lbc 

  changer.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#define _GNU_SOURCE
#include <stdio.h> 
#include <time.h> 
#include <stdlib.h> 
#include <string.h> 
#include <errno.h> 
#include <unistd.h> 
#include <signal.h> 
#include <assert.h> 
#include <klib/klib.h> 
#include "changer.h" 

#define KLOG_CLASS "lbc.changer"

static void changer_show_current_images (Changer *self); // FWD
static void changer_method_gnome2 (const Changer *self); //FWD
static void changer_method_gnome_shell (const Changer *self); //FWD
static void changer_method_xfce4 (const Changer *self); //FWD
static void changer_method_xview (const Changer *self); //FWD
static void changer_method_cmd (const Changer *self); //FWD

/*============================================================================
  
  Changer 

  ==========================================================================*/
struct _Changer
  {
  // Note that Changer never owns the file list, and should not
  //  modify it free it
  const KList *file_list;
  int pos;
  int interval;
  SetBackgroundMethod method;
  BOOL dual;
  const char *cmd;
  };

/*============================================================================
  
  Change methods 

  ==========================================================================*/
typedef void (*ChangerFn) (const Changer *self);

typedef struct _ChangeMethod ChangeMethod;
struct _ChangeMethod
  {
const char *name;
  const char *desc;
  ChangerFn fn;
  };

// NOTE NOTE NOTE
// The ordering of these entries must match the values of the SBM_XXX
//   constants
static ChangeMethod methods[] =
  {
  {"xview", "set image on X root window using xview", changer_method_xview},
  {"gnome2", "Gnome 2 gconftool-2 method", changer_method_gnome2},
  {"gnome-shell", "Gnome 3 gettings method", changer_method_gnome_shell},
  {"xfce4", "Xfce4 desktop method", changer_method_xfce4},
  {"cmd", "User-defined command", changer_method_cmd},
  {NULL, NULL, NULL}
  };


/*============================================================================
  
  changer_new

  ==========================================================================*/
Changer *changer_new (const KList *file_list, int interval, 
            SetBackgroundMethod method, BOOL dual, const char *cmd)
  {
  KLOG_IN

  Changer *self = malloc (sizeof (Changer));
  self->file_list = file_list;
  self->pos = 0;
  self->interval = interval;
  self->method = method;
  self->dual = dual;
  self->cmd = cmd;

  KLOG_OUT
  return self;
  }

/*============================================================================
  
  changer_destroy

  ==========================================================================*/
void changer_destroy (Changer *self)
  {
  KLOG_IN
  if (self)
    {
    free (self);
    }
  KLOG_OUT
  }

/*============================================================================
  
  changer_dump_methods

  ==========================================================================*/
void changer_dump_methods (FILE *f)
  {
  KLOG_IN
  ChangeMethod *method = &methods[0];
  while (method->name)
    {
    fprintf (f, "%s: %s\n", method->name, method->desc); 
    method++;
    }
  KLOG_OUT
  }


/*============================================================================
  
  changer_get_images_per_cycle

  ==========================================================================*/
static int changer_get_images_per_cycle (Changer *self)
  {
  KLOG_IN
  int ret = 1;
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  changer_get_method

  ==========================================================================*/
extern SetBackgroundMethod changer_get_method (const char *method)
  {
  KLOG_IN
  SetBackgroundMethod ret = -1;
  ChangeMethod *m = &methods[0];
  int i = 0;
  while (m->name && ret == -1)
    {
    if (strcmp (m->name, method) == 0) ret = i;
    i++;
    m++;
    }
  KLOG_OUT
  return ret;
  }


/*============================================================================
  
  changer_get_nth_image_pos

  ==========================================================================*/
static int changer_get_nth_image_pos (const Changer *self, int n)
  {
  KLOG_IN
  int ret = (self->pos + n) % klist_length (self->file_list);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  changer_get_nth_image

  ==========================================================================*/
static const KPath *changer_get_nth_image (const Changer *self, int n)
  {
  KLOG_IN
  assert (self != NULL);
  int index = changer_get_nth_image_pos (self, n);
  const KPath *ret = klist_get (self->file_list, index);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  changer_method_cmd

  ==========================================================================*/
void changer_method_cmd (const Changer *self)
  {
  KLOG_IN
  assert (self != NULL);
  klog_debug (KLOG_CLASS, "Change using user command");
 
  char *command;
  if (self->dual)
    {
    const KPath *img_path1 = changer_get_nth_image (self, 0); 
    const KPath *img_path2 = changer_get_nth_image (self, 1); 
    char *filename1 = (char *)kpath_to_utf8 (img_path1);
    char *filename2 = (char *)kpath_to_utf8 (img_path2);
    asprintf (&command, "\"%s\" \"%s\" \"%s\"", self->cmd, filename1, 
          filename2);
    free (filename2);
    free (filename1);
    }
  else
    {
    const KPath *img_path = changer_get_nth_image (self, 0); 
    char *filename = (char *)kpath_to_utf8 (img_path);
    asprintf (&command, "\"%s\" \"%s\"", self->cmd, filename);
    free (filename);
    }
  
  klog_debug (KLOG_CLASS, "Command='%s'", command);
  system (command);
  free (command);

  KLOG_OUT
  }

/*============================================================================
  
  changer_method_gnome2

  ==========================================================================*/
void changer_method_gnome2 (const Changer *self)
  {
  KLOG_IN
  assert (self != NULL);
  klog_debug (KLOG_CLASS, "Change using gnome2 method");
 
  const KPath *img_path = changer_get_nth_image (self, 0); 

  char *filename = (char *)kpath_to_utf8 (img_path);

  char *cmd;
  asprintf (&cmd, "gconftool-2 --set --type=string /desktop/gnome/background/picture_filename \"%s\"",
       filename);

  if (system (cmd) != 0)
    {
    klog_error (KLOG_CLASS, "Error executing command '%s'", cmd);
    }

  free (cmd);
  free (filename);
  KLOG_OUT
  }

/*============================================================================
  
  changer_method_gnome-shell

  ==========================================================================*/
void changer_method_gnome_shell (const Changer *self)
  {
  KLOG_IN
  assert (self != NULL);
  klog_debug (KLOG_CLASS, "Change using gnome-shell method");

  const KPath *img_path = changer_get_nth_image (self, 0); 

  char *filename = (char *)kpath_to_utf8 (img_path);

  char *cmd;
  asprintf (&cmd, "GSETTINGS_BACKEND=dconf gsettings"
        " set org.gnome.desktop.background picture-uri \"file://%s\"",
       filename);

  if (system (cmd) != 0)
    {
    klog_error (KLOG_CLASS, "Error executing command '%s'", cmd);
    }

  free (cmd);
  free (filename);
 
  KLOG_OUT
  }

/*============================================================================
  
  changer_method_xfce4

  ==========================================================================*/
void changer_method_xfce4 (const Changer *self)
  {
  KLOG_IN
  assert (self != NULL);
  klog_debug (KLOG_CLASS, "Change using xfce4 method");
 
  char *cmd;
  if (self->dual)
    {
    const KPath *img_path1 = changer_get_nth_image (self, 0); 
    const KPath *img_path2 = changer_get_nth_image (self, 1); 

    char *filename1 = (char *)kpath_to_utf8 (img_path1);
    char *filename2 = (char *)kpath_to_utf8 (img_path2);

    asprintf (&cmd, "xfconf-query -c xfce4-desktop --list | grep last-image|grep screen0|while read path; do xfconf-query -c xfce4-desktop -p $path --set \"%s\"; done; xfconf-query -c xfce4-desktop --list | grep last-image|grep screen0|while read path; do xfconf-query -c xfce4-desktop -p $path --set \"%s\"; done", 
       filename1, filename2);

    free (filename1);
    free (filename2);
    }
  else
    {
    const KPath *img_path1 = changer_get_nth_image (self, 0); 

    char *filename1 = (char *)kpath_to_utf8 (img_path1);

    asprintf (&cmd, "xfconf-query -c xfce4-desktop --list | grep last-image|grep screen0|while read path; do xfconf-query -c xfce4-desktop -p $path --set \"%s\"; done", 
      filename1);

    free (filename1);
    }

  if (system (cmd) != 0)
    {
    klog_error (KLOG_CLASS, "Error executing command '%s'", cmd);
    }

  free (cmd);
  KLOG_OUT
  }


/*============================================================================
  
  changer_method_xview

  ==========================================================================*/
void changer_method_xview (const Changer *self)
  {
  KLOG_IN
  assert (self != NULL);
  klog_debug (KLOG_CLASS, "Change using xview method");
 
  const KPath *img_path = changer_get_nth_image (self, 0); 

  char *filename = (char *)kpath_to_utf8 (img_path);
  printf ("image = %s\n", filename);

  free (filename);
  KLOG_OUT
  }


/*============================================================================
  
  changer_move_forward

  ==========================================================================*/
static void changer_move_forward (Changer *self)
  {
  KLOG_IN

  self->pos += changer_get_images_per_cycle (self);
  self->pos %= klist_length (self->file_list);

  KLOG_OUT
  }


/*============================================================================
  
  changer_move_back

  ==========================================================================*/
static void changer_move_back (Changer *self)
  {
  KLOG_IN

  self->pos -= changer_get_images_per_cycle (self);
  if (self->pos < 0) 
     self->pos = klist_length (self->file_list) 
       + self->pos; 

  KLOG_OUT
  }


/*============================================================================
  
  changer_next

  ==========================================================================*/
void changer_next (Changer *self)
  {
  KLOG_IN
  klog_info (KLOG_CLASS, "Selecting next image(s)");
  changer_move_forward (self);
  changer_show_current_images (self);
  KLOG_OUT
  }

/*============================================================================
  
  changer_prev

  ==========================================================================*/
void changer_prev (Changer *self)
  {
  KLOG_IN
  klog_info (KLOG_CLASS, "Selecting previous image(s)");
  changer_move_back (self);
  changer_show_current_images (self);
  KLOG_OUT
  }

/*============================================================================
  
  changer_run 

  ==========================================================================*/
void changer_run (Changer *self)
  {
  KLOG_IN

  sigset_t base_mask, waiting_mask;

  sigemptyset (&base_mask);
  sigaddset (&base_mask, SIGINT);
  sigaddset (&base_mask, SIGHUP);
  sigaddset (&base_mask, SIGUSR1);
  sigaddset (&base_mask, SIGUSR2);
  sigprocmask (SIG_SETMASK, &base_mask, NULL);

  changer_show_current_images (self);

  BOOL quit = FALSE;
  int ticks = 0;
  while (!quit)
    {
    // Note that some Unix-like systems don't handle usleep() with
    //   very large values, hence the loop
    for (int i = 0; i < 10; i++)
      usleep(100000);
    sigpending (&waiting_mask);
    if (sigismember (&waiting_mask, SIGINT) ||
        sigismember (&waiting_mask, SIGUSR1) ||
        sigismember (&waiting_mask, SIGUSR2) ||
        sigismember (&waiting_mask, SIGHUP))
      {
      int sig;
      sigwait (&waiting_mask, &sig);
      switch (sig)
        {
	case SIGINT:
	  klog_info (KLOG_CLASS, "Caught interrupt signal");
	  quit = TRUE;
	  break;
	case SIGUSR1:
	  klog_debug (KLOG_CLASS, "Caught USR1 signal");
          changer_next (self);
	  ticks = 0;
	  break;
	case SIGUSR2:
	  klog_debug (KLOG_CLASS, "Caught USR2 signal");
          changer_prev (self);
	  ticks = 0;
	  break;
	}
      }
    else
      {
      // usleep actually terminated normally, without a signal --
      //  check the timeout and advance if necessary
      if (ticks >= self->interval) 
        {
        changer_next (self);
	ticks = 0;
	}
      }
    ticks++;
    }

  KLOG_OUT
  }


/*============================================================================
  
  changer_show_current_images

  ==========================================================================*/
static void changer_show_current_images (Changer *self)
  {
  KLOG_IN

  ChangerFn fn = methods[self->method].fn;
  assert (fn != NULL);

  fn (self);

  KLOG_OUT
  }







