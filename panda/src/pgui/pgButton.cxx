// Filename: pgButton.cxx
// Created by:  drose (13Mar02)
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

#include "pgButton.h"
#include "pgMouseWatcherParameter.h"

#include "throw_event.h"
#include "mouseButton.h"
#include "mouseWatcherParameter.h"
#include "colorAttrib.h"
#include "transformState.h"

TypeHandle PGButton::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PGButton::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
PGButton::
PGButton(const string &name) : PGItem(name)
{
  _button_down = false;
  _click_buttons.insert(MouseButton::one());

  set_active(true);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
PGButton::
~PGButton() {
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PGButton::
PGButton(const PGButton &copy) :
  PGItem(copy),
  _click_buttons(copy._click_buttons)
{
  _button_down = false;
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::make_copy
//       Access: Public, Virtual
//  Description: Returns a newly-allocated Node that is a shallow copy
//               of this one.  It will be a different Node pointer,
//               but its internal data may or may not be shared with
//               that of the original Node.
////////////////////////////////////////////////////////////////////
PandaNode *PGButton::
make_copy() const {
  LightReMutexHolder holder(_lock);
  return new PGButton(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::enter_region
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               mouse enters the region.
////////////////////////////////////////////////////////////////////
void PGButton::
enter_region(const MouseWatcherParameter &param) {
  LightReMutexHolder holder(_lock);
  if (get_active()) {
    set_state(_button_down ? S_depressed : S_rollover);
  }
  PGItem::enter_region(param);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::exit_region
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               mouse exits the region.
////////////////////////////////////////////////////////////////////
void PGButton::
exit_region(const MouseWatcherParameter &param) {
  LightReMutexHolder holder(_lock);
  if (get_active()) {
    set_state(S_ready);
  }
  PGItem::exit_region(param);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::press
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever a
//               mouse or keyboard button is depressed while the mouse
//               is within the region.
////////////////////////////////////////////////////////////////////
void PGButton::
press(const MouseWatcherParameter &param, bool background) {
  LightReMutexHolder holder(_lock);
  if (has_click_button(param.get_button())) {
    if (get_active()) {
      _button_down = true;
      set_state(S_depressed);
    }
  }
  PGItem::press(param, background);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::release
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever a
//               mouse or keyboard button previously depressed with
//               press() is released.
////////////////////////////////////////////////////////////////////
void PGButton::
release(const MouseWatcherParameter &param, bool background) {
  LightReMutexHolder holder(_lock);
  if (has_click_button(param.get_button())) {
    _button_down = false;
    if (get_active()) {
      if (param.is_outside()) {
        set_state(S_ready);
      } else {
        set_state(S_rollover);
        click(param);
      }
    }
  }
  PGItem::release(param, background);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::click
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               button is clicked down-and-up by the user normally.
////////////////////////////////////////////////////////////////////
void PGButton::
click(const MouseWatcherParameter &param) {
  LightReMutexHolder holder(_lock);
  PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
  string event = get_click_event(param.get_button());
  play_sound(event);
  throw_event(event, EventParameter(ep));

  if (has_notify()) {
    get_notify()->button_click(this, param);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::setup
//       Access: Published
//  Description: Sets up the button as a default text button using the
//               indicated label string.  The TextNode defined by
//               PGItem::get_text_node() will be used to create the
//               label geometry.  This automatically sets up the frame
//               according to the size of the text.
////////////////////////////////////////////////////////////////////
void PGButton::
setup(const string &label, float bevel) {
  LightReMutexHolder holder(_lock);
  clear_state_def(S_ready);
  clear_state_def(S_depressed);
  clear_state_def(S_rollover);
  clear_state_def(S_inactive);

  TextNode *text_node = get_text_node();
  text_node->set_text(label);
  PT(PandaNode) geom = text_node->generate();

  LVecBase4f frame = text_node->get_card_actual();
  set_frame(frame[0] - 0.4f, frame[1] + 0.4f, frame[2] - 0.15f, frame[3] + 0.15f);

  PT(PandaNode) ready = new PandaNode("ready");
  PT(PandaNode) depressed = new PandaNode("depressed");
  PT(PandaNode) rollover = new PandaNode("rollover");
  PT(PandaNode) inactive = new PandaNode("inactive");

  PGFrameStyle style;
  style.set_color(0.8f, 0.8f, 0.8f, 1.0f);
  style.set_width(bevel, bevel);

  style.set_type(PGFrameStyle::T_bevel_out);
  set_frame_style(S_ready, style);

  style.set_color(0.9f, 0.9f, 0.9f, 1.0f);
  set_frame_style(S_rollover, style);

  inactive->set_attrib(ColorAttrib::make_flat(Colorf(0.8f, 0.8f, 0.8f, 1.0f)));
  style.set_color(0.6f, 0.6f, 0.6f, 1.0f);
  set_frame_style(S_inactive, style);

  style.set_type(PGFrameStyle::T_bevel_in);
  style.set_color(0.8f, 0.8f, 0.8f, 1.0f);
  set_frame_style(S_depressed, style);
  depressed->set_transform(TransformState::make_pos(LVector3f(0.05f, 0.0f, -0.05f)));

  get_state_def(S_ready).attach_new_node(ready, 1);
  get_state_def(S_depressed).attach_new_node(depressed, 1);
  get_state_def(S_rollover).attach_new_node(rollover, 1);
  get_state_def(S_inactive).attach_new_node(inactive, 1);

  ready->add_child(geom);
  depressed->add_child(geom);
  rollover->add_child(geom);
  inactive->add_child(geom);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::setup
//       Access: Published
//  Description: Sets up the button using the indicated NodePath as
//               arbitrary geometry.
////////////////////////////////////////////////////////////////////
void PGButton::
setup(const NodePath &ready, const NodePath &depressed, 
      const NodePath &rollover, const NodePath &inactive) {
  LightReMutexHolder holder(_lock);
  clear_state_def(S_ready);
  clear_state_def(S_depressed);
  clear_state_def(S_rollover);
  clear_state_def(S_inactive);

  instance_to_state_def(S_ready, ready);
  instance_to_state_def(S_depressed, depressed);
  instance_to_state_def(S_rollover, rollover);
  instance_to_state_def(S_inactive, inactive);

  // Set the button frame size.
  LPoint3f min_point, max_point;
  ready.calc_tight_bounds(min_point, max_point);
  set_frame(min_point[0], max_point[0],
            min_point[2], max_point[2]);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::set_active
//       Access: Published, Virtual
//  Description: Toggles the active/inactive state of the button.  In
//               the case of a PGButton, this also changes its visual
//               appearance.
////////////////////////////////////////////////////////////////////
void PGButton:: 
set_active(bool active) {
  LightReMutexHolder holder(_lock);
  if (active != get_active()) {
    PGItem::set_active(active);
    set_state(active ? S_ready : S_inactive);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::add_click_button
//       Access: Published
//  Description: Adds the indicated button to the set of buttons that
//               can effectively "click" the PGButton.  Normally, this
//               is just MouseButton::one().  Returns true if the
//               button was added, or false if it was already there.
////////////////////////////////////////////////////////////////////
bool PGButton::
add_click_button(const ButtonHandle &button) {
  LightReMutexHolder holder(_lock);
  return _click_buttons.insert(button).second;
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::remove_click_button
//       Access: Published
//  Description: Removes the indicated button from the set of buttons
//               that can effectively "click" the PGButton.  Normally,
//               this is just MouseButton::one().  Returns true if the
//               button was removed, or false if it was not in the
//               set.
////////////////////////////////////////////////////////////////////
bool PGButton::
remove_click_button(const ButtonHandle &button) {
  LightReMutexHolder holder(_lock);
  return (_click_buttons.erase(button) != 0);
}

////////////////////////////////////////////////////////////////////
//     Function: PGButton::has_click_button
//       Access: Published
//  Description: Returns true if the indicated button is on the set of
//               buttons that can effectively "click" the PGButton.
//               Normally, this is just MouseButton::one().
////////////////////////////////////////////////////////////////////
bool PGButton::
has_click_button(const ButtonHandle &button) {
  LightReMutexHolder holder(_lock);
  return (_click_buttons.count(button) != 0);
}

