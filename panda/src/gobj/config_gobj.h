// Filename: config_gobj.h
// Created by:  drose (01Oct99)
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

#ifndef CONFIG_GOBJ_H
#define CONFIG_GOBJ_H

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableBool.h"
#include "configVariableInt.h"
#include "configVariableEnum.h"
#include "configVariableDouble.h"
#include "configVariableFilename.h"
#include "configVariableString.h"

NotifyCategoryDecl(gobj, EXPCL_PANDA_GOBJ, EXPTP_PANDA_GOBJ);

BEGIN_PUBLISH
enum AutoTextureScale {
  ATS_none,
  ATS_down,
  ATS_up,
  ATS_UNSPECIFIED,
};
enum ShaderUtilization {
  SUT_none,
  SUT_basic,
  SUT_advanced,
  SUT_UNSPECIFIED,
};
END_PUBLISH

EXPCL_PANDA_GOBJ ostream &operator << (ostream &out, AutoTextureScale ats);
EXPCL_PANDA_GOBJ istream &operator >> (istream &in, AutoTextureScale &ats);
EXPCL_PANDA_GOBJ ostream &operator << (ostream &out, ShaderUtilization sut);
EXPCL_PANDA_GOBJ istream &operator >> (istream &in, ShaderUtilization &sut);

// Configure variables for gobj package.
extern EXPCL_PANDA_GOBJ ConfigVariableInt max_texture_dimension;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble texture_scale;
extern EXPCL_PANDA_GOBJ ConfigVariableInt texture_scale_limit;
extern EXPCL_PANDA_GOBJ ConfigVariableList exclude_texture_scale;


extern EXPCL_PANDA_GOBJ ConfigVariableBool keep_texture_ram;
extern EXPCL_PANDA_GOBJ ConfigVariableBool compressed_textures;
extern EXPCL_PANDA_GOBJ ConfigVariableBool driver_compress_textures;
extern EXPCL_PANDA_GOBJ ConfigVariableBool driver_generate_mipmaps;
extern EXPCL_PANDA_GOBJ ConfigVariableBool vertex_buffers;
extern EXPCL_PANDA_GOBJ ConfigVariableBool vertex_arrays;
extern EXPCL_PANDA_GOBJ ConfigVariableBool display_lists;
extern EXPCL_PANDA_GOBJ ConfigVariableBool hardware_animated_vertices;
extern EXPCL_PANDA_GOBJ ConfigVariableBool hardware_point_sprites;
extern EXPCL_PANDA_GOBJ ConfigVariableBool hardware_points;
extern EXPCL_PANDA_GOBJ ConfigVariableBool singular_points;
extern EXPCL_PANDA_GOBJ ConfigVariableBool matrix_palette;
extern EXPCL_PANDA_GOBJ ConfigVariableBool display_list_animation;
extern EXPCL_PANDA_GOBJ ConfigVariableBool connect_triangle_strips;
extern EXPCL_PANDA_GOBJ ConfigVariableBool preserve_triangle_strips;
extern EXPCL_PANDA_GOBJ ConfigVariableBool dump_generated_shaders;
extern EXPCL_PANDA_GOBJ ConfigVariableBool enforce_attrib_lock;

extern EXPCL_PANDA_GOBJ ConfigVariableEnum<AutoTextureScale> textures_power_2;
extern EXPCL_PANDA_GOBJ ConfigVariableEnum<AutoTextureScale> textures_square;
extern EXPCL_PANDA_GOBJ ConfigVariableBool textures_auto_power_2;
extern EXPCL_PANDA_GOBJ ConfigVariableBool textures_header_only;
extern EXPCL_PANDA_GOBJ ConfigVariableInt simple_image_size;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble simple_image_threshold;

extern EXPCL_PANDA_GOBJ ConfigVariableEnum<ShaderUtilization> shader_utilization;
extern EXPCL_PANDA_GOBJ ConfigVariableBool shader_auto_utilization;

extern EXPCL_PANDA_GOBJ ConfigVariableInt geom_cache_size;
extern EXPCL_PANDA_GOBJ ConfigVariableInt geom_cache_min_frames;
extern EXPCL_PANDA_GOBJ ConfigVariableInt released_vbuffer_cache_size;
extern EXPCL_PANDA_GOBJ ConfigVariableInt released_ibuffer_cache_size;

extern EXPCL_PANDA_GOBJ ConfigVariableDouble default_near;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble default_far;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble default_fov;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble default_iod;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble default_converge;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble default_keystone;

extern EXPCL_PANDA_GOBJ ConfigVariableFilename vertex_save_file_directory;
extern EXPCL_PANDA_GOBJ ConfigVariableString vertex_save_file_prefix;
extern EXPCL_PANDA_GOBJ ConfigVariableInt vertex_data_small_size;
extern EXPCL_PANDA_GOBJ ConfigVariableInt vertex_data_page_threads;
extern EXPCL_PANDA_GOBJ ConfigVariableInt graphics_memory_limit;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble adaptive_lru_weight;
extern EXPCL_PANDA_GOBJ ConfigVariableInt adaptive_lru_max_updates_per_frame;
extern EXPCL_PANDA_GOBJ ConfigVariableDouble async_load_delay;

#endif


