// Filename: bamToEgg.cxx
// Created by:  drose (25Jun01)
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

#include "bamToEgg.h"

#include "pandaNode.h"
#include "workingNodePath.h"
#include "nodePath.h"
#include "billboardEffect.h"
#include "renderEffects.h"
#include "transformState.h"
#include "colorScaleAttrib.h"
#include "colorAttrib.h"
#include "textureAttrib.h"
#include "cullFaceAttrib.h"
#include "transparencyAttrib.h"
#include "depthWriteAttrib.h"
#include "lodNode.h"
#include "switchNode.h"
#include "sequenceNode.h"
#include "collisionNode.h"
#include "collisionPolygon.h"
#include "collisionPlane.h"
#include "collisionSphere.h"
#include "collisionInvSphere.h"
#include "collisionTube.h"
#include "textureStage.h"
#include "geomNode.h"
#include "geom.h"
#include "geomTriangles.h"
#include "geomVertexReader.h"
#include "transformTable.h"
#include "animBundleNode.h"
#include "animChannelMatrixXfmTable.h"
#include "characterJoint.h"
#include "character.h"
#include "string_utils.h"
#include "bamFile.h"
#include "bamCacheRecord.h"
#include "eggSAnimData.h"
#include "eggXfmAnimData.h"
#include "eggXfmSAnim.h"
#include "eggGroup.h"
#include "eggVertexPool.h"
#include "eggVertex.h"
#include "eggPrimitive.h"
#include "eggPolygon.h"
#include "eggTexture.h"
#include "eggMaterial.h"
#include "eggRenderMode.h"
#include "eggTable.h"
#include "somethingToEggConverter.h"
#include "dcast.h"
#include "pystub.h"

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
BamToEgg::
BamToEgg() :
  SomethingToEgg("bam", ".bam")
{
  add_path_replace_options();
  add_path_store_options();

  set_program_description
    ("This program converts native Panda bam files to egg.  The conversion "
     "is somewhat incomplete; running egg2bam followed by bam2egg should not "
     "be expected to yield the same egg file you started with.");

  redescribe_option
    ("cs",
     "Specify the coordinate system of the input " + _format_name +
     " file.  By default, this is taken from the Config.prc file, which "
     "is currently " + format_string(get_default_coordinate_system()) + ".");

  _coordinate_system = get_default_coordinate_system();
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::run
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void BamToEgg::
run() {
  BamFile bam_file;

  if (!bam_file.open_read(_input_filename)) {
    nout << "Unable to read " << _input_filename << "\n";
    exit(1);
  }

  nout << _input_filename << " : Bam version "
       << bam_file.get_file_major_ver() << "." 
       << bam_file.get_file_minor_ver() << "\n";

  typedef pvector<TypedWritable *> Objects;
  Objects objects;
  TypedWritable *object = bam_file.read_object();

  if (object != (TypedWritable *)NULL && 
      object->is_exact_type(BamCacheRecord::get_class_type())) {
    // Here's a special case: if the first object in the file is a
    // BamCacheRecord, it's really a cache data file and not a true
    // bam file; but skip over the cache data record and let the user
    // treat it like an ordinary bam file.
    object = bam_file.read_object();
  }

  while (object != (TypedWritable *)NULL || !bam_file.is_eof()) {
    if (object != (TypedWritable *)NULL) {
      ReferenceCount *ref_ptr = object->as_reference_count();
      if (ref_ptr != NULL) {
        ref_ptr->ref();
      }
      objects.push_back(object);
    }
    object = bam_file.read_object();
  }
  bam_file.resolve();
  bam_file.close();

  _data->set_coordinate_system(_coordinate_system);
  _vpool = new EggVertexPool("vpool");
  _data->add_child(_vpool);

  if (objects.size() == 1 && 
      objects[0]->is_of_type(PandaNode::get_class_type())) {
    PandaNode *node = DCAST(PandaNode, objects[0]);
    NodePath root(node);
    convert_node(WorkingNodePath(root), _data, false);

  } else {
    nout << "File does not contain a scene graph.\n";
    exit(1);
  }

  // Remove the vertex pool if it has no vertices.
  if (_vpool->empty()) {
    _data->remove_child(_vpool);
  }

  write_egg_file();
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_node
//       Access: Private
//  Description: Converts the indicated node to the corresponding Egg
//               constructs, by first determining what kind of node it
//               is.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_node(const WorkingNodePath &node_path, EggGroupNode *egg_parent,
             bool has_decal) {
  PandaNode *node = node_path.node();
  if (node->is_geom_node()) {
    convert_geom_node(DCAST(GeomNode, node), node_path, egg_parent, has_decal);

  } else if (node->is_of_type(LODNode::get_class_type())) {
    convert_lod_node(DCAST(LODNode, node), node_path, egg_parent, has_decal);

  } else if (node->is_of_type(SequenceNode::get_class_type())) {
    convert_sequence_node(DCAST(SequenceNode, node), node_path, egg_parent, has_decal);

  } else if (node->is_of_type(SwitchNode::get_class_type())) {
    convert_switch_node(DCAST(SwitchNode, node), node_path, egg_parent, has_decal);

  } else if (node->is_of_type(CollisionNode::get_class_type())) {
    convert_collision_node(DCAST(CollisionNode, node), node_path, egg_parent, has_decal);

  } else if (node->is_of_type(AnimBundleNode::get_class_type())) {
    convert_anim_node(DCAST(AnimBundleNode, node), node_path, egg_parent, has_decal);

  } else if (node->is_of_type(Character::get_class_type())) {
    convert_character_node(DCAST(Character, node), node_path, egg_parent, has_decal);

  } else {
    // Just a generic node.
    EggGroup *egg_group = new EggGroup(node->get_name());
    egg_parent->add_child(egg_group);
    apply_node_properties(egg_group, node);
    
    recurse_nodes(node_path, egg_group, has_decal);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_lod_node
//       Access: Private
//  Description: Converts the indicated LODNode to the corresponding
//               Egg constructs.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_lod_node(LODNode *node, const WorkingNodePath &node_path,
                 EggGroupNode *egg_parent, bool has_decal) {
  // An LOD node gets converted to an ordinary EggGroup, but we apply
  // the appropriate switch conditions to each of our children.
  EggGroup *egg_group = new EggGroup(node->get_name());
  egg_parent->add_child(egg_group);
  apply_node_properties(egg_group, node);

  int num_children = node->get_num_children();
  int num_switches = node->get_num_switches();

  num_children = min(num_children, num_switches);

  for (int i = 0; i < num_children; i++) {
    PandaNode *child = node->get_child(i);

    // Convert just this one node to an EggGroup.
    PT(EggGroup) next_group = new EggGroup;
    convert_node(WorkingNodePath(node_path, child), next_group, has_decal);

    if (next_group->size() == 1) {
      // If we have exactly one child, and that child is an EggGroup,
      // collapse.
      EggNode *child_node = *next_group->begin();
      if (child_node->is_of_type(EggGroup::get_class_type())) {
        PT(EggGroup) child = DCAST(EggGroup, child_node);
        next_group->remove_child(child.p());
        next_group = child;
      }
    }

    // Now set up the switching properties appropriately.
    float in = node->get_in(i);
    float out = node->get_out(i);
    LPoint3f center = node->get_center();
    EggSwitchConditionDistance dist(in, out, LCAST(double, center));
    next_group->set_lod(dist);
    egg_group->add_child(next_group.p());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_sequence_node
//       Access: Private
//  Description: Converts the indicated SequenceNode to the corresponding
//               Egg constructs.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_sequence_node(SequenceNode *node, const WorkingNodePath &node_path,
                      EggGroupNode *egg_parent, bool has_decal) {
  // A sequence node gets converted to an ordinary EggGroup, we only apply
  // the appropriate switch attributes to turn it into a sequence
  EggGroup *egg_group = new EggGroup(node->get_name());
  egg_parent->add_child(egg_group);
  apply_node_properties(egg_group, node);

  // turn it into a sequence with the right frame-rate
  egg_group->set_switch_flag(true);
  egg_group->set_switch_fps(node->get_frame_rate());

  int num_children = node->get_num_children();

  for (int i = 0; i < num_children; i++) {
    PandaNode *child = node->get_child(i);

    // Convert just this one node to an EggGroup.
    PT(EggGroup) next_group = new EggGroup;
    convert_node(WorkingNodePath(node_path, child), next_group, has_decal);

    egg_group->add_child(next_group.p());
  }

}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_switch_node
//       Access: Private
//  Description: Converts the indicated SwitchNode to the corresponding
//               Egg constructs.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_switch_node(SwitchNode *node, const WorkingNodePath &node_path,
                    EggGroupNode *egg_parent, bool has_decal) {
  // A sequence node gets converted to an ordinary EggGroup, we only apply
  // the appropriate switch attributes to turn it into a sequence
  EggGroup *egg_group = new EggGroup(node->get_name());
  egg_parent->add_child(egg_group);
  apply_node_properties(egg_group, node);

  // turn it into a switch..
  egg_group->set_switch_flag(true);

  int num_children = node->get_num_children();

  for (int i = 0; i < num_children; i++) {
    PandaNode *child = node->get_child(i);

    // Convert just this one node to an EggGroup.
    PT(EggGroup) next_group = new EggGroup;
    convert_node(WorkingNodePath(node_path, child), next_group, has_decal);

    egg_group->add_child(next_group.p());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_animGroup_node
//       Access: Private
//  Description: Converts the indicated AnimationGroupNodes to the corresponding
//               Egg constructs.
////////////////////////////////////////////////////////////////////
EggGroupNode * BamToEgg::convert_animGroup_node(AnimGroup *animGroup, double fps ) {
  int num_children = animGroup->get_num_children();

  EggGroupNode *eggNode = NULL;
  if (animGroup->is_of_type(AnimBundle::get_class_type())) {
    EggTable *eggTable = new EggTable(animGroup->get_name());
    eggTable ->set_table_type(EggTable::TT_bundle);
    eggNode = eggTable;
  } else if (animGroup->is_of_type(AnimGroup::get_class_type())) {
    EggTable *eggTable = new EggTable(animGroup->get_name());
    eggTable ->set_table_type(EggTable::TT_table);
    eggNode = eggTable;
  }

  if (animGroup->is_of_type(AnimChannelMatrixXfmTable::get_class_type())) {
    AnimChannelMatrixXfmTable *xmfTable = DCAST(AnimChannelMatrixXfmTable, animGroup);
    EggXfmSAnim *egg_anim = new EggXfmSAnim("xform");
    egg_anim->set_fps(fps);
    for (int i = 0; i < num_matrix_components; i++) {
      string componentName(1, matrix_component_letters[i]);
      char table_id = matrix_component_letters[i];
      CPTA_float table = xmfTable->get_table(table_id);

      if (xmfTable->has_table(table_id)) {
        for (unsigned int j = 0; j < table.size(); j++) {
          egg_anim->add_component_data(componentName, table[(int)j]);
        }
      }
    }
    eggNode->add_child(egg_anim);
  }
  for (int i = 0; i < num_children; i++) {
    AnimGroup *animChild = animGroup->get_child(i);
    EggGroupNode *eggChildNode = convert_animGroup_node(animChild, fps);
    if (eggChildNode!=NULL) {
      nassertr(eggNode!=NULL, NULL);
      eggNode->add_child(eggChildNode);
    }
  } 
  return eggNode;
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_anim_node
//       Access: Private
//  Description: Converts the indicated AnimNode to the corresponding
//               Egg constructs.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_anim_node(AnimBundleNode *node, const WorkingNodePath &node_path,
                    EggGroupNode *egg_parent, bool has_decal) {
  
  // A sequence node gets converted to an ordinary EggGroup, we only apply
  // the appropriate switch attributes to turn it into a sequence
  EggTable *eggTable = new EggTable();
  //egg_parent->add_child(eggTable);
  _data->add_child(eggTable);
 
  AnimBundle *animBundle = node->get_bundle();
  // turn it into a switch..
  //egg_group->set_switch_flag(true);

  EggGroupNode *eggAnimation = convert_animGroup_node(animBundle, animBundle->get_base_frame_rate());
  eggTable->add_child(eggAnimation);
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_character_bundle
//       Access: Private
//  Description: Converts the indicated Character Bundle to the corresponding
//               Egg joints structure.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_character_bundle(PartGroup *bundleNode, EggGroupNode *egg_parent, CharacterJointMap *jointMap) {
  int num_children = bundleNode->get_num_children();
  
  EggGroupNode *joint_group = egg_parent;
  if (bundleNode->is_of_type(CharacterJoint::get_class_type())) {
    CharacterJoint *character_joint = DCAST(CharacterJoint, bundleNode);

    LMatrix4f transformf;
    character_joint->get_net_transform(transformf);
    LMatrix4d transformd(LCAST(double, transformf));
    EggGroup *joint = new EggGroup(bundleNode->get_name());
    joint->add_matrix4(transformd);
    joint->set_group_type(EggGroup::GT_joint);
    joint_group = joint;
    egg_parent->add_child(joint_group);
    if (jointMap!=NULL) {
      CharacterJointMap::iterator mi = jointMap->find(character_joint);
      if (mi != jointMap->end()) {
        pvector<pair<EggVertex*,float> > &joint_vertices = (*mi).second;
        pvector<pair<EggVertex*,float> >::const_iterator vi;
        for (vi = joint_vertices.begin(); vi != joint_vertices.end(); ++vi) {
          joint->set_vertex_membership((*vi).first, (*vi).second);
        }
      }
    }
  }

  for (int i = 0; i < num_children ; i++) {
    PartGroup *partGroup= bundleNode->get_child(i);
    convert_character_bundle(partGroup, joint_group, jointMap);
  }

}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_character_node
//       Access: Private
//  Description: Converts the indicated Character to the corresponding
//               Egg constructs.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_character_node(Character *node, const WorkingNodePath &node_path,
                    EggGroupNode *egg_parent, bool has_decal) {
  
  // A sequence node gets converted to an ordinary EggGroup, we only apply
  // the appropriate switch attributes to turn it into a sequence
  EggGroup *egg_group = new EggGroup(node->get_name());
  egg_group->set_dart_type(EggGroup::DT_default);
  egg_parent->add_child(egg_group);
  apply_node_properties(egg_group, node);

  CharacterJointMap jointMap;
  
  // turn it into a switch..
  //egg_group->set_switch_flag(true);

  int num_children = node->get_num_children();
  int num_bundles = node->get_num_bundles();

  for (int i = 0; i < num_children; i++) {
    PandaNode *child = node->get_child(i);

    if (child->is_geom_node()) {
      convert_geom_node(DCAST(GeomNode, child), WorkingNodePath(node_path, child), egg_group, has_decal, &jointMap);
    }
  }

  for (int i = 0; i < num_bundles ; i++) {
    PartBundle *bundle= node->get_bundle(i);
    convert_character_bundle(bundle, egg_group, &jointMap);
  }

}


////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_collision_node
//       Access: Private
//  Description: Converts the indicated CollisionNode to the corresponding
//               Egg constructs.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_collision_node(CollisionNode *node, const WorkingNodePath &node_path,
                       EggGroupNode *egg_parent, bool has_decal) {
  // A sequence node gets converted to an ordinary EggGroup, we only apply
  // the appropriate switch attributes to turn it into a sequence
  EggGroup *egg_group = new EggGroup(node->get_name());
  egg_parent->add_child(egg_group);
  apply_node_properties(egg_group, node, false);

  // turn it into a collision node
  egg_group->set_cs_type(EggGroup::CST_polyset);
  egg_group->set_collide_flags(EggGroup::CF_descend);

  int num_solids = node->get_num_solids();

  if (num_solids > 0) {
    // create vertex pool for collisions
    EggVertexPool *cvpool = new EggVertexPool("vpool-collision");
    egg_group->add_child(cvpool);

    // traverse solids
    for (int i = 0; i < num_solids; i++) {
      CPT(CollisionSolid) child = node->get_solid(i);
      if (child->is_of_type(CollisionPolygon::get_class_type())) {
        EggPolygon *egg_poly = new EggPolygon;
        egg_group->add_child(egg_poly);

        CPT(CollisionPolygon) poly = DCAST(CollisionPolygon, child);
        int num_points = poly->get_num_points();
        for (int j = 0; j < num_points; j++) {
          EggVertex egg_vert;
          egg_vert.set_pos(LCAST(double, poly->get_point(j)));
          egg_vert.set_normal(LCAST(double, poly->get_normal()));

          EggVertex *new_egg_vert = cvpool->create_unique_vertex(egg_vert);
          egg_poly->add_vertex(new_egg_vert);
        }
      } else if (child->is_of_type(CollisionPlane::get_class_type())) {
        nout << "Encountered unhandled collsion type: CollisionPlane" << "\n";
      } else if (child->is_of_type(CollisionSphere::get_class_type())) {
        nout << "Encountered unhandled collsion type: CollisionSphere" << "\n";
      } else if (child->is_of_type(CollisionInvSphere::get_class_type())) {
        nout << "Encountered unhandled collsion type: CollisionInvSphere" << "\n";
      } else if (child->is_of_type(CollisionTube::get_class_type())) {
        nout << "Encountered unhandled collsion type: CollisionTube" << "\n";
      } else {
        nout << "Encountered unknown CollisionSolid" << "\n";
      }
    }
  }

  // recurse over children - hm. do I need to do this?
  recurse_nodes(node_path, egg_group, has_decal);
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_geom_node
//       Access: Private
//  Description: Converts a GeomNode to the corresponding egg
//               structures.
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_geom_node(GeomNode *node, const WorkingNodePath &node_path, 
                  EggGroupNode *egg_parent, bool has_decal, CharacterJointMap *jointMap) {
  PT(EggGroup) egg_group = new EggGroup(node->get_name());
  bool fancy_attributes = apply_node_properties(egg_group, node);

  if (node->get_effects()->has_decal()) {
    has_decal = true;
  }

  if (has_decal) {
    egg_group->set_decal_flag(true);
  }

  if (fancy_attributes || has_decal) {
    // If we have any fancy attributes on the node, or if we're making
    // decal geometry, we have to make a special node to hold the
    // geometry (normally it would just appear within its parent).
    egg_parent->add_child(egg_group.p());
    egg_parent = egg_group;
  }

  NodePath np = node_path.get_node_path();
  CPT(RenderState) net_state = np.get_net_state();
  CPT(TransformState) net_transform = np.get_net_transform();
  LMatrix4f net_mat = net_transform->get_mat();
  LMatrix4f inv = LCAST(float, egg_parent->get_vertex_frame_inv());
  net_mat = net_mat * inv;

  // Now get out all the various kinds of geometry.
  int num_geoms = node->get_num_geoms();
  for (int i = 0; i < num_geoms; ++i) {
    CPT(RenderState) geom_state = net_state->compose(node->get_geom_state(i));

    const Geom *geom = node->get_geom(i);
    int num_primitives = geom->get_num_primitives();
    for (int j = 0; j < num_primitives; ++j) {
      const GeomPrimitive *primitive = geom->get_primitive(j);
      CPT(GeomPrimitive) simple = primitive->decompose();
      if (simple->is_of_type(GeomTriangles::get_class_type())) {
        CPT(GeomVertexData) vdata = geom->get_vertex_data();
        //        vdata = vdata->animate_vertices(true, Thread::get_current_thread());
        convert_triangles(vdata,
                          DCAST(GeomTriangles, simple), geom_state,
                          net_mat, egg_parent, jointMap);
      }
    }
  }
  
  recurse_nodes(node_path, egg_parent, has_decal);
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::convert_triangles
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void BamToEgg::
convert_triangles(const GeomVertexData *vertex_data,
                  const GeomTriangles *primitive, 
                  const RenderState *net_state, 
                  const LMatrix4f &net_mat, EggGroupNode *egg_parent,
                  CharacterJointMap *jointMap) {
  GeomVertexReader reader(vertex_data);

  // Check for a color scale.
  LVecBase4f color_scale(1.0f, 1.0f, 1.0f, 1.0f);
  const ColorScaleAttrib *csa = DCAST(ColorScaleAttrib, net_state->get_attrib(ColorScaleAttrib::get_class_type()));
  if (csa != (const ColorScaleAttrib *)NULL) {
    color_scale = csa->get_scale();
  }

  // Check for a color override.
  bool has_color_override = false;
  bool has_color_off = false;
  Colorf color_override;
  const ColorAttrib *ca = DCAST(ColorAttrib, net_state->get_attrib(ColorAttrib::get_class_type()));
  if (ca != (const ColorAttrib *)NULL) {
    if (ca->get_color_type() == ColorAttrib::T_flat) {
      has_color_override = true;
      color_override = ca->get_color();
      color_override.set(color_override[0] * color_scale[0],
                         color_override[1] * color_scale[1],
                         color_override[2] * color_scale[2],
                         color_override[3] * color_scale[3]);

    } else if (ca->get_color_type() == ColorAttrib::T_off) {
      has_color_off = true;
    }
  }

  // Check for a texture.
  EggTexture *egg_tex = (EggTexture *)NULL;
  const TextureAttrib *ta = DCAST(TextureAttrib, net_state->get_attrib(TextureAttrib::get_class_type()));
  if (ta != (const TextureAttrib *)NULL) {
    egg_tex = get_egg_texture(ta->get_texture());
  }

  // Check the texture environment
  if ((ta != (const TextureAttrib *)NULL) && (egg_tex != (const EggTexture *)NULL)) {
    TextureStage* tex_stage = ta->get_on_stage(0);
    if (tex_stage != (const TextureStage *)NULL) {
      switch (tex_stage->get_mode()) {
        case TextureStage::M_modulate:
          if (has_color_off == true) {
            egg_tex->set_env_type(EggTexture::ET_replace);
          } else {
            egg_tex->set_env_type(EggTexture::ET_modulate);
          }
          break;
        case TextureStage::M_decal:
          egg_tex->set_env_type(EggTexture::ET_decal);
          break;
        case TextureStage::M_blend:
          egg_tex->set_env_type(EggTexture::ET_blend);
          break;
        case TextureStage::M_replace:
          egg_tex->set_env_type(EggTexture::ET_replace);
          break;
        case TextureStage::M_add:
          egg_tex->set_env_type(EggTexture::ET_add);
          break;
        case TextureStage::M_blend_color_scale:
          egg_tex->set_env_type(EggTexture::ET_blend_color_scale);
          break;
        default:
          break;
      }
    }
  }

  // Check the backface flag.
  bool bface = false;
  const RenderAttrib *cf_attrib = net_state->get_attrib(CullFaceAttrib::get_class_type());
  if (cf_attrib != (const RenderAttrib *)NULL) {
    const CullFaceAttrib *cfa = DCAST(CullFaceAttrib, cf_attrib);
    if (cfa->get_effective_mode() == CullFaceAttrib::M_cull_none) {
      bface = true;
    }
  }

  // Check the depth write flag - only needed for AM_blend_no_occlude
  bool has_depthwrite = false;
  DepthWriteAttrib::Mode depthwrite = DepthWriteAttrib::M_on;
  const RenderAttrib *dw_attrib = net_state->get_attrib(DepthWriteAttrib::get_class_type());
  if (dw_attrib != (const RenderAttrib *)NULL) {
    const DepthWriteAttrib *dwa = DCAST(DepthWriteAttrib, dw_attrib);
    depthwrite = dwa->get_mode();
    has_depthwrite = true;
  }

  // Check the transparency flag.
  bool has_transparency = false;
  TransparencyAttrib::Mode transparency = TransparencyAttrib::M_none;
  const RenderAttrib *tr_attrib = net_state->get_attrib(TransparencyAttrib::get_class_type());
  if (tr_attrib != (const RenderAttrib *)NULL) {
    const TransparencyAttrib *tra = DCAST(TransparencyAttrib, tr_attrib);
    transparency = tra->get_mode();
    has_transparency = true;
  }
  if (has_transparency && (egg_tex != (EggTexture *)NULL)) {
    EggRenderMode::AlphaMode tex_trans = EggRenderMode::AM_unspecified;
    switch (transparency) {
      case TransparencyAttrib::M_none:
        tex_trans = EggRenderMode::AM_off;
        break;
      case TransparencyAttrib::M_alpha:
        if (has_depthwrite && (depthwrite == DepthWriteAttrib::M_off)) {
            tex_trans = EggRenderMode::AM_blend_no_occlude;
                has_depthwrite = false;
        } else {
          tex_trans = EggRenderMode::AM_blend;
        }
        break;
      case TransparencyAttrib::M_multisample:
        tex_trans = EggRenderMode::AM_ms;
        break;
      case TransparencyAttrib::M_multisample_mask:
        tex_trans = EggRenderMode::AM_ms_mask;
        break;
      case TransparencyAttrib::M_binary:
        tex_trans = EggRenderMode::AM_binary;
        break;
      case TransparencyAttrib::M_dual:
        tex_trans = EggRenderMode::AM_dual;
        break;
      default:  // intentional fall-through
      case TransparencyAttrib::M_notused:
        break;
    }
    if (tex_trans != EggRenderMode::AM_unspecified) {
      egg_tex->set_alpha_mode(tex_trans);
    }
  }


  Normalf normal;
  Colorf color;
  CPT(TransformBlendTable) transformBlendTable = vertex_data->get_transform_blend_table();

  int nprims = primitive->get_num_primitives();
  for (int i = 0; i < nprims; ++i) {
    EggPolygon *egg_poly = new EggPolygon;
    egg_parent->add_child(egg_poly);
    if (egg_tex != (EggTexture *)NULL) {
      egg_poly->set_texture(egg_tex);
    }

    if (bface) {
      egg_poly->set_bface_flag(true);
    }

    for (int j = 0; j < 3; j++) {
      EggVertex egg_vert;

      // Get per-vertex properties.
      reader.set_row(primitive->get_vertex(i * 3 + j));

      reader.set_column(InternalName::get_vertex());
      Vertexf vertex = reader.get_data3f();
      egg_vert.set_pos(LCAST(double, vertex * net_mat));

      if (vertex_data->has_column(InternalName::get_normal())) {
        reader.set_column(InternalName::get_normal());
        Normalf normal = reader.get_data3f();
        egg_vert.set_normal(LCAST(double, normal * net_mat));
      }
      if (has_color_override) {
        egg_vert.set_color(color_override);

      } else if (!has_color_off) {
        Colorf color(1.0f, 1.0f, 1.0f, 1.0f);
        if (vertex_data->has_column(InternalName::get_color())) {
          reader.set_column(InternalName::get_color());
          color = reader.get_data4f();
        }
        egg_vert.set_color(Colorf(color[0] * color_scale[0],
                                  color[1] * color_scale[1],
                                  color[2] * color_scale[2],
                                  color[3] * color_scale[3]));
      }

      if (vertex_data->has_column(InternalName::get_texcoord())) {
        reader.set_column(InternalName::get_texcoord());
        TexCoordf uv = reader.get_data2f();
        egg_vert.set_uv(LCAST(double, uv));
      }

      EggVertex *new_egg_vert = _vpool->create_unique_vertex(egg_vert);

      if ((vertex_data->has_column(InternalName::get_transform_blend())) && 
      (jointMap!=NULL) && (transformBlendTable!=NULL)) {
        reader.set_column(InternalName::get_transform_blend());
        int idx = reader.get_data1i();
        const TransformBlend &blend = transformBlendTable->get_blend(idx);
        int num_weights = blend.get_num_transforms();
        for (int k = 0; k < num_weights; ++k) {
          float weight = blend.get_weight(k);
          if (weight!=0) {
            const VertexTransform *vertex_transform = blend.get_transform(k);
            if (vertex_transform->is_of_type(JointVertexTransform::get_class_type())) {
              const JointVertexTransform *joint_vertex_transform = DCAST(const JointVertexTransform, vertex_transform);

              CharacterJointMap::iterator mi = jointMap->find(joint_vertex_transform->get_joint());
              if (mi == jointMap->end()) {
                mi = jointMap->insert(CharacterJointMap::value_type(joint_vertex_transform->get_joint(), pvector<pair<EggVertex*,float> >())).first;
              }
              pvector<pair<EggVertex*,float> > &joint_vertices = (*mi).second;
              joint_vertices.push_back(pair<EggVertex*,float>(new_egg_vert, weight));
            }
          }
        }
      }

      egg_poly->add_vertex(new_egg_vert);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::recurse_nodes
//       Access: Private
//  Description: Converts all the children of the indicated node.
////////////////////////////////////////////////////////////////////
void BamToEgg::
recurse_nodes(const WorkingNodePath &node_path, EggGroupNode *egg_parent,
              bool has_decal) {
  PandaNode *node = node_path.node();
  int num_children = node->get_num_children();
  
  for (int i = 0; i < num_children; i++) {
    PandaNode *child = node->get_child(i);
    convert_node(WorkingNodePath(node_path, child), egg_parent, has_decal);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::apply_node_properties
//       Access: Public
//  Description: Applies any special properties that might be stored
//               on the node, like billboarding.  Returns true if any
//               were applied, false otherwise.
////////////////////////////////////////////////////////////////////
bool BamToEgg::
apply_node_properties(EggGroup *egg_group, PandaNode *node, bool allow_backstage) {
  bool any_applied = false;

  if (node->is_overall_hidden() && allow_backstage) {
    // This node is hidden.  We'll go ahead and convert it, but we'll
    // put in the "backstage" flag to mean it's not real geometry.
    // unless the caller wants to keep it (by setting allow_backstage to false)
    egg_group->add_object_type("backstage");
  }

  const RenderEffects *effects = node->get_effects();
  const RenderEffect *effect = effects->get_effect(BillboardEffect::get_class_type());
  if (effect != (RenderEffect *)NULL) {
    const BillboardEffect *bbe = DCAST(BillboardEffect, effect);
    if (bbe->get_axial_rotate()) {
      egg_group->set_billboard_type(EggGroup::BT_axis);
      any_applied = true;

    } else if (bbe->get_eye_relative()) {
      egg_group->set_billboard_type(EggGroup::BT_point_camera_relative);
      any_applied = true;

    } else {
      egg_group->set_billboard_type(EggGroup::BT_point_world_relative);
      any_applied = true;
    }
  }

  const TransformState *transform = node->get_transform();
  if (!transform->is_identity()) {
    if (transform->has_components()) {
      // If the transform can be represented componentwise, we prefer
      // storing it that way in the egg file.
      const LVecBase3f &scale = transform->get_scale();
      const LQuaternionf &quat = transform->get_quat();
      const LVecBase3f &pos = transform->get_pos();
      if (!scale.almost_equal(LVecBase3f(1.0f, 1.0f, 1.0f))) {
        egg_group->add_scale3d(LCAST(double, scale));
      }
      if (!quat.is_identity()) {
        egg_group->add_rotate3d(LCAST(double, quat));
      }
      if (!pos.almost_equal(LVecBase3f::zero())) {
        egg_group->add_translate3d(LCAST(double, pos));
      }

    } else if (transform->has_mat()) {
      // Otherwise, we store the raw matrix.
      const LMatrix4f &mat = transform->get_mat();
      egg_group->set_transform3d(LCAST(double, mat));
    }
    any_applied = true;
  }

  return any_applied;
}

////////////////////////////////////////////////////////////////////
//     Function: BamToEgg::get_egg_texture
//       Access: Public
//  Description: Returns an EggTexture pointer that corresponds to the
//               indicated Texture.
////////////////////////////////////////////////////////////////////
EggTexture *BamToEgg::
get_egg_texture(Texture *tex) {
  if (tex != (Texture *)NULL) {
    if (tex->has_filename()) {
      Filename filename = _path_replace->convert_path(tex->get_filename());
      EggTexture temp(filename.get_basename_wo_extension(), filename);
      if (tex->has_alpha_filename()) {
        Filename alpha = _path_replace->convert_path(tex->get_alpha_filename());
        temp.set_alpha_filename(alpha);
      }

      switch (tex->get_minfilter()) {
      case Texture::FT_nearest:
        temp.set_minfilter(EggTexture::FT_nearest);
        break;
      case Texture::FT_linear:
        temp.set_minfilter(EggTexture::FT_linear);
        break;
      case Texture::FT_nearest_mipmap_nearest:
        temp.set_minfilter(EggTexture::FT_nearest_mipmap_nearest);
        break;
      case Texture::FT_linear_mipmap_nearest:
        temp.set_minfilter(EggTexture::FT_linear_mipmap_nearest);
        break;
      case Texture::FT_nearest_mipmap_linear:
        temp.set_minfilter(EggTexture::FT_nearest_mipmap_linear);
        break;
      case Texture::FT_linear_mipmap_linear:
        temp.set_minfilter(EggTexture::FT_linear_mipmap_linear);
        break;

      default:
        break;
      }

      switch (tex->get_magfilter()) {
      case Texture::FT_nearest:
        temp.set_magfilter(EggTexture::FT_nearest);
        break;
      case Texture::FT_linear:
        temp.set_magfilter(EggTexture::FT_linear);
        break;

      default:
        break;
      }

      switch (tex->get_wrap_u()) {
      case Texture::WM_clamp:
        temp.set_wrap_u(EggTexture::WM_clamp);
        break;
      case Texture::WM_repeat:
        temp.set_wrap_u(EggTexture::WM_repeat);
        break;

      default:
        // There are some new wrap options on Texture that aren't yet
        // supported in egg.
        break;
      }

      switch (tex->get_wrap_v()) {
      case Texture::WM_clamp:
        temp.set_wrap_v(EggTexture::WM_clamp);
        break;
      case Texture::WM_repeat:
        temp.set_wrap_v(EggTexture::WM_repeat);
        break;

      default:
        // There are some new wrap options on Texture that aren't yet
        // supported in egg.
        break;
      }

      switch (tex->get_format()) {
      case Texture::F_red:
        temp.set_format(EggTexture::F_red);
        break;
      case Texture::F_green:
        temp.set_format(EggTexture::F_green);
        break;
      case Texture::F_blue:
        temp.set_format(EggTexture::F_blue);
        break;
      case Texture::F_alpha:
        temp.set_format(EggTexture::F_alpha);
        break;
      case Texture::F_rgb:
        temp.set_format(EggTexture::F_rgb);
        break;
      case Texture::F_rgb5:
        temp.set_format(EggTexture::F_rgb5);
        break;
      case Texture::F_rgb8:
        temp.set_format(EggTexture::F_rgb8);
        break;
      case Texture::F_rgb12:
        temp.set_format(EggTexture::F_rgb12);
        break;
      case Texture::F_rgb332:
        temp.set_format(EggTexture::F_rgb332);
        break;
      case Texture::F_rgba:
        temp.set_format(EggTexture::F_rgba);
        break;
      case Texture::F_rgbm:
        temp.set_format(EggTexture::F_rgbm);
        break;
      case Texture::F_rgba4:
        temp.set_format(EggTexture::F_rgba4);
        break;
      case Texture::F_rgba5:
        temp.set_format(EggTexture::F_rgba5);
        break;
      case Texture::F_rgba8:
        temp.set_format(EggTexture::F_rgba8);
        break;
      case Texture::F_rgba12:
        temp.set_format(EggTexture::F_rgba12);
        break;
      case Texture::F_luminance:
        temp.set_format(EggTexture::F_luminance);
        break;
      case Texture::F_luminance_alpha:
        temp.set_format(EggTexture::F_luminance_alpha);
        break;
      case Texture::F_luminance_alphamask:
        temp.set_format(EggTexture::F_luminance_alphamask);
        break;
      default:
        break;
      }

      return _textures.create_unique_texture(temp, ~EggTexture::E_tref_name);
    }
  }

  return NULL;
}


int main(int argc, char *argv[]) {
  // A call to pystub() to force libpystub.so to be linked in.
  pystub();

  BamToEgg prog;
  prog.parse_command_line(argc, argv);
  prog.run();
  return 0;
}
