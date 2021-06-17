// Filename: config_pgraphnodes.cxx
// Created by:  drose (05Nov08)
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

#include "config_pgraphnodes.h"

#include "ambientLight.h"
#include "callbackData.h"
#include "callbackNode.h"
#include "callbackObject.h"
#include "directionalLight.h"
#include "fadeLodNode.h"
#include "fadeLodNodeData.h"
#include "lightLensNode.h"
#include "lightNode.h"
#include "lodNode.h"
#include "nodeCullCallbackData.h"
#include "pointLight.h"
#include "selectiveChildNode.h"
#include "sequenceNode.h"
#include "shaderGenerator.h"
#include "spotlight.h"
#include "switchNode.h"

#include "dconfig.h"

ConfigureDef(config_pgraphnodes);
NotifyCategoryDef(pgraphnodes, "");

ConfigureFn(config_pgraphnodes) {
  init_libpgraphnodes();
}

ConfigVariableEnum<LODNodeType> default_lod_type
("default-lod-type", LNT_pop,
 PRC_DESC("Set this to either 'pop' or 'fade' to determine the type of "
          "LODNode that is created by LODNode::make_default_lod()."));

ConfigVariableInt parallax_mapping_samples
("parallax-mapping-samples", 3,
 PRC_DESC("Sets the amount of samples to use in the parallax mapping "
          "implementation. A value of 0 means to disable it entirely."));

ConfigVariableDouble parallax_mapping_scale
("parallax-mapping-scale", 0.1,
 PRC_DESC("Sets the strength of the effect of parallax mapping, that is, "
          "how much influence the height values have on the texture "
          "coordinates."));

////////////////////////////////////////////////////////////////////
//     Function: init_libpgraphnodes
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libpgraphnodes() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  AmbientLight::init_type();
  CallbackData::init_type();
  CallbackNode::init_type();
  CallbackObject::init_type();
  DirectionalLight::init_type();
  FadeLODNode::init_type();
  FadeLODNodeData::init_type();
  LightLensNode::init_type();
  LightNode::init_type();
  LODNode::init_type();
  NodeCullCallbackData::init_type();
  PointLight::init_type();
  SelectiveChildNode::init_type();
  SequenceNode::init_type();
  ShaderGenerator::init_type();
  Spotlight::init_type();
  SwitchNode::init_type();

  AmbientLight::register_with_read_factory();
  CallbackNode::register_with_read_factory();
  DirectionalLight::register_with_read_factory();
  FadeLODNode::register_with_read_factory();
  LightLensNode::register_with_read_factory();
  LightNode::register_with_read_factory();
  LODNode::register_with_read_factory();
  PointLight::register_with_read_factory();
  SelectiveChildNode::register_with_read_factory();
  SequenceNode::register_with_read_factory();
  Spotlight::register_with_read_factory();
  SwitchNode::register_with_read_factory();
}
