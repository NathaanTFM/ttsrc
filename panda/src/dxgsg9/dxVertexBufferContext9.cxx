// Filename: dxVertexBufferContext9.cxx
// Created by:  drose (18Mar05)
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

#include "dxVertexBufferContext9.h"
#include "geomVertexArrayData.h"
#include "geomVertexArrayFormat.h"
#include "graphicsStateGuardian.h"
#include "pStatTimer.h"
#include "internalName.h"
#include "config_dxgsg9.h"

#define DEBUG_VERTEX_BUFFER false

TypeHandle DXVertexBufferContext9::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: DXVertexBufferContext9::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
DXVertexBufferContext9::
DXVertexBufferContext9(PreparedGraphicsObjects *pgo, GeomVertexArrayData *data, DXScreenData &scrn) :
  VertexBufferContext(pgo, data),
  _vbuffer(NULL)
{
  // Now fill in the FVF code.
  const GeomVertexArrayFormat *array_format = data->get_array_format();

  // We have to start with the vertex data, and work up from there in
  // order, since that's the way the FVF is defined.
  int index;
  int n = 0;
  int num_columns = array_format->get_num_columns();

  _vertex_element_type_array = 0;

  VERTEX_ELEMENT_TYPE *vertex_element_type_array;

//  if (scrn._dxgsg9 -> _current_shader_context)
  {
    int total_elements;
    unsigned char vertex_element_type_counter_array [VS_TOTAL_TYPES];
    VERTEX_ELEMENT_TYPE *vertex_element_type;

    total_elements = num_columns + 2;
    vertex_element_type_array = new VERTEX_ELEMENT_TYPE [total_elements];
    memset (vertex_element_type_array, 0, total_elements * sizeof (VERTEX_ELEMENT_TYPE));
    memset (vertex_element_type_counter_array, 0, sizeof (vertex_element_type_counter_array));

    // create a simple vertex type mapping from the vertex elements
    vertex_element_type = vertex_element_type_array;
    for (index = 0; index < num_columns; index++)
    {
      int num_values;
      const InternalName *name;

      name = array_format -> get_column (index) -> get_name ( );
      num_values = array_format -> get_column(index) -> get_num_values ( );

      if (false) {

      } else if (name -> get_top ( ) == InternalName::get_vertex ( )) {

        switch (num_values)
        {
          case 3:
            vertex_element_type -> vs_input_type = VS_POSITION_XYZ;
            break;
          case 4:
            vertex_element_type -> vs_input_type = VS_POSITION_XYZW;
            break;
          default:
            dxgsg9_cat.warning ( ) << "VERTEX ERROR: invalid number of position coordinate elements " << num_values << "\n";
            break;
        }

      } else if (name -> get_top ( ) == InternalName::get_texcoord ( )) {

        switch (num_values)
        {
          case 1:
            vertex_element_type -> vs_input_type = VS_TEXTURE_U;
            break;
          case 2:
            vertex_element_type -> vs_input_type = VS_TEXTURE_UV;
            break;
          case 3:
            vertex_element_type -> vs_input_type = VS_TEXTURE_UVW;
            break;
          default:
            dxgsg9_cat.warning ( ) << "VERTEX ERROR: invalid number of vertex texture coordinate elements " << num_values << "\n";
            break;
        }

      } else if (name -> get_top ( ) == InternalName::get_normal ( )) {

        vertex_element_type -> vs_input_type = VS_NORMAL;

      } else if (name -> get_top ( ) == InternalName::get_binormal ( )) {

        vertex_element_type -> vs_input_type = VS_BINORMAL;

      } else if (name -> get_top ( ) == InternalName::get_tangent ( )) {

        vertex_element_type -> vs_input_type = VS_TANGENT;

      } else if (name -> get_top ( ) == InternalName::get_color ( )) {

        vertex_element_type -> vs_input_type = VS_DIFFUSE;

      } else {

        dxgsg9_cat.error ( )
          << "VERTEX ERROR: unsupported vertex element " << name -> get_name ( )
          << "\n";

        vertex_element_type -> vs_input_type = VS_ERROR;
      }

      vertex_element_type -> index = vertex_element_type_counter_array [vertex_element_type -> vs_input_type];
      vertex_element_type_counter_array [vertex_element_type -> vs_input_type]++;

      // SHADER ISSUE: STREAM INDEX ALWAYS 0 FOR VERTEX BUFFER ???
      vertex_element_type -> stream = 0;
      vertex_element_type -> offset = array_format -> get_column(index) -> get_start ( );

      vertex_element_type++;
    }
  }

  _vertex_element_type_array = vertex_element_type_array;

  _fvf = 0;
  _managed = -1;

  _direct_3d_vertex_declaration = 0;
  _shader_context = 0;

  if (n < num_columns &&
      array_format->get_column(n)->get_name() == InternalName::get_vertex()) {
    ++n;

    int num_blend_values = 0;

    if (n < num_columns &&
        array_format->get_column(n)->get_name() == InternalName::get_transform_weight()) {
      // We have hardware vertex animation.
      num_blend_values = array_format->get_column(n)->get_num_values();
      ++n;
    }

    if (n < num_columns &&
        array_format->get_column(n)->get_name() == InternalName::get_transform_index()) {
      // Furthermore, it's indexed vertex animation.
      _fvf |= D3DFVF_LASTBETA_UBYTE4;
      ++num_blend_values;
      ++n;
    }

    switch (num_blend_values) {
    case 0:
      _fvf |= D3DFVF_XYZ;
      break;

    case 1:
      _fvf |= D3DFVF_XYZB1;
      break;

    case 2:
      _fvf |= D3DFVF_XYZB2;
      break;

    case 3:
      _fvf |= D3DFVF_XYZB3;
      break;

    case 4:
      _fvf |= D3DFVF_XYZB4;
      break;

    case 5:
      _fvf |= D3DFVF_XYZB5;
      break;
    }
  }

  if (n < num_columns &&
      array_format->get_column(n)->get_name() == InternalName::get_normal()) {
    _fvf |= D3DFVF_NORMAL;
    ++n;
  }
  if (n < num_columns &&
      array_format->get_column(n)->get_name() == InternalName::get_color()) {
    _fvf |= D3DFVF_DIFFUSE;
    ++n;
  }

  // Now look for all of the texcoord names and enable them in the
  // same order they appear in the array.
  int texcoord_index = 0;
  while (n < num_columns &&
         array_format->get_column(n)->get_contents() == Geom::C_texcoord) {
    const GeomVertexColumn *column = array_format->get_column(n);
    switch (column->get_num_values()) {
    case 1:
      _fvf |= D3DFVF_TEXCOORDSIZE1(texcoord_index);
      ++n;
      break;
    case 2:
      _fvf |= D3DFVF_TEXCOORDSIZE2(texcoord_index);
      ++n;
      break;
    case 3:
      _fvf |= D3DFVF_TEXCOORDSIZE3(texcoord_index);
      ++n;
      break;
    case 4:
      _fvf |= D3DFVF_TEXCOORDSIZE4(texcoord_index);
      ++n;
      break;
    }
    ++texcoord_index;
  }

  switch (texcoord_index) {
  case 0:
    break;
  case 1:
    _fvf |= D3DFVF_TEX1;
    break;
  case 2:
    _fvf |= D3DFVF_TEX2;
    break;
  case 3:
    _fvf |= D3DFVF_TEX3;
    break;
  case 4:
    _fvf |= D3DFVF_TEX4;
    break;
  case 5:
    _fvf |= D3DFVF_TEX5;
    break;
  case 6:
    _fvf |= D3DFVF_TEX6;
    break;
  case 7:
    _fvf |= D3DFVF_TEX7;
    break;
  case 8:
    _fvf |= D3DFVF_TEX8;
    break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DXVertexBufferContext9::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
DXVertexBufferContext9::
~DXVertexBufferContext9() {

  if (_vertex_element_type_array) {
    delete _vertex_element_type_array;
    _vertex_element_type_array = 0;
  }
  if (_direct_3d_vertex_declaration) {
    _direct_3d_vertex_declaration -> Release ( );
    _direct_3d_vertex_declaration = 0;
  }

  free_vbuffer ( );
}

////////////////////////////////////////////////////////////////////
//     Function: DXVertexBufferContext9::evict_lru
//       Access: Public, Virtual
//  Description: Evicts the page from the LRU.  Called internally when
//               the LRU determines that it is full.  May also be
//               called externally when necessary to explicitly evict
//               the page.
//
//               It is legal for this method to either evict the page
//               as requested, do nothing (in which case the eviction
//               will be requested again at the next epoch), or
//               requeue itself on the tail of the queue (in which
//               case the eviction will be requested again much
//               later).
////////////////////////////////////////////////////////////////////
void DXVertexBufferContext9::
evict_lru() {
  dequeue_lru();
  free_vbuffer();
  update_data_size_bytes(0);
  mark_unloaded();
}

////////////////////////////////////////////////////////////////////
//     Function: DXVertexBufferContext9::free_vbuffer
//       Access: Public
//  Description: Frees vertex buffer memory.
////////////////////////////////////////////////////////////////////
void DXVertexBufferContext9::
free_vbuffer(void) {

  if (_vbuffer != NULL) {
    if (DEBUG_VERTEX_BUFFER && dxgsg9_cat.is_debug()) {
      dxgsg9_cat.debug()
        << "deleting vertex buffer " << _vbuffer << "\n";
    }

    if (DEBUG_VERTEX_BUFFER) {
      RELEASE(_vbuffer, dxgsg9, "vertex buffer", RELEASE_ONCE);
    }
    else {
      _vbuffer -> Release ( );
    }

    _vbuffer = NULL;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DXVertexBufferContext9::allocate_vbuffer
//       Access: Public
//  Description: Allocates vertex buffer memory.
////////////////////////////////////////////////////////////////////
void DXVertexBufferContext9::
allocate_vbuffer(DXScreenData &scrn,
                 const GeomVertexArrayDataHandle *reader) {

  int data_size;
  HRESULT hr;
  DWORD usage;
  D3DPOOL pool;

  data_size = reader->get_data_size_bytes();

  _managed = scrn._managed_vertex_buffers;
  if (_managed) {
    pool = D3DPOOL_MANAGED;
    usage = D3DUSAGE_WRITEONLY;
  }
  else {
    pool = D3DPOOL_DEFAULT;
    usage = D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC;
  }

  int attempts;

  attempts = 0;
  do
  {
    hr = scrn._d3d_device->CreateVertexBuffer
        (data_size, usage, _fvf, pool, &_vbuffer, NULL);
    attempts++;
  }
  while (scrn._dxgsg9 -> check_dx_allocation (hr, data_size, attempts));

  if (FAILED(hr)) {
    dxgsg9_cat.warning()
      << "CreateVertexBuffer failed" << D3DERRORSTRING(hr);
      
    printf ("data_size %d \n", data_size);
    
    _vbuffer = NULL;
  } else {
    if (DEBUG_VERTEX_BUFFER && dxgsg9_cat.is_debug()) {
      dxgsg9_cat.debug()
        << "created vertex buffer " << _vbuffer << ": "
        << reader->get_num_rows() << " vertices "
        << *reader->get_array_format() << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DXVertexBufferContext9::create_vbuffer
//       Access: Public
//  Description: Creates a new vertex buffer (but does not upload data
//               to it).
////////////////////////////////////////////////////////////////////
void DXVertexBufferContext9::
create_vbuffer(DXScreenData &scrn,
               const GeomVertexArrayDataHandle *reader,
               string name) {
  nassertv(reader->get_object() == get_data());
  Thread *current_thread = reader->get_current_thread();

  free_vbuffer ( );

  PStatTimer timer(GraphicsStateGuardian::_create_vertex_buffer_pcollector,
                   current_thread);

  int data_size;

  data_size = reader->get_data_size_bytes();

  this -> allocate_vbuffer(scrn, reader);
}

////////////////////////////////////////////////////////////////////
//     Function: DXVertexBufferContext9::upload_data
//       Access: Public
//  Description: Copies the latest data from the client store to
//               DirectX.
////////////////////////////////////////////////////////////////////
bool DXVertexBufferContext9::
upload_data(const GeomVertexArrayDataHandle *reader, bool force) {
  nassertr(reader->get_object() == get_data(), false);
  nassertr(_vbuffer != NULL, false);
  Thread *current_thread = reader->get_current_thread();

  const unsigned char *data_pointer = reader->get_read_pointer(force);
  if (data_pointer == NULL) {
    return false;
  }
  int data_size = reader->get_data_size_bytes();

  if (dxgsg9_cat.is_spam()) {
    dxgsg9_cat.spam()
      << "copying " << data_size
      << " bytes into vertex buffer " << _vbuffer << "\n";
  }
  PStatTimer timer(GraphicsStateGuardian::_load_vertex_buffer_pcollector,
                   current_thread);

  HRESULT hr;
  BYTE *local_pointer;

  if (_managed) {
    hr = _vbuffer->Lock(0, data_size, (void **) &local_pointer, 0);
  }
  else {
    hr = _vbuffer->Lock(0, data_size, (void **) &local_pointer, D3DLOCK_DISCARD);
  }
  if (FAILED(hr)) {
    dxgsg9_cat.error()
      << "VertexBuffer::Lock failed" << D3DERRORSTRING(hr);
    return false;
  }

  GraphicsStateGuardian::_data_transferred_pcollector.add_level(data_size);
  memcpy(local_pointer, data_pointer, data_size);

  _vbuffer->Unlock();
  return true;
}
