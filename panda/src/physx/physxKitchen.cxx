// Filename: physxKitchen.cxx
// Created by:  enn0x (12Oct09)
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

#include "physxKitchen.h"
#include "physxConvexMesh.h"
#include "physxConvexMeshDesc.h"
#include "physxTriangleMesh.h"
#include "physxTriangleMeshDesc.h"
#include "physxFileStream.h"
#include "physxMemoryReadBuffer.h"
#include "physxMemoryWriteBuffer.h"

////////////////////////////////////////////////////////////////////
//     Function: PhysxKitchen::set_cooking_params
//       Access: Published
//  Description: Sets two parameters which affect mesh cooking:
//
//               Skin width for convex meshes:
//               Specifies the amount to inflate the convex mesh by
//               when the new convex hull generator is used.
//               Inflating the mesh allows the user to hide
//               interpenetration errors by increasing the size of
//               the collision mesh with respect to the size of the
//               rendered geometry.
//               Default value: 0.025f 
//
//               Hint to choose speed or less memory for collision
//               structures.
//               Default value: false
////////////////////////////////////////////////////////////////////
void PhysxKitchen::
set_cooking_params(float skinWidth, bool hintCollisionSpeed) {

  NxCookingParams params;

  params.targetPlatform = PLATFORM_PC;
  params.skinWidth = skinWidth;
  params.hintCollisionSpeed = hintCollisionSpeed;

  _cooking->NxSetCookingParams(params);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxKitchen::cook_convex_mesh
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
bool PhysxKitchen::
cook_convex_mesh(const PhysxConvexMeshDesc &meshDesc, const Filename &filename) {

  nassertr_always(!filename.empty(), false);
  nassertr_always(filename.touch(), false);
  nassertr_always(meshDesc.is_valid(), false);

  PhysxFileStream stream = PhysxFileStream(filename, false);
  return _cooking->NxCookConvexMesh(meshDesc.get_desc(), stream);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxKitchen::cook_triangle_mesh
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
bool PhysxKitchen::
cook_triangle_mesh(const PhysxTriangleMeshDesc &meshDesc, const Filename &filename) {

  nassertr_always(!filename.empty(), false);
  nassertr_always(filename.touch(), false);
  nassertr_always(meshDesc.is_valid(), false);

  PhysxFileStream stream = PhysxFileStream(filename, false);
  return _cooking->NxCookTriangleMesh(meshDesc.get_desc(), stream);
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxKitchen::cook_convex_mesh
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
PhysxConvexMesh *PhysxKitchen::
cook_convex_mesh(const PhysxConvexMeshDesc &meshDesc) {

  nassertr_always(meshDesc.is_valid(), false);

  PhysxMemoryWriteBuffer buffer;
  bool status = _cooking->NxCookConvexMesh(meshDesc.get_desc(), buffer);
  nassertr(status, NULL);

  NxPhysicsSDK *sdk = NxGetPhysicsSDK();
  nassertr(sdk, NULL);

  PhysxConvexMesh *mesh = new PhysxConvexMesh();
  nassertr(mesh, NULL);

  NxConvexMesh *meshPtr = sdk->createConvexMesh(PhysxMemoryReadBuffer(buffer.data));
  nassertr(meshPtr, NULL);

  mesh->link(meshPtr);

  return mesh;
}

////////////////////////////////////////////////////////////////////
//     Function: PhysxKitchen::cook_triangle_mesh
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
PhysxTriangleMesh *PhysxKitchen::
cook_triangle_mesh(const PhysxTriangleMeshDesc &meshDesc) {

  nassertr_always(meshDesc.is_valid(), false);

  PhysxMemoryWriteBuffer buffer;
  bool status = _cooking->NxCookTriangleMesh(meshDesc.get_desc(), buffer);
  nassertr(status, NULL);

  NxPhysicsSDK *sdk = NxGetPhysicsSDK();
  nassertr(sdk, NULL);

  PhysxTriangleMesh *mesh = new PhysxTriangleMesh();
  nassertr(mesh, NULL);

  NxTriangleMesh *meshPtr = sdk->createTriangleMesh(PhysxMemoryReadBuffer(buffer.data));
  nassertr(meshPtr, NULL);

  mesh->link(meshPtr);

  return mesh;
}

