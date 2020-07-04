/*============================================================================
  
  lbc 
  
  programcontext.h

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#pragma once

#include <klib/klib.h>

struct _ProgramContext;
typedef struct _ProgramContext ProgramContext;

BEGIN_DECLS

extern ProgramContext *program_context_new (void);
extern void            program_context_destroy (ProgramContext *self);

/** Checks the command-line and RC-file settings for validity, and sets up
 * any properties that are derived from them. This function returns TRUE
 * if it is safe for the program to proceed. If it returns FALSE, it must
 * alredy have alerted the user. */
extern BOOL program_context_check_and_resolve (ProgramContext *self);

/** program_context_parse_command_line should called after 
  program_context_read_rc_files, in order for command-line values to overwrite
  rc file values (if they have corresponding names)

  This method just process the command line, and turn the arguments into
  either context properties (program_context_put) on simply into values of
  attributes in the ProgramContext structure itself. Using properties is
  probably best, as it allows command-line arguments to override the proerties
  read from RC files. i
  
  In either case, it needs to be clear to the main program how the
  command-line arguments have been translated in context data. 

  This method should only return TRUE if the intention is to proceed to run
  the rest of the program. Command-line switches that have the effect of
  terminating (like --help) should be handled internally, and FALSE returned. 
*/
extern BOOL      program_context_parse_command_line 
                   (ProgramContext *self, int argc, char **argv);

/** Gets a stored property as a UTF8 string. The caller must free this
    value, if it is not null. */
extern char     *program_context_get (const ProgramContext *self, 
                   const char *key);

extern BOOL      program_context_get_boolean (const ProgramContext *self, 
                   const char *key, BOOL deflt);

extern int       program_context_get_integer (const ProgramContext *self, 
                   const char *key, int deflt);

extern int       program_context_get_nonswitch_argc 
                    (const ProgramContext *self);

extern char    **program_context_get_nonswitch_argv 
                    (const ProgramContext *self);

extern void      program_context_put (ProgramContext *self, const char *name, 
                   const char *value);

extern void      program_context_put_boolean (ProgramContext *self, 
                   const char *name, BOOL value);

extern void      program_context_put_integer (ProgramContext *self, 
                   const char *name, int value);

extern void      program_context_read_rc_files (ProgramContext *self);
extern void      program_context_read_system_rc_file (ProgramContext *self);
extern void      program_context_read_user_rc_file (ProgramContext *self);


END_DECLS

