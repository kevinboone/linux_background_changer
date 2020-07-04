/*============================================================================
  
  lbc 
  
  changer.h

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#pragma once

#include <klib/klib.h>

/** Define the numeric values of the specific changer methods. */
typedef enum
  {
  SBM_XVIEW = 0, SBM_GNOME2 = 1, SBM_GNOMESHELL = 2, SBM_XFCE4 = 3, SBM_CMD = 4
  } SetBackgroundMethod;

/** Define the numeric values of the aspect ratio filters. */
typedef enum
  {
  ASPECT_ANY = 0, ASPECT_LANDSCAPE = 1, ASPECT_PORTRAIT = 2 
  } Aspect;

struct _Changer;
typedef struct _Changer Changer;

extern Changer   *changer_new (const KList *file_list, int interval,
                    SetBackgroundMethod method, BOOL dual, const char *cmd);

extern void       changer_destroy (Changer *self);

/** Print the enabled changer methods to the specified stream, one per line. */
extern void       changer_dump_methods (FILE *f);

/** Switch to the next image. */
extern void       changer_next (Changer *self);

/** Switch to the previous image. */
extern void       changer_prev (Changer *self);

/** Run the changer loop. This method ends only when a SIGINT is caught. */
extern void       changer_run (Changer *self);

/** Get the numeric value corresponding to the specified changer name,
    or -1 if there is not one. */
extern SetBackgroundMethod changer_get_method (const char *method);

