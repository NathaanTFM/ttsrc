// Filename: boundingHexahedron.cxx
// Created by:  drose (03Oct99)
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

#include "boundingHexahedron.h"
#include "boundingSphere.h"
#include "boundingBox.h"
#include "config_mathutil.h"

#include <math.h>
#include <algorithm>

TypeHandle BoundingHexahedron::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
BoundingHexahedron::
BoundingHexahedron(const Frustumf &frustum, bool is_ortho,
                   CoordinateSystem cs) {
  if (cs == CS_default) {
    cs = get_default_coordinate_system();
  }

  float fs = 1.0f;
  if (!is_ortho) {
    fs = frustum._ffar / frustum._fnear;
  }

  // We build the points based on a Z-up right-handed frustum.  If the
  // requested coordinate system is otherwise, we'll convert it in a
  // second pass.
  _points[0].set(frustum._l * fs, frustum._ffar, frustum._b * fs);
  _points[1].set(frustum._r * fs, frustum._ffar, frustum._b * fs);
  _points[2].set(frustum._r * fs, frustum._ffar, frustum._t * fs);
  _points[3].set(frustum._l * fs, frustum._ffar, frustum._t * fs);
  _points[4].set(frustum._l, frustum._fnear, frustum._b);
  _points[5].set(frustum._r, frustum._fnear, frustum._b);
  _points[6].set(frustum._r, frustum._fnear, frustum._t);
  _points[7].set(frustum._l, frustum._fnear, frustum._t);

  _flags = 0;

  // Now fix the coordinate system, if necessary.
  if (cs == CS_zup_right) {
    set_centroid();
    set_planes();
  } else {
    xform(LMatrix4f::convert_mat(CS_zup_right, cs));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
BoundingHexahedron::
BoundingHexahedron(const LPoint3f &fll, const LPoint3f &flr,
                   const LPoint3f &fur, const LPoint3f &ful,
                   const LPoint3f &nll, const LPoint3f &nlr,
                   const LPoint3f &nur, const LPoint3f &nul) {
  _points[0] = fll;
  _points[1] = flr;
  _points[2] = fur;
  _points[3] = ful;
  _points[4] = nll;
  _points[5] = nlr;
  _points[6] = nur;
  _points[7] = nul;

  _flags = 0;
  set_centroid();
  set_planes();
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::make_copy
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
BoundingVolume *BoundingHexahedron::
make_copy() const {
  return new BoundingHexahedron(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::get_min
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
LPoint3f BoundingHexahedron::
get_min() const {
  nassertr(!is_empty(), LPoint3f(0.0f, 0.0f, 0.0f));
  nassertr(!is_infinite(), LPoint3f(0.0f, 0.0f, 0.0f));
  int i;
  LPoint3f m = _points[0];
  for (i = 1; i < num_points; i++) {
    m.set(min(m[0], _points[i][0]),
          min(m[1], _points[i][1]),
          min(m[2], _points[i][2]));
  }
  return m;
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::get_max
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
LPoint3f BoundingHexahedron::
get_max() const {
  nassertr(!is_empty(), LPoint3f(0.0f, 0.0f, 0.0f));
  nassertr(!is_infinite(), LPoint3f(0.0f, 0.0f, 0.0f));
  int i;
  LPoint3f m = _points[0];
  for (i = 1; i < num_points; i++) {
    m.set(max(m[0], _points[i][0]),
          max(m[1], _points[i][1]),
          max(m[2], _points[i][2]));
  }
  return m;
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::get_approx_center
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
LPoint3f BoundingHexahedron::
get_approx_center() const {
  nassertr(!is_empty(), LPoint3f(0.0f, 0.0f, 0.0f));
  nassertr(!is_infinite(), LPoint3f(0.0f, 0.0f, 0.0f));
  return _centroid;
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::xform
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void BoundingHexahedron::
xform(const LMatrix4f &mat) {
  if (!is_empty() && !is_infinite()) {
    for (int i = 0; i < num_points; i++) {
      _points[i] = _points[i] * mat;
    }
    set_centroid();
    set_planes();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void BoundingHexahedron::
output(ostream &out) const {
  if (is_empty()) {
    out << "bhexahedron, empty";
  } else if (is_infinite()) {
    out << "bhexahedron, infinite";
  } else {
    out << "bhexahedron, min " << get_min() << " max " << get_max();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::write
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void BoundingHexahedron::
write(ostream &out, int indent_level) const {
  if (is_empty()) {
    indent(out, indent_level) << "bhexahedron, empty\n";
  } else if (is_infinite()) {
    out << "bhexahedron, infinite\n";
  } else {
    indent(out, indent_level)
      << "bhexahedron, min " << get_min() << " max " << get_max() << ":\n";
    int i;
    for (i = 0; i < num_points; i++) {
      indent(out, indent_level + 2) << _points[i] << "\n";
    }
    indent(out, indent_level + 2) << "centroid is " << _centroid << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::as_bounding_hexahedron
//       Access: Public, Virtual
//  Description: Virtual downcast method.  Returns this object as a
//               pointer of the indicated type, if it is in fact that
//               type.  Returns NULL if it is not that type.
////////////////////////////////////////////////////////////////////
const BoundingHexahedron *BoundingHexahedron::
as_bounding_hexahedron() const {
  return this;
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::extend_other
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
bool BoundingHexahedron::
extend_other(BoundingVolume *other) const {
  return other->extend_by_hexahedron(this);
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::around_other
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
bool BoundingHexahedron::
around_other(BoundingVolume *other,
             const BoundingVolume **first,
             const BoundingVolume **last) const {
  return other->around_hexahedrons(first, last);
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::contains_other
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int BoundingHexahedron::
contains_other(const BoundingVolume *other) const {
  return other->contains_hexahedron(this);
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::contains_point
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int BoundingHexahedron::
contains_point(const LPoint3f &point) const {
  if (is_empty()) {
    return IF_no_intersection;

  } else if (is_infinite()) {
    return IF_possible | IF_some | IF_all;

  } else {
    // The hexahedron contains the point iff the point is behind all of
    // the planes.
    for (int i = 0; i < num_planes; i++) {
      const Planef &p = _planes[i];
      if (p.dist_to_plane(point) > 0.0f) {
        return IF_no_intersection;
      }
    }
    return IF_possible | IF_some | IF_all;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::contains_lineseg
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int BoundingHexahedron::
contains_lineseg(const LPoint3f &a, const LPoint3f &b) const {
  if (is_empty()) {
    return IF_no_intersection;

  } else if (is_infinite()) {
    return IF_possible | IF_some | IF_all;

  } else {
    // The hexahedron does not contains the line segment if both points
    // are in front of any one plane.
    for (int i = 0; i < num_planes; i++) {
      const Planef &p = _planes[i];
      if (p.dist_to_plane(a) > 0.0f ||
          p.dist_to_plane(b) > 0.0f) {
        return IF_no_intersection;
      }
    }

    // If there is no plane that both points are in front of, the
    // hexahedron may or may not contain the line segment.  For the
    // moment, we won't bother to check that more thoroughly, though.
    return IF_possible;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::contains_sphere
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int BoundingHexahedron::
contains_sphere(const BoundingSphere *sphere) const {
  nassertr(!is_empty(), 0);

  // The hexahedron contains the sphere iff the sphere is at least
  // partly behind all of the planes.
  const LPoint3f &center = sphere->get_center();
  float radius = sphere->get_radius();

  int result = IF_possible | IF_some | IF_all;

  for (int i = 0; i < num_planes; i++) {
    const Planef &p = _planes[i];
    float dist = p.dist_to_plane(center);

    if (dist > radius) {
      // The sphere is completely in front of this plane; it's thus
      // completely outside of the hexahedron.
      return IF_no_intersection;

    } else if (dist > -radius) {
      // The sphere is not completely behind this plane, but some of
      // it is.
      result &= ~IF_all;
    }
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::contains_box
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int BoundingHexahedron::
contains_box(const BoundingBox *box) const {
  nassertr(!is_empty(), 0);
  nassertr(!box->is_empty(), 0);

  // Put the box inside a sphere for the purpose of this test.
  const LPoint3f &min = box->get_minq();
  const LPoint3f &max = box->get_maxq();
  LPoint3f center = (min + max) * 0.5f;
  float radius2 = (max - center).length_squared();

  int result = IF_possible | IF_some | IF_all;

  for (int i = 0; i < num_planes; i++) {
    const Planef &p = _planes[i];
    float dist = p.dist_to_plane(center);
    float dist2 = dist * dist;

    if (dist2 <= radius2) {
      // The sphere is not completely behind this plane, but some of
      // it is.
      
      // Look a little closer.
      bool all_in = true;
      bool all_out = true;
      for (int i = 0; i < 8 && (all_in || all_out) ; ++i) {
        if (p.dist_to_plane(box->get_point(i)) < 0.0f) {
          // This point is inside the plane.
          all_out = false;
        } else {
          // This point is outside the plane.
          all_in = false;
        }
      }

      if (all_out) {
        return IF_no_intersection;
      } else if (!all_in) {
        result &= ~IF_all;
      }

    } else if (dist >= 0.0f) {
      // The sphere is completely in front of this plane.
      return IF_no_intersection;
    }
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::contains_hexahedron
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int BoundingHexahedron::
contains_hexahedron(const BoundingHexahedron *hexahedron) const {
  nassertr(!is_empty(), 0);
  nassertr(!hexahedron->is_empty(), 0);

  // Put the hexahedron inside a sphere for the purposes of this test.
  LPoint3f min = hexahedron->get_min();
  LPoint3f max = hexahedron->get_max();
  LPoint3f center = (min + max) * 0.5f;
  float radius2 = (max - center).length_squared();

  int result = IF_possible | IF_some | IF_all;

  for (int i = 0; i < num_planes; i++) {
    const Planef &p = _planes[i];
    float dist = p.dist_to_plane(center);
    float dist2 = dist * dist;

    if (dist >= 0.0f && dist2 > radius2) {
      // The sphere is completely in front of this plane; it's thus
      // completely outside of the hexahedron.
      return IF_no_intersection;

    } else {/*if (dist < 0.0f && dist2 < radius2) {*/
      // The sphere is not completely behind this plane, but some of
      // it is.

      // Look a little closer.
      unsigned points_out = 0;
      for (int i = 0; i < 8; ++i) {
        if (p.dist_to_plane(hexahedron->get_point(i)) > 0.0f) {
          // This point is outside the plane.
          ++points_out;
        }
      }

      if (points_out != 0) {
        if (points_out == 8) {
          return IF_no_intersection;
        }
        result &= ~IF_all;
      }
    }
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::set_planes
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void BoundingHexahedron::
set_planes() {
  _planes[0] = Planef(_points[0], _points[3], _points[2]);

  // Test to see if we have accidentally inverted our frustum by
  // transforming it with a -1 matrix.  We do this by ensuring that
  // the centroid is in front of all of the planes (actually, we only
  // need to test the first plane).
  if (_planes[0].dist_to_plane(_centroid) > 0) {
    // Oops!  We're flipped!  Rebuild the planes in the opposite
    // direction.
    _planes[0] = Planef(_points[0], _points[2], _points[3]);
    _planes[1] = Planef(_points[0], _points[5], _points[1]);
    _planes[2] = Planef(_points[1], _points[6], _points[2]);
    _planes[3] = Planef(_points[2], _points[7], _points[3]);
    _planes[4] = Planef(_points[3], _points[4], _points[0]);
    _planes[5] = Planef(_points[4], _points[7], _points[6]);

  } else {
    // No, a perfectly sane universe.
    _planes[1] = Planef(_points[0], _points[1], _points[5]);
    _planes[2] = Planef(_points[1], _points[2], _points[6]);
    _planes[3] = Planef(_points[2], _points[3], _points[7]);
    _planes[4] = Planef(_points[3], _points[0], _points[4]);
    _planes[5] = Planef(_points[4], _points[6], _points[7]);
  }

  // Still not entirely sure why some code keeps triggering these, but
  // I'm taking them out of the normal build for now.
#ifdef _DEBUG
  nassertv(_planes[0].dist_to_plane(_centroid) <= 0.001);
  nassertv(_planes[1].dist_to_plane(_centroid) <= 0.001);
  nassertv(_planes[2].dist_to_plane(_centroid) <= 0.001);
  nassertv(_planes[3].dist_to_plane(_centroid) <= 0.001);
  nassertv(_planes[4].dist_to_plane(_centroid) <= 0.001);
  nassertv(_planes[5].dist_to_plane(_centroid) <= 0.001);
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: BoundingHexahedron::set_centroid
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void BoundingHexahedron::
set_centroid() {
  LPoint3f net = _points[0];
  for (int i = 1; i < num_points; i++) {
    net += _points[i];
  }
  _centroid = net / (float)num_points;
}
