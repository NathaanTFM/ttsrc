// Filename: nodePath.h
// Created by:  drose (25Feb02)
// Updated by:  fperazzi, PandaSE (06Apr10) (added more overloads
//   for set_shader_input)
// Updated by: weifengh, PandaSE(30Apr10)
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

#ifndef NODEPATH_H
#define NODEPATH_H

#include "pandabase.h"

#include "pandaNode.h"
#include "renderState.h"
#include "transformState.h"
#include "renderModeAttrib.h"
#include "transparencyAttrib.h"
#include "nodePathComponent.h"
#include "pointerTo.h"
#include "referenceCount.h"
#include "pnotify.h"
#include "typedObject.h"
#include "pta_float.h"
#include "pta_double.h"
#include "pta_LMatrix4f.h"
#include "pta_LMatrix3f.h"
#include "pta_LVecBase4f.h"
#include "pta_LVecBase3f.h"
#include "pta_LVecBase2f.h"

class NodePathCollection;
class FindApproxPath;
class FindApproxLevelEntry;
class Light;
class PolylightNode;
class InternalNameCollection;
class Texture;
class TextureStage;
class TextureCollection;
class TextureStageCollection;
class Material;
class MaterialCollection;
class Fog;
class GlobPattern;
class PreparedGraphicsObjects;
class Shader;
class ShaderInput;

//
// A NodePath is the fundamental unit of high-level interaction with
// the scene graph.  It encapsulates the complete path down to a node
// from some other node, usually the root of the scene graph.  This is
// used to resolve ambiguities associated with instancing.
//
// NodePath also contains a number of handy high-level methods for
// common scene-graph manipulations, such as reparenting, and common
// state changes, such as repositioning.
//
// There are also a number of NodePath methods for finding nodes deep
// within the tree by name or by type.  These take a path string,
// which at its simplest consists of a series of node names separated
// by slashes, like a directory pathname.
//
// Each component of the path string may optionally consist of one of
// the following special names, instead of a node name:
//
//   *          -- matches exactly one node, with any name.
//   **         -- matches any sequence of zero or more nodes.
//   +typename  -- matches any node that is or derives from the given type.
//   -typename  -- matches any node that is the given type exactly.
//   =tag       -- matches any node that has the indicated tag.
//   =tag=value -- matches any node whose tag matches the indicated value.
//
// Furthermore, a node name may itself contain standard filename
// globbing characters, like *, ?, and [a-z], that will be accepted as
// a partial match.  (In fact, the '*' special name may be seen as
// just a special case of this.)  The globbing characters may not be
// used with the typename matches or with tag matches, but they may
// be used to match a tag's value in the =tag=value syntax.
//
// The special characters "@@", appearing at the beginning of a node
// name, indicate a stashed node.  Normally, stashed nodes are not
// returned by a find (but see the special flags, below), but a
// stashed node may be found if it is explicitly named with its
// leading @@ characters.  By extension, "@@*" may be used to identify
// any stashed node.
//
// Examples:
//
// "room//graph" will look for a node named "graph", which is a child
// of an unnamed node, which is a child of a node named "room", which
// is a child of the starting path.
//
// "**/red*" will look for any node anywhere in the tree (below the
// starting path) with a name that begins with "red".
//
// "**/+PartBundleNode/**/head" will look for a node named "head",
// somewhere below a PartBundleNode anywhere in the tree.
//
//
// The search is always potentially ambiguous, even if the special
// wildcard operators are not used, because there may be multiple
// nodes in the tree with the same name.  In general, in the case of
// an ambiguity, the shortest path is preferred; when a method (such
// as extend_by) must choose only only one of several possible paths,
// it will choose the shortest available; on the other hand, when a
// method (such as find_all_matches) is to return all of the matching
// paths, it will sort them so that the shortest paths appear first in
// the output.
//
//
// Special flags.  The entire string may optionally be followed by the
// ";" character, followed by one or more of the following special
// control flags, with no intervening spaces or punctuation:
//
//    -h    Do not return hidden nodes.
//    +h    Do return hidden nodes.
//    -s    Do not return stashed nodes unless explicitly referenced with @@.
//    +s    Return stashed nodes even without any explicit @@ characters.
//    -i    Node name comparisons are not case insensitive: case must match
//          exactly.
//    +i    Node name comparisons are case insensitive: case is not important.
//          This affects matches against the node name only; node type
//          and tag strings are always case sensitive.
//
// The default flags are +h-s-i.
//


////////////////////////////////////////////////////////////////////
//       Class : NodePath
// Description : NodePath is the fundamental system for disambiguating
//               instances, and also provides a higher-level interface
//               for manipulating the scene graph.
//
//               A NodePath is a list of connected nodes from the root
//               of the graph to any sub-node.  Each NodePath
//               therefore uniquely describes one instance of a node.
//
//               NodePaths themselves are lightweight objects that may
//               easily be copied and passed by value.  Their data is
//               stored as a series of NodePathComponents that are
//               stored on the nodes.  Holding a NodePath will keep a
//               reference count to all the nodes in the path.
//               However, if any node in the path is removed or
//               reparented (perhaps through a different NodePath),
//               the NodePath will automatically be updated to reflect
//               the changes.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_PGRAPH NodePath {
PUBLISHED:
  // This enumeration is returned by get_error_type() for an empty
  // NodePath to report the reason it's empty.
  enum ErrorType {
    ET_ok = 0,     // i.e. not empty, or never assigned to anything.
    ET_not_found,  // returned from a failed find() or similar function.
    ET_removed,    // remove_node() was previously called on this NodePath.
    ET_fail,       // general failure return from some function.
  };

  INLINE NodePath();
  INLINE NodePath(const string &top_node_name, Thread *current_thread = Thread::get_current_thread());
  INLINE NodePath(PandaNode *node, Thread *current_thread = Thread::get_current_thread());
  INLINE static NodePath any_path(PandaNode *node, Thread *current_thread = Thread::get_current_thread());
  NodePath(const NodePath &parent, PandaNode *child_node,
           Thread *current_thread = Thread::get_current_thread());

  INLINE NodePath(const NodePath &copy);
  INLINE void operator = (const NodePath &copy);

#ifdef HAVE_PYTHON
  NodePath __copy__() const;
  PyObject *__deepcopy__(PyObject *self, PyObject *memo) const;
  PyObject *__reduce__(PyObject *self) const;
  PyObject *__reduce_persist__(PyObject *self, PyObject *pickler) const;
#endif

  INLINE static NodePath not_found();
  INLINE static NodePath removed();
  INLINE static NodePath fail();

  INLINE static void set_max_search_depth(int max_search_depth);
  INLINE static int get_max_search_depth();

  // Methods to query a NodePath's contents.
  INLINE bool is_empty() const;
  operator bool () const;

  INLINE bool is_singleton(Thread *current_thread = Thread::get_current_thread()) const;
  int get_num_nodes(Thread *current_thread = Thread::get_current_thread()) const;
  PandaNode *get_node(int index, Thread *current_thread = Thread::get_current_thread()) const;
  MAKE_SEQ(get_nodes, get_num_nodes, get_node);
  NodePath get_ancestor(int index, Thread *current_thread = Thread::get_current_thread()) const;
  MAKE_SEQ(get_ancestors, get_num_nodes, get_ancestor);

  INLINE ErrorType get_error_type() const;

  INLINE PandaNode *get_top_node(Thread *current_thread = Thread::get_current_thread()) const;
  NodePath get_top(Thread *current_thread = Thread::get_current_thread()) const;

  INLINE PandaNode *node() const;

  INLINE int get_key() const;

  INLINE bool is_same_graph(const NodePath &other, Thread *current_thread = Thread::get_current_thread()) const;
  INLINE bool is_ancestor_of(const NodePath &other, Thread *current_thread = Thread::get_current_thread()) const;
  INLINE NodePath get_common_ancestor(const NodePath &other, Thread *current_thread = Thread::get_current_thread()) const;

  // Methods that return collections of NodePaths derived from or
  // related to this one.

  NodePathCollection get_children(Thread *current_thread = Thread::get_current_thread()) const;
  INLINE int get_num_children(Thread *current_thread = Thread::get_current_thread()) const;
  INLINE NodePath get_child(int n, Thread *current_thread = Thread::get_current_thread()) const;
  NodePathCollection get_stashed_children(Thread *current_thread = Thread::get_current_thread()) const;

  INLINE int count_num_descendants() const;

  INLINE bool has_parent(Thread *current_thread = Thread::get_current_thread()) const;
  INLINE NodePath get_parent(Thread *current_thread = Thread::get_current_thread()) const;
  int get_sort(Thread *current_thread = Thread::get_current_thread()) const;

  NodePath find(const string &path) const;
  NodePath find_path_to(PandaNode *node) const;
  NodePathCollection find_all_matches(const string &path) const;
  NodePathCollection find_all_paths_to(PandaNode *node) const;

  // Methods that actually move nodes around in the scene graph.  The
  // optional "sort" parameter can be used to force a particular
  // ordering between sibling nodes, useful when dealing with LOD's
  // and similar switch nodes.  If the sort value is the same, nodes
  // will be arranged in the order they were added.
  void reparent_to(const NodePath &other, int sort = 0,
                   Thread *current_thread = Thread::get_current_thread());
  void stash_to(const NodePath &other, int sort = 0,
                Thread *current_thread = Thread::get_current_thread());
  void wrt_reparent_to(const NodePath &other, int sort = 0,
                       Thread *current_thread = Thread::get_current_thread());
  NodePath instance_to(const NodePath &other, int sort = 0,
                       Thread *current_thread = Thread::get_current_thread()) const;
  NodePath instance_under_node(const NodePath &other, const string &name,
                               int sort = 0,
                               Thread *current_thread = Thread::get_current_thread()) const;
  NodePath copy_to(const NodePath &other, int sort = 0,
                   Thread *current_thread = Thread::get_current_thread()) const;
  NodePath attach_new_node(PandaNode *node, int sort = 0,
                           Thread *current_thread = Thread::get_current_thread()) const;
  INLINE NodePath attach_new_node(const string &name, int sort = 0,
                                  Thread *current_thread = Thread::get_current_thread()) const;
  void remove_node(Thread *current_thread = Thread::get_current_thread());
  void detach_node(Thread *current_thread = Thread::get_current_thread());

  // Handy ways to look at what's there, and other miscellaneous
  // operations.

  void output(ostream &out) const;

  INLINE void ls() const;
  INLINE void ls(ostream &out, int indent_level = 0) const;
  INLINE void reverse_ls() const;
  INLINE int reverse_ls(ostream &out, int indent_level = 0) const;


  // Aggregate transform and state information.
  const RenderState *get_state(Thread *current_thread = Thread::get_current_thread()) const;
  INLINE void set_state(const RenderState *state, Thread *current_thread = Thread::get_current_thread());
  CPT(RenderState) get_state(const NodePath &other, Thread *current_thread = Thread::get_current_thread()) const;
  void set_state(const NodePath &other, const RenderState *state, Thread *current_thread = Thread::get_current_thread());
  INLINE CPT(RenderState) get_net_state(Thread *current_thread = Thread::get_current_thread()) const;

  INLINE void set_attrib(const RenderAttrib *attrib, int priority = 0);
  INLINE const RenderAttrib *get_attrib(TypeHandle type) const;
  INLINE bool has_attrib(TypeHandle type) const;
  INLINE void clear_attrib(TypeHandle type);

  INLINE void set_effect(const RenderEffect *effect);
  INLINE const RenderEffect *get_effect(TypeHandle type) const;
  INLINE bool has_effect(TypeHandle type) const;
  INLINE void clear_effect(TypeHandle type);

  INLINE void set_effects(const RenderEffects *effects);
  INLINE const RenderEffects *get_effects() const;
  INLINE void clear_effects();

  const TransformState *get_transform(Thread *current_thread = Thread::get_current_thread()) const;
  INLINE void clear_transform(Thread *current_thread = Thread::get_current_thread());
  INLINE void set_transform(const TransformState *transform, Thread *current_thread = Thread::get_current_thread());
  CPT(TransformState) get_transform(const NodePath &other, Thread *current_thread = Thread::get_current_thread()) const;
  INLINE void clear_transform(const NodePath &other, Thread *current_thread = Thread::get_current_thread());
  void set_transform(const NodePath &other, const TransformState *transform, Thread *current_thread = Thread::get_current_thread());
  INLINE CPT(TransformState) get_net_transform(Thread *current_thread = Thread::get_current_thread()) const;

  const TransformState *get_prev_transform(Thread *current_thread = Thread::get_current_thread()) const;
  INLINE void set_prev_transform(const TransformState *transform, Thread *current_thread = Thread::get_current_thread());
  CPT(TransformState) get_prev_transform(const NodePath &other, Thread *current_thread = Thread::get_current_thread()) const;
  void set_prev_transform(const NodePath &other, const TransformState *transform, Thread *current_thread = Thread::get_current_thread());
  INLINE CPT(TransformState) get_net_prev_transform(Thread *current_thread = Thread::get_current_thread()) const;


  // Methods that get and set the matrix transform: pos, hpr, scale,
  // in the local coordinate system.

  INLINE void set_pos(float x, float y, float z);
  void set_pos(const LVecBase3f &pos);
  void set_x(float x);
  void set_y(float y);
  void set_z(float z);
  INLINE void set_fluid_pos(float x, float y, float z);
  void set_fluid_pos(const LVecBase3f &pos);
  void set_fluid_x(float x);
  void set_fluid_y(float y);
  void set_fluid_z(float z);
  LPoint3f get_pos() const;
  INLINE float get_x() const;
  INLINE float get_y() const;
  INLINE float get_z() const;

  LVector3f get_pos_delta() const;

  INLINE void set_hpr(float h, float p, float r);
  void set_hpr(const LVecBase3f &hpr);
  void set_h(float h);
  void set_p(float p);
  void set_r(float r);
  LVecBase3f get_hpr() const;
  INLINE float get_h() const;
  INLINE float get_p() const;
  INLINE float get_r() const;

  void set_quat(const LQuaternionf &quat);
  LQuaternionf get_quat() const;

  INLINE void set_scale(float scale);
  INLINE void set_scale(float sx, float sy, float sz);
  void set_scale(const LVecBase3f &scale);
  void set_sx(float sx);
  void set_sy(float sy);
  void set_sz(float sz);
  LVecBase3f get_scale() const;
  INLINE float get_sx() const;
  INLINE float get_sy() const;
  INLINE float get_sz() const;

  INLINE void set_shear(float shxy, float shxz, float shyz);
  void set_shear(const LVecBase3f &shear);
  void set_shxy(float shxy);
  void set_shxz(float shxz);
  void set_shyz(float shyz);
  LVecBase3f get_shear() const;
  INLINE float get_shxy() const;
  INLINE float get_shxz() const;
  INLINE float get_shyz() const;

  INLINE void set_pos_hpr(float x, float y, float z,
                          float h, float p, float r);
  void set_pos_hpr(const LVecBase3f &pos,
                   const LVecBase3f &hpr);
  void set_pos_quat(const LVecBase3f &pos,
                    const LQuaternionf &quat);

  INLINE void set_hpr_scale(float h, float p, float r,
                            float sx, float sy, float sz);
  void set_hpr_scale(const LVecBase3f &hpr,
                     const LVecBase3f &scale);
  void set_quat_scale(const LQuaternionf &quat,
                      const LVecBase3f &scale);
  INLINE void set_pos_hpr_scale(float x, float y, float z,
                                float h, float p, float r,
                                float sx, float sy, float sz);
  void set_pos_hpr_scale(const LVecBase3f &pos,
                         const LVecBase3f &hpr,
                         const LVecBase3f &scale);
  void set_pos_quat_scale(const LVecBase3f &pos,
                          const LQuaternionf &quat,
                          const LVecBase3f &scale);
  void set_pos_hpr_scale_shear(const LVecBase3f &pos,
                               const LVecBase3f &hpr,
                               const LVecBase3f &scale,
                               const LVecBase3f &shear);
  void set_pos_quat_scale_shear(const LVecBase3f &pos,
                                const LQuaternionf &quat,
                                const LVecBase3f &scale,
                                const LVecBase3f &shear);

  void set_mat(const LMatrix4f &mat);
  INLINE void clear_mat();
  INLINE bool has_mat() const;
  INLINE const LMatrix4f &get_mat() const;

  INLINE void look_at(float x, float y, float z);
  void look_at(const LPoint3f &point, const LVector3f &up = LVector3f::up());
  INLINE void heads_up(float x, float y, float z);
  void heads_up(const LPoint3f &point, const LVector3f &up = LVector3f::up());

  // Methods that get and set the matrix transforms relative to some
  // other node in the scene graph.  These perform an implicit wrt().

  INLINE void set_pos(const NodePath &other, float x, float y, float z);
  void set_pos(const NodePath &other, const LVecBase3f &pos);
  void set_x(const NodePath &other, float x);
  void set_y(const NodePath &other, float y);
  void set_z(const NodePath &other, float z);
  INLINE void set_fluid_pos(const NodePath &other, float x, float y, float z);
  void set_fluid_pos(const NodePath &other, const LVecBase3f &pos);
  void set_fluid_x(const NodePath &other, float x);
  void set_fluid_y(const NodePath &other, float y);
  void set_fluid_z(const NodePath &other, float z);
  LPoint3f get_pos(const NodePath &other) const;
  INLINE float get_x(const NodePath &other) const;
  INLINE float get_y(const NodePath &other) const;
  INLINE float get_z(const NodePath &other) const;

  LVector3f get_pos_delta(const NodePath &other) const;

  INLINE void set_hpr(const NodePath &other, float h, float p, float r);
  void set_hpr(const NodePath &other, const LVecBase3f &hpr);
  void set_h(const NodePath &other, float h);
  void set_p(const NodePath &other, float p);
  void set_r(const NodePath &other, float r);
  LVecBase3f get_hpr(const NodePath &other) const;
  INLINE float get_h(const NodePath &other) const;
  INLINE float get_p(const NodePath &other) const;
  INLINE float get_r(const NodePath &other) const;

  void set_quat(const NodePath &other, const LQuaternionf &quat);
  LQuaternionf get_quat(const NodePath &other) const;

  INLINE void set_scale(const NodePath &other, float scale);
  INLINE void set_scale(const NodePath &other, float sx, float sy, float sz);
  void set_scale(const NodePath &other, const LVecBase3f &scale);
  void set_sx(const NodePath &other, float sx);
  void set_sy(const NodePath &other, float sy);
  void set_sz(const NodePath &other, float sz);
  LVecBase3f get_scale(const NodePath &other) const;
  INLINE float get_sx(const NodePath &other) const;
  INLINE float get_sy(const NodePath &other) const;
  INLINE float get_sz(const NodePath &other) const;

  INLINE void set_shear(const NodePath &other, float shxy, float shxz, float shyz);
  void set_shear(const NodePath &other, const LVecBase3f &shear);
  void set_shxy(const NodePath &other, float shxy);
  void set_shxz(const NodePath &other, float shxz);
  void set_shyz(const NodePath &other, float shyz);
  LVecBase3f get_shear(const NodePath &other) const;
  INLINE float get_shxy(const NodePath &other) const;
  INLINE float get_shxz(const NodePath &other) const;
  INLINE float get_shyz(const NodePath &other) const;

  INLINE void set_pos_hpr(const NodePath &other,
                          float x, float y, float z,
                          float h, float p, float r);
  void set_pos_hpr(const NodePath &other,
                   const LVecBase3f &pos,
                   const LVecBase3f &hpr);
  void set_pos_quat(const NodePath &other,
                    const LVecBase3f &pos,
                    const LQuaternionf &quat);
  INLINE void set_hpr_scale(const NodePath &other,
                            float h, float p, float r,
                            float sx, float sy, float sz);
  void set_hpr_scale(const NodePath &other,
                     const LVecBase3f &hpr,
                     const LVecBase3f &scale);
  void set_quat_scale(const NodePath &other,
                      const LQuaternionf &quat,
                      const LVecBase3f &scale);
  INLINE void set_pos_hpr_scale(const NodePath &other,
                                float x, float y, float z,
                                float h, float p, float r,
                                float sx, float sy, float sz);
  void set_pos_hpr_scale(const NodePath &other,
                         const LVecBase3f &pos,
                         const LVecBase3f &hpr,
                         const LVecBase3f &scale);
  void set_pos_quat_scale(const NodePath &other,
                          const LVecBase3f &pos,
                          const LQuaternionf &quat,
                          const LVecBase3f &scale);
  void set_pos_hpr_scale_shear(const NodePath &other,
                               const LVecBase3f &pos,
                               const LVecBase3f &hpr,
                               const LVecBase3f &scale,
                               const LVecBase3f &shear);
  void set_pos_quat_scale_shear(const NodePath &other,
                                const LVecBase3f &pos,
                                const LQuaternionf &quat,
                                const LVecBase3f &scale,
                                const LVecBase3f &shear);

  LMatrix4f get_mat(const NodePath &other) const;
  void set_mat(const NodePath &other, const LMatrix4f &mat);

  LPoint3f get_relative_point(const NodePath &other, const LVecBase3f &point) const;
  LVector3f get_relative_vector(const NodePath &other, const LVecBase3f &vec) const;

  INLINE void look_at(const NodePath &other,
                      float x, float y, float z);
  void look_at(const NodePath &other,
               const LPoint3f &point = LPoint3f(0.0, 0.0, 0.0),
               const LVector3f &up = LVector3f::up());
  INLINE void heads_up(const NodePath &other,
                       float x, float y, float z);
  void heads_up(const NodePath &other,
                const LPoint3f &point = LPoint3f(0.0, 0.0, 0.0),
                const LVector3f &up = LVector3f::up());

  INLINE float get_distance(const NodePath &other) const;


  // Methods that affect appearance of geometry: color, texture, etc.
  // These affect the state at the bottom level only.

  void set_color(float r, float g, float b, float a = 1.0,
                 int priority = 0);
  void set_color(const Colorf &color, int priority = 0);
  void set_color_off(int priority = 0);
  void clear_color();
  bool has_color() const;
  Colorf get_color() const;

  bool has_color_scale() const;
  void clear_color_scale();
  void set_color_scale(const LVecBase4f &scale,
                       int priority = 0);
  INLINE void set_color_scale(float sx, float sy, float sz, float sa,
                              int priority = 0);
  void compose_color_scale(const LVecBase4f &scale,
                           int priority = 0);
  INLINE void compose_color_scale(float sx, float sy, float sz, float sa,
                                  int priority = 0);
  void set_color_scale_off(int priority = 0);
  
  void set_alpha_scale(float scale, int priority = 0);
  void set_all_color_scale(float scale, int priority = 0);
  INLINE void set_sr(float sr);
  INLINE void set_sg(float sg);
  INLINE void set_sb(float sb);
  INLINE void set_sa(float sa);

  const LVecBase4f &get_color_scale() const;
  INLINE float get_sr() const;
  INLINE float get_sg() const;
  INLINE float get_sb() const;
  INLINE float get_sa() const;

  void set_light(const NodePath &light, int priority = 0);
  void set_light_off(int priority = 0);
  void set_light_off(const NodePath &light, int priority = 0);
  void clear_light();
  void clear_light(const NodePath &light);
  bool has_light(const NodePath &light) const;
  bool has_light_off() const;
  bool has_light_off(const NodePath &light) const;

  void set_clip_plane(const NodePath &clip_plane, int priority = 0);
  void set_clip_plane_off(int priority = 0);
  void set_clip_plane_off(const NodePath &clip_plane, int priority = 0);
  void clear_clip_plane();
  void clear_clip_plane(const NodePath &clip_plane);
  bool has_clip_plane(const NodePath &clip_plane) const;
  bool has_clip_plane_off() const;
  bool has_clip_plane_off(const NodePath &clip_plane) const;

  void set_scissor(float left, float right, float bottom, float top);
  void set_scissor(const LPoint3f &a, const LPoint3f &b);
  void set_scissor(const LPoint3f &a, const LPoint3f &b, 
                   const LPoint3f &c, const LPoint3f &d);
  void set_scissor(const NodePath &other, 
                   const LPoint3f &a, const LPoint3f &b);
  void set_scissor(const NodePath &other,
                   const LPoint3f &a, const LPoint3f &b, 
                   const LPoint3f &c, const LPoint3f &d);
  void clear_scissor();
  bool has_scissor() const;

  void set_bin(const string &bin_name, int draw_order, int priority = 0);
  void clear_bin();
  bool has_bin() const;
  string get_bin_name() const;
  int get_bin_draw_order() const;

  void set_texture(Texture *tex, int priority = 0);
  void set_texture(TextureStage *stage, Texture *tex, int priority = 0);
  void set_texture_off(int priority = 0);
  void set_texture_off(TextureStage *stage, int priority = 0);
  void clear_texture();
  void clear_texture(TextureStage *stage);
  bool has_texture() const;
  bool has_texture(TextureStage *stage) const;
  bool has_texture_off() const;
  bool has_texture_off(TextureStage *stage) const;
  Texture *get_texture() const;
  Texture *get_texture(TextureStage *stage) const;

  void set_shader(const Shader *sha, int priority = 0);
  void set_shader_off(int priority = 0);
  void set_shader_auto(int priority = 0);
  void set_shader_auto(BitMask32 shader_switch, int priority=0);
  void clear_shader();
  
  void set_shader_input(const ShaderInput *inp);
  void set_shader_input(InternalName *id, Texture *tex,       int priority=0);
  void set_shader_input(InternalName *id, const NodePath &np, int priority=0);
  void set_shader_input(InternalName *id, const PTA_float &v, int priority=0);
  void set_shader_input(InternalName *id, const PTA_double &v, int priority=0);
  void set_shader_input(InternalName *id, const PTA_LVecBase4f &v, int priority=0);
  void set_shader_input(InternalName *id, const PTA_LVecBase3f &v, int priority=0);
  void set_shader_input(InternalName *id, const PTA_LVecBase2f &v, int priority=0);
  void set_shader_input(InternalName *id, const PTA_LMatrix4f &v, int priority=0); 
  void set_shader_input(InternalName *id, const PTA_LMatrix3f &v, int priority=0); 
  void set_shader_input(InternalName *id, const LVecBase4f &v, int priority=0); 
  void set_shader_input(InternalName *id, const LVecBase3f &v, int priority=0); 
  void set_shader_input(InternalName *id, const LVecBase2f &v, int priority=0); 
  void set_shader_input(InternalName *id, const LMatrix4f &v, int priority=0); 
  void set_shader_input(InternalName *id, const LMatrix3f &v, int priority=0);
  void set_shader_input(InternalName *id, double  n1=0, double n2=0, double n3=0, 
      double n4=1, int priority=0);

  void set_shader_input(const string &id, Texture *tex,       int priority=0);
  void set_shader_input(const string &id, const NodePath &np, int priority=0);
  void set_shader_input(const string &id, const PTA_float &v, int priority=0); 
  void set_shader_input(const string &id, const PTA_double &v, int priority=0); 
  void set_shader_input(const string &id, const PTA_LVecBase4f &v, int priority=0); 
  void set_shader_input(const string &id, const PTA_LVecBase3f &v, int priority=0); 
  void set_shader_input(const string &id, const PTA_LVecBase2f &v, int priority=0); 
  void set_shader_input(const string &id, const PTA_LMatrix4f &v, int priority=0);
  void set_shader_input(const string &id, const PTA_LMatrix3f &v, int priority=0);
  void set_shader_input(const string &id, const LVecBase4f &v, int priority=0); 
  void set_shader_input(const string &id, const LVecBase3f &v, int priority=0); 
  void set_shader_input(const string &id, const LVecBase2f &v, int priority=0); 
  void set_shader_input(const string &id, const LMatrix4f &v, int priority=0); 
  void set_shader_input(const string &id, const LMatrix3f &v, int priority=0); 
  void set_shader_input(const string &id, double  n1=0, double n2=0, double n3=0, 
      double n4=1, int priority=0);
  
  void clear_shader_input(InternalName *id);
  void clear_shader_input(const string &id);
  void set_instance_count(int instance_count);

  const Shader *get_shader() const;
  const ShaderInput *get_shader_input(InternalName *id) const;
  const ShaderInput *get_shader_input(const string &id) const;
  const int get_instance_count() const;
  
  void set_tex_transform(TextureStage *stage, const TransformState *transform);
  void clear_tex_transform();
  void clear_tex_transform(TextureStage *stage);
  bool has_tex_transform(TextureStage *stage) const;
  CPT(TransformState) get_tex_transform(TextureStage *stage) const;

  INLINE void set_tex_offset(TextureStage *stage, float u, float v);
  INLINE void set_tex_offset(TextureStage *stage, const LVecBase2f &uv);
  INLINE void set_tex_rotate(TextureStage *stage, float r);
  INLINE void set_tex_scale(TextureStage *stage, float scale);
  INLINE void set_tex_scale(TextureStage *stage, float su, float sv);
  INLINE void set_tex_scale(TextureStage *stage, const LVecBase2f &scale);
  INLINE LVecBase2f get_tex_offset(TextureStage *stage) const;
  INLINE float get_tex_rotate(TextureStage *stage) const;
  INLINE LVecBase2f get_tex_scale(TextureStage *stage) const;

  INLINE void set_tex_pos(TextureStage *stage, float u, float v, float w);
  INLINE void set_tex_pos(TextureStage *stage, const LVecBase3f &uvw);
  INLINE void set_tex_hpr(TextureStage *stage, float h, float p, float r);
  INLINE void set_tex_hpr(TextureStage *stage, const LVecBase3f &hpr);
  INLINE void set_tex_scale(TextureStage *stage, float su, float sv, float sw);
  INLINE void set_tex_scale(TextureStage *stage, const LVecBase3f &scale);
  INLINE LVecBase3f get_tex_pos(TextureStage *stage) const;
  INLINE LVecBase3f get_tex_hpr(TextureStage *stage) const;
  INLINE LVecBase3f get_tex_scale_3d(TextureStage *stage) const;

  void set_tex_transform(const NodePath &other, TextureStage *stage, const TransformState *transform);
  CPT(TransformState) get_tex_transform(const NodePath &other, TextureStage *stage) const;

  INLINE void set_tex_offset(const NodePath &other, TextureStage *stage, float u, float v);
  INLINE void set_tex_offset(const NodePath &other, TextureStage *stage, const LVecBase2f &uv);
  INLINE void set_tex_rotate(const NodePath &other, TextureStage *stage, float r);
  INLINE void set_tex_scale(const NodePath &other, TextureStage *stage, float scale);
  INLINE void set_tex_scale(const NodePath &other, TextureStage *stage, float su, float sv);
  INLINE void set_tex_scale(const NodePath &other, TextureStage *stage, const LVecBase2f &scale);
  INLINE LVecBase2f get_tex_offset(const NodePath &other, TextureStage *stage) const;
  INLINE float get_tex_rotate(const NodePath &other, TextureStage *stage) const;
  INLINE LVecBase2f get_tex_scale(const NodePath &other, TextureStage *stage) const;

  INLINE void set_tex_pos(const NodePath &other, TextureStage *stage, float u, float v, float w);
  INLINE void set_tex_pos(const NodePath &other, TextureStage *stage, const LVecBase3f &uvw);
  INLINE void set_tex_hpr(const NodePath &other, TextureStage *stage, float h, float p, float r);
  INLINE void set_tex_hpr(const NodePath &other, TextureStage *stage, const LVecBase3f &hpr);
  INLINE void set_tex_scale(const NodePath &other, TextureStage *stage, float su, float sv, float sw);
  INLINE void set_tex_scale(const NodePath &other, TextureStage *stage, const LVecBase3f &scale);
  INLINE LVecBase3f get_tex_pos(const NodePath &other, TextureStage *stage) const;
  INLINE LVecBase3f get_tex_hpr(const NodePath &other, TextureStage *stage) const;
  INLINE LVecBase3f get_tex_scale_3d(const NodePath &other, TextureStage *stage) const;

  void set_tex_gen(TextureStage *stage, RenderAttrib::TexGenMode mode, int priority = 0);
  void set_tex_gen(TextureStage *stage, RenderAttrib::TexGenMode mode, 
                   const string &source_name, const NodePath &light, 
                   int priority = 0);
  void set_tex_gen(TextureStage *stage, RenderAttrib::TexGenMode mode, 
                   const TexCoord3f &constant_value,
                   int priority = 0);
  void clear_tex_gen();
  void clear_tex_gen(TextureStage *stage);
  bool has_tex_gen(TextureStage *stage) const;
  RenderAttrib::TexGenMode get_tex_gen(TextureStage *stage) const;
  NodePath get_tex_gen_light(TextureStage *stage) const;

  void set_tex_projector(TextureStage *stage, const NodePath &from, const NodePath &to);
  void clear_tex_projector(TextureStage *stage);
  void clear_tex_projector();
  bool has_tex_projector(TextureStage *stage) const;
  NodePath get_tex_projector_from(TextureStage *stage) const;
  NodePath get_tex_projector_to(TextureStage *stage) const;

  void project_texture(TextureStage *stage, Texture *tex, const NodePath &projector);
  INLINE void clear_project_texture(TextureStage *stage);

  void set_normal_map(Texture *normal_map, const string &texcoord_name = string(),
                      bool preserve_color = false);
  void clear_normal_map();

  INLINE bool has_texcoord(const string &texcoord_name) const;
  bool has_vertex_column(const InternalName *name) const;
  InternalNameCollection find_all_vertex_columns() const;
  InternalNameCollection find_all_vertex_columns(const string &name) const;
  InternalNameCollection find_all_texcoords() const;
  InternalNameCollection find_all_texcoords(const string &name) const;

  Texture *find_texture(const string &name) const;
  Texture *find_texture(TextureStage *stage) const;
  TextureCollection find_all_textures() const;
  TextureCollection find_all_textures(const string &name) const;
  TextureCollection find_all_textures(TextureStage *stage) const;

  TextureStage *find_texture_stage(const string &name) const;
  TextureStageCollection find_all_texture_stages() const;
  TextureStageCollection find_all_texture_stages(const string &name) const;

  void unify_texture_stages(TextureStage *stage);

  Material *find_material(const string &name) const;
  MaterialCollection find_all_materials() const;
  MaterialCollection find_all_materials(const string &name) const;

  void set_material(Material *tex, int priority = 0);
  void set_material_off(int priority = 0);
  void clear_material();
  bool has_material() const;
  PT(Material) get_material() const;

  void set_fog(Fog *fog, int priority = 0);
  void set_fog_off(int priority = 0);
  void clear_fog();
  bool has_fog() const;
  bool has_fog_off() const;
  Fog *get_fog() const;

  void set_render_mode_wireframe(int priority = 0);
  void set_render_mode_filled(int priority = 0);
  void set_render_mode_thickness(float thickness, int priority = 0);
  void set_render_mode_perspective(bool perspective, int priority = 0);
  void set_render_mode(RenderModeAttrib::Mode mode, float thickness, int priority = 0);
  void clear_render_mode();
  bool has_render_mode() const;
  RenderModeAttrib::Mode get_render_mode() const;
  float get_render_mode_thickness() const;
  bool get_render_mode_perspective() const;

  void set_two_sided(bool two_sided, int priority = 0);
  void clear_two_sided();
  bool has_two_sided() const;
  bool get_two_sided() const;

  void set_depth_test(bool depth_test, int priority = 0);
  void clear_depth_test();
  bool has_depth_test() const;
  bool get_depth_test() const;

  void set_depth_write(bool depth_write, int priority = 0);
  void clear_depth_write();
  bool has_depth_write() const;
  bool get_depth_write() const;

  void set_depth_offset(int bias, int priority = 0);
  void clear_depth_offset();
  bool has_depth_offset() const;
  int get_depth_offset() const;

  void do_billboard_axis(const NodePath &camera, float offset);
  void do_billboard_point_eye(const NodePath &camera, float offset);
  void do_billboard_point_world(const NodePath &camera, float offset);
  INLINE void set_billboard_axis(float offset = 0.0);
  INLINE void set_billboard_point_eye(float offset = 0.0);
  INLINE void set_billboard_point_world(float offset = 0.0);
  void set_billboard_axis(const NodePath &camera, float offset);
  void set_billboard_point_eye(const NodePath &camera, float offset);
  void set_billboard_point_world(const NodePath &camera, float offset);
  void clear_billboard();
  bool has_billboard() const;

  void set_compass(const NodePath &reference = NodePath());
  void clear_compass();
  bool has_compass() const;

  void set_transparency(TransparencyAttrib::Mode mode, int priority = 0);
  void clear_transparency();
  bool has_transparency() const;
  TransparencyAttrib::Mode get_transparency() const;

  void set_antialias(unsigned short mode, int priority = 0);
  void clear_antialias();
  bool has_antialias() const;
  unsigned short get_antialias() const;

  bool has_audio_volume() const;
  void clear_audio_volume();
  void set_audio_volume(float volume,
                        int priority = 0);
  void set_audio_volume_off(int priority = 0);
  float get_audio_volume() const;
  float get_net_audio_volume() const;

  INLINE void adjust_all_priorities(int adjustment);

  // Variants on show and hide
  INLINE void show();
  INLINE void show(DrawMask camera_mask);
  INLINE void show_through();
  INLINE void show_through(DrawMask camera_mask);
  INLINE void hide();
  INLINE void hide(DrawMask camera_mask);
  INLINE bool is_hidden(DrawMask camera_mask = PandaNode::get_overall_bit()) const;
  NodePath get_hidden_ancestor(DrawMask camera_mask = PandaNode::get_overall_bit(),
                               Thread *current_thread = Thread::get_current_thread()) const;

  void stash(int sort = 0, Thread *current_thread = Thread::get_current_thread());
  void unstash(int sort = 0, Thread *current_thread = Thread::get_current_thread());
  void unstash_all(Thread *current_thread = Thread::get_current_thread());
  INLINE bool is_stashed() const;
  NodePath get_stashed_ancestor(Thread *current_thread = Thread::get_current_thread()) const;

  INLINE CollideMask get_collide_mask() const;
  INLINE void set_collide_mask(CollideMask new_mask, CollideMask bits_to_change = CollideMask::all_on(),
                               TypeHandle node_type = TypeHandle::none());

  // Comparison methods
  INLINE bool operator == (const NodePath &other) const;
  INLINE bool operator != (const NodePath &other) const;
  INLINE bool operator < (const NodePath &other) const;
  INLINE int compare_to(const NodePath &other) const;

  // Miscellaneous
  bool verify_complete(Thread *current_thread = Thread::get_current_thread()) const;

  void premunge_scene(GraphicsStateGuardianBase *gsg = NULL);
  void prepare_scene(GraphicsStateGuardianBase *gsg);

  void show_bounds();
  void show_tight_bounds();
  void hide_bounds();
  PT(BoundingVolume) get_bounds(Thread *current_thread = Thread::get_current_thread()) const;
  void force_recompute_bounds();
  void write_bounds(ostream &out) const;
  bool calc_tight_bounds(LPoint3f &min_point, LPoint3f &max_point,
                         Thread *current_thread = Thread::get_current_thread()) const;

  //  void analyze() const;

  int flatten_light();
  int flatten_medium();
  int flatten_strong();
  void apply_texture_colors();
  INLINE int clear_model_nodes();

  INLINE void set_tag(const string &key, const string &value);
  INLINE string get_tag(const string &key) const;
  INLINE bool has_tag(const string &key) const;
  INLINE void clear_tag(const string &key);
  INLINE string get_net_tag(const string &key) const;
  INLINE bool has_net_tag(const string &key) const;
  NodePath find_net_tag(const string &key) const;

#ifdef HAVE_PYTHON
  INLINE void set_python_tag(const string &key, PyObject *value);
  INLINE PyObject *get_python_tag(const string &key) const;
  INLINE bool has_python_tag(const string &key) const;
  INLINE void clear_python_tag(const string &key);
  INLINE PyObject *get_net_python_tag(const string &key) const;
  INLINE bool has_net_python_tag(const string &key) const;
  NodePath find_net_python_tag(const string &key) const;
#endif  // HAVE_PYTHON

  INLINE void list_tags() const;

  INLINE void set_name(const string &name);
  INLINE string get_name() const;

  BLOCKING bool write_bam_file(const string &filename) const;
  BLOCKING bool write_bam_stream(ostream &out) const;

  INLINE string encode_to_bam_stream() const;
  bool encode_to_bam_stream(string &data, BamWriter *writer = NULL) const;
  static NodePath decode_from_bam_stream(const string &data, BamReader *reader = NULL);

private:
  static NodePathComponent *
  find_common_ancestor(const NodePath &a, const NodePath &b,
                       int &a_count, int &b_count, 
                       Thread *current_thread);

  CPT(RenderState) r_get_net_state(NodePathComponent *comp, 
                                   Thread *current_thread) const;
  CPT(RenderState) r_get_partial_state(NodePathComponent *comp, int n,
                                       Thread *current_thread) const;
  CPT(TransformState) r_get_net_transform(NodePathComponent *comp, 
                                          Thread *current_thread) const;
  CPT(TransformState) r_get_partial_transform(NodePathComponent *comp, int n,
                                              Thread *current_thread) const;
  CPT(TransformState) r_get_net_prev_transform(NodePathComponent *comp,
                                               Thread *current_thread) const;
  CPT(TransformState) r_get_partial_prev_transform(NodePathComponent *comp, 
                                                   int n, Thread *current_thread) const;

  void find_matches(NodePathCollection &result,
                    const string &approx_path_str,
                    int max_matches) const;
  void find_matches(NodePathCollection &result,
                    FindApproxPath &approx_path,
                    int max_matches) const;
  void find_matches(NodePathCollection &result, 
                    FindApproxLevelEntry *level,
                    int max_matches) const;

  int r_clear_model_nodes(PandaNode *node);
  void r_adjust_all_priorities(PandaNode *node, int adjustment);

  void r_force_recompute_bounds(PandaNode *node);

  void r_set_collide_mask(PandaNode *node, 
                          CollideMask and_mask, CollideMask or_mask,
                          TypeHandle node_type);

  typedef phash_set<InternalName *, pointer_hash> InternalNames;
  bool r_has_vertex_column(PandaNode *node, const InternalName *name) const;
  void r_find_all_vertex_columns(PandaNode *node, 
                                 InternalNames &vertex_columns) const;

  typedef phash_set<Texture *, pointer_hash> Textures;
  Texture *r_find_texture(PandaNode *node, const RenderState *state,
                          const GlobPattern &glob) const;
  void r_find_all_textures(PandaNode *node, const RenderState *state,
                           Textures &textures) const;
  Texture *r_find_texture(PandaNode *node, TextureStage *stage) const;
  void r_find_all_textures(PandaNode *node, TextureStage *stage,
                           Textures &textures) const;

  typedef phash_set<TextureStage *, pointer_hash> TextureStages;
  TextureStage *r_find_texture_stage(PandaNode *node, const RenderState *state,
                                     const GlobPattern &glob) const;
  void r_find_all_texture_stages(PandaNode *node, const RenderState *state,
                                 TextureStages &texture_stages) const;

  void r_unify_texture_stages(PandaNode *node, TextureStage *stage);

  typedef phash_set<Material *, pointer_hash> Materials;
  Material *r_find_material(PandaNode *node, const RenderState *state,
                          const GlobPattern &glob) const;
  void r_find_all_materials(PandaNode *node, const RenderState *state,
                           Materials &materials) const;

  PT(NodePathComponent) _head;
  int _backup_key;
  ErrorType _error_type;
  static int _max_search_depth;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    register_type(_type_handle, "NodePath");
  }

private:
  static TypeHandle _type_handle;

  friend class NodePathCollection;
  friend class WorkingNodePath;
  friend class WeakNodePath;
};

INLINE ostream &operator << (ostream &out, const NodePath &node_path);

#ifdef HAVE_PYTHON
BEGIN_PUBLISH
NodePath py_decode_NodePath_from_bam_stream(const string &data);
NodePath py_decode_NodePath_from_bam_stream_persist(PyObject *unpickler, const string &data);
END_PUBLISH
#endif

#include "nodePath.I"

#endif
