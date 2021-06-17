// Filename: shader.h
// Created by:  jyelon (01Sep05)
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

#ifndef SHADER_H
#define SHADER_H

#include "pandabase.h"
#include "typedReferenceCount.h"
#include "namable.h"
#include "graphicsStateGuardianBase.h"
#include "internalName.h"

#ifdef HAVE_CG
// I don't want to include the Cg header file into panda as a 
// whole.  Instead, I'll just excerpt some opaque declarations.
typedef struct _CGcontext *CGcontext;
typedef struct _CGprogram *CGprogram;
typedef struct _CGparameter *CGparameter;
#endif

////////////////////////////////////////////////////////////////////
//       Class : Shader
//      Summary: 
////////////////////////////////////////////////////////////////////

class EXPCL_PANDA_GOBJ Shader: public TypedReferenceCount {

PUBLISHED:
  
  enum ShaderLanguage {
    SL_none,
    SL_Cg,
    SL_GLSL
  };
  enum ShaderType {
    ST_none = 0,
    ST_vertex,
    ST_fragment,
    ST_geometry,
  };

  static PT(Shader) load(const Filename &file, const ShaderLanguage &lang = SL_none);
  static PT(Shader) make(const string &body, const ShaderLanguage &lang = SL_none);
  static PT(Shader) load(const ShaderLanguage &lang, const Filename &vertex, const Filename &fragment, const Filename &geometry = "");
  static PT(Shader) make(const ShaderLanguage &lang, const string &vertex, const string &fragment, const string &geometry = "");

  INLINE const Filename get_filename(const ShaderType &type = ST_none) const;
  INLINE const string   get_text(const ShaderType &type = ST_none) const;
  INLINE const bool get_error_flag() const;
  INLINE const ShaderLanguage get_language() const;

  INLINE static ShaderUtilization get_shader_utilization();
  INLINE static void set_shader_utilization(ShaderUtilization utl);
  INLINE static bool have_shader_utilization();

  void prepare(PreparedGraphicsObjects *prepared_objects);
  bool is_prepared(PreparedGraphicsObjects *prepared_objects) const;
  bool release(PreparedGraphicsObjects *prepared_objects);
  int release_all();

  ShaderContext *prepare_now(PreparedGraphicsObjects *prepared_objects, 
                             GraphicsStateGuardianBase *gsg);

public:

  enum ShaderMatInput {
    SMO_identity,

    SMO_window_size,
    SMO_pixel_size,
    SMO_texpad_x,
    SMO_texpix_x,
    
    SMO_attr_material,
    SMO_attr_color,
    SMO_attr_colorscale,
    
    SMO_alight_x,
    SMO_dlight_x,
    SMO_plight_x,
    SMO_slight_x,
    SMO_satten_x,
    SMO_texmat_x,
    SMO_plane_x,
    SMO_clipplane_x,
    
    SMO_mat_constant_x,
    SMO_vec_constant_x,
    
    SMO_world_to_view,
    SMO_view_to_world,

    SMO_model_to_view,
    SMO_view_to_model,

    SMO_apiview_to_view,
    SMO_view_to_apiview,

    SMO_clip_to_view,
    SMO_view_to_clip,

    SMO_apiclip_to_view,
    SMO_view_to_apiclip,
    
    SMO_view_x_to_view,
    SMO_view_to_view_x,

    SMO_apiview_x_to_view,
    SMO_view_to_apiview_x,

    SMO_clip_x_to_view,
    SMO_view_to_clip_x,

    SMO_apiclip_x_to_view,
    SMO_view_to_apiclip_x,
    
    SMO_INVALID
  };

  enum ShaderArgType {
    SAT_float1,
    SAT_float2,
    SAT_float3,
    SAT_float4,
    SAT_float4x4,
    SAT_sampler1d,
    SAT_sampler2d,
    SAT_sampler3d,
    SAT_samplercube,
    SAT_unknown,
  };

  enum ShaderArgDir {
    SAD_in,
    SAD_out,
    SAD_inout,
    SAD_unknown,
  };

  enum ShaderMatPiece {
    SMP_whole,
    SMP_transpose,
    SMP_row0,
    SMP_row1,
    SMP_row2,
    SMP_row3,
    SMP_col0,
    SMP_col1,
    SMP_col2,
    SMP_col3,
    SMP_row3x1,
    SMP_row3x2,
    SMP_row3x3,
  };
  
  enum ShaderStateDep {
    SSD_NONE          =  0,
    SSD_general       =  1,
    SSD_transform     =  2,
    SSD_color         =  4,
    SSD_colorscale    =  8,
    SSD_material      = 16,
    SSD_shaderinputs  = 32,
  };

  enum ShaderBug {
    SBUG_ati_draw_buffers,
  };
  
  enum ShaderMatFunc {
    SMF_compose,
    SMF_transform_dlight,
    SMF_transform_plight,
    SMF_transform_slight,
    SMF_first,
  };

  struct ShaderArgId {
    string     _name;
    ShaderType _type;
    int        _seqno;
  };
  
  struct ShaderMatSpec {
    ShaderArgId       _id;
    ShaderMatFunc     _func;
    ShaderMatInput    _part[2];
    PT(InternalName)  _arg[2];
    int               _dep[2];
    LMatrix4f         _cache[2];
    LMatrix4f         _value;
    ShaderMatPiece    _piece;
  };

  struct ShaderTexSpec {
    ShaderArgId       _id;
    PT(InternalName)  _name;
    int               _stage;
    int               _desired_type;
    PT(InternalName)  _suffix;
  };

  struct ShaderVarSpec {
    ShaderArgId       _id;
    PT(InternalName)  _name;
    int               _append_uv;
  };

  struct ShaderArgInfo {
    ShaderArgId       _id;
    ShaderArgType     _type;
    ShaderArgDir      _direction;
    bool              _varying;
    NotifyCategory   *_cat;
  };

  struct ShaderCaps {
    bool _supports_glsl;

#ifdef HAVE_CG
    int _active_vprofile;
    int _active_fprofile;
    int _active_gprofile;

    int _ultimate_vprofile;
    int _ultimate_fprofile;
    int _ultimate_gprofile;

    pset <ShaderBug> _bug_list;
#endif
    void clear();
    INLINE bool operator == (const ShaderCaps &other) const;
    INLINE ShaderCaps();
  };

  struct ShaderFile : public ReferenceCount {
    ShaderFile(string shared) { _separate = false; _shared = shared; }
    ShaderFile(string vertex, string fragment, string geometry = "") {
      _separate = true;     _vertex   = vertex;
      _fragment = fragment; _geometry = geometry;
    }
    bool _separate;
    string _shared;
    string _vertex;
    string _fragment;
    string _geometry;
  };

 private:
  // These routines help split the shader into sections,
  // for those shader implementations that need to do so.
  // Don't use them when you use separate shader programs.
  void parse_init();
  void parse_line(string &result, bool rt, bool lt);
  void parse_upto(string &result, string pattern, bool include);
  void parse_rest(string &result);
  bool parse_eof();
  
  void cp_report_error(ShaderArgInfo &arg, const string &msg);
  bool cp_errchk_parameter_words(ShaderArgInfo &arg, int len);
  bool cp_errchk_parameter_in(ShaderArgInfo &arg);
  bool cp_errchk_parameter_varying(ShaderArgInfo &arg);
  bool cp_errchk_parameter_uniform(ShaderArgInfo &arg);
  bool cp_errchk_parameter_float(ShaderArgInfo &arg, int lo, int hi);
  bool cp_errchk_parameter_sampler(ShaderArgInfo &arg);
  bool cp_parse_eol(ShaderArgInfo &arg,
                    vector_string &pieces, int &next);
  bool cp_parse_delimiter(ShaderArgInfo &arg, 
                          vector_string &pieces, int &next);
  string cp_parse_non_delimiter(vector_string &pieces, int &next);
  bool cp_parse_coord_sys(ShaderArgInfo &arg,
                          vector_string &pieces, int &next,
                          ShaderMatSpec &spec, bool fromflag);
  int cp_dependency(ShaderMatInput inp);
  void cp_optimize_mat_spec(ShaderMatSpec &spec);
  
  bool compile_parameter(const ShaderArgId    &arg_id,
                         ShaderArgType         arg_type,
                         ShaderArgDir          arg_direction,
                         bool                  arg_varying,
                         NotifyCategory       *arg_cat);

  void clear_parameters();

 public:
  pvector<int> _glsl_parameter_map;

#ifdef HAVE_CG
 private:
  ShaderArgType cg_parameter_type(CGparameter p);
  ShaderArgDir  cg_parameter_dir(CGparameter p);

  CGprogram     cg_compile_entry_point(const char *entry, const ShaderCaps &caps, ShaderType type = ST_vertex);

  bool          cg_analyze_entry_point(CGprogram prog, ShaderType type);

  bool          cg_analyze_shader(const ShaderCaps &caps);
  bool          cg_compile_shader(const ShaderCaps &caps);
  void          cg_release_resources();
  void          cg_report_errors();
  
  // Determines the appropriate cg profile settings and stores them in the active shader caps
  // based on any profile settings stored in the shader's header
  void          cg_get_profile_from_header(ShaderCaps& caps);

  ShaderCaps _cg_last_caps;
  CGcontext  _cg_context;
  CGprogram  _cg_vprogram;
  CGprogram  _cg_fprogram;
  CGprogram  _cg_gprogram;

  int        _cg_vprofile;
  int        _cg_fprofile;
  int        _cg_gprofile;

  CGprogram     cg_program_from_shadertype(ShaderType type);

 public:

  bool          cg_compile_for(const ShaderCaps &caps,
                               CGcontext &ctx,
                               CGprogram &vprogram,
                               CGprogram &fprogram,
                               CGprogram &gprogram,
                               pvector<CGparameter> &map);
  
#endif

 public:
  pvector <ShaderMatSpec> _mat_spec;
  pvector <ShaderTexSpec> _tex_spec;
  pvector <ShaderVarSpec> _var_spec;
  
  bool           _error_flag;
  CPT(ShaderFile) _text;

 protected:
  CPT(ShaderFile)_filename;
  int            _parse;
  bool           _loaded;
  ShaderLanguage _language;
  
  static ShaderCaps _default_caps;
  static ShaderUtilization _shader_utilization;
  static int _shaders_generated;

  typedef pmap < CPT(ShaderFile), Shader * > ShaderTable;

  static ShaderTable _load_table;
  static ShaderTable _make_table;

  friend class ShaderContext;
  friend class PreparedGraphicsObjects;

  typedef pmap <PreparedGraphicsObjects *, ShaderContext *> Contexts;
  Contexts _contexts;

 private:  
  void clear_prepared(PreparedGraphicsObjects *prepared_objects);

 public:
  Shader(CPT(ShaderFile) name, CPT(ShaderFile) text, const ShaderLanguage &lang = SL_none);
  static void register_with_read_factory();
  
  ~Shader();
  
 public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "Shader",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

 private:
  static TypeHandle _type_handle;
};

#include "shader.I"

#endif
