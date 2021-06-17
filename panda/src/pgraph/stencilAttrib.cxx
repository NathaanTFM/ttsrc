// Filename: stencilAttrib.cxx
// Created by:  aignacio (18May06)
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

#include "stencilAttrib.h"
#include "graphicsStateGuardianBase.h"
#include "dcast.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagram.h"
#include "datagramIterator.h"

TypeHandle StencilAttrib::_type_handle;
int StencilAttrib::_attrib_slot;

const char *StencilAttrib::
stencil_render_state_name_array [StencilAttrib::SRS_total] =
{
  "SRS_front_enable",
  "SRS_back_enable",

  "SRS_front_comparison_function",
  "SRS_front_stencil_fail_operation",
  "SRS_front_stencil_pass_z_fail_operation",
  "SRS_front_stencil_pass_z_pass_operation",

  "SRS_reference",
  "SRS_read_mask",
  "SRS_write_mask",

  "SRS_back_comparison_function",
  "SRS_back_stencil_fail_operation",
  "SRS_back_stencil_pass_z_fail_operation",
  "SRS_back_stencil_pass_z_pass_operation",

  "SRS_clear",
  "SRS_clear_value",
};

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::Constructor
//       Access: Private
//  Description: Use StencilAttrib::make() to construct a new
//               StencilAttrib object.
////////////////////////////////////////////////////////////////////
StencilAttrib::
StencilAttrib() {

  _stencil_render_states [SRS_front_enable] = 0;
  _stencil_render_states [SRS_back_enable] = 0;

  _stencil_render_states [SRS_front_comparison_function] = SCF_always;
  _stencil_render_states [SRS_front_stencil_fail_operation] = SO_keep;
  _stencil_render_states [SRS_front_stencil_pass_z_fail_operation] = SO_keep;
  _stencil_render_states [SRS_front_stencil_pass_z_pass_operation] = SO_keep;

  _stencil_render_states [SRS_reference] = 0;
  _stencil_render_states [SRS_read_mask] = ~0;
  _stencil_render_states [SRS_write_mask] = ~0;

  _stencil_render_states [SRS_back_comparison_function] = SCF_always;
  _stencil_render_states [SRS_back_stencil_fail_operation] = SO_keep;
  _stencil_render_states [SRS_back_stencil_pass_z_fail_operation] = SO_keep;
  _stencil_render_states [SRS_back_stencil_pass_z_pass_operation] = SO_keep;

  _stencil_render_states [SRS_clear] = 0;
  _stencil_render_states [SRS_clear_value] = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::make_off
//       Access: Published, Static
//  Description: Constructs a StencilAttrib that has stenciling 
//               turned off.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) StencilAttrib::
make_off()
{
  StencilAttrib *attrib = new StencilAttrib;

  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::make_default
//       Access: Published, Static
//  Description: Returns a RenderAttrib that corresponds to whatever
//               the standard default properties for render attributes
//               of this type ought to be.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) StencilAttrib::
make_default() {
  return return_new(new StencilAttrib);
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::make
//       Access: Published, Static
//  Description: Constructs a front face StencilAttrib.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) StencilAttrib::
make(
  unsigned int front_enable,
  unsigned int front_comparison_function,
  unsigned int stencil_fail_operation,
  unsigned int stencil_pass_z_fail_operation,
  unsigned int front_stencil_pass_z_pass_operation,
  unsigned int reference,
  unsigned int read_mask,
  unsigned int write_mask)
{
  StencilAttrib *attrib = new StencilAttrib;

  attrib->_stencil_render_states [SRS_front_enable] = front_enable;
  attrib->_stencil_render_states [SRS_back_enable] = 0;

  attrib->_stencil_render_states [SRS_front_comparison_function] = front_comparison_function;
  attrib->_stencil_render_states [SRS_front_stencil_fail_operation] = stencil_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_fail_operation] = stencil_pass_z_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_pass_operation] = front_stencil_pass_z_pass_operation;

  attrib->_stencil_render_states [SRS_reference] = reference;
  attrib->_stencil_render_states [SRS_read_mask] = read_mask;
  attrib->_stencil_render_states [SRS_write_mask] = write_mask;

  attrib->_stencil_render_states [SRS_back_comparison_function] = SCF_always;
  attrib->_stencil_render_states [SRS_back_stencil_fail_operation] = SO_keep;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_fail_operation] = SO_keep;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_pass_operation] = SO_keep;

  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::make_2_sided
//       Access: Published, Static
//  Description: Constructs a two-sided StencilAttrib.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) StencilAttrib::
make_2_sided(
  unsigned int front_enable,
  unsigned int back_enable,
  unsigned int front_comparison_function,
  unsigned int stencil_fail_operation,
  unsigned int stencil_pass_z_fail_operation,
  unsigned int front_stencil_pass_z_pass_operation,
  unsigned int reference,
  unsigned int read_mask,
  unsigned int write_mask,
  unsigned int back_comparison_function,
  unsigned int back_stencil_fail_operation,
  unsigned int back_stencil_pass_z_fail_operation,
  unsigned int back_stencil_pass_z_pass_operation)
{
  StencilAttrib *attrib = new StencilAttrib;

  attrib->_stencil_render_states [SRS_front_enable] = front_enable;
  attrib->_stencil_render_states [SRS_back_enable] = back_enable;

  attrib->_stencil_render_states [SRS_front_comparison_function] = front_comparison_function;
  attrib->_stencil_render_states [SRS_front_stencil_fail_operation] = stencil_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_fail_operation] = stencil_pass_z_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_pass_operation] = front_stencil_pass_z_pass_operation;

  attrib->_stencil_render_states [SRS_reference] = reference;
  attrib->_stencil_render_states [SRS_read_mask] = read_mask;
  attrib->_stencil_render_states [SRS_write_mask] = write_mask;

  attrib->_stencil_render_states [SRS_back_comparison_function] = back_comparison_function;
  attrib->_stencil_render_states [SRS_back_stencil_fail_operation] = back_stencil_fail_operation;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_fail_operation] = back_stencil_pass_z_fail_operation;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_pass_operation] = back_stencil_pass_z_pass_operation;

  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::make
//       Access: Published, Static
//  Description: Constructs a front face StencilAttrib.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) StencilAttrib::
make_with_clear(
  unsigned int front_enable,
  unsigned int front_comparison_function,
  unsigned int stencil_fail_operation,
  unsigned int stencil_pass_z_fail_operation,
  unsigned int front_stencil_pass_z_pass_operation,
  unsigned int reference,
  unsigned int read_mask,
  unsigned int write_mask,
  unsigned int clear,
  unsigned int clear_value)
{
  StencilAttrib *attrib = new StencilAttrib;

  attrib->_stencil_render_states [SRS_front_enable] = front_enable;
  attrib->_stencil_render_states [SRS_back_enable] = 0;

  attrib->_stencil_render_states [SRS_front_comparison_function] = front_comparison_function;
  attrib->_stencil_render_states [SRS_front_stencil_fail_operation] = stencil_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_fail_operation] = stencil_pass_z_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_pass_operation] = front_stencil_pass_z_pass_operation;

  attrib->_stencil_render_states [SRS_reference] = reference;
  attrib->_stencil_render_states [SRS_read_mask] = read_mask;
  attrib->_stencil_render_states [SRS_write_mask] = write_mask;

  attrib->_stencil_render_states [SRS_back_comparison_function] = SCF_always;
  attrib->_stencil_render_states [SRS_back_stencil_fail_operation] = SO_keep;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_fail_operation] = SO_keep;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_pass_operation] = SO_keep;

  attrib->_stencil_render_states [SRS_clear] = clear;
  attrib->_stencil_render_states [SRS_clear_value] = clear_value;

  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::make_2_sided
//       Access: Published, Static
//  Description: Constructs a two-sided StencilAttrib.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) StencilAttrib::
make_2_sided_with_clear(
  unsigned int front_enable,
  unsigned int back_enable,
  unsigned int front_comparison_function,
  unsigned int stencil_fail_operation,
  unsigned int stencil_pass_z_fail_operation,
  unsigned int front_stencil_pass_z_pass_operation,
  unsigned int reference,
  unsigned int read_mask,
  unsigned int write_mask,
  unsigned int back_comparison_function,
  unsigned int back_stencil_fail_operation,
  unsigned int back_stencil_pass_z_fail_operation,
  unsigned int back_stencil_pass_z_pass_operation,
  unsigned int clear,
  unsigned int clear_value)
{
  StencilAttrib *attrib = new StencilAttrib;

  attrib->_stencil_render_states [SRS_front_enable] = front_enable;
  attrib->_stencil_render_states [SRS_back_enable] = back_enable;

  attrib->_stencil_render_states [SRS_front_comparison_function] = front_comparison_function;
  attrib->_stencil_render_states [SRS_front_stencil_fail_operation] = stencil_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_fail_operation] = stencil_pass_z_fail_operation;
  attrib->_stencil_render_states [SRS_front_stencil_pass_z_pass_operation] = front_stencil_pass_z_pass_operation;

  attrib->_stencil_render_states [SRS_reference] = reference;
  attrib->_stencil_render_states [SRS_read_mask] = read_mask;
  attrib->_stencil_render_states [SRS_write_mask] = write_mask;

  attrib->_stencil_render_states [SRS_back_comparison_function] = back_comparison_function;
  attrib->_stencil_render_states [SRS_back_stencil_fail_operation] = back_stencil_fail_operation;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_fail_operation] = back_stencil_pass_z_fail_operation;
  attrib->_stencil_render_states [SRS_back_stencil_pass_z_pass_operation] = back_stencil_pass_z_pass_operation;

  attrib->_stencil_render_states [SRS_clear] = clear;
  attrib->_stencil_render_states [SRS_clear_value] = clear_value;

  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::output
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void StencilAttrib::
output(ostream &out) const {

  int index;
  for (index = 0; index < SRS_total; index++) {
    out
      << "(" << stencil_render_state_name_array [index]
      << ", " << _stencil_render_states [index] << ")";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::compare_to_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived StencilAttrib
//               types to return a unique number indicating whether
//               this StencilAttrib is equivalent to the other one.
//
//               This should return 0 if the two StencilAttrib objects
//               are equivalent, a number less than zero if this one
//               should be sorted before the other one, and a number
//               greater than zero otherwise.
//
//               This will only be called with two StencilAttrib
//               objects whose get_type() functions return the same.
////////////////////////////////////////////////////////////////////
int StencilAttrib::
compare_to_impl(const RenderAttrib *other) const {
  const StencilAttrib *sa;
  DCAST_INTO_R(sa, other, 0);

  int a;
  int b;
  int index;
  int compare_result = 0;

  for (index = 0; index < SRS_total; index++) {
    a = (int) sa -> _stencil_render_states [index];
    b = (int) _stencil_render_states [index];
    compare_result = (a - b);
    if (compare_result) {
      break;
    }
  }

  return compare_result;
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               StencilAttrib.
////////////////////////////////////////////////////////////////////
void StencilAttrib::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void StencilAttrib::
write_datagram(BamWriter *manager, Datagram &dg) {
  RenderAttrib::write_datagram(manager, dg);

  int index;
  for (index = 0; index < SRS_total; index++) {
    dg.add_int32(_stencil_render_states [index]);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type StencilAttrib is encountered
//               in the Bam file.  It should create the StencilAttrib
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *StencilAttrib::
make_from_bam(const FactoryParams &params) {
  StencilAttrib *attrib = new StencilAttrib;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  attrib->fillin(scan, manager);

  return attrib;
}

////////////////////////////////////////////////////////////////////
//     Function: StencilAttrib::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new StencilAttrib.
////////////////////////////////////////////////////////////////////
void StencilAttrib::
fillin(DatagramIterator &scan, BamReader *manager) {
  RenderAttrib::fillin(scan, manager);

  int index;
  for (index = 0; index < SRS_total; index++) {
    _stencil_render_states [index] = scan.get_int32();
  }
}
