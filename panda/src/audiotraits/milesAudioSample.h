// Filename: milesAudioSample.h
// Created by:  skyler (June 6, 2001)
// Prior system by: cary
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

#ifndef MILESAUDIOSAMPLE_H
#define MILESAUDIOSAMPLE_H

#include "pandabase.h"
#ifdef HAVE_RAD_MSS //[

#include "milesAudioSound.h"
#include "milesAudioManager.h"
#include "mss.h"

////////////////////////////////////////////////////////////////////
//       Class : MilesAudioSample
// Description : A sound file, such as a WAV or MP3 file, that is
//               preloaded into memory and played from memory.
////////////////////////////////////////////////////////////////////
class EXPCL_MILES_AUDIO MilesAudioSample : public MilesAudioSound {
private:
  MilesAudioSample(MilesAudioManager *manager, 
                   MilesAudioManager::SoundData *sd,
                   const string &file_name);

public:
  virtual ~MilesAudioSample();
  
  virtual void play();
  virtual void stop();
  
  virtual float get_time() const;
  
  virtual void set_volume(float volume=1.0f);
  virtual void set_balance(float balance_right=0.0f);
  virtual void set_play_rate(float play_rate=1.0f);
  
  virtual float length() const;

  virtual AudioSound::SoundStatus status() const;

  virtual void cleanup();
  virtual void output(ostream &out) const;

  // 3D spatialized sound support.
  // Spatialized sound was originally added for FMOD, so there are parts of the
  // interface in the Miles implementation that are a little more awkward than
  // they would be otherwise.
  void set_3d_attributes(float px, float py, float pz, float vx, float vy, float vz);
  void get_3d_attributes(float *px, float *py, float *pz, float *vx, float *vy, float *vz);
  void set_3d_min_distance(float dist);
  float get_3d_min_distance() const;
  void set_3d_max_distance(float dist);
  float get_3d_max_distance() const;

  virtual float get_speaker_level(int index);
  virtual void set_speaker_levels(float level1, float level2=-1.0f, float level3=-1.0f, float level4=-1.0f, float level5=-1.0f, float level6=-1.0f, float level7=-1.0f, float level8=-1.0f, float level9=-1.0f);

private:
  void internal_stop();
  static void AILCALLBACK finish_callback(HSAMPLE sample);
  void do_set_time(float time);

  PT(MilesAudioManager::SoundData) _sd;
  HSAMPLE _sample;
  size_t _sample_index;
  S32 _original_playback_rate;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    MilesAudioSound::init_type();
    register_type(_type_handle, "MilesAudioSample",
                  MilesAudioSound::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class GlobalMilesManager;
  friend class MilesAudioManager;
};

#include "milesAudioSample.I"

#endif //]

#endif  /* MILESAUDIOSAMPLE_H */
