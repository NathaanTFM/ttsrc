// Filename: collisionParabola.cxx
// Created by:  drose (11Oct07)
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

#include "collisionParabola.h"
#include "collisionEntry.h"
#include "datagram.h"
#include "datagramIterator.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "geom.h"
#include "geomLinestrips.h"
#include "geomVertexWriter.h"
#include "boundingHexahedron.h"
#include "look_at.h"

PStatCollector CollisionParabola::_volume_pcollector(
  "Collision Volumes:CollisionParabola");
PStatCollector CollisionParabola::_test_pcollector(
  "Collision Tests:CollisionParabola");
TypeHandle CollisionParabola::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::get_collision_origin
//       Access: Public, Virtual
//  Description: Returns the point in space deemed to be the "origin"
//               of the solid for collision purposes.  The closest
//               intersection point to this origin point is considered
//               to be the most significant.
////////////////////////////////////////////////////////////////////
LPoint3f CollisionParabola::
get_collision_origin() const {
  return _parabola.calc_point(_t1);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::make_copy
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
CollisionSolid *CollisionParabola::
make_copy() {
  return new CollisionParabola(*this);
}


////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::test_intersection
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(CollisionEntry) CollisionParabola::
test_intersection(const CollisionEntry &entry) const {
  return entry.get_into()->test_intersection_from_parabola(entry);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::xform
//       Access: Public, Virtual
//  Description: Transforms the solid by the indicated matrix.
////////////////////////////////////////////////////////////////////
void CollisionParabola::
xform(const LMatrix4f &mat) {
  _parabola.xform(mat);

  mark_viz_stale();
  mark_internal_bounds_stale();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::get_volume_pcollector
//       Access: Public, Virtual
//  Description: Returns a PStatCollector that is used to count the
//               number of bounding volume tests made against a solid
//               of this type in a given frame.
////////////////////////////////////////////////////////////////////
PStatCollector &CollisionParabola::
get_volume_pcollector() {
  return _volume_pcollector;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::get_test_pcollector
//       Access: Public, Virtual
//  Description: Returns a PStatCollector that is used to count the
//               number of intersection tests made against a solid
//               of this type in a given frame.
////////////////////////////////////////////////////////////////////
PStatCollector &CollisionParabola::
get_test_pcollector() {
  return _test_pcollector;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::output
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void CollisionParabola::
output(ostream &out) const {
  out << _parabola << ", t1 = " << _t1 << ", t2 = " << _t2;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::compute_internal_bounds
//       Access: Protected, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(BoundingVolume) CollisionParabola::
compute_internal_bounds() const {
  LPoint3f p1 = _parabola.calc_point(get_t1());
  LPoint3f p2 = _parabola.calc_point(get_t2());
  LVector3f pdelta = p2 - p1;

  // If p1 and p2 are sufficiently close, just put a sphere around
  // them.
  float d2 = pdelta.length_squared();
  if (d2 < collision_parabola_bounds_threshold * collision_parabola_bounds_threshold) {
    LPoint3f pmid = (p1 + p2) * 0.5f;
    return new BoundingSphere(pmid, csqrt(d2) * 0.5f);
  }

  // OK, the more general bounding volume.  We use BoundingHexahedron
  // to define a very thin box that roughly bounds the parabola's arc.
  // We must use BoundingHexahedron instead of BoundingBox, because
  // the box will not be axis-aligned, and might be inflated too large
  // if we insist on using the axis-aligned BoundingBox.

  // We first define "parabola space" as a coordinate space such that
  // the YZ plane of parabola space corresponds to the plane of the
  // parabola.

  // We have to be explicit about the coordinate system--we
  // specifically mean CS_zup_right here, to make the YZ plane.

  LMatrix4f from_parabola;
  look_at(from_parabola, pdelta, -_parabola.get_a(), CS_zup_right);
  from_parabola.set_row(3, p1);

  // The matrix that computes from world space to parabola space is
  // the inverse of that which we just computed.
  LMatrix4f to_parabola;
  to_parabola.invert_from(from_parabola);

  // Now convert the parabola itself into parabola space.
  Parabolaf psp = _parabola;
  psp.xform(to_parabola);

  LPoint3f pp2 = psp.calc_point(get_t2());
  float max_y = pp2[1];

  // We compute a few points along the parabola to attempt to get the
  // minmax.
  float min_z = 0.0f;
  float max_z = 0.0f;
  int num_points = collision_parabola_bounds_sample;
  for (int i = 0; i < num_points; ++i) {
    double t = (double)(i + 1) / (double)(num_points + 1);
    LPoint3f p = psp.calc_point(get_t1() + t * (get_t2() - get_t1()));
    min_z = min(min_z, p[2]);
    max_z = max(max_z, p[2]);
  }

  // That gives us a simple bounding volume in parabola space.
  PT(BoundingHexahedron) volume = 
    new BoundingHexahedron(LPoint3f(-0.01, max_y, min_z), LPoint3f(0.01, max_y, min_z),
                           LPoint3f(0.01, max_y, max_z), LPoint3f(-0.01, max_y, max_z), 
                           LPoint3f(-0.01, 0, min_z), LPoint3f(0.01, 0, min_z), 
                           LPoint3f(0.01, 0, max_z), LPoint3f(-0.01, 0, max_z));
  // And convert that back into real space.
  volume->xform(from_parabola);
  return volume.p();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::fill_viz_geom
//       Access: Protected, Virtual
//  Description: Fills the _viz_geom GeomNode up with Geoms suitable
//               for rendering this solid.
////////////////////////////////////////////////////////////////////
void CollisionParabola::
fill_viz_geom() {
  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "Recomputing viz for " << *this << "\n";
  }

  static const int num_points = 100;

  PT(GeomVertexData) vdata = new GeomVertexData
    ("collision", GeomVertexFormat::get_v3cp(),
     Geom::UH_static);
  GeomVertexWriter vertex(vdata, InternalName::get_vertex());
  GeomVertexWriter color(vdata, InternalName::get_color());
  
  for (int i = 0; i < num_points; i++) {
    double t = ((double)i / (double)num_points);
    vertex.add_data3f(_parabola.calc_point(_t1 + t * (_t2 - _t1)));
    
    color.add_data4f(Colorf(1.0f, 1.0f, 1.0f, 0.0f) +
                     t * Colorf(0.0f, 0.0f, 0.0f, 1.0f));
  }

  PT(GeomLinestrips) line = new GeomLinestrips(Geom::UH_static);
  line->add_next_vertices(num_points);
  line->close_primitive();
  
  PT(Geom) geom = new Geom(vdata);
  geom->add_primitive(line);
  
  _viz_geom->add_geom(geom, get_other_viz_state());
  _bounds_viz_geom->add_geom(geom, get_other_bounds_viz_state());
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::register_with_read_factory
//       Access: Public, Static
//  Description: Factory method to generate a CollisionParabola object
////////////////////////////////////////////////////////////////////
void CollisionParabola::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::write_datagram
//       Access: Public
//  Description: Function to write the important information in
//               the particular object to a Datagram
////////////////////////////////////////////////////////////////////
void CollisionParabola::
write_datagram(BamWriter *manager, Datagram &me) {
  CollisionSolid::write_datagram(manager, me);
  _parabola.write_datagram(me);
  me.add_float32(_t1);
  me.add_float32(_t2);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::make_from_bam
//       Access: Protected
//  Description: Factory method to generate a CollisionParabola object
////////////////////////////////////////////////////////////////////
TypedWritable *CollisionParabola::
make_from_bam(const FactoryParams &params) {
  CollisionParabola *me = new CollisionParabola;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionParabola::fillin
//       Access: Protected
//  Description: Function that reads out of the datagram (or asks
//               manager to read) all of the data that is needed to
//               re-create this object and stores it in the appropiate
//               place
////////////////////////////////////////////////////////////////////
void CollisionParabola::
fillin(DatagramIterator& scan, BamReader* manager) {
  CollisionSolid::fillin(scan, manager);
  _parabola.read_datagram(scan);
  _t1 = scan.get_float32();
  _t2 = scan.get_float32();
}
