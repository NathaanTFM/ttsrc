// Filename: multitexReducer.cxx
// Created by:  drose (30Nov04)
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

#include "multitexReducer.h"
#include "pandaNode.h"
#include "geomNode.h"
#include "geom.h"
#include "geomTransformer.h"
#include "accumulatedAttribs.h"
#include "sceneGraphReducer.h"
#include "renderState.h"
#include "transformState.h"
#include "graphicsOutput.h"
#include "displayRegion.h"
#include "camera.h"
#include "orthographicLens.h"
#include "cardMaker.h"
#include "colorAttrib.h"
#include "colorScaleAttrib.h"
#include "colorBlendAttrib.h"
#include "alphaTestAttrib.h"
#include "textureAttrib.h"
#include "config_grutil.h"
#include "config_gobj.h"
#include "dcast.h"
#include "geom.h"
#include "geomVertexWriter.h"
#include "geomVertexReader.h"

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
MultitexReducer::
MultitexReducer() {
  _target_stage = TextureStage::get_default();
  _use_geom = false;
  _allow_tex_mat = false;
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::Destructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
MultitexReducer::
~MultitexReducer() {
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::clear
//       Access: Published
//  Description: Removes the record of nodes that were previously
//               discovered by scan().
////////////////////////////////////////////////////////////////////
void MultitexReducer::
clear() {
  _stages.clear();
  _geom_node_list.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::scan
//       Access: Published
//  Description: Starts scanning the hierarchy beginning at the
//               indicated node.  Any GeomNodes discovered in the
//               hierarchy with multitexture will be added to internal
//               structures in the MultitexReducer so that a future
//               call to flatten() will operate on all of these at
//               once.
//
//               The indicated transform and state are the state
//               inherited from the node's ancestors; any multitexture
//               operations will be accumulated from the indicated
//               starting state.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
scan(PandaNode *node, const RenderState *state, const TransformState *transform) {
  if (grutil_cat.is_debug()) {
    grutil_cat.debug()
      << "scan(" << *node << ", " << *state << ", " << *transform << ")\n";
  }

  CPT(RenderState) next_state = state->compose(node->get_state());
  CPT(TransformState) next_transform = transform->compose(node->get_transform());

  // We must turn off any textures we come across in the scan()
  // operation, since the flattened texture will be applied to the
  // Geoms after the flatten() operation, and we don't want to still
  // have a multitexture specified.
  node->set_state(node->get_state()->remove_attrib(TextureAttrib::get_class_slot()));

  if (node->is_geom_node()) {
    scan_geom_node(DCAST(GeomNode, node), next_state, next_transform);
  }

  PandaNode::Children cr = node->get_children();
  int num_children = cr.get_num_children();
  for (int i = 0; i < num_children; i++) {
    scan(cr.get_child(i), next_state, next_transform);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::set_target
//       Access: Published
//  Description: Specifies the target TextureStage (and InternalName)
//               that will be left on each multitexture node after the
//               flatten operation has completed.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
set_target(TextureStage *stage) {
  _target_stage = stage;
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::set_use_geom
//       Access: Published
//  Description: Indicates whether the actual geometry will be used to
//               generate the textures.  
//
//               If this is set to true, the geometry discovered by
//               scan() will be used to generate the textures, which
//               allows for the vertex and polygon colors to be made
//               part of the texture itself (and makes the M_decal
//               multitexture mode more reliable).  However, this only
//               works if the geometry does not contain multiple
//               different polygons that map to the same UV range.
//
//               If this is set to false (the default), a plain flat
//               card will be used to generate the textures, which is
//               more robust in general, but the resulting texture
//               will not include vertex colors and M_decal won't work
//               properly.
//
//               Note that in case multiple sets of texture
//               coordinates are in effect, then the additional sets
//               will always use the geometry anyway regardless of the
//               setting of this flag (but this will not affect vertex
//               color).
////////////////////////////////////////////////////////////////////
void MultitexReducer::
set_use_geom(bool use_geom) {
  _use_geom = use_geom;
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::set_allow_tex_mat
//       Access: Published
//  Description: Indicates whether the resulting texture should be
//               expected to be animated beyond its current range via
//               a texture matrix (true), or whether the current range
//               of texture coordinates will be sufficient forever
//               (false).
//
//               If this is set to true, then the entire texture image
//               must be generated, in the assumption that the user
//               may animate the texture around on the surface after
//               it has been composed.
//
//               If this is set to false (the default), then only the
//               portion of the texture image which is actually in use
//               must be generated, which may be a significant savings
//               in texture memory.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
set_allow_tex_mat(bool allow_tex_mat) {
  _allow_tex_mat = allow_tex_mat;
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::flatten
//       Access: Published
//  Description: Actually performs the reducing operations on the
//               nodes that were previously scanned.
//
//               A window that can be used to create texture buffers
//               suitable for rendering this geometry must be
//               supplied.  This specifies the particular GSG that
//               will be used to composite the textures.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
flatten(GraphicsOutput *window) {
  if (grutil_cat.is_debug()) {
    grutil_cat.debug()
      << "Beginning flatten operation\n";
    Stages::const_iterator mi;
    for (mi = _stages.begin(); mi != _stages.end(); ++mi) {
      const StageList &stage_list = (*mi).first;
      const GeomList &geom_list = (*mi).second;
      grutil_cat.debug(false)
        << "stage_list for:";
      for (GeomList::const_iterator gi = geom_list.begin();
           gi != geom_list.end(); 
           ++gi) {
        const GeomInfo &geom_info = (*gi);
        grutil_cat.debug(false)
          << " (" << geom_info._geom_node->get_name() << " g" 
          << geom_info._index << ")";
      }
      grutil_cat.debug(false) << ":\n";

      StageList::const_iterator si;
      for (si = stage_list.begin(); si != stage_list.end(); ++si) {
        const StageInfo &stage_info = (*si);
        grutil_cat.debug(false)
          << "  " << *stage_info._stage << " " << *stage_info._tex
          << " " << *stage_info._tex_mat << "\n";
      }
    }
  }
  Stages::const_iterator mi;
  for (mi = _stages.begin(); mi != _stages.end(); ++mi) {
    const StageList &stage_list = (*mi).first;
    const GeomList &geom_list = (*mi).second;

    //determine whether this texture needs a white or transparent background
    bool use_transparent_bg = false;
    if(stage_list.size() > 0) {
      if(stage_list[0]._stage->get_mode() == TextureStage::M_decal)
        use_transparent_bg = true;
      else
        use_transparent_bg = false;
    }
    grutil_cat.debug(false) << "use transparent bg = " << use_transparent_bg << "\n";



    // Create an offscreen buffer in which to render the new texture.

    // Start by choosing a model TextureStage to determine the new
    // texture's properties.
    const StageInfo &model_stage = stage_list[choose_model_stage(stage_list)];

    Texture *model_tex = model_stage._tex;
    int aniso_degree = model_tex->get_anisotropic_degree();
    Texture::FilterType minfilter = model_tex->get_minfilter();
    Texture::FilterType magfilter = model_tex->get_magfilter();

    // What is the UV range of the model stage?
    TexCoordf min_uv, max_uv;
    determine_uv_range(min_uv, max_uv, model_stage, geom_list);

    // Maybe we only use a small portion of the texture, or maybe we
    // need to repeat the texture several times.
    LVecBase2f uv_scale;
    LVecBase2f uv_trans;
    get_uv_scale(uv_scale, uv_trans, min_uv, max_uv);

    // Also, if there is now a scale on the UV's (in conjunction with
    // whatever texture matrix might be applied on the model stage),
    // we may be able to adjust the image size accordingly, to keep
    // the pixels at about the same scale--but we have to keep it to a
    // power of 2.
    int x_size;
    int y_size;
    choose_texture_size(x_size, y_size, model_stage, uv_scale,
                        window);

    static int multitex_id = 1;
    ostringstream multitex_name_strm;
    multitex_name_strm << "multitex" << multitex_id;
    multitex_id++;

    GraphicsOutput *buffer = window->make_texture_buffer
      (multitex_name_strm.str(), x_size, y_size, NULL, false);
    buffer->set_one_shot(true);
    Texture *tex = buffer->get_texture();
    tex->set_anisotropic_degree(aniso_degree);
    tex->set_minfilter(minfilter);
    tex->set_magfilter(magfilter);

    // Set up the offscreen buffer to render 0,0 to 1,1.  This will be
    // the whole texture, but nothing outside the texture.
    DisplayRegion *dr = buffer->make_display_region();
    PT(Camera) cam_node = new Camera("multitexCam");
    PT(Lens) lens = new OrthographicLens();
    lens->set_film_size(1.0f, 1.0f);
    lens->set_film_offset(0.5f, 0.5f);
    lens->set_near_far(-1000.0f, 1000.0f);
    lens->set_view_mat(LMatrix4f(uv_scale[0], 0.0f, 0.0, 0.0f,
                                 0.0f, 1.0f, 0.0, 0.0f,
                                 0.0f, 0.0f, uv_scale[1], 0.0f,
                                 uv_trans[0], 0.0f, uv_trans[1], 1.0f));
    cam_node->set_lens(lens);

    // Create a root node for the buffer's scene graph, and set up
    // some appropriate properties for it.
    NodePath render("buffer");
    render.set_bin("unsorted", 0);
    render.set_depth_test(false);
    render.set_depth_write(false);
    render.set_two_sided(1);

    NodePath cam = render.attach_new_node(cam_node);
    dr->set_camera(cam);

    // If the geometry has vertex color and M_decal is in use, we must
    // render with use_geom in effect.  Otherwise we need not (and we
    // might prefer not to).
    bool force_use_geom = _use_geom;
    bool bake_in_color = _use_geom;
    Colorf geom_color(1.0f, 1.0f, 1.0f, 1.0f);
    
    //override the base color in the transparent pass down case.
    if(use_transparent_bg)
      geom_color = Colorf(0.0f,0.0f,0.0f,0.0f);

    if (!force_use_geom) {
      bool uses_decal = scan_decal(stage_list);
      if (uses_decal) {
        // If we have M_decal, we need to bake in the flat color
        // even if there is no vertex color.
        bake_in_color = true;
        
        /*
        int num_colors = 0;
        scan_color(geom_list, geom_color, num_colors);

        if (num_colors > 1) {
          // But if there is also vertex color, then we need to render
          // with the geometry.
          force_use_geom = true;
          }*/
      }
    }

    if (!force_use_geom) {
      // Put one plain white (or flat-colored) card in the background
      // for the first texture layer to apply onto.
      
      CardMaker cm("background");
      cm.set_frame(min_uv[0], max_uv[0], min_uv[1], max_uv[1]);
      if (bake_in_color) {
        cm.set_color(geom_color);
      }
      render.attach_new_node(cm.generate());

    } else {
      // Put a vertex-colored model of the geometry in the background
      // for the first texture layer to apply only.
      nassertv(bake_in_color);
      PT(GeomNode) geom_node = new GeomNode("background");
      transfer_geom(geom_node, NULL, geom_list, true);
      
      render.attach_new_node(geom_node);
    }

    StageList::const_iterator si;
    for (si = stage_list.begin(); si != stage_list.end(); ++si) {
      const StageInfo &stage_info = (*si);

      make_texture_layer(render, stage_info, geom_list, 
                           min_uv, max_uv, force_use_geom, use_transparent_bg);
    }

    // Now modify the geometry to apply the new texture, instead of
    // the old multitexture.
    CPT(RenderAttrib) new_ta = DCAST(TextureAttrib, TextureAttrib::make())->
      add_on_stage(_target_stage, tex);

    GeomList::const_iterator gi;
    for (gi = geom_list.begin(); gi != geom_list.end(); ++gi) {
      const GeomInfo &geom_info = (*gi);
      
      CPT(RenderState) geom_state = 
        geom_info._geom_node->get_geom_state(geom_info._index);
      int override = geom_info._geom_net_state->get_override(TextureAttrib::get_class_slot());
      geom_state = geom_state->add_attrib(new_ta, override);

      if (bake_in_color) {
        // If we have baked the color into the texture, we have to be
        // sure to disable coloring on the new fragment.
        geom_state = geom_state->add_attrib(ColorAttrib::make_flat(Colorf(1.0f, 1.0f, 1.0f, 1.0f)));

        // And we invent a ColorScaleAttrib to undo the effect of any
        // color scale we're getting from above.  This is not the same
        // thing as a ColorScaleAttrib::make_off(), since that would
        // prohibit any future changes to the color scale.
        const RenderAttrib *attrib = 
          geom_info._geom_net_state->get_attrib(ColorScaleAttrib::get_class_slot());

        if (attrib != (const RenderAttrib *)NULL) {
          geom_state = geom_state->add_attrib
            (attrib->invert_compose(ColorScaleAttrib::make_identity()));
        }
      }

      // Determine what tex matrix should be on the Geom.
      CPT(TransformState) tex_mat = TransformState::make_identity();

      const RenderAttrib *ra = geom_info._state->get_attrib(TexMatrixAttrib::get_class_slot());
      if (ra != (const RenderAttrib *)NULL) {
        // There is a texture matrix inherited from above; put an
        // inverse matrix on the Geom to compensate.
        const TexMatrixAttrib *tma = DCAST(TexMatrixAttrib, ra);
        CPT(TransformState) tex_mat = tma->get_transform(_target_stage);
      }

      tex_mat = tex_mat->compose(TransformState::make_pos_hpr_scale
                                 (LVecBase3f(uv_trans[0], uv_trans[1], 0.0f),
                                  LVecBase3f(0.0f, 0.0f, 0.0f),
                                  LVecBase3f(uv_scale[0], uv_scale[1], 1.0f)));

      if (tex_mat->is_identity()) {
        // There should be no texture matrix on the Geom.
        geom_state = geom_state->remove_attrib(TexMatrixAttrib::get_class_slot());
      } else {
        // The texture matrix should be as computed.
        CPT(RenderAttrib) new_tma = TexMatrixAttrib::make
          (_target_stage, tex_mat->invert_compose(TransformState::make_identity()));
        geom_state = geom_state->add_attrib(new_tma);
      }


      geom_info._geom_node->set_geom_state(geom_info._index, geom_state);
    }
  }

  // Now that we've copied all of the geometry and applied texture
  // matrices, flatten out those texture matrices where possible.
  GeomTransformer transformer;

  GeomNodeList::const_iterator gni;
  for (gni = _geom_node_list.begin(); gni != _geom_node_list.end(); ++gni) {
    const GeomNodeInfo &geom_node_info = (*gni);
    AccumulatedAttribs attribs;
    attribs._texture = 
      geom_node_info._state->get_attrib(TextureAttrib::get_class_slot());
    geom_node_info._geom_node->apply_attribs_to_vertices
      (attribs, SceneGraphReducer::TT_tex_matrix, transformer);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::scan_geom_node
//       Access: Private
//  Description: Adds the Geoms in the indicated GeomNode to the
//               internal database of multitexture elements.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
scan_geom_node(GeomNode *node, const RenderState *state, 
               const TransformState *transform) {
  if (grutil_cat.is_debug()) {
    grutil_cat.debug()
      << "scan_geom_node(" << *node << ", " << *state << ", "
      << *transform << ")\n";
  }

  _geom_node_list.push_back(GeomNodeInfo(state, node));

  int num_geoms = node->get_num_geoms();
  for (int gi = 0; gi < num_geoms; gi++) {
    CPT(RenderState) geom_net_state = 
      state->compose(node->get_geom_state(gi));

    if (grutil_cat.is_debug()) {
      grutil_cat.debug()
        << "geom " << gi << " net_state =\n";
      geom_net_state->write(cerr, 2);
    }

    // Get out the net TextureAttrib and TexMatrixAttrib from the state.
    const RenderAttrib *attrib;
    const TextureAttrib *ta = NULL;

    attrib = geom_net_state->get_attrib(TextureAttrib::get_class_slot());
    if (attrib != (const RenderAttrib *)NULL) {
      ta = DCAST(TextureAttrib, attrib);
    }

    if (ta == (TextureAttrib *)NULL) {
      // No texture should be on the Geom.
      CPT(RenderState) geom_state = node->get_geom_state(gi);
      geom_state = geom_state->remove_attrib(TextureAttrib::get_class_slot());
      node->set_geom_state(gi, geom_state);

    } else if (ta->get_num_on_stages() < 2) {
      // Just a single texture on the Geom; we don't really need to do
      // anything to flatten the textures, then.  But we should ensure
      // that the correct TextureAttrib is applied to the Geom.
      int override = geom_net_state->get_override(TextureAttrib::get_class_slot());
      CPT(RenderState) geom_state = node->get_geom_state(gi);
      geom_state = geom_state->add_attrib(ta, override);
      node->set_geom_state(gi, geom_state);

    } else {
      // Ok, we have multitexture.  Record the Geom.
      CPT(TexMatrixAttrib) tma = DCAST(TexMatrixAttrib, TexMatrixAttrib::make());
      attrib = geom_net_state->get_attrib(TexMatrixAttrib::get_class_slot());
      if (attrib != (const RenderAttrib *)NULL) {
        tma = DCAST(TexMatrixAttrib, attrib);
      }
      
      StageList stage_list;
      
      int num_stages = ta->get_num_on_stages();
      for (int si = 0; si < num_stages; si++) {
        TextureStage *stage = ta->get_on_stage(si);
        Texture *tex = ta->get_on_texture(stage);
        if (tex->get_x_size() != 0 && tex->get_y_size() != 0) {
          stage_list.push_back(StageInfo(stage, ta, tma));
          
        } else {
          grutil_cat.info()
            << "Ignoring invalid texture stage " << stage->get_name() << "\n";
        }
      }
      
      if (stage_list.size() >= 2) {
        record_stage_list(stage_list, GeomInfo(state, geom_net_state, node, gi));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::record_stage_list
//       Access: Private
//  Description: Adds the record of this one Geom and its associated
//               StageList.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
record_stage_list(const MultitexReducer::StageList &stage_list, 
                  const MultitexReducer::GeomInfo &geom_info) {
  if (grutil_cat.is_debug()) {
    grutil_cat.debug()
      << "record_stage_list for " << geom_info._geom_node->get_name() << " g" 
      << geom_info._index << ":\n";
    StageList::const_iterator si;
    for (si = stage_list.begin(); si != stage_list.end(); ++si) {
      const StageInfo &stage_info = (*si);
      grutil_cat.debug(false)
        << "  " << *stage_info._stage << " " << *stage_info._tex
        << " " << *stage_info._tex_mat << "\n";
    }
  }

  _stages[stage_list].push_back(geom_info);
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::choose_model_stage
//       Access: Private
//  Description: Chooses one of the TextureStages in the stage_list to
//               serve as the model to determine the size and
//               properties of the resulting texture.
////////////////////////////////////////////////////////////////////
size_t MultitexReducer::
choose_model_stage(const MultitexReducer::StageList &stage_list) const {
  for (size_t si = 0; si < stage_list.size(); si++) {
    const StageInfo &stage_info = stage_list[si];
    if (stage_info._stage == _target_stage) {
      // If we find the target stage, use that.
      return si;
    }
  }

  // If none of the stages are the target stage, use the bottom image.
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::determine_uv_range
//       Access: Private
//  Description: Determines what the effective UV range for the
//               indicated texture is across its geoms.  Returns true
//               if any UV's are found, false otherwise.
////////////////////////////////////////////////////////////////////
bool MultitexReducer::
determine_uv_range(TexCoordf &min_uv, TexCoordf &max_uv,
                   const MultitexReducer::StageInfo &model_stage,
                   const MultitexReducer::GeomList &geom_list) const {
  const InternalName *model_name = model_stage._stage->get_texcoord_name();
  bool got_any = false;

  GeomList::const_iterator gi;
  for (gi = geom_list.begin(); gi != geom_list.end(); ++gi) {
    const GeomInfo &geom_info = (*gi);
    
    PT(Geom) geom = 
      geom_info._geom_node->get_geom(geom_info._index)->make_copy();

    CPT(GeomVertexData) vdata = geom->get_vertex_data();
    CPT(GeomVertexFormat) format = vdata->get_format();
    if (format->has_column(model_name)) {
      GeomVertexReader texcoord(vdata, model_name);

      if (!texcoord.is_at_end()) {
        const LVecBase2f &uv = texcoord.get_data2f();
        if (!got_any) {
          min_uv = max_uv = uv;
          got_any = true;
          
        } else {
          min_uv.set(min(min_uv[0], uv[0]), min(min_uv[1], uv[1]));
          max_uv.set(max(max_uv[0], uv[0]), max(max_uv[1], uv[1]));
        }

        while (!texcoord.is_at_end()) {
          const LVecBase2f &uv = texcoord.get_data2f();
          min_uv.set(min(min_uv[0], uv[0]), min(min_uv[1], uv[1]));
          max_uv.set(max(max_uv[0], uv[0]), max(max_uv[1], uv[1]));
        }
      }
    }
  }

  if (!got_any) {
    min_uv.set(0.0f, 0.0f);
    max_uv.set(1.0f, 1.0f);
  }

  return got_any;
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::get_uv_scale
//       Access: Private
//  Description: Chooses an appropriate transform to apply to all of
//               the UV's on the generated texture, based on the
//               coverage of the model stage.  If only a portion of
//               the model stage is used, we scale the UV's up to zoom
//               into that one portion; on the other hand, if the
//               texture repeats many times, we scale the UV's down to
//               to include all of the repeating image.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
get_uv_scale(LVecBase2f &uv_scale, LVecBase2f &uv_trans,
             const TexCoordf &min_uv, const TexCoordf &max_uv) const {
  if (max_uv[0] != min_uv[0]) {
    uv_scale[0] = (max_uv[0] - min_uv[0]);
  } else {
    uv_scale[0] = 1.0f;
  }

  if (max_uv[1] != min_uv[1]) {
    uv_scale[1] = (max_uv[1] - min_uv[1]);
  } else {
    uv_scale[1] = 1.0f;
  }

  uv_trans[0] = (min_uv[0] + max_uv[0]) / 2.0f - uv_scale[0] * 0.5f;
  uv_trans[1] = (min_uv[1] + max_uv[1]) / 2.0f - uv_scale[1] * 0.5f;
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::choose_texture_size
//       Access: Private
//  Description: Chooses an appropriate size to make the new texture,
//               based on the size of the original model stage's
//               texture, and the scale applied to the UV's.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
choose_texture_size(int &x_size, int &y_size, 
                    const MultitexReducer::StageInfo &model_stage, 
                    const LVecBase2f &uv_scale,
                    GraphicsOutput *window) const {
  Texture *model_tex = model_stage._tex;
  
  // Start with the same size as the model texture.
  x_size = model_tex->get_x_size();
  y_size = model_tex->get_y_size();

  // But we might be looking at just a subset of that texture (|scale| <
  // 1) or a superset of the texture (|scale| > 1).  In this case, we
  // should adjust the pixel size accordingly, although we have to
  // keep it to a power of 2.

  LVecBase3f inherited_scale = model_stage._tex_mat->get_scale();
  
  float u_scale = cabs(inherited_scale[0]) * uv_scale[0];
  if (u_scale != 0.0f) {
    while (u_scale >= 2.0f) {
      x_size *= 2;
      u_scale *= 0.5f;
    }
    while (u_scale <= 0.5f && x_size > 0) {
      x_size /= 2;
      u_scale *= 2.0f;
    }
  }

  float v_scale = cabs(inherited_scale[1]) * uv_scale[1];
  if (v_scale != 0.0f) {
    while (v_scale >= 2.0f) {
      y_size *= 2;
      v_scale *= 0.5f;
    }
    while (v_scale <= 0.5f && y_size > 0) {
      y_size /= 2;
      v_scale *= 2.0f;
    }
  }

  if (x_size == 0 || y_size == 0) {
    grutil_cat.warning()
      << "Texture size " << model_tex->get_x_size() << " " 
      << model_tex->get_y_size() << " with scale " 
      << model_stage._tex_mat->get_scale() << ", reduced to size "
      << x_size << " " << y_size << "; constraining to 1 1.\n";
    x_size = 1;
    y_size = 1;
  }

  // Constrain the x_size and y_size to the max_texture_dimension.
  if (max_texture_dimension > 0) {
    x_size = min(x_size, (int)max_texture_dimension);
    y_size = min(y_size, (int)max_texture_dimension);
  }

  // Finally, make sure the new sizes fit within the window, so we can
  // use a parasite buffer.
  int win_x_size = window->get_x_size();
  if (win_x_size != 0 && x_size > win_x_size) {
    x_size /= 2;
    while (x_size > win_x_size) {
      x_size /= 2;
    }
  }

  int win_y_size = window->get_y_size();
  if (win_y_size != 0 && y_size > win_y_size) {
    y_size /= 2;
    while (y_size > win_y_size) {
      y_size /= 2;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::make_texture_layer
//       Access: Private
//  Description: Creates geometry to render the texture into the
//               offscreen buffer using the same effects that were
//               requested by its multitexture specification.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
make_texture_layer(const NodePath &render, 
                   const MultitexReducer::StageInfo &stage_info, 
                   const MultitexReducer::GeomList &geom_list,
                   const TexCoordf &min_uv, const TexCoordf &max_uv,
                   bool force_use_geom, bool transparent_base) {
  CPT(RenderAttrib) cba;

  switch (stage_info._stage->get_mode()) {
  case TextureStage::M_normal:
  case TextureStage::M_normal_height:
  case TextureStage::M_glow:
  case TextureStage::M_gloss:
  case TextureStage::M_height:
  case TextureStage::M_selector:
    // Don't know what to do with these funny modes.  We should
    // probably raise an exception or something.  Fall through for
    // now.

  case TextureStage::M_modulate_glow:
  case TextureStage::M_modulate_gloss:
  case TextureStage::M_modulate:
    cba = ColorBlendAttrib::make
      (ColorBlendAttrib::M_add, ColorBlendAttrib::O_fbuffer_color,
       ColorBlendAttrib::O_zero);
    break;

  case TextureStage::M_decal:
    if(transparent_base) {
      cba = AlphaTestAttrib::make
        (AlphaTestAttrib::M_greater, 0.0f);
    } else {
      cba = ColorBlendAttrib::make
        (ColorBlendAttrib::M_add, ColorBlendAttrib::O_incoming_alpha,
         ColorBlendAttrib::O_one_minus_incoming_alpha);      
    }
    break;

  case TextureStage::M_blend:
    cba = ColorBlendAttrib::make
      (ColorBlendAttrib::M_add, ColorBlendAttrib::O_constant_color,
       ColorBlendAttrib::O_one_minus_incoming_color,
       stage_info._stage->get_color());
    break;

  case TextureStage::M_replace:
    cba = ColorBlendAttrib::make_off();
    break;

  case TextureStage::M_add:
    cba = ColorBlendAttrib::make
      (ColorBlendAttrib::M_add, ColorBlendAttrib::O_one,
       ColorBlendAttrib::O_one);
    break;

  case TextureStage::M_combine:
    // We only support certain modes of M_combine.
    switch (stage_info._stage->get_combine_rgb_mode()) {
    case TextureStage::CM_modulate:
      {
        TextureStage::CombineSource source0 = stage_info._stage->get_combine_rgb_source0();
        TextureStage::CombineOperand operand0 = stage_info._stage->get_combine_rgb_operand0();
        TextureStage::CombineSource source1 = stage_info._stage->get_combine_rgb_source1();
        TextureStage::CombineOperand operand1 = stage_info._stage->get_combine_rgb_operand1();
        // Since modulate doesn't care about order, let's establish
        // the convention that the lowest-numbered source 
        // operand is in slot 0 (just for purposes of comparison).
        if (source1 < source0) {
          source0 = stage_info._stage->get_combine_rgb_source1();
          operand0 = stage_info._stage->get_combine_rgb_operand1();
          source1 = stage_info._stage->get_combine_rgb_source0();
          operand1 = stage_info._stage->get_combine_rgb_operand0();
        }
        
        if (source0 == TextureStage::CS_primary_color &&
            source1 == TextureStage::CS_previous) {
          // This is just a trick to re-apply the vertex (lighting)
          // color on the top of the texture stack.  We can ignore it,
          // since the flattened texture will do this anyway.
          return;
          
        } else if (source0 == TextureStage::CS_texture &&
                   source1 == TextureStage::CS_constant) {
          // Scaling the texture by a flat color.
          cba = ColorBlendAttrib::make
            (ColorBlendAttrib::M_add, ColorBlendAttrib::O_constant_color,
             ColorBlendAttrib::O_zero, stage_info._stage->get_color());
          
        } else if (source0 == TextureStage::CS_texture &&
                   source1 == TextureStage::CS_previous) {
          // Just an ordinary modulate.
          cba = ColorBlendAttrib::make
            (ColorBlendAttrib::M_add, ColorBlendAttrib::O_fbuffer_color,
             ColorBlendAttrib::O_zero);
          
        } else {
          // Some other kind of modulate; we don't support it.
          return;
        }
      }
      break;

    default:
      // Ignore this stage; we don't support it.
      return;
    }
    break;

  case TextureStage::M_blend_color_scale:
    // TODO: make a distinction between this and M_blend.
    cba = ColorBlendAttrib::make
      (ColorBlendAttrib::M_add, ColorBlendAttrib::O_constant_color,
       ColorBlendAttrib::O_one_minus_incoming_color,
       stage_info._stage->get_color());
    break;
  }

  NodePath geom;

  if (!force_use_geom && stage_info._stage->get_texcoord_name() == _target_stage->get_texcoord_name()) {
    // If this TextureStage uses the target texcoords, we can just
    // generate a simple card the fills the entire buffer.
    CardMaker cm(stage_info._tex->get_name());
    cm.set_uv_range(min_uv, max_uv);
    cm.set_has_uvs(true);
    cm.set_frame(min_uv[0], max_uv[0], min_uv[1], max_uv[1]);
    
    geom = render.attach_new_node(cm.generate());

  } else {
    // If this TextureStage uses some other texcoords (or if use_geom
    // is true), we have to generate geometry that maps the texcoords
    // to the target space.  This will work only for very simple cases
    // where the geometry is not too extensive and doesn't repeat over
    // the same UV's.
    PT(GeomNode) geom_node = new GeomNode(stage_info._tex->get_name());
    transfer_geom(geom_node, stage_info._stage->get_texcoord_name(), 
                  geom_list, false);
    
    geom = render.attach_new_node(geom_node);

    geom.set_color(Colorf(1.0f, 1.0f, 1.0f, 1.0f));
  }

  if (!stage_info._tex_mat->is_identity()) {
    geom.set_tex_transform(TextureStage::get_default(), stage_info._tex_mat);
  }

  geom.set_texture(stage_info._tex);
  geom.node()->set_attrib(cba);
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::transfer_geom
//       Access: Private
//  Description: Copy the vertices from the indicated geom_list,
//               mapping the vertex coordinates so that the geometry
//               will render the appropriate distortion on the texture
//               to map UV's from the specified set of texture
//               coordinates to the target set.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
transfer_geom(GeomNode *geom_node, const InternalName *texcoord_name,
              const MultitexReducer::GeomList &geom_list,
              bool preserve_color) {
  Thread *current_thread = Thread::get_current_thread();
  GeomList::const_iterator gi;
  for (gi = geom_list.begin(); gi != geom_list.end(); ++gi) {
    const GeomInfo &geom_info = (*gi);
    const Geom *orig_geom = geom_info._geom_node->get_geom(geom_info._index);

    // Copy the Geom.  This actually performs just a pointer copy of
    // the original GeomVertexData and other associated structures.
    PT(Geom) geom = orig_geom->make_copy();

    // Ensure that any vertex animation has been applied.
    geom->set_vertex_data(geom->get_vertex_data(current_thread)->animate_vertices(true, current_thread));

    // Now get a modifiable pointer to the vertex data in the new
    // Geom.  This will actually perform a deep copy of the vertex
    // data.
    PT(GeomVertexData) vdata = geom->modify_vertex_data();
    vdata->set_usage_hint(Geom::UH_stream);
    
    if (vdata->has_column(_target_stage->get_texcoord_name())) {
      GeomVertexWriter vertex(vdata, InternalName::get_vertex(), current_thread);
      GeomVertexReader texcoord(vdata, _target_stage->get_texcoord_name(), current_thread);
      
      while (!texcoord.is_at_end()) {
        const LVecBase2f &tc = texcoord.get_data2f();
        vertex.set_data3f(tc[0], 0.0f, tc[1]);
      }
    }
    
    if (texcoord_name != (const InternalName *)NULL &&
        texcoord_name != InternalName::get_texcoord()) {
      // Copy the texture coordinates from the indicated name over
      // to the default name.
      const GeomVertexColumn *column = 
        vdata->get_format()->get_column(texcoord_name);
      if (column != (const GeomVertexColumn *)NULL) {
        vdata = vdata->replace_column
          (InternalName::get_texcoord(), column->get_num_components(),
           column->get_numeric_type(), column->get_contents());
        geom->set_vertex_data(vdata);
        
        GeomVertexReader from(vdata, texcoord_name, current_thread);
        GeomVertexWriter to(vdata, InternalName::get_texcoord(), current_thread);
        while (!from.is_at_end()) {
          to.add_data2f(from.get_data2f());
        }
      }
    }
    
    CPT(RenderState) geom_state = RenderState::make_empty();
    if (preserve_color) {
      // Be sure to preserve whatever colors are on the geom.
      const RenderAttrib *ca = geom_info._geom_net_state->get_attrib(ColorAttrib::get_class_slot());
      if (ca != (const RenderAttrib *)NULL) {
        geom_state = geom_state->add_attrib(ca);
      }
      const RenderAttrib *csa = geom_info._geom_net_state->get_attrib(ColorScaleAttrib::get_class_slot());
      if (csa != (const RenderAttrib *)NULL) {
        geom_state = geom_state->add_attrib(csa);
      }
    }
    
    geom_node->add_geom(geom, geom_state);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::scan_color
//       Access: Private
//  Description: Checks all the geoms in the list to see if they all
//               use flat color, or if there is per-vertex color in
//               use.
//
//               Assumption: num_colors = 0 on entry.  On exit,
//               num_colors = 1 if there is exactly one color in use,
//               or 2 if there is more than one color in use.  If
//               num_colors = 1, then geom_color is filled in with the
//               color in use.
////////////////////////////////////////////////////////////////////
void MultitexReducer::
scan_color(const MultitexReducer::GeomList &geom_list, Colorf &geom_color, 
           int &num_colors) const {
  GeomList::const_iterator gi;
  for (gi = geom_list.begin(); gi != geom_list.end() && num_colors < 2; ++gi) {
    const GeomInfo &geom_info = (*gi);
    
    Colorf flat_color;
    bool has_flat_color = false;
    bool has_vertex_color = false;

    Colorf color_scale(1.0f, 1.0f, 1.0f, 1.0f);
    const RenderAttrib *csa = geom_info._geom_net_state->get_attrib(ColorScaleAttrib::get_class_slot());
    if (csa != (const RenderAttrib *)NULL) {
      const ColorScaleAttrib *a = DCAST(ColorScaleAttrib, csa);
      if (a->has_scale()) {
        color_scale = a->get_scale();
      }
    }

    ColorAttrib::Type color_type = ColorAttrib::T_vertex;
    const RenderAttrib *ca = geom_info._geom_net_state->get_attrib(ColorAttrib::get_class_slot());
    if (ca != (const RenderAttrib *)NULL) {
      color_type = DCAST(ColorAttrib, ca)->get_color_type();
    }

    if (color_type == ColorAttrib::T_flat) {
      // This geom has a flat color attrib, which overrides the vertices.
      flat_color = DCAST(ColorAttrib, ca)->get_color();
      has_flat_color = true;

    } else if (color_type == ColorAttrib::T_vertex) {
      // This geom gets its color from its vertices.
      const Geom *geom = geom_info._geom_node->get_geom(geom_info._index);
      if (geom->get_vertex_data()->has_column(InternalName::get_color())) {
        // This geom has per-vertex color.  Assume the colors in the
        // table are actually different from each other.
        has_vertex_color = true;
      }
    }

    if (has_vertex_color) {
      num_colors = 2;

    } else if (has_flat_color) {
      flat_color.set(flat_color[0] * color_scale[0],
                     flat_color[1] * color_scale[1],
                     flat_color[2] * color_scale[2],
                     flat_color[3] * color_scale[3]);

      if (num_colors == 0) {
        num_colors = 1;
        geom_color = flat_color;

      } else if (!flat_color.almost_equal(geom_color)) {
        // Too bad; there are multiple colors.
        num_colors = 2;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::scan_decal
//       Access: Private
//  Description: Checks all the stages in the list to see if any of
//               them apply a texture via M_decal.  Returns true if
//               so, false otherwise.
////////////////////////////////////////////////////////////////////
bool MultitexReducer::
scan_decal(const MultitexReducer::StageList &stage_list) const {
  StageList::const_iterator si;
  for (si = stage_list.begin(); si != stage_list.end(); ++si) {
    const StageInfo &stage_info = (*si);

    if (stage_info._stage->get_mode() == TextureStage::M_decal) {
      return true;
    }
  }

  return false;
}


////////////////////////////////////////////////////////////////////
//     Function: MultitexReducer::StageInfo::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
MultitexReducer::StageInfo::
StageInfo(TextureStage *stage, const TextureAttrib *ta, 
          const TexMatrixAttrib *tma) :
  _stage(stage),
  _tex_mat(TransformState::make_identity())
{
  _tex = ta->get_on_texture(_stage);
  if (tma->has_stage(stage)) {
    _tex_mat = tma->get_transform(stage);
  }
}

