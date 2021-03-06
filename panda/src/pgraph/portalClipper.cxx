// Filename: portalClipper.cxx
// Created by:  masad (4May04)
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

#include "portalClipper.h"
#include "cullTraverser.h"
#include "cullTraverserData.h"
#include "transformState.h"
#include "renderState.h"
#include "fogAttrib.h"
#include "cullHandler.h"
#include "dcast.h"
#include "geomNode.h"
#include "config_pgraph.h"
#include "boundingSphere.h"
#include "colorAttrib.h"
#include "renderModeAttrib.h"
#include "cullFaceAttrib.h"
#include "depthOffsetAttrib.h"
#include "geomVertexWriter.h"
#include "geomLinestrips.h"
#include "geomPoints.h"

TypeHandle PortalClipper::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PortalClipper::
PortalClipper(GeometricBoundingVolume *frustum, SceneSetup *scene_setup):
_reduced_viewport_min(-1,-1),
_reduced_viewport_max(1,1),
_clip_state(0)
{
  _previous = new GeomNode("my_frustum");

  _view_frustum = _reduced_frustum = DCAST(BoundingHexahedron, frustum);


  _scene_setup = scene_setup;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
PortalClipper::
~PortalClipper() {
}

////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::move_to
//       Access: Public
//  Description: Moves the pen to the given point without drawing a
//               line.  When followed by draw_to(), this marks the
//               first point of a line segment; when followed by
//               move_to() or create(), this creates a single point.
////////////////////////////////////////////////////////////////////
void PortalClipper::
move_to(const LVecBase3f &v) {
  // We create a new SegmentList with the initial point in it.
  SegmentList segs;
  segs.push_back(Point(v, _color));

  // And add this list to the list of segments.
  _list.push_back(segs);
}

////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::draw_to
//       Access: Public
//  Description: Draws a line segment from the pen's last position
//               (the last call to move_to or draw_to) to the
//               indicated point.  move_to() and draw_to() only update
//               tables; the actual drawing is performed when create()
//               is called.
////////////////////////////////////////////////////////////////////
void PortalClipper::
draw_to(const LVecBase3f &v) {
  if (_list.empty()) {
    // Let our first call to draw_to() be an implicit move_to().
    move_to(v);

  } else {
    // Get the current SegmentList, which was the last one we added to
    // the LineList.
    SegmentList &segs = _list.back();

    // Add the new point.
    segs.push_back(Point(v, _color));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::draw a portal frustum
//       Access: Public
//  Description: Given the BoundingHexahedron draw it using lines
//           
////////////////////////////////////////////////////////////////////
void PortalClipper::
draw_hexahedron(BoundingHexahedron *frustum) {
  // walk the view frustum as it should be drawn
  move_to(frustum->get_point(0));
  draw_to(frustum->get_point(1));
  draw_to(frustum->get_point(2));
  draw_to(frustum->get_point(3));

  move_to(frustum->get_point(4));
  draw_to(frustum->get_point(0));
  draw_to(frustum->get_point(3));
  draw_to(frustum->get_point(7));

  move_to(frustum->get_point(5));
  draw_to(frustum->get_point(4));
  draw_to(frustum->get_point(7));
  draw_to(frustum->get_point(6));

  move_to(frustum->get_point(1));
  draw_to(frustum->get_point(5));
  draw_to(frustum->get_point(6));
  draw_to(frustum->get_point(2));
}
////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::draw the current visible portal
//       Access: Public
//  Description: _portal_node is the current portal, draw it.
//           
////////////////////////////////////////////////////////////////////
void PortalClipper::
draw_current_portal()
{
  move_to(_portal_node->get_vertex(0));
  draw_to(_portal_node->get_vertex(1));
  draw_to(_portal_node->get_vertex(2));
  draw_to(_portal_node->get_vertex(3));
}

////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::draw the lines
//       Access: Public
//  Description: Draw all the lines in the buffer
//               Cyan portal is the original geometry of the portal
//               Yellow portal is the AA minmax & clipped portal
//               Blue frustum is the frustum through portal
//               White frustum is the camera frustum
////////////////////////////////////////////////////////////////////
void PortalClipper::
draw_lines() {
  if (!_list.empty()) {
    _created_data = NULL;
      
    PT(GeomVertexData) vdata = new GeomVertexData
      ("portal", GeomVertexFormat::get_v3cp(), Geom::UH_static);
    GeomVertexWriter vertex(vdata, InternalName::get_vertex());
    GeomVertexWriter color(vdata, InternalName::get_color());
    
    PT(GeomLinestrips) lines = new GeomLinestrips(Geom::UH_static);
    PT(GeomPoints) points = new GeomPoints(Geom::UH_static);
    
    int v = 0;
    LineList::const_iterator ll;
    SegmentList::const_iterator sl;
    
    for (ll = _list.begin(); ll != _list.end(); ll++) {
      const SegmentList &segs = (*ll);
      
      if (segs.size() < 2) {
        // A segment of length 1 is just a point.
        for (sl = segs.begin(); sl != segs.end(); sl++) {
          points->add_vertex(v);
          vertex.add_data3f((*sl)._point);
          color.add_data4f((*sl)._color);
          v++;
        }
        points->close_primitive();
        
      } else {
        // A segment of length 2 or more is a line segment or
        // segments.
        for (sl = segs.begin(); sl != segs.end(); sl++) {
          lines->add_vertex(v);
          vertex.add_data3f((*sl)._point);
          color.add_data4f((*sl)._color);
          v++;
        }
        lines->close_primitive();
      }
    }

    if (lines->get_num_vertices() != 0) {
      PT(Geom) geom = new Geom(vdata);
      geom->add_primitive(lines);
      _previous->add_geom(geom);
    }
    if (points->get_num_vertices() != 0) {
      PT(Geom) geom = new Geom(vdata);
      geom->add_primitive(points);
      _previous->add_geom(geom);
    }
  }
}
////////////////////////////////////////////////////////////////////
//     Function: PortalClipper::prepare the portal
//       Access: Public
//  Description: Given the portal draw the frustum with line segs
//               for now. More functionalities coming up
////////////////////////////////////////////////////////////////////
bool PortalClipper::
prepare_portal(const NodePath &node_path)
{
  // Get the Portal Node from this node_path
  PandaNode *node = node_path.node();
  _portal_node = NULL;
  if (node->is_of_type(PortalNode::get_class_type())) {  
    _portal_node = DCAST(PortalNode, node);
  }

  // Get the geometry from the portal    
  portal_cat.spam() << *_portal_node << endl;

  // Get the camera transformation matrix
  CPT(TransformState) ctransform = node_path.get_transform(_scene_setup->get_cull_center());
  //CPT(TransformState) ctransform = node_path.get_transform(_scene_setup->get_camera_path());
  LMatrix4f cmat = ctransform->get_mat();
  portal_cat.spam() << cmat << endl;
 
  Vertexf temp[4];
  temp[0] = _portal_node->get_vertex(0);
  temp[1] = _portal_node->get_vertex(1);
  temp[2] = _portal_node->get_vertex(2);
  temp[3] = _portal_node->get_vertex(3);

  portal_cat.spam() << "before transformation to camera space" << endl;
  portal_cat.spam() << temp[0] << endl;
  portal_cat.spam() << temp[1] << endl;
  portal_cat.spam() << temp[2] << endl;
  portal_cat.spam() << temp[3] << endl;
  
  temp[0] = temp[0]*cmat;
  temp[1] = temp[1]*cmat;
  temp[2] = temp[2]*cmat;
  temp[3] = temp[3]*cmat;

  Planef portal_plane(temp[0], temp[1], temp[2]);
  if (!is_facing_view(portal_plane)) {
    portal_cat.debug() << "portal failed 1st level test (isn't facing the camera)\n";
    return false;
  }

  portal_cat.spam() << "after transformation to camera space" << endl;
  portal_cat.spam() << temp[0] << endl;
  portal_cat.spam() << temp[1] << endl;
  portal_cat.spam() << temp[2] << endl;
  portal_cat.spam() << temp[3] << endl;

  // check if the portal intersects with the cameras 0 point (center of projection). In that case the portal will invert itself.
  // portals intersecting the near plane or the 0 point are a weird case anyhow, therefore we don't reduce the frustum any further
  // and just return true. In effect the portal doesn't reduce visibility but will draw everything in its out cell
  if ((temp[0][1] <= 0) || (temp[1][1] <= 0) || (temp[2][1] <= 0) || (temp[3][1] <= 0)) {
      portal_cat.debug() << "portal intersects with center of projection.." << endl;
      return true;
  }
  
  // project portal points, so they are in the -1..1 range
  LPoint3f projected_coords[4];
  const Lens *lens = _scene_setup->get_lens();
  lens->project(temp[0], projected_coords[0]);
  lens->project(temp[1], projected_coords[1]);
  lens->project(temp[2], projected_coords[2]);
  lens->project(temp[3], projected_coords[3]);

  portal_cat.spam() << "after projection to 2d" << endl;
  portal_cat.spam() << projected_coords[0] << endl;
  portal_cat.spam() << projected_coords[1] << endl;
  portal_cat.spam() << projected_coords[2] << endl;
  portal_cat.spam() << projected_coords[3] << endl;

  // calculate axis aligned bounding box of the portal
  float min_x, max_x, min_y, max_y;
  min_x = min(min(min(projected_coords[0][0], projected_coords[1][0]), projected_coords[2][0]), projected_coords[3][0]);
  max_x = max(max(max(projected_coords[0][0], projected_coords[1][0]), projected_coords[2][0]), projected_coords[3][0]);
  min_y = min(min(min(projected_coords[0][1], projected_coords[1][1]), projected_coords[2][1]), projected_coords[3][1]);
  max_y = max(max(max(projected_coords[0][1], projected_coords[1][1]), projected_coords[2][1]), projected_coords[3][1]);

  portal_cat.spam() << "min_x " << min_x << ";max_x " << max_x << ";min_y " << min_y << ";max_y " << max_y << endl;

  // clip the minima and maxima against the viewport
  min_x = max(min_x, _reduced_viewport_min[0]);
  min_y = max(min_y, _reduced_viewport_min[1]);
  max_x = min(max_x, _reduced_viewport_max[0]);
  max_y = min(max_y, _reduced_viewport_max[1]);

  portal_cat.spam() << "after clipping: min_x " << min_x << ";max_x " << max_x << ";min_y " << min_y << ";max_y " << max_y << endl;

  if ((min_x >= max_x) || (min_y >= max_y)) {
      portal_cat.debug() << "portal got clipped away \n";
      return false;
  }

  // here we know the portal is in view and we have its clipped extents
  _reduced_viewport_min.set(min_x, min_y);
  _reduced_viewport_max.set(max_x, max_y);

  // calculate the near and far points so we can construct a frustum
  LPoint3f near_point[4];
  LPoint3f far_point[4];
  lens->extrude(LPoint2f(min_x, min_y), near_point[0], far_point[0]);
  lens->extrude(LPoint2f(max_x, min_y), near_point[1], far_point[1]);
  lens->extrude(LPoint2f(max_x, max_y), near_point[2], far_point[2]);
  lens->extrude(LPoint2f(min_x, max_y), near_point[3], far_point[3]);

  // With these points, construct the new reduced frustum
  _reduced_frustum = new BoundingHexahedron(far_point[0], far_point[1], far_point[2], far_point[3],  
                                            near_point[0], near_point[1], near_point[2], near_point[3]);

  portal_cat.debug() << *_reduced_frustum << endl;

  // do debug rendering, if requested
  if (debug_portal_cull) {
    // draw the reduced frustum
    _color = Colorf(0,0,1,1);
    draw_hexahedron(DCAST(BoundingHexahedron, _reduced_frustum));

    // lets first add the clipped portal (in yellow)
    _color = Colorf(1,1,0,1);
    move_to((near_point[0]+far_point[0])/2.0); // I choose a point in the middle between near and far.. could also be some other z value.. 
    draw_to((near_point[1]+far_point[1])/2.0);
    draw_to((near_point[2]+far_point[2])/2.0);
    draw_to((near_point[3]+far_point[3])/2.0);
    draw_to((near_point[0]+far_point[0])/2.0);

    // ok, now lets add the original portal (in cyan) 
    _color = Colorf(0,1,1,1);
    move_to(temp[0]);
    draw_to(temp[1]);
    draw_to(temp[2]);
    draw_to(temp[3]);
    draw_to(temp[0]);
  }

  return true;
}

