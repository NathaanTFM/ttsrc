// Filename: virtualMouse.cxx
// Created by:  drose (12Mar02)
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

#include "virtualMouse.h"
#include "dataNodeTransmit.h"

TypeHandle VirtualMouse::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: VirtualMouse::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
VirtualMouse::
VirtualMouse(const string &name) :
  DataNode(name)
{
  _pixel_xy_output = define_output("pixel_xy", EventStoreVec2::get_class_type());
  _pixel_size_output = define_output("pixel_size", EventStoreVec2::get_class_type());
  _xy_output = define_output("xy", EventStoreVec2::get_class_type());
  _button_events_output = define_output("button_events", ButtonEventList::get_class_type());

  _pixel_xy = new EventStoreVec2(LPoint2f(0.0f, 0.0f));
  _pixel_size = new EventStoreVec2(LPoint2f(0.0f, 0.0f));
  _xy = new EventStoreVec2(LPoint2f(0.0f, 0.0f));
  _button_events = new ButtonEventList;
  _next_button_events = new ButtonEventList;

  _mouse_x = 0;
  _mouse_y = 0;
  _win_width = 100;
  _win_height = 100;
  _mouse_on = false;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualMouse::set_mouse_pos
//       Access: Published
//  Description: Sets the current mouse pixel location, where (0,0) is
//               the upper left, and (width-1, height-1) is the lower
//               right pixel of the virtual window.
////////////////////////////////////////////////////////////////////
void VirtualMouse::
set_mouse_pos(int x, int y) {
  _mouse_x = x;
  _mouse_y = y;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualMouse::set_window_size
//       Access: Published
//  Description: Sets the size of the "window" in which the mouse
//               rolls.  This changes the meaning of the values passed
//               to set_mouse_pos().
////////////////////////////////////////////////////////////////////
void VirtualMouse::
set_window_size(int width, int height) {
  _win_width = width;
  _win_height = height;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualMouse::set_mouse_on
//       Access: Published
//  Description: Sets whether the mouse should appear to be within the
//               window or not.  If this is true, the mouse is within
//               the window; if false, the mouse is not within the
//               window (and set_mouse_pos() means nothing).
////////////////////////////////////////////////////////////////////
void VirtualMouse::
set_mouse_on(bool flag) {
  _mouse_on = flag;
}
  
////////////////////////////////////////////////////////////////////
//     Function: VirtualMouse::press_button
//       Access: Published
//  Description: Simulates a mouse or keyboard button being depressed.
//               This should be followed up by a call to
//               release_button() sometime later (possibly
//               immediately).
////////////////////////////////////////////////////////////////////
void VirtualMouse::
press_button(ButtonHandle button) {
  _next_button_events->add_event(ButtonEvent(button, ButtonEvent::T_down));
}
  
////////////////////////////////////////////////////////////////////
//     Function: VirtualMouse::release_button
//       Access: Published
//  Description: Simulates the button being released.  This should
//               follow a previous call to press_button().
////////////////////////////////////////////////////////////////////
void VirtualMouse::
release_button(ButtonHandle button) {
  _next_button_events->add_event(ButtonEvent(button, ButtonEvent::T_up));
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualMouse::do_transmit_data
//       Access: Protected, Virtual
//  Description: The virtual implementation of transmit_data().  This
//               function receives an array of input parameters and
//               should generate an array of output parameters.  The
//               input parameters may be accessed with the index
//               numbers returned by the define_input() calls that
//               were made earlier (presumably in the constructor);
//               likewise, the output parameters should be set with
//               the index numbers returned by the define_output()
//               calls.
////////////////////////////////////////////////////////////////////
void VirtualMouse::
do_transmit_data(DataGraphTraverser *, const DataNodeTransmit &,
                 DataNodeTransmit &output) {
  // Swap in the button events, and clear them for next time.
  PT(ButtonEventList) events = _button_events;
  _button_events = _next_button_events;
  _next_button_events = events;
  _next_button_events->clear();
  output.set_data(_button_events_output, EventParameter(_button_events));

  _pixel_size->set_value(LPoint2f(_win_width, _win_height));
  output.set_data(_pixel_size_output, EventParameter(_pixel_size));

  if (_mouse_on) {
    // The mouse is within the window.
    _pixel_xy->set_value(LPoint2f(_mouse_x, _mouse_y));
    output.set_data(_pixel_xy_output, EventParameter(_pixel_xy));

    // Normalize pixel motion to range [-1,1].
    float xf = (2.0f * (float)_mouse_x) / (float)_win_width - 1.0f;
    float yf = 1.0f - (2.0f * (float)_mouse_y) / (float)_win_height;
    _xy->set_value(LPoint2f(xf, yf));
    output.set_data(_xy_output, EventParameter(_xy));
  }
}
