// Filename: startup.h
// Created by:  drose (19Jun09)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#ifndef STARTUP_H
#define STARTUP_H

#include "nppanda3d_common.h"

#ifndef OSCALL
#define OSCALL
#endif

extern "C" {
#ifdef _WIN32
  NPError OSCALL NP_Initialize(NPNetscapeFuncs *browserFuncs);
#else
  NPError OSCALL NP_Initialize(NPNetscapeFuncs *browserFuncs,
                               NPPluginFuncs *pluginFuncs);
#endif

  char* NP_GetMIMEDescription(void);
  NPError NP_GetValue(void*, NPPVariable variable, void* value);
  NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *pluginFuncs);
  NPError OSCALL NP_Shutdown(void);
}

void request_ready(P3D_instance *instance);

#endif
