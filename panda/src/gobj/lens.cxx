// Filename: lens.cxx
// Created by:  drose (18Feb99)
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

#include "lens.h"
#include "throw_event.h"
#include "compose_matrix.h"
#include "look_at.h"
#include "geom.h"
#include "geomLinestrips.h"
#include "geomVertexWriter.h"
#include "boundingHexahedron.h"
#include "indent.h"
#include "config_gobj.h"
#include "plane.h"

TypeHandle Lens::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: Lens::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
Lens::
Lens() {
  clear();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
Lens::
Lens(const Lens &copy) {
  (*this) = copy;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::Copy Assignment Operator
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void Lens::
operator = (const Lens &copy) {
  _change_event = copy._change_event;
  _cs = copy._cs;
  _film_size = copy._film_size;
  _film_offset = copy._film_offset;
  _focal_length = copy._focal_length;
  _fov = copy._fov;
  _aspect_ratio = copy._aspect_ratio;
  _near_distance = copy._near_distance;
  _far_distance = copy._far_distance;

  _view_hpr = copy._view_hpr;
  _view_vector = copy._view_vector;
  _interocular_distance = copy._interocular_distance;
  _convergence_distance = copy._convergence_distance;
  _keystone = copy._keystone;

  _user_flags = copy._user_flags;
  _comp_flags = 0;

  _focal_length_seq = copy._focal_length_seq;
  _fov_seq = copy._fov_seq;
  _film_size_seq = copy._film_size_seq;

  // We don't copy the _geom_data.  That's unique to each Lens.
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_coordinate_system
//       Access: Published
//  Description: Specifies the coordinate system that all 3-d
//               computations are performed within for this
//               Lens.  Normally, this is CS_default.
////////////////////////////////////////////////////////////////////
void Lens::
set_coordinate_system(CoordinateSystem cs) {
  _cs = cs;
  adjust_comp_flags(CF_mat | CF_view_hpr | CF_view_vector, 0);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::clear
//       Access: Published
//  Description: Resets all lens parameters to their initial default
//               settings.
////////////////////////////////////////////////////////////////////
void Lens::
clear() {
  _change_event = "";
  _cs = CS_default;
  _film_size.set(1.0f, 1.0f);
  _film_offset.set(0.0f, 0.0f);
  _focal_length = 1.0f;
  _fov.set(default_fov, default_fov);
  _aspect_ratio = 1.0f;
  _near_distance = default_near;
  _far_distance = default_far;
  _view_hpr.set(0.0f, 0.0f, 0.0f);
  _view_vector.set(0.0f, 1.0f, 0.0f);
  _up_vector.set(0.0f, 0.0f, 1.0f);
  _keystone.set(0.0f, 0.0f);

  _user_flags = 0;
  _comp_flags = CF_fov;

  set_interocular_distance(default_iod);
  set_convergence_distance(default_converge);

  if (default_keystone.has_value()) {
    _keystone.set(default_keystone[0], default_keystone[1]);
    _user_flags |= UF_keystone;
  }

  // Assign an initial arbitrary sequence to these three.
  _film_size_seq = 0;
  _focal_length_seq = 1;
  _fov_seq = 2;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_film_size
//       Access: Published
//  Description: Sets the horizontal size of the film without changing
//               its shape.  The aspect ratio remains unchanged; this
//               computes the vertical size of the film to
//               automatically maintain the aspect ratio.
////////////////////////////////////////////////////////////////////
void Lens::
set_film_size(float width) {
  nassertv(!cnan(width));
  _film_size.set(width, width / get_aspect_ratio());

  // We can't specify all three of focal length, fov, and film size.
  // Throw out the oldest one.
  resequence_fov_triad(_film_size_seq, _focal_length_seq, _fov_seq);

  if (_fov_seq == 0) {
    // Throw out fov if it's oldest.
    adjust_user_flags(UF_hfov | UF_vfov | UF_min_fov | UF_film_height,
                      UF_film_width);
  } else {
    // Otherwise, throw out focal length.
    nassertv(_focal_length_seq == 0);
    adjust_user_flags(UF_focal_length | UF_film_height,
                      UF_film_width);
  }
  adjust_comp_flags(CF_mat | CF_focal_length | CF_fov,
                    CF_film_size);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_film_size
//       Access: Published
//  Description: Sets the size and shape of the "film" within the
//               lens.  This both establishes the units used by
//               calls like set_focal_length(), and establishes the
//               aspect ratio of the frame.
//
//               In a physical camera, the field of view of a lens is
//               determined by the lens' focal length and by the size
//               of the film area exposed by the lens.  For instance,
//               a 35mm camera exposes a rectangle on the film about
//               24mm x 36mm, which means a 50mm lens gives about a
//               40-degree horizontal field of view.
//
//               In the virtual camera, you may set the film size to
//               any units here, and specify a focal length in the
//               same units to simulate the same effect.  Or, you may
//               ignore this parameter, and specify the field of view
//               and aspect ratio of the lens directly.
////////////////////////////////////////////////////////////////////
void Lens::
set_film_size(const LVecBase2f &film_size) {
  nassertv(!film_size.is_nan());
  _film_size = film_size;

  // We can't specify all three of focal length, fov, and film size.
  // Throw out the oldest one.
  resequence_fov_triad(_film_size_seq, _focal_length_seq, _fov_seq);

  if (_fov_seq == 0) {
    // Throw out fov if it's oldest.
    adjust_user_flags(UF_hfov | UF_vfov | UF_min_fov | UF_aspect_ratio,
                      UF_film_width | UF_film_height);
  } else {
    // Otherwise, throw out focal length.
    nassertv(_focal_length_seq == 0);
    adjust_user_flags(UF_focal_length | UF_vfov | UF_aspect_ratio,
                      UF_film_width | UF_film_height);
  }
  adjust_comp_flags(CF_mat | CF_focal_length | CF_fov | CF_aspect_ratio,
                    CF_film_size);

  // Also, the user has implicitly specified an aspect ratio.  Make it
  // stick until the user tells us something different.
  compute_aspect_ratio();
  adjust_user_flags(0, UF_aspect_ratio);

  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_film_size
//       Access: Published
//  Description: Returns the horizontal and vertical film size of
//               the virtual film.  See set_film_size().
////////////////////////////////////////////////////////////////////
const LVecBase2f &Lens::
get_film_size() const {
  if ((_comp_flags & CF_film_size) == 0) {
    // We pretend this is a const method, even though it may call a
    // non-const method to recompute the internal values.  We can do
    // this because this is just compute-on-demand.
    ((Lens *)this)->compute_film_size();
  }
  return _film_size;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_focal_length
//       Access: Published
//  Description: Sets the focal length of the lens.  This may adjust
//               the field-of-view correspondingly, and is an
//               alternate way to specify field of view.
//
//               For certain kinds of lenses (e.g. OrthographicLens),
//               the focal length has no meaning.
////////////////////////////////////////////////////////////////////
void Lens::
set_focal_length(float focal_length) {
  nassertv(!cnan(focal_length));
  _focal_length = focal_length;

  // We can't specify all three of focal length, fov, and film size.
  // Throw out the oldest one.
  resequence_fov_triad(_focal_length_seq, _film_size_seq, _fov_seq);

  if (_film_size_seq == 0) {
    // Throw out film size if it's oldest.
    adjust_user_flags(UF_film_width | UF_film_height,
                      UF_focal_length);
  } else {
    // Otherwise, throw out the fov.
    nassertv(_fov_seq == 0);
    adjust_user_flags(UF_hfov | UF_vfov | UF_min_fov,
                      UF_focal_length);
  }

  adjust_comp_flags(CF_mat | CF_fov | CF_film_size,
                    CF_focal_length);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_focal_length
//       Access: Published
//  Description: Returns the focal length of the lens.  This may have
//               been set explicitly by a previous call to
//               set_focal_length(), or it may be computed based on
//               the lens' fov and film_size.  For certain kinds of
//               lenses, the focal length has no meaning.
////////////////////////////////////////////////////////////////////
float Lens::
get_focal_length() const {
  if ((_comp_flags & CF_focal_length) == 0) {
    ((Lens *)this)->compute_focal_length();
  }
  return _focal_length;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_min_fov
//       Access: Published
//  Description: Sets the field of view of the smallest dimension of
//               the window.  If the window is wider than it is tall,
//               this specifies the vertical field of view; if it is
//               taller than it is wide, this specifies the horizontal
//               field of view.
//
//               In many cases, this is preferable to setting either
//               the horizontal or vertical field of view explicitly.
//               Setting this parameter means that pulling the window
//               wider will widen the field of view, which is usually
//               what you expect to happen.
////////////////////////////////////////////////////////////////////
void Lens::
set_min_fov(float min_fov) {
  nassertv(!cnan(min_fov));
  _min_fov = min_fov;

  // We can't specify all three of focal length, fov, and film size.
  // Throw out the oldest one.
  resequence_fov_triad(_fov_seq, _focal_length_seq, _film_size_seq);

  if (_focal_length_seq == 0) {
    // Throw out focal length if it's oldest.
    adjust_user_flags(UF_focal_length | UF_vfov | UF_hfov,
                      UF_min_fov);
  } else {
    // Otherwise, throw out film size.
    nassertv(_film_size_seq == 0);

    // Make sure we save the aspect ratio first.
    compute_aspect_ratio();
    adjust_user_flags(UF_film_width | UF_film_height | UF_vfov | UF_hfov,
                      UF_min_fov);
  }
  adjust_comp_flags(CF_mat | CF_focal_length | CF_fov | CF_film_size,
                    0);
  // We leave CF_fov off of comp_flags, because we will still need to
  // recompute the vertical fov.  It's not exactly the same as hfov *
  // get_aspect_ratio().
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_fov
//       Access: Published
//  Description: Sets the horizontal field of view of the lens without
//               changing the aspect ratio.  The vertical field of
//               view is adjusted to maintain the same aspect ratio.
////////////////////////////////////////////////////////////////////
void Lens::
set_fov(float hfov) {
  nassertv(!cnan(hfov));
  _fov[0] = hfov;

  // We can't specify all three of focal length, fov, and film size.
  // Throw out the oldest one.
  resequence_fov_triad(_fov_seq, _focal_length_seq, _film_size_seq);

  if (_focal_length_seq == 0) {
    // Throw out focal length if it's oldest.
    adjust_user_flags(UF_focal_length | UF_vfov | UF_min_fov,
                      UF_hfov);
  } else {
    // Otherwise, throw out film size.
    nassertv(_film_size_seq == 0);

    // Make sure we save the aspect ratio first.
    compute_aspect_ratio();
    adjust_user_flags(UF_film_width | UF_film_height | UF_vfov | UF_min_fov,
                      UF_hfov);
  }
  adjust_comp_flags(CF_mat | CF_focal_length | CF_fov | CF_film_size,
                    0);
  // We leave CF_fov off of comp_flags, because we will still need to
  // recompute the vertical fov.  It's not exactly the same as hfov *
  // get_aspect_ratio().
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_fov
//       Access: Published
//  Description: Sets the field of view of the lens in both
//               dimensions.  This establishes both the field of view
//               and the aspect ratio of the lens.  This is one way to
//               specify the field of view of a lens;
//               set_focal_length() is another way.
//
//               For certain kinds of lenses (like OrthographicLens),
//               the field of view has no meaning.
////////////////////////////////////////////////////////////////////
void Lens::
set_fov(const LVecBase2f &fov) {
  nassertv(!fov.is_nan());
  _fov = fov;

  // We can't specify all three of focal length, fov, and film size.
  // Throw out the oldest one.
  resequence_fov_triad(_fov_seq, _focal_length_seq, _film_size_seq);

  if (_focal_length_seq == 0) {
    // Throw out focal length if it's oldest.
    adjust_user_flags(UF_focal_length | UF_film_height | UF_aspect_ratio,
                      UF_hfov | UF_vfov | UF_min_fov);
  } else {
    // Otherwise, throw out film size.
    nassertv(_film_size_seq == 0);
    adjust_user_flags(UF_film_width | UF_film_height | UF_aspect_ratio,
                      UF_hfov | UF_vfov | UF_min_fov);
  }
  adjust_comp_flags(CF_mat | CF_focal_length | CF_film_size | CF_aspect_ratio,
                    CF_fov);

  // Also, the user has implicitly specified an aspect ratio.  Make it
  // stick until the user tells us something different.
  compute_aspect_ratio();
  adjust_user_flags(0, UF_aspect_ratio);

  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_fov
//       Access: Published
//  Description: Returns the horizontal and vertical film size of
//               the virtual film.  See set_fov().
////////////////////////////////////////////////////////////////////
const LVecBase2f &Lens::
get_fov() const {
  if ((_comp_flags & CF_fov) == 0) {
    ((Lens *)this)->compute_fov();
  }
  return _fov;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_min_fov
//       Access: Published
//  Description: Returns the field of view of the narrowest dimension
//               of the window.  See set_min_fov().
////////////////////////////////////////////////////////////////////
float Lens::
get_min_fov() const {
  if ((_comp_flags & CF_fov) == 0) {
    ((Lens *)this)->compute_fov();
  }
  return _min_fov;
}
                

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_aspect_ratio
//       Access: Published
//  Description: Sets the aspect ratio of the lens.  This is the ratio
//               of the height to the width of the generated image.
//               Setting this overrides the two-parameter fov or film
//               size setting.
////////////////////////////////////////////////////////////////////
void Lens::
set_aspect_ratio(float aspect_ratio) {
  nassertv(!cnan(aspect_ratio));
  _aspect_ratio = aspect_ratio;
  adjust_user_flags(UF_film_height | UF_vfov,
                    UF_aspect_ratio);
  adjust_comp_flags(CF_mat | CF_film_size | CF_fov | CF_focal_length,
                    CF_aspect_ratio);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_aspect_ratio
//       Access: Published
//  Description: Returns the aspect ratio of the Lens.  This is
//               determined based on the indicated film size; see
//               set_film_size().
////////////////////////////////////////////////////////////////////
float Lens::
get_aspect_ratio() const {
  if ((_comp_flags & CF_aspect_ratio) == 0) {
    ((Lens *)this)->compute_aspect_ratio();
  }
  return _aspect_ratio;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_default_near
//       Access: Published, Static
//  Description: Returns the default near plane distance that will be
//               assigned to each newly-created lens.  This is read
//               from the Configrc file.
////////////////////////////////////////////////////////////////////
float Lens::
get_default_near() {
  return default_near;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_default_far
//       Access: Published, Static
//  Description: Returns the default far plane distance that will be
//               assigned to each newly-created lens.  This is read
//               from the Configrc file.
////////////////////////////////////////////////////////////////////
float Lens::
get_default_far() {
  return default_far;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_view_hpr
//       Access: Published
//  Description: Sets the direction in which the lens is facing.
//               Normally, this is down the forward axis (usually the
//               Y axis), but it may be rotated.  This is only one way
//               of specifying the rotation; you may also specify an
//               explicit vector in which to look, or you may give a
//               complete transformation matrix.
////////////////////////////////////////////////////////////////////
void Lens::
set_view_hpr(const LVecBase3f &view_hpr) {
  nassertv(!view_hpr.is_nan());
  _view_hpr = view_hpr;
  adjust_user_flags(UF_view_vector | UF_view_mat,
                    UF_view_hpr);
  adjust_comp_flags(CF_mat | CF_view_vector,
                    CF_view_hpr);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_view_hpr
//       Access: Published
//  Description: Returns the direction in which the lens is facing.
////////////////////////////////////////////////////////////////////
const LVecBase3f &Lens::
get_view_hpr() const {
  if ((_comp_flags & CF_view_hpr) == 0) {
    ((Lens *)this)->compute_view_hpr();
  }
  return _view_hpr;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_view_vector
//       Access: Published
//  Description: Specifies the direction in which the lens is facing
//               by giving an axis to look along, and a perpendicular
//               (or at least non-parallel) up axis.
//
//               See also set_view_hpr().
////////////////////////////////////////////////////////////////////
void Lens::
set_view_vector(const LVector3f &view_vector, const LVector3f &up_vector) {
  nassertv(!view_vector.is_nan());
  _view_vector = view_vector;
  _up_vector = up_vector;
  adjust_user_flags(UF_view_hpr | UF_view_mat,
                    UF_view_vector);
  adjust_comp_flags(CF_mat | CF_view_hpr,
                    CF_view_vector);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_view_vector
//       Access: Published
//  Description: Returns the axis along which the lens is facing.
////////////////////////////////////////////////////////////////////
const LVector3f &Lens::
get_view_vector() const {
  if ((_comp_flags & CF_view_vector) == 0) {
    ((Lens *)this)->compute_view_vector();
  }
  return _view_vector;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_up_vector
//       Access: Published
//  Description: Returns the axis perpendicular to the camera's view
//               vector that indicates the "up" direction.
////////////////////////////////////////////////////////////////////
const LVector3f &Lens::
get_up_vector() const {
  if ((_comp_flags & CF_view_vector) == 0) {
    ((Lens *)this)->compute_view_vector();
  }
  return _up_vector;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_nodal_point
//       Access: Published
//  Description: Returns the center point of the lens: the point from
//               which the lens is viewing.
////////////////////////////////////////////////////////////////////
LPoint3f Lens::
get_nodal_point() const {
  return get_view_mat().get_row3(3);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_interocular_distance
//       Access: Published
//  Description: Sets the distance between the left and right eyes of
//               a stereo camera.  This distance is used to apply a
//               stereo effect when the lens is rendered on a stereo
//               display region.  It only has an effect on a
//               PerspectiveLens.
//
//               The left eye and the right eye are each offset along
//               the X axis by half of this distance, so that this
//               parameter specifies the total distance between them.
//
//               Also see set_convergence_distance(), which relates.
////////////////////////////////////////////////////////////////////
void Lens::
set_interocular_distance(float interocular_distance) {
  nassertv(!cnan(interocular_distance));
  _interocular_distance = interocular_distance;
  if (_interocular_distance == 0.0f) {
    adjust_user_flags(UF_interocular_distance, 0);
  } else {
    adjust_user_flags(0, UF_interocular_distance);
  }

  adjust_comp_flags(CF_mat, 0);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_interocular_distance
//       Access: Published
//  Description: See set_interocular_distance().
////////////////////////////////////////////////////////////////////
float Lens::
get_interocular_distance() const {
  return _interocular_distance;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_convergence_distance
//       Access: Published
//  Description: Sets the distance between between the camera plane
//               and the point in the distance that the left and right
//               eyes are both looking at.  This distance is used to
//               apply a stereo effect when the lens is rendered on a
//               stereo display region.  It only has an effect on a
//               PerspectiveLens.
//
//               This parameter must be greater than 0, but may be as
//               large as you like.  It controls the amount to which
//               the two eyes are directed inwards towards each other,
//               which is a normal property of stereo vision.  It is a
//               distance, not an angle; normally this should be set
//               to the distance from the camera to the area of
//               interest in your scene.  If you want to simulate
//               parallel stereo, set this value to a very large
//               number.
//
//               Also see set_interocular_distance(), which relates.
////////////////////////////////////////////////////////////////////
void Lens::
set_convergence_distance(float convergence_distance) {
  nassertv(!cnan(convergence_distance));
  _convergence_distance = convergence_distance;
  if (_convergence_distance == 0.0f) {
    adjust_user_flags(UF_convergence_distance, 0);
  } else {
    adjust_user_flags(0, UF_convergence_distance);
  }

  adjust_comp_flags(CF_mat, 0);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_convergence_distance
//       Access: Published
//  Description: See set_convergence_distance().
////////////////////////////////////////////////////////////////////
float Lens::
get_convergence_distance() const {
  return _convergence_distance;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_view_mat
//       Access: Published
//  Description: Sets an arbitrary transformation on the lens.  This
//               replaces the individual transformation components
//               like set_view_hpr().
//
//               Setting a transformation here will have a slightly
//               different effect than putting one on the LensNode
//               that contains this lens.  In particular, lighting and
//               other effects computations will still be performed on
//               the lens in its untransformed (facing forward)
//               position, but the actual projection matrix will be
//               transformed by this matrix.
////////////////////////////////////////////////////////////////////
void Lens::
set_view_mat(const LMatrix4f &view_mat) {
  nassertv(!view_mat.is_nan());
  _lens_mat = view_mat;
  adjust_user_flags(UF_view_vector | UF_view_hpr,
                    UF_view_mat);
  adjust_comp_flags(CF_projection_mat | CF_projection_mat_inv | 
                    CF_projection_mat_left_inv | CF_projection_mat_right_inv | 
                    CF_lens_mat_inv | CF_view_hpr | CF_view_vector,
                    CF_lens_mat);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_view_mat
//       Access: Published
//  Description: Returns the direction in which the lens is facing.
////////////////////////////////////////////////////////////////////
const LMatrix4f &Lens::
get_view_mat() const {
  if ((_comp_flags & CF_lens_mat) == 0) {
    ((Lens *)this)->compute_lens_mat();
  }
  return _lens_mat;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::clear_view_mat
//       Access: Published
//  Description: Resets the lens transform to identity.
////////////////////////////////////////////////////////////////////
void Lens::
clear_view_mat() {
  _lens_mat = LMatrix4f::ident_mat();
  adjust_user_flags(0, UF_view_vector | UF_view_hpr | UF_view_mat);
  adjust_comp_flags(CF_projection_mat | CF_projection_mat_inv | 
                    CF_projection_mat_left_inv | CF_projection_mat_right_inv | 
                    CF_lens_mat_inv | CF_view_hpr | CF_view_vector,
                    CF_lens_mat);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_keystone
//       Access: Published
//  Description: Indicates the ratio of keystone correction to perform
//               on the lens, in each of three axes.  This will build
//               a special non-affine scale factor into the projection
//               matrix that will compensate for keystoning of a
//               projected image; this can be used to compensate for a
//               projector that for physical reasons cannot be aimed
//               directly at its screen.  
//
//               The default value is taken from the default-keystone
//               Config variable.  0, 0 indicates no keystone
//               correction; specify a small value (usually in the
//               range -1 .. 1) in either the x or y position to
//               generate a keystone correction in that axis.
////////////////////////////////////////////////////////////////////
void Lens::
set_keystone(const LVecBase2f &keystone) {
  nassertv(!keystone.is_nan());
  _keystone = keystone;
  adjust_user_flags(0, UF_keystone);
  adjust_comp_flags(CF_projection_mat | CF_projection_mat_inv |
                    CF_projection_mat_left_inv | CF_projection_mat_right_inv | 
                    CF_film_mat | CF_film_mat_inv, 0);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::clear_keystone
//       Access: Published
//  Description: Disables the lens keystone correction.
////////////////////////////////////////////////////////////////////
void Lens::
clear_keystone() {
  _keystone.set(0.0f, 0.0f);
  adjust_user_flags(UF_keystone, 0);
  adjust_comp_flags(CF_projection_mat | CF_projection_mat_inv | 
                    CF_projection_mat_left_inv | CF_projection_mat_right_inv | 
                    CF_film_mat | CF_film_mat_inv, 0);
  throw_change_event();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::set_frustum_from_corners
//       Access: Published
//  Description: Sets up the lens to use the frustum defined by the
//               four indicated points.  This is most useful for a
//               PerspectiveLens, but it may be called for other kinds
//               of lenses as well.
//
//               The frustum will be rooted at the origin (or by
//               whatever translation might have been specified in a
//               previous call to set_view_mat).
//
//               It is legal for the four points not to be arranged in
//               a rectangle; if this is the case, the frustum will be
//               fitted as tightly as possible to cover all four
//               points.
//
//               The flags parameter contains the union of one or more
//               of the following bits to control the behavior of this
//               function:
//
//               FC_roll - If this is included, the camera may be
//               rotated so that its up vector is perpendicular to the
//               top line.  Otherwise, the standard up vector is used.
//
//               FC_camera_plane - This allows the camera plane to be
//               adjusted to be as nearly perpendicular to the center
//               of the frustum as possible.  Without this bit, the
//               orientation camera plane is defined by position of
//               the four points (which should all be coplanar).  With
//               this bit, the camera plane is arbitarary, and may be
//               chosen so that the four points do not themselves lie
//               in the camera plane (but the points will still be
//               within the frustum).
//
//               FC_off_axis - This allows the resulting frustum to be
//               off-axis to get the tightest possible fit.  Without
//               this bit, the viewing axis will be centered within
//               the frustum, but there may be more wasted space along
//               the edges.
//
//               FC_aspect_ratio - This allows the frustum to be
//               scaled non-proportionately in the vertical and
//               horizontal dimensions, if necessary, to get a tighter
//               fit.  Without this bit, the current aspect ratio will
//               be preserved.
//
//               FC_shear - This allows the frustum to be sheared, if
//               necessary, to get the tightest possible fit.  This
//               may result in a parallelogram-based frustum, which
//               will give a slanted appearance to the rendered image.
//               Without this bit, the frustum will be
//               rectangle-based.
//
//               In general, if 0 is passed in as the value for flags,
//               the generated frustum will be a loose fit but sane;
//               if -1 is passed in, it will be a tighter fit and
//               possibly screwy.
////////////////////////////////////////////////////////////////////
void Lens::
set_frustum_from_corners(const LVecBase3f &ul, const LVecBase3f &ur,
                         const LVecBase3f &ll, const LVecBase3f &lr,
                         int flags) {
  nassertv(!ul.is_nan() && !ur.is_nan() && !ll.is_nan() && !lr.is_nan());
  // We'll need to know the pre-existing eyepoint translation from the
  // center, so we can preserve it in the new frustum.  This is
  // usually (0, 0, 0), but it could be an arbitrary vector.
  const LMatrix4f &lens_mat_inv = get_lens_mat_inv();
  LVector3f eye_offset;
  lens_mat_inv.get_row3(eye_offset, 3);

  // Now choose the viewing axis.  If FC_camera_plane is specified,
  // we'll pass it through the centroid for the best camera plane;
  // otherwise, it's perpendicular to the plane in which the points
  // lie.
  LVector3f view_vector;
  if ((flags & FC_camera_plane) != 0) {
    view_vector = (ul + ur + ll + lr) * 0.25f;
  } else {
    Planef plane(ll, ul, ur);
    view_vector = plane.get_normal();
    nassertv(!view_vector.is_nan() && view_vector.length_squared() != 0.0f);
  }

  // Now determine the up axis.  If FC_roll is specified, or if our
  // view vector is straight up, it is the vector perpendicular to
  // both the viewing axis and the top line.  Otherwise, it is the
  // standard up axis.
  LVector3f up_vector = LVector3f::up(_cs);
  if (view_vector == up_vector || ((flags & FC_roll) != 0)) {
    LVector3f top = ul - ur;
    up_vector = view_vector.cross(top);
    nassertv(!up_vector.is_nan() && up_vector.length_squared() != 0.0f);
  }

  // Now compute the matrix that applies this rotation.
  LMatrix4f rot_mat;
  look_at(rot_mat, view_vector, up_vector, CS_zup_right);

  // And invert it.
  LMatrix4f inv_rot_mat;
  inv_rot_mat.invert_affine_from(rot_mat);

  // Use that inverse matrix to convert the four corners to a local
  // coordinate system, looking down the Y axis.
  LPoint3f cul = inv_rot_mat.xform_point(ul);
  LPoint3f cur = inv_rot_mat.xform_point(ur);
  LPoint3f cll = inv_rot_mat.xform_point(ll);
  LPoint3f clr = inv_rot_mat.xform_point(lr);

  // Project all points into the Y == 1 plane, so we can do 2-d
  // manipulation on them.
  nassertv(cul[1] != 0.0f && cur[1] != 0.0f && cll[1] != 0.0f && clr[1] != 0.0f);
  cul /= cul[1];
  cur /= cur[1];
  cll /= cll[1];
  clr /= clr[1];

  LMatrix4f shear_mat = LMatrix4f::ident_mat();
  LMatrix4f inv_shear_mat = LMatrix4f::ident_mat();

  // Now, if we're allowed to shear the frustum, do so.
  if ((flags & FC_shear) != 0) {
    build_shear_mat(shear_mat, cul, cur, cll, clr);
    inv_shear_mat.invert_from(shear_mat);
  } 

  // Now build the complete view matrix.
  LMatrix4f inv_view_mat =
    inv_rot_mat *
    inv_shear_mat;

  // And reapply the eye offset to this matrix.
  inv_view_mat.set_row(3, eye_offset);

  LMatrix4f view_mat;
  view_mat.invert_from(inv_view_mat);
  set_view_mat(view_mat);

  LPoint3f ful = inv_view_mat.xform_point(ul);
  LPoint3f fur = inv_view_mat.xform_point(ur);
  LPoint3f fll = inv_view_mat.xform_point(ll);
  LPoint3f flr = inv_view_mat.xform_point(lr);

  // Normalize *these* points into the y == 1 plane.
  nassertv(ful[1] != 0.0f && fur[1] != 0.0f && fll[1] != 0.0f && flr[1] != 0.0f);
  ful /= ful[1];
  fur /= fur[1];
  fll /= fll[1];
  flr /= flr[1];

  // Determine the minimum field of view necesary to cover all four
  // transformed points.
  float min_x = min(min(ful[0], fur[0]), min(fll[0], flr[0]));
  float max_x = max(max(ful[0], fur[0]), max(fll[0], flr[0]));
  float min_z = min(min(ful[2], fur[2]), min(fll[2], flr[2]));
  float max_z = max(max(ful[2], fur[2]), max(fll[2], flr[2]));

  float x_spread, x_center, z_spread, z_center;

  if ((flags & FC_off_axis) != 0) {
    // If we're allowed to make an off-axis projection, then pick the
    // best center.
    x_center = (max_x + min_x) * 0.5f;
    z_center = (max_z + min_z) * 0.5f;
    x_spread = x_center - min_x;
    z_spread = z_center - min_z;
  } else {
    // Otherwise, the center must be (0, 0).
    x_center = 0.0f;
    z_center = 0.0f;
    x_spread = max(cabs(max_x), cabs(min_x));
    z_spread = max(cabs(max_z), cabs(min_z));
  }

  float aspect_ratio = get_aspect_ratio();
  nassertv(aspect_ratio != 0.0f);
  if ((flags & FC_aspect_ratio) == 0) {
    // If we must preserve the aspect ratio, then the x and z spreads
    // must be adjusted to match.
    if (x_spread < z_spread * aspect_ratio) {
      // x_spread is too small.
      x_spread = z_spread * aspect_ratio;
    } else if (z_spread < x_spread / aspect_ratio) {
      // z_spread is too small.
      z_spread = x_spread / aspect_ratio;
    }
  }

  float hfov = rad_2_deg(catan(x_spread)) * 2.0f;
  float vfov = rad_2_deg(catan(z_spread)) * 2.0f;

  set_fov(hfov, vfov);

  if ((flags & FC_aspect_ratio) == 0) {
    // If we must preserve the aspect ratio, store it one more time.
    // This is mainly in case we have a non-perspective lens with a
    // funny relationship between fov and aspect ratio.
    set_aspect_ratio(aspect_ratio);
  }

  const LVecBase2f &film_size = get_film_size();
  nassertv(x_spread != 0.0f && z_spread != 0.0f);
  set_film_offset(film_size[0] * x_center / (x_spread * 2.0f),
                  film_size[1] * z_center / (z_spread * 2.0f));
}


////////////////////////////////////////////////////////////////////
//     Function: Lens::recompute_all
//       Access: Published
//  Description: Forces all internal parameters of the Lens to be
//               recomputed.  Normally, this should never need to be
//               called; it is provided only to assist in debugging.
////////////////////////////////////////////////////////////////////
void Lens::
recompute_all() {
  _comp_flags = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::is_linear
//       Access: Published, Virtual
//  Description: Returns true if the lens represents a linear
//               projection (e.g. PerspectiveLens, OrthographicLens),
//               and therefore there is a valid matrix returned by
//               get_projection_mat(), or false otherwise.
////////////////////////////////////////////////////////////////////
bool Lens::
is_linear() const {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::is_perspective
//       Access: Published, Virtual
//  Description: Returns true if the lens represents a perspective
//               projection (i.e. it is a PerspectiveLens), false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool Lens::
is_perspective() const {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::is_orthographic
//       Access: Published, Virtual
//  Description: Returns true if the lens represents a orthographic
//               projection (i.e. it is a OrthographicLens), false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool Lens::
is_orthographic() const {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::make_geometry
//       Access: Published, Virtual
//  Description: Allocates and returns a new Geom that can be rendered
//               to show a visible representation of the frustum used
//               for this kind of lens, if it makes sense to do
//               so.  If a visible representation cannot be created,
//               returns NULL.
////////////////////////////////////////////////////////////////////
PT(Geom) Lens::
make_geometry() {
  // The default behavior for make_geometry() will be to draw a
  // hexahedron around the eight vertices of the frustum.  If the lens
  // is non-linear, the hexahedron will be curved; in that case, we'll
  // subdivide the lines into several segments to get an approximation
  // of the curve.

  // First, define all the points we'll use in this Geom.  That's one
  // point at each corner of the near and far planes (and possibly
  // more points along the edges).
  int num_segments = define_geom_data();
  if (num_segments == 0) {
    // Can't do a frustum.
    _geom_data.clear();
    return (Geom *)NULL;
  }
  
  // Now string together the line segments.
  PT(GeomLinestrips) line = new GeomLinestrips(Geom::UH_static);
  
  // Draw a frame around the near plane.
  int i, si;
  for (i = 0; i < 4; ++i) {
    for (si = 0; si < num_segments; ++si) {
      line->add_vertex(i * 2 + si * (4 * 2) + 0);
    }
  }
  line->add_vertex(0);
  line->close_primitive();
  
  // Draw a frame around the far plane.
  for (i = 0; i < 4; ++i) {
    for (si = 0; si < num_segments; ++si) {
      line->add_vertex(i * 2 + si * (4 * 2) + 1);
    }
  }
  line->add_vertex(1);
  line->close_primitive();
  
  // Draw connecting lines at the corners.
  line->add_vertex(0 * 2 + 0);
  line->add_vertex(0 * 2 + 1);
  line->close_primitive();
  
  line->add_vertex(1 * 2 + 0);
  line->add_vertex(1 * 2 + 1);
  line->close_primitive();
  
  line->add_vertex(2 * 2 + 0);
  line->add_vertex(2 * 2 + 1);
  line->close_primitive();
  
  line->add_vertex(3 * 2 + 0);
  line->add_vertex(3 * 2 + 1);
  line->close_primitive();
  
  // And one more line for the viewing axis.
  line->add_vertex(num_segments * (4 * 2) + 0);
  line->add_vertex(num_segments * (4 * 2) + 1);
  line->close_primitive();
  
  PT(Geom) geom = new Geom(_geom_data);
  geom->add_primitive(line);
  
  return geom.p();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::make_bounds
//       Access: Published, Virtual
//  Description: Allocates and returns a new BoundingVolume that
//               encloses the frustum used for this kind of
//               lens, if possible.  If a suitable bounding
//               volume cannot be created, returns NULL.
////////////////////////////////////////////////////////////////////
PT(BoundingVolume) Lens::
make_bounds() const {
  // The default bounding volume is a hexahedron based on the eight
  // corners of the frustum.
  LPoint3f fll, flr, ful, fur;
  LPoint3f nll, nlr, nul, nur;
  LPoint2f corner;

  corner[0] = -1.0f; corner[1] = 1.0f;

  // Upper left.
  if (!extrude(corner, nul, ful)) {
    return (BoundingVolume *)NULL;
  }

  corner[0] = 1.0f; corner[1] = 1.0f;

  // Upper right.
  if (!extrude(corner, nur, fur)) {
    return (BoundingVolume *)NULL;
  }

  corner[0] = 1.0f; corner[1] = -1.0f;

  // Lower right.
  if (!extrude(corner, nlr, flr)) {
    return (BoundingVolume *)NULL;
  }

  corner[0] = -1.0f; corner[1] = -1.0f;

  // Lower left.
  if (!extrude(corner, nll, fll)) {
    return (BoundingVolume *)NULL;
  }

  return new BoundingHexahedron(fll, flr, fur, ful, nll, nlr, nur, nul);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_projection_mat
//       Access: Published
//  Description: Returns the complete transformation matrix from a 3-d
//               point in space to a point on the film, if such a
//               matrix exists, or the identity matrix if the lens is
//               nonlinear.
////////////////////////////////////////////////////////////////////
const LMatrix4f &Lens::
get_projection_mat(StereoChannel channel) const {
  if ((_comp_flags & CF_projection_mat) == 0) {
    ((Lens *)this)->compute_projection_mat();
  }

  switch (channel) {
  case SC_left:
    return _projection_mat_left;
  case SC_right:
    return _projection_mat_right;
  case SC_mono:
  case SC_stereo:
    return _projection_mat;
  }

  return _projection_mat;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_projection_mat_inv
//       Access: Published
//  Description: Returns the matrix that transforms from a 2-d point
//               on the film to a 3-d vector in space, if such a
//               matrix exists.
////////////////////////////////////////////////////////////////////
const LMatrix4f &Lens::
get_projection_mat_inv(StereoChannel stereo_channel) const {
  switch (stereo_channel) {
  case SC_left:
    {
      if ((_comp_flags & CF_projection_mat_left_inv) == 0) {
        Lens *non_const = (Lens *)this;
        const LMatrix4f &projection_mat_left = get_projection_mat(SC_left);
        non_const->_projection_mat_left_inv.invert_from(projection_mat_left);
        non_const->adjust_comp_flags(0, CF_projection_mat_left_inv);
      }
    }
    return _projection_mat_left_inv;

  case SC_right:
    {
      if ((_comp_flags & CF_projection_mat_right_inv) == 0) {
        Lens *non_const = (Lens *)this;
        const LMatrix4f &projection_mat_right = get_projection_mat(SC_right);
        non_const->_projection_mat_right_inv.invert_from(projection_mat_right);
        non_const->adjust_comp_flags(0, CF_projection_mat_right_inv);
      }
    }
    return _projection_mat_right_inv;

  case SC_mono:
  case SC_stereo:
    break;
  }

  if ((_comp_flags & CF_projection_mat_inv) == 0) {
    Lens *non_const = (Lens *)this;
    const LMatrix4f &projection_mat = get_projection_mat();
    non_const->_projection_mat_inv.invert_from(projection_mat);
    non_const->adjust_comp_flags(0, CF_projection_mat_inv);
  }
  return _projection_mat_inv;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_film_mat
//       Access: Published
//  Description: Returns the matrix that transforms from a point
//               behind the lens to a point on the film.
////////////////////////////////////////////////////////////////////
const LMatrix4f &Lens::
get_film_mat() const {
  if ((_comp_flags & CF_film_mat) == 0) {
    ((Lens *)this)->compute_film_mat();
  }
  return _film_mat;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_film_mat_inv
//       Access: Published
//  Description: Returns the matrix that transforms from a point on
//               the film to a point behind the lens.
////////////////////////////////////////////////////////////////////
const LMatrix4f &Lens::
get_film_mat_inv() const {
  if ((_comp_flags & CF_film_mat_inv) == 0) {
    Lens *non_const = (Lens *)this;
    const LMatrix4f &film_mat = get_film_mat();
    non_const->_film_mat_inv.invert_from(film_mat);
    non_const->adjust_comp_flags(0, CF_film_mat_inv);
  }
  return _film_mat_inv;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_lens_mat
//       Access: Published
//  Description: Returns the matrix that transforms from a point
//               in front of the lens to a point in space.
////////////////////////////////////////////////////////////////////
const LMatrix4f &Lens::
get_lens_mat() const {
  if ((_comp_flags & CF_lens_mat) == 0) {
    ((Lens *)this)->compute_lens_mat();
  }
  return _lens_mat;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::get_lens_mat_inv
//       Access: Published
//  Description: Returns the matrix that transforms from a point in
//               space to a point in front of the lens.
////////////////////////////////////////////////////////////////////
const LMatrix4f &Lens::
get_lens_mat_inv() const {
  if ((_comp_flags & CF_lens_mat_inv) == 0) {
    Lens *non_const = (Lens *)this;
    const LMatrix4f &lens_mat = get_lens_mat();
    non_const->_lens_mat_inv.invert_from(lens_mat);
    non_const->adjust_comp_flags(0, CF_lens_mat_inv);
  }
  return _lens_mat_inv;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::output
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void Lens::
output(ostream &out) const {
  out << get_type();
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::write
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void Lens::
write(ostream &out, int indent_level) const {
  indent(out, indent_level) << get_type() << " fov = " << get_fov() << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::throw_change_event
//       Access: Protected
//  Description: Throws the event associated with changing properties
//               on this Lens, if any.
////////////////////////////////////////////////////////////////////
void Lens::
throw_change_event() {
  ++_last_change;

  if (!_change_event.empty()) {
    throw_event(_change_event, this);
  }

  if (!_geom_data.is_null()) {
    if (_geom_data->get_ref_count() == 1) {
      // No one's using the data any more (there are no references to
      // it other than this one), so don't bother to recompute it;
      // just release it.
      _geom_data.clear();
    } else {
      // Someone still has a handle to the data, so recompute it for
      // them.
      define_geom_data();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::extrude_impl
//       Access: Protected, Virtual
//  Description: Given a 2-d point in the range (-1,1) in both
//               dimensions, where (0,0) is the center of the
//               lens and (-1,-1) is the lower-left corner,
//               compute the corresponding vector in space that maps
//               to this point, if such a vector can be determined.
//               The vector is returned by indicating the points on
//               the near plane and far plane that both map to the
//               indicated 2-d point.
//
//               The z coordinate of the 2-d point is ignored.
//
//               Returns true if the vector is defined, or false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool Lens::
extrude_impl(const LPoint3f &point2d, LPoint3f &near_point, LPoint3f &far_point) const {
  const LMatrix4f &projection_mat_inv = get_projection_mat_inv();
  {
    LVecBase4f full(point2d[0], point2d[1], -1.0f, 1.0f);
    full = projection_mat_inv.xform(full);

    float recip_full3 = 1.0 / max((double)full[3], (double)lens_far_limit);
    near_point.set(full[0] * recip_full3, 
                   full[1] * recip_full3, 
                   full[2] * recip_full3);
  }
  {
    LVecBase4f full(point2d[0], point2d[1], 1.0f, 1.0f);
    full = projection_mat_inv.xform(full);

    // We can truncate the weight factor at near 0.  If it goes too
    // close to zero, or becomes negative, the far plane moves out
    // past infinity and comes back in behind the lens, which is just
    // crazy.  Truncating it to zero keeps the far plane from moving
    // too far out.
    float recip_full3 = 1.0 / max((double)full[3], (double)lens_far_limit);
    far_point.set(full[0] * recip_full3, 
                  full[1] * recip_full3, 
                  full[2] * recip_full3);
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::extrude_vec_impl
//       Access: Protected, Virtual
//  Description: Given a 2-d point in the range (-1,1) in both
//               dimensions, where (0,0) is the center of the
//               lens and (-1,-1) is the lower-left corner,
//               compute the vector that corresponds to the view
//               direction.  This will be parallel to the normal on
//               the surface (the far plane) corresponding to the lens
//               shape at this point.
//
//               Generally, for all rational lenses, the center of the
//               film at (0,0) computes a vector that is in the same
//               direction as the vector specified by
//               set_view_vector().
//
//               For all linear lenses, including perspective and
//               orthographic lenses, all points on the film compute
//               this same vector (the far plane is a flat plane, so
//               the normal is the same everywhere).  For curved
//               lenses like fisheye and cylindrical lenses, different
//               points may compute different vectors (the far "plane"
//               on these lenses is a curved surface).
//
//               The z coordinate of the 2-d point is ignored.
//
//               Returns true if the vector is defined, or false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool Lens::
extrude_vec_impl(const LPoint3f &point2d, LVector3f &vec) const {
  vec = LVector3f::forward(_cs) * get_lens_mat();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::project_impl
//       Access: Protected, Virtual
//  Description: Given a 3-d point in space, determine the 2-d point
//               this maps to, in the range (-1,1) in both dimensions,
//               where (0,0) is the center of the lens and
//               (-1,-1) is the lower-left corner.
//
//               The z coordinate will also be set to a value in the
//               range (-1, 1), where 1 represents a point on the near
//               plane, and -1 represents a point on the far plane.
//
//               Returns true if the 3-d point is in front of the lens
//               and within the viewing frustum (in which case point2d
//               is filled in), or false otherwise (in which case
//               point2d will be filled in with something, which may
//               or may not be meaningful).
////////////////////////////////////////////////////////////////////
bool Lens::
project_impl(const LPoint3f &point3d, LPoint3f &point2d) const {
  const LMatrix4f &projection_mat = get_projection_mat();
  LVecBase4f full(point3d[0], point3d[1], point3d[2], 1.0f);
  full = projection_mat.xform(full);
  if (full[3] == 0.0f) {
    point2d.set(0.0f, 0.0f, 0.0f);
    return false;
  }
  float recip_full3 = 1.0f/full[3];
  point2d.set(full[0] * recip_full3, full[1] * recip_full3, full[2] * recip_full3);
  return
    (full[3] > 0.0f) &&
    (point2d[0] >= -1.0f) && (point2d[0] <= 1.0f) && 
    (point2d[1] >= -1.0f) && (point2d[1] <= 1.0f);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_film_size
//       Access: Protected, Virtual
//  Description: Computes the size and shape of the film behind the
//               camera, based on the aspect ratio and fov.
////////////////////////////////////////////////////////////////////
void Lens::
compute_film_size() {
  if ((_user_flags & (UF_min_fov | UF_focal_length)) == (UF_min_fov | UF_focal_length)) {
    // If we just have a min FOV and a focal length, that determines
    // the smaller of the two film_sizes, and the larger is simply
    // chosen according to the aspect ratio.
    float fs = fov_to_film(_min_fov, _focal_length, true);
    nassertv((_user_flags & UF_aspect_ratio) != 0 ||
             (_comp_flags & CF_aspect_ratio) != 0);

    if (_aspect_ratio < 1.0f) {
      _film_size[1] = fs / _aspect_ratio;
      _film_size[0] = fs;

    } else {
      _film_size[0] = fs * _aspect_ratio;
      _film_size[1] = fs;
    }

  } else {
    if ((_user_flags & UF_film_width) == 0) {
      if ((_user_flags & (UF_hfov | UF_focal_length)) == (UF_hfov | UF_focal_length)) {
        _film_size[0] = fov_to_film(_fov[0], _focal_length, true);
      } else {
        _film_size[0] = 1.0f;
      }
    }
    
    if ((_user_flags & UF_film_height) == 0) {
      if ((_user_flags & (UF_vfov | UF_focal_length)) == (UF_vfov | UF_focal_length)) {
        _film_size[1] = fov_to_film(_fov[1], _focal_length, false);
        
      } else if ((_user_flags & (UF_hfov | UF_vfov)) == (UF_hfov | UF_vfov)) {
        // If we don't have a focal length, but we have an explicit vfov
        // and hfov, we can infer the focal length is whatever makes the
        // film width, above, be what it is.
        if ((_comp_flags & CF_focal_length) == 0) {
          _focal_length = fov_to_focal_length(_fov[0], _film_size[0], true);
          adjust_comp_flags(0, CF_focal_length);
        }
        _film_size[1] = fov_to_film(_fov[1], _focal_length, false);
        
      } else if ((_user_flags & UF_aspect_ratio) != 0 ||
                 (_comp_flags & CF_aspect_ratio) != 0) {
        _film_size[1] = _film_size[0] / _aspect_ratio;
        
      } else {
        // Default is an aspect ratio of 1.
        _film_size[1] = _film_size[0];
      }
    }
  }

  adjust_comp_flags(0, CF_film_size);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_focal_length
//       Access: Protected, Virtual
//  Description: Computes the focal length of the lens, based on the
//               fov and film size.  This is based on the horizontal
//               dimension.
////////////////////////////////////////////////////////////////////
void Lens::
compute_focal_length() {
  if ((_user_flags & UF_focal_length) == 0) {
    const LVecBase2f &film_size = get_film_size();
    const LVecBase2f &fov = get_fov();
    _focal_length = fov_to_focal_length(fov[0], film_size[0], true);
  }

  adjust_comp_flags(0, CF_focal_length);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_fov
//       Access: Protected, Virtual
//  Description: Computes the field of view of the lens, based on the
//               film size and focal length.
////////////////////////////////////////////////////////////////////
void Lens::
compute_fov() {
  const LVecBase2f &film_size = get_film_size();

  bool got_hfov = ((_user_flags & UF_hfov) != 0);
  bool got_vfov = ((_user_flags & UF_vfov) != 0);
  bool got_min_fov = ((_user_flags & UF_min_fov) != 0);

  if (!got_hfov && !got_vfov && !got_min_fov) {
    // If the user hasn't specified any FOV, we have to compute it.
    if ((_user_flags & UF_focal_length) != 0) {
      // The FOV is determined from the film size and focal length.
      _fov[0] = film_to_fov(film_size[0], _focal_length, true);
      _fov[1] = film_to_fov(film_size[1], _focal_length, true);
      got_hfov = true;
      got_vfov = true;

    } else {
      // We can't compute the FOV; take the default.
      _min_fov = default_fov;
      got_min_fov = true;
    }
  }

  if (got_min_fov) {
    // If we have just a min_fov, use it to derive whichever fov is
    // smaller.
    if (film_size[0] < film_size[1]) {
      _fov[0] = _min_fov;
      got_hfov = true;
    } else {
      _fov[1] = _min_fov;
      got_vfov = true;
    }
  }

  // Now compute whichever fov is remaining.
  if (!got_hfov) {
    if ((_user_flags & UF_focal_length) == 0 &&
        (_comp_flags & CF_focal_length) == 0) {
      // If we don't have an explicit focal length, we can infer it
      // from the above.
      nassertv(got_vfov);
      _focal_length = fov_to_focal_length(_fov[1], film_size[1], true);
      adjust_comp_flags(0, CF_focal_length);
    }
    _fov[0] = film_to_fov(film_size[0], _focal_length, false);
    got_hfov = true;
  }
  
  if (!got_vfov) {
    if ((_user_flags & UF_focal_length) == 0 &&
        (_comp_flags & CF_focal_length) == 0) {
      // If we don't have an explicit focal length, we can infer it
      // from the above.
      nassertv(got_hfov);
      _focal_length = fov_to_focal_length(_fov[0], film_size[0], true);
      adjust_comp_flags(0, CF_focal_length);
    }
    _fov[1] = film_to_fov(film_size[1], _focal_length, false);
    got_vfov = true;
  }

  if (!got_min_fov) {
    _min_fov = film_size[0] < film_size[1] ? _fov[0] : _fov[1];
    got_min_fov = true;
  }

  nassertv(got_hfov && got_vfov && got_min_fov);
  adjust_comp_flags(0, CF_fov);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_aspect_ratio
//       Access: Protected, Virtual
//  Description: Computes the aspect ratio of the film rectangle, as a
//               ratio of width to height.
////////////////////////////////////////////////////////////////////
void Lens::
compute_aspect_ratio() {
  if ((_user_flags & UF_aspect_ratio) == 0) {
    const LVecBase2f &film_size = get_film_size();
    if (film_size[1] == 0.0f) {
      _aspect_ratio = 1.0f;
    } else {
      _aspect_ratio = film_size[0] / film_size[1];
    }
  }
  adjust_comp_flags(0, CF_aspect_ratio);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_view_hpr
//       Access: Protected, Virtual
//  Description: Computes the Euler angles representing the lens'
//               rotation.
////////////////////////////////////////////////////////////////////
void Lens::
compute_view_hpr() {
  if ((_user_flags & UF_view_hpr) == 0) {
    const LMatrix4f &view_mat = get_view_mat();
    LVecBase3f scale, shear, translate;
    decompose_matrix(view_mat, scale, shear, _view_hpr, translate, _cs);
  }
  adjust_comp_flags(0, CF_view_hpr);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_view_vector
//       Access: Protected, Virtual
//  Description: Computes the view vector and up vector for the lens.
////////////////////////////////////////////////////////////////////
void Lens::
compute_view_vector() {
  if ((_user_flags & UF_view_vector) == 0) {
    const LMatrix4f &view_mat = get_view_mat();
    _view_vector = LVector3f::forward(_cs) * view_mat;
    _up_vector = LVector3f::up(_cs) * view_mat;
  }
  adjust_comp_flags(0, CF_view_vector);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_projection_mat
//       Access: Protected, Virtual
//  Description: Computes the complete transformation matrix from 3-d
//               point to 2-d point, if the lens is linear.
////////////////////////////////////////////////////////////////////
void Lens::
compute_projection_mat() {
  _projection_mat = 
    _projection_mat_left = 
    _projection_mat_right =
    _projection_mat_inv = 
    _projection_mat_left_inv = 
    _projection_mat_right_inv = 
    LMatrix4f::ident_mat();
  adjust_comp_flags(0, CF_projection_mat | CF_projection_mat_inv |
                    CF_projection_mat_left_inv |CF_projection_mat_right_inv);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_film_mat
//       Access: Protected, Virtual
//  Description: Computes the matrix that transforms from a point
//               behind the lens to a point on the film.
////////////////////////////////////////////////////////////////////
void Lens::
compute_film_mat() {
  // The lens will return a point in the range [-film_size/2,
  // film_size/2] in each dimension.  Convert this to [-1, 1], and
  // also apply the offset.

  // We declare these two as local variables, instead of references,
  // to work around a VC7 compiler bug.
  LVecBase2f film_size = get_film_size();
  LVector2f film_offset = get_film_offset();

  float scale_x = 2.0f / film_size[0];
  float scale_y = 2.0f / film_size[1];
  _film_mat.set(scale_x,      0.0f,   0.0f,  0.0f,
                   0.0f,   scale_y,   0.0f,  0.0f,
                   0.0f,      0.0f,   1.0f,  0.0f,
        -film_offset[0] * scale_x, -film_offset[1] * scale_y, 0.0f,  1.0f);

  if ((_user_flags & UF_keystone) != 0) {
    _film_mat = LMatrix4f(1.0f, 0.0f, _keystone[0], _keystone[0],
                          0.0f, 1.0f, _keystone[1], _keystone[1],
                          0.0f, 0.0f, 1.0f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f) * _film_mat;
  }

  adjust_comp_flags(CF_film_mat_inv,
                    CF_film_mat);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::compute_lens_mat
//       Access: Protected, Virtual
//  Description: Computes the matrix that transforms from a point
//               in front of the lens to a point in space.
////////////////////////////////////////////////////////////////////
void Lens::
compute_lens_mat() {
  if ((_user_flags & UF_view_mat) == 0) {
    if ((_user_flags & UF_view_hpr) != 0) {
      compose_matrix(_lens_mat,
                     LVecBase3f(1.0f, 1.0f, 1.0f),
                     LVecBase3f(0.0f, 0.0f, 0.0f),
                     _view_hpr,
                     LVecBase3f(0.0f, 0.0f, 0.0f), _cs);

    } else if ((_user_flags & UF_view_vector) != 0) {
      look_at(_lens_mat, _view_vector, _up_vector, _cs);

    } else {
      _lens_mat = LMatrix4f::ident_mat();
    }
  }
  adjust_comp_flags(CF_lens_mat_inv,
                    CF_lens_mat);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::fov_to_film
//       Access: Protected, Virtual
//  Description: Given a field of view in degrees and a focal length,
//               compute the corresponding width (or height) on the
//               film.  If horiz is true, this is in the horizontal
//               direction; otherwise, it is in the vertical direction
//               (some lenses behave differently in each direction).
////////////////////////////////////////////////////////////////////
float Lens::
fov_to_film(float, float, bool) const {
  return 1.0f;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::fov_to_focal_length
//       Access: Protected, Virtual
//  Description: Given a field of view in degrees and a width (or
//               height) on the film, compute the focal length of the
//               lens.  If horiz is true, this is in the horizontal
//               direction; otherwise, it is in the vertical direction
//               (some lenses behave differently in each direction).
////////////////////////////////////////////////////////////////////
float Lens::
fov_to_focal_length(float, float, bool) const {
  return 1.0f;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::film_to_fov
//       Access: Protected, Virtual
//  Description: Given a width (or height) on the film and a focal
//               length, compute the field of view in degrees.  If
//               horiz is true, this is in the horizontal direction;
//               otherwise, it is in the vertical direction (some
//               lenses behave differently in each direction).
////////////////////////////////////////////////////////////////////
float Lens::
film_to_fov(float, float, bool) const {
  return default_fov;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::resequence_fov_triad
//       Access: Private, Static
//  Description: Called whenever the user changes one of the three FOV
//               parameters: fov, focal length, or film size.  This
//               rearranges the three sequence numbers so the newest
//               parameter has value 2, and the older parameters are
//               kept in sequence order.
//
//               This is used to determine which two parameters of the
//               three are the most recently changed, and conversely,
//               which one the user has *not* changed recently.  It is
//               this third value which should be discarded.
////////////////////////////////////////////////////////////////////
void Lens::
resequence_fov_triad(char &newest, char &older_a, char &older_b) {
  nassertv(newest + older_a + older_b == 3);
  switch (newest) {
  case 0:
    newest = 2;
    older_a--;
    older_b--;
    nassertv(older_a + older_b == 1);
    return;

  case 1:
    newest = 2;
    if (older_a == 2) {
      nassertv(older_b == 0);
      older_a = 1;
    } else {
      nassertv(older_a == 0 && older_b == 2);
      older_b = 1;
    }
    return;

  case 2:
    nassertv(older_a + older_b == 1);
    return;

  default:
    gobj_cat.error()
      << "Invalid fov sequence numbers in lens: " << newest << ", " << older_a
      << ", " << older_b << "\n";
    nassertv(false);
    return;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::define_geom_data
//       Access: Private
//  Description: Adjusts (or defines for the first time) all the
//               vertices in the _geom_data to match the properties of
//               the lens.  This will update the visual representation
//               of the lens's frustum to match the changing
//               parameters.  Returns the number of line segments per
//               edge.
////////////////////////////////////////////////////////////////////
int Lens::
define_geom_data() {
  int num_segments = 1;
  if (!is_linear()) {
    num_segments = 10;
  }
  
  if (_geom_data == (GeomVertexData *)NULL) {
    _geom_data = new GeomVertexData
      ("lens", GeomVertexFormat::get_v3(),
       Geom::UH_dynamic);
  }

  GeomVertexWriter vertex(_geom_data, InternalName::get_vertex());
  LPoint3f near_point, far_point;
  for (int si = 0; si < num_segments; si++) {
    float t = 2.0f * (float)si / (float)num_segments;

    // Upper left, top edge.
    LPoint2f p1(-1.0f + t, 1.0f);
    if (!extrude(p1, near_point, far_point)) {
      // Hey, this point is off the lens!  Can't do a frustum.
      return 0;
    }
    vertex.add_data3f(near_point);
    vertex.add_data3f(far_point);

    // Upper right, right edge.
    LPoint2f p2(1.0f, 1.0f - t);
    if (!extrude(p2, near_point, far_point)) {
      // Hey, this point is off the lens!  Can't do a frustum.
      return 0;
    }
    vertex.add_data3f(near_point);
    vertex.add_data3f(far_point);

    // Lower right, bottom edge.
    LPoint2f p3(1.0f - t, -1.0f);
    if (!extrude(p3, near_point, far_point)) {
      // Hey, this point is off the lens!  Can't do a frustum.
      return 0;
    }
    vertex.add_data3f(near_point);
    vertex.add_data3f(far_point);

    // Lower left, left edge.
    LPoint2f p4(-1.0f, -1.0f + t);
    if (!extrude(p4, near_point, far_point)) {
      // Hey, this point is off the lens!  Can't do a frustum.
      return 0;
    }
    vertex.add_data3f(near_point);
    vertex.add_data3f(far_point);
  }

  // Finally, add one more pair for the viewing axis.
  LPoint3f near_axis = LPoint3f::origin(_cs) + LVector3f::forward(_cs) * _near_distance;
  LPoint3f far_axis = LPoint3f::origin(_cs) + LVector3f::forward(_cs) * _far_distance;
  const LMatrix4f &lens_mat = get_lens_mat();
  near_axis = near_axis * lens_mat;
  far_axis = far_axis * lens_mat;
  vertex.add_data3f(near_axis);
  vertex.add_data3f(far_axis);

  return num_segments;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::build_shear_mat
//       Access: Private, Static
//  Description: A support function for set_frustum_from_corners(),
//               this computes a matrix that will shear the four
//               indicated points to the most nearly rectangular.
////////////////////////////////////////////////////////////////////
void Lens::
build_shear_mat(LMatrix4f &shear_mat,
                const LPoint3f &cul, const LPoint3f &cur,
                const LPoint3f &cll, const LPoint3f &clr) {
  // Fit a parallelogram around these four points.

  // Put the points in an array so we can rotate it around to find
  // the longest edge.
  LPoint3f points[4] = {
    cul, cur, clr, cll
  };

  float max_edge_length = -1.0f;
  int base_edge = -1;
  for (int i = 0; i < 4; i++) {
    LVector3f edge = points[(i + 1) % 4] - points[i];
    float length = edge.length_squared();
    if (length > max_edge_length) {
      base_edge = i;
      max_edge_length = length;
    }
  }

  const LPoint3f &base_origin = points[base_edge];
  LVector3f base_vec = points[(base_edge + 1) % 4] - base_origin;

  float base_edge_length = csqrt(max_edge_length);

  // The longest edge is the base of our parallelogram.  The parallel
  // edge must pass through the point furthest from this edge.

  int a = (base_edge + 2) % 4;
  int b = (base_edge + 3) % 4;

  float a_dist = sqr_dist_to_line(points[a], base_origin, base_vec);
  float b_dist = sqr_dist_to_line(points[b], base_origin, base_vec);

  int far_point;
  float dist;
  if (a_dist > b_dist) {
    far_point = a;
    dist = csqrt(a_dist);
  } else {
    far_point = b;
    dist = csqrt(b_dist);
  }

  // Try to make the parallelogram as nearly rectangular as possible.
  // How suitable is a true rectangle?
  LVector3f perpendic = base_vec.cross(LVector3f(0.0f, -1.0f, 0.0f));
  perpendic.normalize();
  perpendic *= dist;
  LPoint3f parallel_origin = points[base_edge] + perpendic;

  // It follows that far_point is on the line passing through the
  // parallel edge.  Is it within the endpoints?
  LVector3f base_norm_vec = base_vec / base_edge_length;

  LVector3f far_point_delta = points[far_point] - parallel_origin;
  float far_point_pos = far_point_delta.dot(base_norm_vec);

  if (far_point_pos < 0.0f) {
    // We have to slide the parallel_origin back to include far_point.
    parallel_origin += base_norm_vec * far_point_pos;

  } else if (far_point_pos > base_edge_length) {
    // We have to slide the parallel_origin forward to include
    // far_point.
    parallel_origin += base_norm_vec * (far_point_pos - base_edge_length);
  }

  // Finally, is the other point within the parallelogram?
  float t;
  float Ox = parallel_origin[0];
  float Oy = parallel_origin[2];
  float Vx = base_vec[0];
  float Vy = base_vec[2];
  float Ax, Ay, Bx, By;

  if (far_point == a) {
    // near point is b
    LVector3f v = points[b] - base_origin;
    Ax = points[b][0];
    Ay = points[b][2];
    Bx = v[0];
    By = v[2];
  } else {
    // near point is a
    LVector3f v = points[a] - (base_origin + base_vec);
    Ax = points[a][0];
    Ay = points[a][2];
    Bx = v[0];
    By = v[2];
  }
  t = ((Ox - Ax) * By + (Ay - Oy) * Bx) / (Bx * Vy - By * Vx);

  if (t < 0.0f) {
    // We need to slide the parallel_origin back to include
    // the near point.
    parallel_origin += base_vec * t;
  } else if (t > 1.0f) {
    // We need to slide the parallel_origin forward to include the far
    // point.
    parallel_origin += base_vec * (1.0f - t);
  }

  LVector3f adjacent_norm_vec = parallel_origin - base_origin;
  adjacent_norm_vec.normalize();

  // Now we've defined a parallelogram that includes all four points,
  // and we're ready to build a shear transform.
  shear_mat = LMatrix4f::ident_mat();

  // The edges of the parallelogram become the axes.
  switch (base_edge) {
  case 0:
    // The base_origin is the upper-left corner.  X axis is base_norm_vec,
    // Z axis is -adjacent_norm_vec.
    shear_mat.set_row(0, base_norm_vec);
    shear_mat.set_row(2, -adjacent_norm_vec);
    break;

  case 1:
    // The base_origin is the upper-right corner.  X axis is
    // -adjacent_norm_vec, Z axis is -base_norm_vec.
    shear_mat.set_row(0, -adjacent_norm_vec);
    shear_mat.set_row(2, -base_norm_vec);
    break;

  case 2:
    // The base_origin is the lower-right corner.  X axis is
    // -base_norm_vec, Z axis is adjacent_norm_vec.
    shear_mat.set_row(0, -base_norm_vec);
    shear_mat.set_row(2, adjacent_norm_vec);
    break;

  case 3:
    // The base_origin is the lower-left corner.  X axis is
    // adjacent_norm_vec, Z axis is base_norm_vec.
    shear_mat.set_row(0, adjacent_norm_vec);
    shear_mat.set_row(2, base_norm_vec);
    break;
    
  default:
    nassertv(false);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::sqr_dist_to_line
//       Access: Private, Static
//  Description: A support function for build_shear_mat(), this
//               computes the minimum distance from a point to a line,
//               and returns the distance squared.
////////////////////////////////////////////////////////////////////
float Lens::
sqr_dist_to_line(const LPoint3f &point, const LPoint3f &origin, 
                 const LVector3f &vec) {
  LVector3f norm = vec;
  norm.normalize();
  LVector3f d = point - origin;
  float hyp_2 = d.length_squared();
  float leg = d.dot(norm);
  return hyp_2 - leg * leg;
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void Lens::
write_datagram(BamWriter *manager, Datagram &dg) {
  TypedWritable::write_datagram(manager, dg);

  dg.add_string(_change_event);
  dg.add_uint8((int)_cs);
  _film_size.write_datagram(dg);
  _film_offset.write_datagram(dg);
  dg.add_float32(_focal_length);
  _fov.write_datagram(dg);
  dg.add_float32(_aspect_ratio);
  dg.add_float32(_near_distance);
  dg.add_float32(_far_distance);
  dg.add_uint16(_user_flags);
}

////////////////////////////////////////////////////////////////////
//     Function: Lens::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new Lens.
////////////////////////////////////////////////////////////////////
void Lens::
fillin(DatagramIterator &scan, BamReader *manager) {
  TypedWritable::fillin(scan, manager);

  _change_event = scan.get_string();
  _cs = (CoordinateSystem)scan.get_uint8();
  _film_size.read_datagram(scan);
  _film_offset.read_datagram(scan);
  _focal_length = scan.get_float32();
  _fov.read_datagram(scan);
  _aspect_ratio = scan.get_float32();
  _near_distance = scan.get_float32();
  _far_distance = scan.get_float32();
  _user_flags = scan.get_uint16();

  _comp_flags = 0;
}
