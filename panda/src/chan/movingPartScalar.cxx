// Filename: movingPartScalar.cxx
// Created by:  drose (23Feb99)
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


#include "movingPartScalar.h"
#include "animChannelScalarDynamic.h"
#include "datagram.h"
#include "datagramIterator.h"
#include "bamReader.h"
#include "bamWriter.h"

// Tell GCC that we'll take care of the instantiation explicitly here.
#ifdef __GNUC__
#pragma implementation
#endif

TypeHandle MovingPartScalar::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: MovingPartScalar::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
MovingPartScalar::
~MovingPartScalar() {
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartScalar::get_blend_value
//       Access: Public
//  Description: Attempts to blend the various scalar values
//               indicated, and sets the _value member to the
//               resulting scalar.
////////////////////////////////////////////////////////////////////
void MovingPartScalar::
get_blend_value(const PartBundle *root) {
  // If a forced channel is set on this particular scalar, we always
  // return that value instead of performing the blend.  Furthermore,
  // the frame number is always 0 for the forced channel.
  if (_forced_channel != (AnimChannelBase *)NULL) {
    ChannelType *channel = DCAST(ChannelType, _forced_channel);
    channel->get_value(0, _value);
    return;
  }

  PartBundle::CDReader cdata(root->_cycler);

  if (cdata->_blend.empty()) {
    // No channel is bound; supply the default value.
    if (restore_initial_pose) {
      _value = _default_value;
    }

  } else if (_effective_control != (AnimControl *)NULL &&
             !cdata->_frame_blend_flag) {
    // A single value, the normal case.
    ChannelType *channel = DCAST(ChannelType, _effective_channel);
    channel->get_value(_effective_control->get_frame(), _value);

  } else {
    // A blend of two or more values.
    _value = 0.0f;
    float net = 0.0f;

    PartBundle::ChannelBlend::const_iterator cbi;
    for (cbi = cdata->_blend.begin(); cbi != cdata->_blend.end(); ++cbi) {
      AnimControl *control = (*cbi).first;
      float effect = (*cbi).second;
      nassertv(effect != 0.0f);

      ChannelType *channel = NULL;
      int channel_index = control->get_channel_index();
      if (channel_index >= 0 && channel_index < (int)_channels.size()) {
        channel = DCAST(ChannelType, _channels[channel_index]);
      }
      if (channel != NULL) {
        ValueType v;
        channel->get_value(control->get_frame(), v);
        
        if (!cdata->_frame_blend_flag) {
          // Hold the current frame until the next one is ready.
          _value += v * effect;
        } else {
          // Blend between successive frames.
          float frac = (float)control->get_frac();
          _value += v * (effect * (1.0f - frac));

          channel->get_value(control->get_next_frame(), v);
          _value += v * (effect * frac);
        }
        net += effect;
      }
    }

    if (net == 0.0f) {
      if (restore_initial_pose) {
        _value = _default_value;
      }

    } else {
      _value /= net;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartScalar::apply_freeze_scalar
//       Access: Public, Virtual
//  Description: Freezes this particular joint so that it will always
//               hold the specified transform.  Returns true if this
//               is a joint that can be so frozen, false otherwise.
//               This is called internally by
//               PartBundle::freeze_joint().
////////////////////////////////////////////////////////////////////
bool MovingPartScalar::
apply_freeze_scalar(float value) {
  _forced_channel = new AnimChannelFixed<ACScalarSwitchType>(get_name(), value);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartScalar::apply_control
//       Access: Public, Virtual
//  Description: Specifies a node to influence this particular joint
//               so that it will always hold the node's transform.
//               Returns true if this is a joint that can be so
//               controlled, false otherwise.  This is called
//               internally by PartBundle::control_joint().
////////////////////////////////////////////////////////////////////
bool MovingPartScalar::
apply_control(PandaNode *node) {
  AnimChannelScalarDynamic *chan = new AnimChannelScalarDynamic(get_name());
  chan->set_value_node(node);
  _forced_channel = chan;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartScalar::make_MovingPartScalar
//       Access: Protected
//  Description: Factory method to generate a MovingPartScalar object
////////////////////////////////////////////////////////////////////
TypedWritable* MovingPartScalar::
make_MovingPartScalar(const FactoryParams &params) {
  MovingPartScalar *me = new MovingPartScalar;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartScalar::register_with_factory
//       Access: Public, Static
//  Description: Factory method to generate a MovingPartScalar object
////////////////////////////////////////////////////////////////////
void MovingPartScalar::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_MovingPartScalar);
}

