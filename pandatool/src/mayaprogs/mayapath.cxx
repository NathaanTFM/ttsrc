// Filename: mayapath.cxx
// Created by:  drose (07Apr08)
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

// This program works as a stub to launch maya2egg, egg2maya, and
// similar programs that invoke OpenMaya and require certain
// environment variables to be set first.

#include "dtoolbase.h"
#include "filename.h"
#include "dSearchPath.h"
#include "executionEnvironment.h"
#include <stdlib.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

int 
main(int argc, char *argv[]) {
  // First, get the command line and append _bin, so we will actually
  // run maya2egg_bin.exe, egg2maya_bin.exe, etc.
  Filename command = Filename::from_os_specific(argv[0]);
  if (!command.is_fully_qualified()) {
    DSearchPath path;
    path.append_path(ExecutionEnvironment::get_environment_variable("PATH"));
#ifdef _WIN32
    command.set_extension("exe");
#endif
    command.resolve_filename(path);
  }

#ifdef _WIN32
  if (command.get_extension() == "exe") {
    command.set_extension("");
  }
#endif

  command = command.get_fullpath() + string("_bin");
#ifdef _WIN32
  command.set_extension("exe");
#endif
  string os_command = command.to_os_specific();


  // Now look up $MAYA_LOCATION.  We insist that it be set and
  // pointing to an actual Maya installation.
  Filename maya_location = Filename::expand_from("$MAYA_LOCATION");
  cerr << "MAYA_LOCATION: " << maya_location << endl;
  if (maya_location.empty()) {
    cerr << "$MAYA_LOCATION is not set!\n";
    exit(1);
  }
  if (!maya_location.is_directory()) {
    cerr << "The directory referred to by $MAYA_LOCATION does not exist!\n";
    exit(1);
  }

  // Reset maya_location to its full long name, since Maya seems to
  // require that.
  maya_location.make_canonical();
  maya_location = Filename::from_os_specific(maya_location.to_os_long_name());
  
  // Look for OpenMaya.dll as a sanity check.
#ifdef IS_OSX
  Filename openMaya = Filename::dso_filename(Filename(maya_location, "MacOS/libOpenMaya.dylib"));
  if (!openMaya.is_regular_file()) {
    cerr << "Could not find $MAYA_LOCATION/MacOS/" << openMaya.get_basename() << "!\n";
    exit(1); 
  }

#else  // IS_OSX
  Filename openMaya = Filename::dso_filename(Filename(maya_location, "bin/OpenMaya.so"));
  if (!openMaya.is_regular_file()) {
    cerr << "Could not find $MAYA_LOCATION/bin/" << Filename(openMaya.get_basename()).to_os_specific() << "!\n";
    exit(1);
  }
#endif

  // Re-set MAYA_LOCATION to its properly sanitized form.
  {
    string putenv_str = "MAYA_LOCATION=" + maya_location.to_os_specific();
    char *putenv_cstr = strdup(putenv_str.c_str());
    putenv(putenv_cstr);
  }

  // Now set PYTHONHOME & PYTHONPATH.  Maya2008 requires this to be
  // set and pointing within $MAYA_LOCATION, or it might get itself
  // confused with another Python installation (e.g. Panda's).
  Filename python = Filename(maya_location, "Python");
  if (python.is_directory()) {
    {
      string putenv_str = "PYTHONHOME=" + python.to_os_specific();
      char *putenv_cstr = strdup(putenv_str.c_str());
      putenv(putenv_cstr);
    }
    {
      string putenv_str = "PYTHONPATH=" + python.to_os_specific();
      char *putenv_cstr = strdup(putenv_str.c_str());
      putenv(putenv_cstr);
    }
  }

  // Also put the Maya bin directory on the PATH.
  Filename bin = Filename(maya_location, "bin");
  if (bin.is_directory()) {
    char *path = getenv("PATH");
    if (path == NULL) {
      path = "";
    }
#ifdef WIN32
    string sep = ";";
#else
    string sep = ":";
#endif
    string putenv_str = "PATH=" + bin.to_os_specific() + sep + path;
    char *putenv_cstr = strdup(putenv_str.c_str());
    putenv(putenv_cstr);
  }

#ifdef IS_OSX
  // And on DYLD_LIBRARY_PATH.
  if (bin.is_directory()) {
    char *path = getenv("DYLD_LIBRARY_PATH");
    if (path == NULL) {
      path = "";
    }
    string sep = ":";
    string putenv_str = "DYLD_LIBRARY_PATH=" + bin.to_os_specific() + sep + path;
    char *putenv_cstr = strdup(putenv_str.c_str());
    putenv(putenv_cstr);
  }

#elif !defined(_WIN32)
  // Linux (or other non-Windows OS) gets it added to LD_LIBRARY_PATH.
  if (bin.is_directory()) {
    char *path = getenv("LD_LIBRARY_PATH");
    if (path == NULL) {
      path = "";
    }
    string sep = ":";
    string putenv_str = "LD_LIBRARY_PATH=" + bin.to_os_specific() + sep + path;
    char *putenv_cstr = strdup(putenv_str.c_str());
    putenv(putenv_cstr);
  }

#endif // IS_OSX

  // When this is set, Panda3D will try not to use any functions from the
  // CPython API.  This is necessary because Maya links with its own copy
  // of Python, which may be incompatible with ours.
  putenv("PANDA_INCOMPATIBLE_PYTHON=1");

  // Now that we have set up the environment variables properly, chain
  // to the actual maya2egg_bin (or whichever) executable.

#ifdef _WIN32
  // Windows case.
  char *command_line = strdup(GetCommandLine());
  STARTUPINFO startup_info;
  PROCESS_INFORMATION process_info;
  GetStartupInfo(&startup_info);
  BOOL result = CreateProcess(os_command.c_str(),
                              command_line, 
                              NULL, NULL, true, 0,
                              NULL, NULL,
                              &startup_info,
                              &process_info);
  if (result) {
    WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 0;

    if (GetExitCodeProcess(process_info.hProcess, &exit_code)) {
      if (exit_code != 0) {
        cerr << "Program exited with status " << exit_code << "\n";
      }
    }

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    exit(exit_code);
  }
  cerr << "Couldn't execute " << command << ": " << GetLastError() << "\n";

#else
  // Unix case.
  execvp(os_command.c_str(), argv);
#endif

  // Couldn't execute for some reason.
  return 1;
}
