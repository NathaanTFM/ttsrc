// Filename: physxSphereShape.cxx
// Created by:  enn0x (16Sep09)
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

#include "physxSphereShape.h"
#include "physxSphereShapeDesc.h"

TypeHandle PhysxSphereShape::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PhysxSphereShape::link
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void PhysxSphereShape::
link(NxShape *shapePtr) {

  _ptr = shapePtr->isSphere();
  _ptr->userData = this;
  _error_type = ET_ok;

  PhysxActor *actor = (PhysxActor *)_ptr->getActor().userData;
  actor->_shapes.add(this);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxSphereShape::unlink
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void PhysxSphereShape::
unlink() {

  _ptr->userData = NULL;
  _error_type = ET_released;

  PhysxActor *actor = (PhysxActor *)_ptr->getActor().userData;
  actor->_shapes.remove(this);
}

////////////////////////////////////////////////////////////////////
//     Function : PhysxSphereShape::save_to_desc
//       Access : Published
//  Description : Saves the state of the shape object to a 
//                descriptor.
////////////////////////////////////////////////////////////////////
void PhysxSphereShape::
save_to_desc(PhysxSphereShapeDesc &shapeDesc) const {

  nassertv(_error_type == ET_ok);
  _ptr->saveToDesc(shapeDesc._desc);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxSphereShape::set_radius
//       Access: Published
//  Description: Sets the sphere radius. 
////////////////////////////////////////////////////////////////////
void PhysxSphereShape::
set_radius(float radius) {

  nassertv(_error_type == ET_ok);
  _ptr->setRadius(radius);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxSphereShape::get_radius
//       Access: Published
//  Description: Returns the radius of the sphere.
////////////////////////////////////////////////////////////////////
float PhysxSphereShape::
get_radius() const {

  nassertr(_error_type == ET_ok, 0.0f);
  return _ptr->getRadius();
}

