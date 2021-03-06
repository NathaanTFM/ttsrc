// Filename: pandagles.cxx
// Created by:  pro-rsoft (8Jun09)
//
////////////////////////////////////////////////////////////////////

#include "pandagles.h"

#define OPENGLES_1
#include "config_glesgsg.h"

#ifdef HAVE_EGL
#include "config_egldisplay.h"
#include "eglGraphicsPipe.h"
#endif

// By including checkPandaVersion.h, we guarantee that runtime
// attempts to load libpandagles.so/.dll will fail if they inadvertently
// link with the wrong version of libdtool.so/.dll.

#include "checkPandaVersion.h"

////////////////////////////////////////////////////////////////////
//     Function: init_libpandagles
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libpandagles() {
  init_libglesgsg();

#ifdef HAVE_EGL
  init_libegldisplay();
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: get_pipe_type_pandagles
//  Description: Returns the TypeHandle index of the recommended
//               graphics pipe type defined by this module.
////////////////////////////////////////////////////////////////////
int
get_pipe_type_pandagles() {
#ifdef HAVE_EGL
  return eglGraphicsPipe::get_class_type().get_index();
#endif

  return 0;
}
