// Filename: shaderAttrib.cxx
// Created by:  sshodhan (10Jul04)
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

#include "pandabase.h"
#include "shaderAttrib.h"
#include "graphicsStateGuardianBase.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagram.h"
#include "datagramIterator.h"

TypeHandle ShaderAttrib::_type_handle;
int ShaderAttrib::_attrib_slot;

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::make_off
//       Access: Published, Static
//  Description: Constructs a new ShaderAttrib object that disables
//               the use of shaders (it does not clear out all shader
//               data, however.)
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
make_off() {
  static CPT(RenderAttrib) _off_attrib;
  if (_off_attrib == 0) {
    ShaderAttrib *attrib = new ShaderAttrib;
    attrib->_has_shader = true;
    _off_attrib = return_new(attrib);
  }
  return _off_attrib;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::make
//       Access: Published, Static
//  Description: Constructs a new ShaderAttrib object with nothing
//               set.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
make() {
  static CPT(RenderAttrib) _null_attrib;
  if (_null_attrib == 0) {
    ShaderAttrib *attrib = new ShaderAttrib;
    _null_attrib = return_new(attrib);
  }
  return _null_attrib;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::make_default
//       Access: Published, Static
//  Description: Returns a RenderAttrib that corresponds to whatever
//               the standard default properties for render attributes
//               of this type ought to be.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
make_default() {
  return return_new(new ShaderAttrib);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader(const Shader *s, int priority) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  result->_shader = s;
  result->_shader_priority = priority;
  result->_auto_shader = false;
  result->_has_shader = true;
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_off
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_off(int priority) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  result->_shader = NULL;
  result->_shader_priority = priority;
  result->_auto_shader = false;
  result->_has_shader = true;
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_auto
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_auto(int priority) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  result->_shader = NULL;
  result->_shader_priority = priority;
  result->_auto_shader = true;
  result->_has_shader = true;
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::clear_shader
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
clear_shader() const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  result->_shader = NULL;
  result->_shader_priority = 0;
  result->_auto_shader = false;
  result->_has_shader = false;
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_flag
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_flag(int flag, bool value) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  int bit = 1<<flag;
  if (value) {
    result->_flags |= bit;
  } else {
    result->_flags &= ~bit;
  }
  result->_has_flags |= bit;
  return return_new(result);
}
  
////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::clear_flag
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
clear_flag(int flag) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  int bit = 1<<flag;
  result->_flags &= ~bit;
  result->_has_flags &= ~bit;
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(const ShaderInput *input) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  Inputs::iterator i = result->_inputs.find(input->get_name());
  if (i == result->_inputs.end()) {
    result->_inputs.insert(Inputs::value_type(input->get_name(),input));
  } else {
    i->second = input;
  }
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(InternalName *id, Texture *tex, int priority) const {
  return set_shader_input(new ShaderInput(id, tex, priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(InternalName *id, const NodePath &np, int priority) const {
  return set_shader_input(new ShaderInput(id, np, priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(InternalName *id, const LVector4f &v, int priority) const {
  return set_shader_input(new ShaderInput(id, v, priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(InternalName *id, double n1, double n2, double n3, double n4, int priority) const {
  return set_shader_input(new ShaderInput(id, LVector4f(n1,n2,n3,n4), priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(const string &id, Texture *tex, int priority) const {
  return set_shader_input(new ShaderInput(InternalName::make(id), tex, priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(const string &id, const NodePath &np, int priority) const {
  return set_shader_input(new ShaderInput(InternalName::make(id), np, priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(const string &id, const LVector4f &v, int priority) const {
  return set_shader_input(new ShaderInput(InternalName::make(id), v, priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_shader_input(const string &id, double n1, double n2, double n3, double n4, int priority) const {
  return set_shader_input(new ShaderInput(InternalName::make(id), LVector4f(n1,n2,n3,n4), priority));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::set_instance_count
//       Access: Published
//  Description: Sets the geometry instance count. Do not confuse
//               this with instanceTo, which is used for animation
//               instancing, and has nothing to do with this.
//               A value of 0 means not to use instancing at all.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
set_instance_count(int instance_count) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  result->_instance_count = instance_count;
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::clear_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
clear_shader_input(InternalName *id) const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  result->_inputs.erase(id);
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::clear_shader_input
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
clear_shader_input(const string &id) const {
  return clear_shader_input(InternalName::make(id));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::clear_all_shader_inputs
//       Access: Published
//  Description: Clears all the shader inputs on the attrib.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
clear_all_shader_inputs() const {
  ShaderAttrib *result = new ShaderAttrib(*this);
  result->_inputs.clear();
  return return_new(result);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::get_shader_input
//       Access: Published
//  Description: Returns the ShaderInput of the given name.  If
//               no such name is found, this function does not return
//               NULL --- it returns the "blank" ShaderInput.
////////////////////////////////////////////////////////////////////
const ShaderInput *ShaderAttrib::
get_shader_input(InternalName *id) const {
  Inputs::const_iterator i = _inputs.find(id);
  if (i == _inputs.end()) {
    return ShaderInput::get_blank();
  } else {
    return (*i).second;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::get_shader_input
//       Access: Published
//  Description: Returns the ShaderInput of the given name.  If
//               no such name is found, this function does not return
//               NULL --- it returns the "blank" ShaderInput.
////////////////////////////////////////////////////////////////////
const ShaderInput *ShaderAttrib::
get_shader_input(const string &id) const {
  return get_shader_input(InternalName::make(id));
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::get_shader_input_nodepath
//       Access: Published
//  Description: Returns the ShaderInput as a nodepath.  Assertion 
//               fails if there is none, or if it is not a nodepath.
////////////////////////////////////////////////////////////////////
const NodePath &ShaderAttrib::
get_shader_input_nodepath(InternalName *id) const {
  static NodePath resfail;
  Inputs::const_iterator i = _inputs.find(id);
  if (i == _inputs.end()) {
    ostringstream strm;
    strm << "Shader input " << id->get_name() << " is not present.\n";
    nassert_raise(strm.str());
    return resfail;
  } else {
    const ShaderInput *p = (*i).second;
    if (p->get_value_type() != ShaderInput::M_nodepath) {
      ostringstream strm;
      strm << "Shader input " << id->get_name() << " is not a nodepath.\n";
      nassert_raise(strm.str());
      return resfail;
    }
    return p->get_nodepath();
  }

  // Satisfy compiler.
  return resfail;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::get_shader_input_vector
//       Access: Published
//  Description: Returns the ShaderInput as a vector.  Assertion 
//               fails if there is none, or if it is not a vector.
////////////////////////////////////////////////////////////////////
const LVector4f &ShaderAttrib::
get_shader_input_vector(InternalName *id) const {
  static LVector4f resfail(0,0,0,0);
  Inputs::const_iterator i = _inputs.find(id);
  if (i == _inputs.end()) {
    ostringstream strm;
    strm << "Shader input " << id->get_name() << " is not present.\n";
    nassert_raise(strm.str());
    return resfail;
  } else {
    const ShaderInput *p = (*i).second;
    if (p->get_value_type() != ShaderInput::M_vector) {
      ostringstream strm;
      strm << "Shader input " << id->get_name() << " is not a vector.\n";
      nassert_raise(strm.str());
      return resfail;
    }
    return p->get_vector();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::get_shader_input_texture
//       Access: Published
//  Description: Returns the ShaderInput as a texture.  Assertion 
//               fails if there is none, or if it is not a texture.
////////////////////////////////////////////////////////////////////
Texture *ShaderAttrib::
get_shader_input_texture(InternalName *id) const {
  Inputs::const_iterator i = _inputs.find(id);
  if (i == _inputs.end()) {
    ostringstream strm;
    strm << "Shader input " << id->get_name() << " is not present.\n";
    nassert_raise(strm.str());
    return NULL;
  } else {
    const ShaderInput *p = (*i).second;
    if (p->get_value_type() != ShaderInput::M_texture) {
      ostringstream strm;
      strm <<  "Shader input " << id->get_name() << " is not a texture.\n";
      nassert_raise(strm.str());
      return NULL;
    }
    return p->get_texture();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::get_shader
//       Access: Published
//  Description: Returns the shader object associated with the node.
//               If get_override returns true, but get_shader 
//               returns NULL, that means that this attribute should
//               disable the shader.
////////////////////////////////////////////////////////////////////
const Shader *ShaderAttrib::
get_shader() const {
  return _shader;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::compare_to_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived ShaderAttrib
//               types to return a unique number indicating whether
//               this ShaderAttrib is equivalent to the other one.
//
//               This should return 0 if the two ShaderAttrib objects
//               are equivalent, a number less than zero if this one
//               should be sorted before the other one, and a number
//               greater than zero otherwise.
//
//               This will only be called with two ShaderAttrib
//               objects whose get_type() functions return the same.
////////////////////////////////////////////////////////////////////
int ShaderAttrib::
compare_to_impl(const RenderAttrib *other) const {
  const ShaderAttrib *that;
  DCAST_INTO_R(that, other, 0);
  
  if (this->_shader != that->_shader) {
    return (this->_shader < that->_shader) ? -1 : 1;
  }
  if (this->_shader_priority != that->_shader_priority) {
    return (this->_shader_priority < that->_shader_priority) ? -1 : 1;
  }
  if (this->_auto_shader != that->_auto_shader) {
    return (this->_auto_shader < that->_auto_shader) ? -1 : 1;
  }
  if (this->_has_shader != that->_has_shader) {
    return (this->_has_shader < that->_has_shader) ? -1 : 1;
  }
  if (this->_flags != that->_flags) {
    return (this->_flags < that->_flags) ? -1 : 1;
  }
  if (this->_has_flags != that->_has_flags) {
    return (this->_has_flags < that->_has_flags) ? -1 : 1;
  }
  if (this->_instance_count != that->_instance_count) {
    return (this->_instance_count < that->_instance_count) ? -1 : 1;
  }
  
  Inputs::const_iterator i1 = this->_inputs.begin();
  Inputs::const_iterator i2 = that->_inputs.begin();
  while ((i1 != this->_inputs.end()) && (i2 != that->_inputs.end())) {
    if (i1->second != i2->second) {
      return (i1->second < i2->second) ? -1 : 1;
    }
    ++i1;
    ++i2;
  }
  if (i1 != this->_inputs.end()) {
    return 1;
  }
  if (i2 != that->_inputs.end()) {
    return -1;
  }
  
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::compose_impl
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderAttrib::
compose_impl(const RenderAttrib *other) const {
  ShaderAttrib *attr = new ShaderAttrib(*this);
  const ShaderAttrib *over;
  DCAST_INTO_R(over, other, 0);
  // Update the shader portion.
  if (over->_has_shader) {
    if ((attr->_has_shader == false) ||
        (over->_shader_priority >= attr->_shader_priority)) {
      attr->_shader = over->_shader;
      attr->_shader_priority = over->_shader_priority;
      attr->_auto_shader = over->_auto_shader;
      attr->_has_shader = over->_has_shader;
    }
  }
  // Update the shader-data portion.
  Inputs::const_iterator iover;
  for (iover=over->_inputs.begin(); iover!=over->_inputs.end(); ++iover) {
    const InternalName *id = (*iover).first;
    const ShaderInput *dover = (*iover).second;
    Inputs::iterator iattr = attr->_inputs.find(id);
    if (iattr == attr->_inputs.end()) {
      attr->_inputs.insert(Inputs::value_type(id,dover));
    } else {
      const ShaderInput *dattr = (*iattr).second;
      if (dattr->get_priority() <= dover->get_priority()) {
        iattr->second = iover->second;
      }
    }
  }
  // Just copy the instance count.
  attr->_instance_count = over->_instance_count;
  // Update the flags.
  attr->_flags &= ~(over->_has_flags);
  attr->_flags |= over->_flags;
  attr->_has_flags |= (over->_has_flags);
  return return_new(attr);
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderAttrib::register_with_read_factory
//       Access: Public, Static
//  Description: Factory method to generate a Shader object
////////////////////////////////////////////////////////////////////
void ShaderAttrib::
register_with_read_factory() {
  // IMPLEMENT ME
}

