// Filename: physxBoxForceFieldShape.cxx
// Created by:  enn0x (15Nov09)
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

#include "physxBoxForceFieldShape.h"
#include "physxBoxForceFieldShapeDesc.h"
#include "physxManager.h"

TypeHandle PhysxBoxForceFieldShape::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PhysxBoxForceFieldShape::link
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void PhysxBoxForceFieldShape::
link(NxForceFieldShape *shapePtr) {

  _ptr = shapePtr->isBox();
  _ptr->userData = this;
  _error_type = ET_ok;

  PhysxForceFieldShapeGroup *group = (PhysxForceFieldShapeGroup *)_ptr->getShapeGroup().userData;
  group->_shapes.add(this);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxBoxForceFieldShape::unlink
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void PhysxBoxForceFieldShape::
unlink() {

  _ptr->userData = NULL;
  _error_type = ET_released;

  PhysxForceFieldShapeGroup *group = (PhysxForceFieldShapeGroup *)_ptr->getShapeGroup().userData;
  group->_shapes.remove(this);
}

////////////////////////////////////////////////////////////////////
//     Function : PhysxBoxForceFieldShape::save_to_desc
//       Access : Published
//  Description : Saves the state of the shape object to a 
//                descriptor.
////////////////////////////////////////////////////////////////////
void PhysxBoxForceFieldShape::
save_to_desc(PhysxBoxForceFieldShapeDesc &shapeDesc) const {

  nassertv(_error_type == ET_ok);
  _ptr->saveToDesc(shapeDesc._desc);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxBoxForceFieldShape::set_dimensions
//       Access: Published
//  Description: Sets the box dimensions. 
//
//               The dimensions are the 'radii' of the box,
//               meaning 1/2 extents in x dimension, 1/2 extents
//               in y dimension, 1/2 extents in z dimension.
////////////////////////////////////////////////////////////////////
void PhysxBoxForceFieldShape::
set_dimensions(const LVector3f &vec) {

  nassertv(_error_type == ET_ok);
  _ptr->setDimensions(PhysxManager::vec3_to_nxVec3(vec));
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxBoxForceFieldShape::get_dimensions
//       Access: Published
//  Description: Retrieves the dimensions of the box. 
//
//               The dimensions are the 'radii' of the box,
//               meaning 1/2 extents in x dimension, 1/2 extents
//               in y dimension, 1/2 extents in z dimension.
////////////////////////////////////////////////////////////////////
LVector3f PhysxBoxForceFieldShape::
get_dimensions() const {

  nassertr(_error_type == ET_ok, LVector3f::zero());
  return PhysxManager::nxVec3_to_vec3(_ptr->getDimensions());
}

