// Filename: physxForceField.I
// Created by:  enn0x (06Nov09)
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



////////////////////////////////////////////////////////////////////
//     Function: PhysxForceField::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
INLINE PhysxForceField::
PhysxForceField() : PhysxObject() {

}

////////////////////////////////////////////////////////////////////
//     Function: PhysxForceField::Destructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
INLINE PhysxForceField::
~PhysxForceField() {

}

////////////////////////////////////////////////////////////////////
//     Function: PhysxForceField::ls
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
INLINE void PhysxForceField::
ls() const {

  ls(nout);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxForceField::ls
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
INLINE void PhysxForceField::
ls(ostream &out, int indent_level) const {

  indent(out, indent_level) << get_type().get_name()
                            << " " << get_name()
                            << " (at 0x" << this << ")\n";
}

