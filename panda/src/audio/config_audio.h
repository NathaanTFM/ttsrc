// Filename: config_audio.h
// Created by:  cary (22Sep00)
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

#ifndef __CONFIG_AUDIO_H__
#define __CONFIG_AUDIO_H__

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableInt.h"
#include "configVariableBool.h"
#include "configVariableDouble.h"
#include "configVariableString.h"
#include "configVariableFilename.h"

#ifdef WIN32_VC
#pragma warning (disable : 4231)
#endif

NotifyCategoryDecl(audio, EXPCL_PANDA_AUDIO, EXPTP_PANDA_AUDIO);

extern EXPCL_PANDA_AUDIO ConfigVariableBool audio_active;
extern EXPCL_PANDA_AUDIO ConfigVariableInt audio_cache_limit;
extern EXPCL_PANDA_AUDIO ConfigVariableDouble audio_volume;
extern EXPCL_PANDA_AUDIO ConfigVariableFilename audio_dls_file;

//We Need This one.
extern EXPCL_PANDA_AUDIO ConfigVariableString audio_library_name;

// Config vars for Fmod:

extern EXPCL_PANDA_AUDIO ConfigVariableInt fmod_number_of_sound_channels;
extern EXPCL_PANDA_AUDIO ConfigVariableBool fmod_use_surround_sound;

// Config vars for OpenAL:

extern EXPCL_PANDA_AUDIO ConfigVariableDouble audio_doppler_factor; 
extern EXPCL_PANDA_AUDIO ConfigVariableDouble audio_distance_factor; 
extern EXPCL_PANDA_AUDIO ConfigVariableDouble audio_drop_off_factor; 
extern EXPCL_PANDA_AUDIO ConfigVariableDouble audio_buffering_seconds;
extern EXPCL_PANDA_AUDIO ConfigVariableInt    audio_preload_threshold;

// Config vars for Miles:

extern EXPCL_PANDA_AUDIO ConfigVariableBool audio_software_midi;
extern EXPCL_PANDA_AUDIO ConfigVariableFilename audio_dls_file;
extern EXPCL_PANDA_AUDIO ConfigVariableBool audio_play_midi;
extern EXPCL_PANDA_AUDIO ConfigVariableBool audio_play_wave;
extern EXPCL_PANDA_AUDIO ConfigVariableBool audio_play_mp3;
extern EXPCL_PANDA_AUDIO ConfigVariableInt audio_output_rate;
extern EXPCL_PANDA_AUDIO ConfigVariableInt audio_output_bits;
extern EXPCL_PANDA_AUDIO ConfigVariableInt audio_output_channels;



#ifdef NOTIFY_DEBUG //[
  // Non-release build:
  #define audio_debug(msg) \
  if (audio_cat.is_debug()) { \
    audio_cat->debug() << msg << endl; \
  } else {}
#else //][
  // Release build:
  #define audio_debug(msg) ((void)0);
#endif //]

#define audio_info(msg) \
  audio_cat->info() << msg << endl

#define audio_warning(msg) \
  audio_cat->warning() << msg << endl

#define audio_error(msg) \
  audio_cat->error() << msg << endl

#endif /* __CONFIG_AUDIO_H__ */
