// Filename: matrixLens.cxx
// Created by:  drose (12Dec01)
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

#include "matrixLens.h"
#include "indent.h"
#include "bamReader.h"

TypeHandle MatrixLens::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: MatrixLens::make_copy
//       Access: Public, Virtual
//  Description: Allocates a new Lens just like this one.
////////////////////////////////////////////////////////////////////
PT(Lens) MatrixLens::
make_copy() const {
  return new MatrixLens(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: MatrixLens::is_linear
//       Access: Published, Virtual
//  Description: Returns true if the lens represents a linear
//               projection (e.g. PerspectiveLens, MatrixLens),
//               and therefore there is a valid matrix returned by
//               get_projection_mat(), or false otherwise.
////////////////////////////////////////////////////////////////////
bool MatrixLens::
is_linear() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: MatrixLens::write
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void MatrixLens::
write(ostream &out, int indent_level) const {
  indent(out, indent_level) << get_type() << ":\n";
  get_projection_mat().write(out, indent_level + 2);
}

////////////////////////////////////////////////////////////////////
//     Function: MatrixLens::compute_projection_mat
//       Access: Protected, Virtual
//  Description: Computes the complete transformation matrix from 3-d
//               point to 2-d point, if the lens is linear.
////////////////////////////////////////////////////////////////////
void MatrixLens::
compute_projection_mat() {
  _projection_mat = get_lens_mat_inv() * _user_mat * get_film_mat();
  
  if (_ml_flags & MF_has_left_eye) {
    _projection_mat_left = get_lens_mat_inv() * _left_eye_mat * get_film_mat();
  } else {
    _projection_mat_left = _projection_mat;
  }
  
  if (_ml_flags & MF_has_right_eye) {
    _projection_mat_right = get_lens_mat_inv() * _right_eye_mat * get_film_mat();
  } else {
    _projection_mat_right = _projection_mat;
  }
  
  adjust_comp_flags(CF_projection_mat_inv, 
                    CF_projection_mat);
}

////////////////////////////////////////////////////////////////////
//     Function: MatrixLens::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               Lens.
////////////////////////////////////////////////////////////////////
void MatrixLens::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: MatrixLens::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type Lens is encountered
//               in the Bam file.  It should create the Lens
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *MatrixLens::
make_from_bam(const FactoryParams &params) {
  MatrixLens *lens = new MatrixLens;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  lens->fillin(scan, manager);

  return lens;
}
