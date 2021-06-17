// Filename: config_fmodAudio.cxx
// Created by:  cort
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

#include "pandabase.h"
#ifdef HAVE_FMODEX //[

#include "config_fmodAudio.h"
#include "audioManager.h"
#include "fmodAudioManager.h"
#include "fmodAudioSound.h"
#include "pandaSystem.h"
#include "dconfig.h"

ConfigureDef(config_fmodAudio);
NotifyCategoryDef(fmodAudio, ":audio");

ConfigureFn(config_fmodAudio) {
  init_libFmodAudio();
}

////////////////////////////////////////////////////////////////////
//     Function: init_libFmodAudio
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libFmodAudio() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  initialized = true;
  FmodAudioManager::init_type();
  FmodAudioSound::init_type();

  PandaSystem *ps = PandaSystem::get_global_ptr();
  ps->add_system("FMOD");
  ps->add_system("audio");
  ps->set_system_tag("audio", "implementation", "FMOD");
}

////////////////////////////////////////////////////////////////////
//     Function: get_audio_manager_func_fmod_audio
//  Description: This function is called when the dynamic library is
//               loaded; it should return the Create_AudioManager
//               function appropriate to create a FmodAudioManager.
///////////////////////////////////////////////////////////////////
Create_AudioManager_proc *
get_audio_manager_func_fmod_audio() {
  init_libFmodAudio();
  return &Create_FmodAudioManager;
}

#endif //]
