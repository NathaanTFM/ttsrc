// Filename: cullTraverser.cxx
// Created by:  drose (23Feb02)
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

#include "config_pgraph.h"
#include "cullTraverser.h"
#include "cullTraverserData.h"
#include "transformState.h"
#include "renderState.h"
#include "fogAttrib.h"
#include "colorAttrib.h"
#include "renderModeAttrib.h"
#include "cullFaceAttrib.h"
#include "depthOffsetAttrib.h"
#include "cullHandler.h"
#include "dcast.h"
#include "geomNode.h"
#include "config_pgraph.h"
#include "boundingSphere.h"
#include "boundingBox.h"
#include "boundingHexahedron.h"
#include "portalClipper.h"
#include "geom.h"
#include "geomTristrips.h"
#include "geomTriangles.h"
#include "geomLinestrips.h"
#include "geomVertexWriter.h"

PStatCollector CullTraverser::_nodes_pcollector("Nodes");
PStatCollector CullTraverser::_geom_nodes_pcollector("Nodes:GeomNodes");
PStatCollector CullTraverser::_geoms_pcollector("Geoms");
PStatCollector CullTraverser::_geoms_occluded_pcollector("Geoms:Occluded");

TypeHandle CullTraverser::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CullTraverser::
CullTraverser() :
  _gsg(NULL),
  _current_thread(Thread::get_current_thread())
{
  _camera_mask = DrawMask::all_on();
  _has_tag_state_key = false;
  _initial_state = RenderState::make_empty();
  _cull_handler = (CullHandler *)NULL;
  _portal_clipper = (PortalClipper *)NULL;
  _effective_incomplete_render = true;
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::Copy Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CullTraverser::
CullTraverser(const CullTraverser &copy) :
  _gsg(copy._gsg),
  _current_thread(copy._current_thread),
  _scene_setup(copy._scene_setup),
  _camera_mask(copy._camera_mask),
  _has_tag_state_key(copy._has_tag_state_key),
  _tag_state_key(copy._tag_state_key),
  _initial_state(copy._initial_state),
  _depth_offset_decals(copy._depth_offset_decals),
  _view_frustum(copy._view_frustum),
  _cull_handler(copy._cull_handler),
  _portal_clipper(copy._portal_clipper),
  _effective_incomplete_render(copy._effective_incomplete_render)
{
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::set_scene
//       Access: Published, Virtual
//  Description: Sets the SceneSetup object that indicates the initial
//               camera position, etc.  This must be called before
//               traversal begins.
////////////////////////////////////////////////////////////////////
void CullTraverser::
set_scene(SceneSetup *scene_setup, GraphicsStateGuardianBase *gsg,
          bool dr_incomplete_render) {
  _scene_setup = scene_setup;
  _gsg = gsg;

  _initial_state = scene_setup->get_initial_state();
  _depth_offset_decals = _gsg->depth_offset_decals() && depth_offset_decals;

  _current_thread = Thread::get_current_thread();

  const Camera *camera = scene_setup->get_camera_node();
  _tag_state_key = camera->get_tag_state_key();
  _has_tag_state_key = !_tag_state_key.empty();
  _camera_mask = camera->get_camera_mask();

  _effective_incomplete_render = _gsg->get_incomplete_render() && dr_incomplete_render;
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::traverse
//       Access: Published
//  Description: Begins the traversal from the indicated node.
////////////////////////////////////////////////////////////////////
void CullTraverser::
traverse(const NodePath &root) {
  nassertv(_cull_handler != (CullHandler *)NULL);
  nassertv(_scene_setup != (SceneSetup *)NULL);

  if (allow_portal_cull) {
    // This _view_frustum is in cull_center space
    //Erik: obsolete?
    //PT(GeometricBoundingVolume) vf = _view_frustum;

    GeometricBoundingVolume *local_frustum = NULL;
    PT(BoundingVolume) bv = _scene_setup->get_lens()->make_bounds();
    if (bv != (BoundingVolume *)NULL &&
        bv->is_of_type(GeometricBoundingVolume::get_class_type())) {
      
      local_frustum = DCAST(GeometricBoundingVolume, bv);
    }
      
    // This local_frustum is in camera space
    PortalClipper portal_viewer(local_frustum, _scene_setup);
    if (debug_portal_cull) {
      portal_viewer.draw_camera_frustum();
    }
    
    // Store this pointer in this
    set_portal_clipper(&portal_viewer);

    CullTraverserData data(root, TransformState::make_identity(),
                           _initial_state, _view_frustum, 
                           _current_thread);
    
    traverse(data);
    
    // Finally add the lines to be drawn
    if (debug_portal_cull) {
      portal_viewer.draw_lines();
    }
    
    // Render the frustum relative to the cull center.
    NodePath cull_center = _scene_setup->get_cull_center();
    CPT(TransformState) transform = cull_center.get_transform(root);
    
    CullTraverserData my_data(data, portal_viewer._previous);
    my_data._net_transform = my_data._net_transform->compose(transform);
    traverse(my_data);

  } else {
    CullTraverserData data(root, TransformState::make_identity(),
                           _initial_state, _view_frustum, 
                           _current_thread);
    
    traverse(data);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::traverse
//       Access: Published
//  Description: Traverses from the next node with the given
//               data, which has been constructed with the node but
//               has not yet been converted into the node's space.
////////////////////////////////////////////////////////////////////
void CullTraverser::
traverse(CullTraverserData &data) {
  if (is_in_view(data)) {
    if (pgraph_cat.is_spam()) {
      pgraph_cat.spam() 
        << "\n" << data._node_path
        << " " << data._draw_mask << "\n";
    }

    PandaNodePipelineReader *node_reader = data.node_reader();
    int fancy_bits = node_reader->get_fancy_bits();

    if ((fancy_bits & (PandaNode::FB_transform |
                       PandaNode::FB_state |
                       PandaNode::FB_effects |
                       PandaNode::FB_tag |
                       PandaNode::FB_draw_mask |
                       PandaNode::FB_cull_callback)) == 0 &&
        data._cull_planes->is_empty()) {
      // Nothing interesting in this node; just move on.
      traverse_below(data);

    } else {
      // Something in this node is worth taking a closer look.
      const RenderEffects *node_effects = node_reader->get_effects();
      if (node_effects->has_show_bounds()) {
        // If we should show the bounding volume for this node, make it
        // up now.
        show_bounds(data, node_effects->has_show_tight_bounds());
      }
      
      data.apply_transform_and_state(this);
      
      const FogAttrib *fog = DCAST(FogAttrib, node_reader->get_state()->get_attrib(FogAttrib::get_class_slot()));
      if (fog != (const FogAttrib *)NULL && fog->get_fog() != (Fog *)NULL) {
        // If we just introduced a FogAttrib here, call adjust_to_camera()
        // now.  This maybe isn't the perfect time to call it, but it's
        // good enough; and at this time we have all the information we
        // need for it.
        fog->get_fog()->adjust_to_camera(get_camera_transform());
      }
      
      if (fancy_bits & PandaNode::FB_cull_callback) {
        PandaNode *node = data.node();
        if (!node->cull_callback(this, data)) {
          return;
        }
      }

      traverse_below(data);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::traverse_below
//       Access: Published, Virtual
//  Description: Traverses all the children of the indicated node,
//               with the given data, which has been converted into
//               the node's space.
////////////////////////////////////////////////////////////////////
void CullTraverser::
traverse_below(CullTraverserData &data) {
  _nodes_pcollector.add_level(1);
  PandaNodePipelineReader *node_reader = data.node_reader();
  PandaNode *node = data.node();

  bool this_node_hidden = data.is_this_node_hidden(this);

  const RenderEffects *node_effects = node_reader->get_effects();
  bool has_decal = !this_node_hidden && node_effects->has_decal();
  if (has_decal && !_depth_offset_decals) {
    // Start the three-pass decal rendering if we're not using
    // DepthOffsetAttribs to implement decals.
    start_decal(data);
    
  } else {
    if (!this_node_hidden) {
      node->add_for_draw(this, data);
    }

    if (has_decal) {
      // If we *are* implementing decals with DepthOffsetAttribs,
      // apply it now, so that each child of this node gets offset by
      // a tiny amount.
      data._state = data._state->compose(get_depth_offset_state());
#ifndef NDEBUG
      // This is just a sanity check message.
      if (!node->is_geom_node()) {
        pgraph_cat.error()
          << "DecalEffect applied to " << *node << ", not a GeomNode.\n";
      }
#endif
    }

    // Now visit all the node's children.
    PandaNode::Children children = node_reader->get_children();
    node_reader->release();
    int num_children = children.get_num_children();
    if (node->has_selective_visibility()) {
      int i = node->get_first_visible_child();
      while (i < num_children) {
        CullTraverserData next_data(data, children.get_child(i));
        traverse(next_data);
        i = node->get_next_visible_child(i);
      }
      
    } else {
      for (int i = 0; i < num_children; i++) {
        CullTraverserData next_data(data, children.get_child(i));
        traverse(next_data);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::end_traverse
//       Access: Published, Virtual
//  Description: Should be called when the traverser has finished
//               traversing its scene, this gives it a chance to do
//               any necessary finalization.
////////////////////////////////////////////////////////////////////
void CullTraverser::
end_traverse() {
  _cull_handler->end_traverse();
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::draw_bounding_volume
//       Access: Published
//  Description: Draws an appropriate visualization of the indicated
//               bounding volume.
////////////////////////////////////////////////////////////////////
void CullTraverser::
draw_bounding_volume(const BoundingVolume *vol, 
                     const TransformState *net_transform,
                     const TransformState *modelview_transform) {
  PT(Geom) bounds_viz = make_bounds_viz(vol);
  
  if (bounds_viz != (Geom *)NULL) {
    _geoms_pcollector.add_level(2);
    CullableObject *outer_viz = 
      new CullableObject(bounds_viz, get_bounds_outer_viz_state(), 
                         net_transform, modelview_transform, get_gsg());
    _cull_handler->record_object(outer_viz, this);
    
    CullableObject *inner_viz = 
      new CullableObject(bounds_viz, get_bounds_inner_viz_state(), 
                         net_transform, modelview_transform, get_gsg());
    _cull_handler->record_object(inner_viz, this);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::is_in_view
//       Access: Protected, Virtual
//  Description: Returns true if the current node is fully or
//               partially within the viewing area and should be
//               drawn, or false if it (and all of its children)
//               should be pruned.
////////////////////////////////////////////////////////////////////
bool CullTraverser::
is_in_view(CullTraverserData &data) {
  return data.is_in_view(_camera_mask);
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::show_bounds
//       Access: Private
//  Description: Draws an appropriate visualization of the node's
//               external bounding volume.
////////////////////////////////////////////////////////////////////
void CullTraverser::
show_bounds(CullTraverserData &data, bool tight) {
  PandaNode *node = data.node();
  CPT(TransformState) net_transform = data.get_net_transform(this);
  CPT(TransformState) modelview_transform = data.get_modelview_transform(this);

  if (tight) {
    PT(Geom) bounds_viz = make_tight_bounds_viz(node);

    if (bounds_viz != (Geom *)NULL) {
      _geoms_pcollector.add_level(1);
      CullableObject *outer_viz = 
        new CullableObject(bounds_viz, get_bounds_outer_viz_state(), 
                           net_transform, modelview_transform,
                           get_gsg());
      _cull_handler->record_object(outer_viz, this);
    }
    
  } else {
    draw_bounding_volume(node->get_bounds(),
                         net_transform, modelview_transform);

    if (node->is_geom_node()) {
      // Also show the bounding volumes of included Geoms.
      net_transform = net_transform->compose(node->get_transform());
      modelview_transform = modelview_transform->compose(node->get_transform());
      GeomNode *gnode = DCAST(GeomNode, node);
      int num_geoms = gnode->get_num_geoms();
      for (int i = 0; i < num_geoms; ++i) {
        draw_bounding_volume(gnode->get_geom(i)->get_bounds(), 
                             net_transform, modelview_transform);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::make_bounds_viz
//       Access: Private
//  Description: Returns an appropriate visualization of the indicated
//               bounding volume.
////////////////////////////////////////////////////////////////////
PT(Geom) CullTraverser::
make_bounds_viz(const BoundingVolume *vol) {
  PT(Geom) geom;
  if (vol->is_infinite() || vol->is_empty()) {
    // No way to draw an infinite or empty bounding volume.

  } else if (vol->is_of_type(BoundingSphere::get_class_type())) {
    const BoundingSphere *sphere = DCAST(BoundingSphere, vol);

    static const int num_slices = 16;
    static const int num_stacks = 8;

    PT(GeomVertexData) vdata = new GeomVertexData
      ("bounds", GeomVertexFormat::get_v3(),
       Geom::UH_stream);
    GeomVertexWriter vertex(vdata, InternalName::get_vertex());
    
    PT(GeomTristrips) strip = new GeomTristrips(Geom::UH_stream);
    for (int sl = 0; sl < num_slices; ++sl) {
      float longitude0 = (float)sl / (float)num_slices;
      float longitude1 = (float)(sl + 1) / (float)num_slices;
      vertex.add_data3f(compute_point(sphere, 0.0, longitude0));
      for (int st = 1; st < num_stacks; ++st) {
        float latitude = (float)st / (float)num_stacks;
        vertex.add_data3f(compute_point(sphere, latitude, longitude0));
        vertex.add_data3f(compute_point(sphere, latitude, longitude1));
      }
      vertex.add_data3f(compute_point(sphere, 1.0, longitude0));
      
      strip->add_next_vertices(num_stacks * 2);
      strip->close_primitive();
    }
    
    geom = new Geom(vdata);
    geom->add_primitive(strip);

  } else if (vol->is_of_type(FiniteBoundingVolume::get_class_type())) {
    const FiniteBoundingVolume *fvol = DCAST(FiniteBoundingVolume, vol);

    BoundingBox box(fvol->get_min(), fvol->get_max());
    box.local_object();

    PT(GeomVertexData) vdata = new GeomVertexData
      ("bounds", GeomVertexFormat::get_v3(),
       Geom::UH_stream);
    GeomVertexWriter vertex(vdata, InternalName::get_vertex());

    for (int i = 0; i < 8; ++i ) {
      vertex.add_data3f(box.get_point(i));
    }
    
    PT(GeomTriangles) tris = new GeomTriangles(Geom::UH_stream);
    tris->add_vertices(0, 4, 5);
    tris->close_primitive();
    tris->add_vertices(0, 5, 1);
    tris->close_primitive();
    tris->add_vertices(4, 6, 7);
    tris->close_primitive();
    tris->add_vertices(4, 7, 5);
    tris->close_primitive();
    tris->add_vertices(6, 2, 3);
    tris->close_primitive();
    tris->add_vertices(6, 3, 7);
    tris->close_primitive();
    tris->add_vertices(2, 0, 1);
    tris->close_primitive();
    tris->add_vertices(2, 1, 3);
    tris->close_primitive();
    tris->add_vertices(1, 5, 7);
    tris->close_primitive();
    tris->add_vertices(1, 7, 3);
    tris->close_primitive();
    tris->add_vertices(2, 6, 4);
    tris->close_primitive();
    tris->add_vertices(2, 4, 0);
    tris->close_primitive();

    geom = new Geom(vdata);
    geom->add_primitive(tris);

  } else {
    pgraph_cat.warning()
      << "Don't know how to draw a representation of "
      << vol->get_class_type() << "\n";
  }

  return geom;
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::make_tight_bounds_viz
//       Access: Private
//  Description: Returns a bounding-box visualization of the indicated
//               node's "tight" bounding volume.
////////////////////////////////////////////////////////////////////
PT(Geom) CullTraverser::
make_tight_bounds_viz(PandaNode *node) {
  PT(Geom) geom;

  NodePath np = NodePath::any_path(node);

  LPoint3f n, x;
  bool found_any = false;
  node->calc_tight_bounds(n, x, found_any, TransformState::make_identity(),
                          _current_thread);
  if (found_any) {
    PT(GeomVertexData) vdata = new GeomVertexData
      ("bounds", GeomVertexFormat::get_v3(),
      Geom::UH_stream);
    GeomVertexWriter vertex(vdata, InternalName::get_vertex(),
                            _current_thread);
    
    vertex.add_data3f(n[0], n[1], n[2]);
    vertex.add_data3f(n[0], n[1], x[2]);
    vertex.add_data3f(n[0], x[1], n[2]);
    vertex.add_data3f(n[0], x[1], x[2]);
    vertex.add_data3f(x[0], n[1], n[2]);
    vertex.add_data3f(x[0], n[1], x[2]);
    vertex.add_data3f(x[0], x[1], n[2]);
    vertex.add_data3f(x[0], x[1], x[2]);
  
    PT(GeomLinestrips) strip = new GeomLinestrips(Geom::UH_stream);

    // We wind one long linestrip around the wireframe cube.  This
    // does require backtracking a few times here and there.
    strip->add_vertex(0);
    strip->add_vertex(1);
    strip->add_vertex(3);
    strip->add_vertex(2);
    strip->add_vertex(0);
    strip->add_vertex(4);
    strip->add_vertex(5);
    strip->add_vertex(7);
    strip->add_vertex(6);
    strip->add_vertex(4);
    strip->add_vertex(6);
    strip->add_vertex(2);
    strip->add_vertex(3);
    strip->add_vertex(7);
    strip->add_vertex(5);
    strip->add_vertex(1);
    strip->close_primitive();
      
    geom = new Geom(vdata);
    geom->add_primitive(strip);
  }

  return geom;
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::compute_point
//       Access: Private, Static
//  Description: Returns a point on the surface of the sphere.
//               latitude and longitude range from 0.0 to 1.0.  
////////////////////////////////////////////////////////////////////
Vertexf CullTraverser::
compute_point(const BoundingSphere *sphere, 
              float latitude, float longitude) {
  float s1, c1;
  csincos(latitude * MathNumbers::pi_f, &s1, &c1);

  float s2, c2;
  csincos(longitude * 2.0f * MathNumbers::pi_f, &s2, &c2);

  Vertexf p(s1 * c2, s1 * s2, c1);
  return p * sphere->get_radius() + sphere->get_center();
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::get_bounds_outer_viz_state
//       Access: Private, Static
//  Description: Returns a RenderState for rendering the outside
//               surfaces of the bounding volume visualizations.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullTraverser::
get_bounds_outer_viz_state() {
  // Once someone asks for this pointer, we hold its reference count
  // and never free it.
  static CPT(RenderState) state = (const RenderState *)NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make
      (ColorAttrib::make_flat(Colorf(0.3f, 1.0f, 0.5f, 1.0f)),
       RenderModeAttrib::make(RenderModeAttrib::M_wireframe),
       CullFaceAttrib::make(CullFaceAttrib::M_cull_clockwise));
  }
  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::get_bounds_inner_viz_state
//       Access: Private, Static
//  Description: Returns a RenderState for rendering the inside
//               surfaces of the bounding volume visualizations.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullTraverser::
get_bounds_inner_viz_state() {
  // Once someone asks for this pointer, we hold its reference count
  // and never free it.
  static CPT(RenderState) state = (const RenderState *)NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make
      (ColorAttrib::make_flat(Colorf(0.15f, 0.5f, 0.25f, 1.0f)),
       RenderModeAttrib::make(RenderModeAttrib::M_wireframe),
       CullFaceAttrib::make(CullFaceAttrib::M_cull_counter_clockwise));
  }
  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::get_depth_offset_state
//       Access: Private, Static
//  Description: Returns a RenderState for increasing the DepthOffset
//               by one.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullTraverser::
get_depth_offset_state() {
  // Once someone asks for this pointer, we hold its reference count
  // and never free it.
  static CPT(RenderState) state = (const RenderState *)NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make
      (DepthOffsetAttrib::make(1));
  }
  return state;
}


////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::start_decal
//       Access: Private
//  Description: Collects a base node and all of the decals applied to
//               it.  This involves recursing below the base GeomNode
//               to find all the decal geoms.
////////////////////////////////////////////////////////////////////
void CullTraverser::
start_decal(const CullTraverserData &data) {
  PandaNode *node = data.node();
  if (!node->is_geom_node()) {
    pgraph_cat.error()
      << "DecalEffect applied to " << *node << ", not a GeomNode.\n";
    return;
  }

  const PandaNodePipelineReader *node_reader = data.node_reader();

  // Build a chain of CullableObjects.  The head of the chain will be
  // all of the base Geoms in order, followed by an empty
  // CullableObject node, followed by all of the decal Geoms, in
  // order.

  // Since the CullableObject is a linked list which gets built in
  // LIFO order, we start with the decals.
  CullableObject *decals = (CullableObject *)NULL;
  PandaNode::Children cr = node_reader->get_children();
  int num_children = cr.get_num_children();
  if (node->has_selective_visibility()) {
    int i = node->get_first_visible_child();
    while (i < num_children) {
      CullTraverserData next_data(data, cr.get_child(i));
      decals = r_get_decals(next_data, decals);
      i = node->get_next_visible_child(i);
    }
    
  } else {
    for (int i = num_children - 1; i >= 0; i--) {
      CullTraverserData next_data(data, cr.get_child(i));
      decals = r_get_decals(next_data, decals);
    }
  }

  // Now create a new, empty CullableObject to separate the decals
  // from the non-decals.
  CullableObject *separator = new CullableObject;
  separator->set_next(decals);

  // And now get the base Geoms, again in reverse order.
  CullableObject *object = separator;
  GeomNode *geom_node = DCAST(GeomNode, node);
  GeomNode::Geoms geoms = geom_node->get_geoms();
  int num_geoms = geoms.get_num_geoms();
  _geoms_pcollector.add_level(num_geoms);
  CPT(TransformState) net_transform = data.get_net_transform(this);
  CPT(TransformState) modelview_transform = data.get_modelview_transform(this);
  CPT(TransformState) internal_transform = get_gsg()->get_cs_transform()->compose(modelview_transform);
  
  for (int i = num_geoms - 1; i >= 0; i--) {
    const Geom *geom = geoms.get_geom(i);
    if (geom->is_empty()) {
      continue;
    }

    CPT(RenderState) state = data._state->compose(geoms.get_geom_state(i));
    if (state->has_cull_callback() && !state->cull_callback(this, data)) {
      // Cull.
      continue;
    }
    
    // Cull the Geom bounding volume against the view frustum
    // and/or the cull planes.  Don't bother unless we've got more
    // than one Geom, since otherwise the bounding volume of the
    // GeomNode is (probably) the same as that of the one Geom,
    // and we've already culled against that.
    if (num_geoms > 1) {
      if (data._view_frustum != (GeometricBoundingVolume *)NULL) {
        // Cull the individual Geom against the view frustum.
        CPT(BoundingVolume) geom_volume = geom->get_bounds();
        const GeometricBoundingVolume *geom_gbv =
          DCAST(GeometricBoundingVolume, geom_volume);
        
        int result = data._view_frustum->contains(geom_gbv);
        if (result == BoundingVolume::IF_no_intersection) {
          // Cull this Geom.
          continue;
        }
      }
      if (!data._cull_planes->is_empty()) {
        // Also cull the Geom against the cull planes.
        CPT(BoundingVolume) geom_volume = geom->get_bounds();
        const GeometricBoundingVolume *geom_gbv =
          DCAST(GeometricBoundingVolume, geom_volume);
        int result;
        data._cull_planes->do_cull(result, state, geom_gbv);
        if (result == BoundingVolume::IF_no_intersection) {
          // Cull.
          continue;
        }
      }
    }

    CullableObject *next = object;
    object =
      new CullableObject(geom, state, net_transform, 
                         modelview_transform, internal_transform);
    object->set_next(next);
  }

  if (object != separator) {
    // Finally, send the whole list down to the CullHandler for
    // processing.  The first Geom in the node now represents the
    // overall state.
    _cull_handler->record_object(object, this);
  } else {
    // Never mind; there's nothing to render.
    delete object;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullTraverser::r_get_decals
//       Access: Private
//  Description: Recursively gets all the decals applied to a
//               particular GeomNode.  These are built into a
//               CullableObject list in LIFO order (so that the
//               traversing the list will extract them in the order
//               they were encountered in the scene graph).
////////////////////////////////////////////////////////////////////
CullableObject *CullTraverser::
r_get_decals(CullTraverserData &data, CullableObject *decals) {
  if (is_in_view(data)) {
    PandaNodePipelineReader *node_reader = data.node_reader();
    PandaNode *node = data.node();

    const RenderEffects *node_effects = node_reader->get_effects();
    if (node_effects->has_show_bounds()) {
      // If we should show the bounding volume for this node, make it
      // up now.
      show_bounds(data, node_effects->has_show_tight_bounds());
    }

    data.apply_transform_and_state(this);

    // First, visit all of the node's children.
    int num_children = node_reader->get_num_children();
    if (node->has_selective_visibility()) {
      int i = node->get_first_visible_child();
      while (i < num_children) {
        CullTraverserData next_data(data, node_reader->get_child(i));
        decals = r_get_decals(next_data, decals);
        i = node->get_next_visible_child(i);
      }
      
    } else {
      for (int i = num_children - 1; i >= 0; i--) {
        CullTraverserData next_data(data, node_reader->get_child(i));
        decals = r_get_decals(next_data, decals);
      }
    }

    // Now, tack on any geoms within the node.
    if (node->is_geom_node()) {
      GeomNode *geom_node = DCAST(GeomNode, node);
      GeomNode::Geoms geoms = geom_node->get_geoms();
      int num_geoms = geoms.get_num_geoms();
      _geoms_pcollector.add_level(num_geoms);
      CPT(TransformState) net_transform = data.get_net_transform(this);
      CPT(TransformState) modelview_transform = data.get_modelview_transform(this);
      CPT(TransformState) internal_transform = get_gsg()->get_cs_transform()->compose(modelview_transform);

      for (int i = num_geoms - 1; i >= 0; i--) {
        const Geom *geom = geoms.get_geom(i);
        if (geom->is_empty()) {
          continue;
        }

        CPT(RenderState) state = data._state->compose(geoms.get_geom_state(i));
        if (state->has_cull_callback() && !state->cull_callback(this, data)) {
          // Cull.
          continue;
        }

        // Cull the Geom bounding volume against the view frustum
        // and/or the cull planes.  Don't bother unless we've got more
        // than one Geom, since otherwise the bounding volume of the
        // GeomNode is (probably) the same as that of the one Geom,
        // and we've already culled against that.
        if (num_geoms > 1) {
          if (data._view_frustum != (GeometricBoundingVolume *)NULL) {
            // Cull the individual Geom against the view frustum.
            CPT(BoundingVolume) geom_volume = geom->get_bounds();
            const GeometricBoundingVolume *geom_gbv =
              DCAST(GeometricBoundingVolume, geom_volume);

            int result = data._view_frustum->contains(geom_gbv);
            if (result == BoundingVolume::IF_no_intersection) {
              // Cull this Geom.
              continue;
            }
          }
          if (!data._cull_planes->is_empty()) {
            // Also cull the Geom against the cull planes.
            CPT(BoundingVolume) geom_volume = geom->get_bounds();
            const GeometricBoundingVolume *geom_gbv =
              DCAST(GeometricBoundingVolume, geom_volume);
            int result;
            data._cull_planes->do_cull(result, state, geom_gbv);
            if (result == BoundingVolume::IF_no_intersection) {
              // Cull.
              continue;
            }
          }
        }

        CullableObject *next = decals;
        decals =
          new CullableObject(geom, state, net_transform, 
                             modelview_transform, internal_transform);
        decals->set_next(next);
      }
    }
  }

  return decals;
}
