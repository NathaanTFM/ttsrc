// Filename: shader.I
// Heavily Modified:  jyelon (Sep05)
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

////////////////////////////////////////////////////////////////////
//     Function: Shader::get_filename
//       Access: Public
//  Description: Return the Shader's filename for the given shader
//               type.
////////////////////////////////////////////////////////////////////
INLINE const Filename Shader::
get_filename(const ShaderType &type) const {
  if (_filename->_separate) {
    nassertr(type != ST_none || !_filename->_shared.empty(), "");
    switch (type) {
      case ST_vertex:
        return _filename->_vertex;
        break;
      case ST_fragment:
        return _filename->_fragment;
        break;
      case ST_geometry:
        return _filename->_geometry;
        break;
      default:
        return _filename->_shared;
    }
  } else {
    return _filename->_shared;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Shader::get_text
//       Access: Public
//  Description: Return the Shader's text for the given shader type.
////////////////////////////////////////////////////////////////////
INLINE const string Shader::
get_text(const ShaderType &type) const {
  if (_text->_separate) {
    nassertr(type != ST_none || !_text->_shared.empty(), "");
    switch (type) {
      case ST_vertex:
        return _text->_vertex;
        break;
      case ST_fragment:
        return _text->_fragment;
        break;
      case ST_geometry:
        return _text->_geometry;
        break;
      default:
        return _text->_shared;
    }
  } else {
    return _text->_shared;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Shader::get_error_flag
//       Access: Public
//  Description: Returns true if the shader contains a compile-time
//               error.  This doesn't tell you whether or not the
//               shader is supported on the current video card.
////////////////////////////////////////////////////////////////////
INLINE const bool Shader::
get_error_flag() const {
  return _error_flag;
}

////////////////////////////////////////////////////////////////////
//     Function: Shader::set_shader_utilization
//       Access: Published, Static
//  Description: Set this flag to SUT_none, SUT_basic, or
//               SUT_advanced to limit panda's automatic shader
//               generation facilities.
////////////////////////////////////////////////////////////////////
INLINE void Shader::
set_shader_utilization(ShaderUtilization sut) {
  _shader_utilization = sut;
}

////////////////////////////////////////////////////////////////////
//     Function: Shader::get_shader_utilization
//       Access: Published, Static
//  Description: This flag returns SUT_none, SUT_basic, or
//               SUT_advanced and controls the automatic generation
//               of shaders.  It is initialized from the config
//               variable of the same name, but it can be 
//               subsequently adjusted.
////////////////////////////////////////////////////////////////////
INLINE ShaderUtilization Shader::
get_shader_utilization() {
  if (_shader_utilization == SUT_UNSPECIFIED) {
    return shader_utilization;
  } else {
    return _shader_utilization;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Shader::have_shader_utilization
//       Access: Published, Static
//  Description: If true, then get_shader_utilization has been
//               set using set_shader_utilization.
//               If false, then get_shader_utilization simply
//               returns the config variable of the same name.
////////////////////////////////////////////////////////////////////
INLINE bool Shader::
have_shader_utilization() {
  return (_shader_utilization != SUT_UNSPECIFIED);
}

////////////////////////////////////////////////////////////////////
//     Function: Shader::get_language
//       Access: Published
//  Description: Returns the shader language in which this shader
//               was written.
////////////////////////////////////////////////////////////////////
INLINE const Shader::ShaderLanguage Shader::
get_language() const {
  return _language;
}

////////////////////////////////////////////////////////////////////
//  Function: Shader::ShaderCapabilities Constructor
//  Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
INLINE Shader::ShaderCaps::
ShaderCaps() {
  clear();
}

////////////////////////////////////////////////////////////////////
//  Function: Shader::ShaderCapabilities::operator ==
//  Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
INLINE bool Shader::ShaderCaps::
operator == (const ShaderCaps &other) const {
#ifdef HAVE_CG
  if ((_active_vprofile != other._active_vprofile) ||
      (_active_fprofile != other._active_fprofile) ||
      (_active_gprofile != other._active_gprofile) ||
      (_ultimate_vprofile != other._ultimate_vprofile) ||
      (_ultimate_fprofile != other._ultimate_fprofile) ||
      (_ultimate_gprofile != other._ultimate_gprofile)) {
     return false;
  }
#endif
  return true;
}
