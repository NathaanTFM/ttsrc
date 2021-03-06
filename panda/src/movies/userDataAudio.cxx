// Filename: userDataAudio.cxx
// Created by: jyelon (02Jul07)
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

#include "userDataAudio.h"
#include "userDataAudioCursor.h"

TypeHandle UserDataAudio::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::Constructor
//       Access: Public
//  Description: This constructor returns a UserDataAudio --- 
//               a means to supply raw audio samples manually.
////////////////////////////////////////////////////////////////////
UserDataAudio::
UserDataAudio(int rate, int channels) :
  MovieAudio("User Data Audio"),
  _desired_rate(rate),
  _desired_channels(channels),
  _cursor(NULL),
  _aborted(false)
{
}

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
UserDataAudio::
~UserDataAudio() {
}

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::open
//       Access: Published, Virtual
//  Description: Open this audio, returning a UserDataAudioCursor.  A
//               UserDataAudio can only be opened by one consumer
//               at a time.
////////////////////////////////////////////////////////////////////
PT(MovieAudioCursor) UserDataAudio::
open() {
  if (_cursor) {
    nassert_raise("A UserDataAudio can only be opened by one consumer at a time.");
    return NULL;
  }
  _cursor = new UserDataAudioCursor(this);
  return _cursor;
}

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::read_samples
//       Access: Private
//  Description: Read audio samples from the stream.  N is the
//               number of samples you wish to read.  Your buffer
//               must be equal in size to N * channels.  
//               Multiple-channel audio will be interleaved. 
////////////////////////////////////////////////////////////////////
void UserDataAudio::
read_samples(int n, PN_int16 *data) {
  int ready = (_data.size() / _desired_channels);
  int desired = n * _desired_channels;
  int avail = ready * _desired_channels;
  if (avail > desired) avail = desired;
  for (int i=0; i<avail; i++) {
    data[i] = _data[i];
  }
  for (int i=avail; i<desired; i++) {
    data[i] = 0;
  }
  for (int i=0; i<avail; i++) {
    _data.pop_front();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::append
//       Access: Public
//  Description: Appends audio samples to the buffer.
////////////////////////////////////////////////////////////////////
void UserDataAudio::
append(PN_int16 *data, int n) {
  nassertv(!_aborted);
  int words = n * _desired_channels;
  for (int i=0; i<words; i++) {
    _data.push_back(data[i]);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::append
//       Access: Published
//  Description: Appends audio samples to the buffer from a 
//               datagram.  This is intended to make it easy to 
//               send streaming raw audio over a network.
////////////////////////////////////////////////////////////////////
void UserDataAudio::
append(DatagramIterator *src, int n) {
  nassertv(!_aborted);
  int maxlen = src->get_remaining_size() / (2 * _desired_channels);
  if (n > maxlen) n = maxlen;
  int words = n * _desired_channels;
  for (int i=0; i<words; i++) {
    _data.push_back(src->get_int16());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::append
//       Access: Published
//  Description: Appends audio samples to the buffer from a 
//               string.  The samples must be stored little-endian
//               in the string.  This is not particularly efficient,
//               but it may be convenient to deal with samples in
//               python.
////////////////////////////////////////////////////////////////////
void UserDataAudio::
append(const string &str) {
  nassertv(!_aborted);
  int samples = str.size() / (2 * _desired_channels);
  int words = samples * _desired_channels;
  for (int i=0; i<words; i++) {
    int c1 = ((unsigned char)str[i*2+0]);
    int c2 = ((unsigned char)str[i*2+1]);
    PN_int16 n = (c1 | (c2 << 8));
    _data.push_back(n);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: UserDataAudio::done
//       Access: Published
//  Description: Promises not to append any more samples, ie, this
//               marks the end of the audio stream.
////////////////////////////////////////////////////////////////////
void UserDataAudio::
done() {
  _aborted = true;
}
