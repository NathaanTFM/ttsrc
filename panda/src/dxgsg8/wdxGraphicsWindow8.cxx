// Filename: wdxGraphicsWindow8.cxx
// Created by:  mike (09Jan00)
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

#include "wdxGraphicsPipe8.h"
#include "wdxGraphicsWindow8.h"
#include "config_dxgsg8.h"
#include "config_display.h"
#include "keyboardButton.h"
#include "mouseButton.h"
#include "throw_event.h"
#include "pStatTimer.h"
#include "pmap.h"
#include <ddraw.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <tchar.h>

TypeHandle wdxGraphicsWindow8::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
wdxGraphicsWindow8::
wdxGraphicsWindow8(GraphicsEngine *engine, GraphicsPipe *pipe,
                   const string &name,
                   const FrameBufferProperties &fb_prop,
                   const WindowProperties &win_prop,
                   int flags,
                   GraphicsStateGuardian *gsg,
                   GraphicsOutput *host):
  WinGraphicsWindow(engine, pipe, name, fb_prop, win_prop, flags, gsg, host)
{
  // dont actually create the window in the constructor.  reason:
  // multi-threading requires panda C++ window object to exist in
  // separate thread from actual API window

  _dxgsg = DCAST(DXGraphicsStateGuardian8, gsg);
  _depth_buffer_bpp = 0;
  _awaiting_restore = false;
  ZeroMemory(&_wcontext, sizeof(_wcontext));
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
wdxGraphicsWindow8::
~wdxGraphicsWindow8() {
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::begin_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               before beginning rendering for a given frame.  It
//               should do whatever setup is required, and return true
//               if the frame should be rendered, or false if it
//               should be skipped.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
begin_frame(FrameMode mode, Thread *current_thread) {
  begin_frame_spam(mode);
  if (_gsg == (GraphicsStateGuardian *)NULL) {
    return false;
  }
  
  if (_awaiting_restore) {
    // The fullscreen window was recently restored; we can't continue
    // until the GSG says we can.
    if (!_dxgsg->check_cooperative_level()) {
      // Keep waiting.
      return false;
    }
    _awaiting_restore = false;
    init_resized_window();
  }

  make_current();

  if (mode == FM_render) {
    clear_cube_map_selection();
  }

  _gsg->set_current_properties(&get_fb_properties());
  bool return_val = _gsg->begin_frame(current_thread);
  _dxgsg->set_render_target();
  return return_val;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::end_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               after rendering is completed for a given frame.  It
//               should do whatever finalization is required.
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
end_frame(FrameMode mode, Thread *current_thread) {

  end_frame_spam(mode);
  nassertv(_gsg != (GraphicsStateGuardian *)NULL);

  if (mode == FM_render) {
    copy_to_textures();
  }

  _gsg->end_frame(current_thread);

  if (mode == FM_render) {
    trigger_flip();
    if (_one_shot) {
      prepare_for_deletion();
    }
    clear_cube_map_selection();
  }
}
////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::make_current
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
make_current() {
  PStatTimer timer(_make_current_pcollector);

  _dxgsg->set_context(&_wcontext);

  // Now that we have made the context current to a window, we can
  // reset the GSG state if this is the first time it has been used.
  // (We can't just call reset() when we construct the GSG, because
  // reset() requires having a current context.)
  _dxgsg->reset_if_new();
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::end_flip
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               after begin_flip() has been called on all windows, to
//               finish the exchange of the front and back buffers.
//
//               This should cause the window to wait for the flip, if
//               necessary.
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
end_flip() {
  if (_dxgsg != (DXGraphicsStateGuardian8 *)NULL && is_active()) {
    _dxgsg->show_frame();
  }
  GraphicsWindow::end_flip();
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::verify_window_sizes
//       Access: Public, Virtual
//  Description: Determines which of the indicated window sizes are
//               supported by available hardware (e.g. in fullscreen
//               mode).
//
//               On entry, dimen is an array containing contiguous x, y
//               pairs specifying possible display sizes; it is
//               numsizes*2 words long.  The function will zero out
//               any invalid x, y size pairs.  The return value is the
//               number of valid sizes that were found.
////////////////////////////////////////////////////////////////////
int wdxGraphicsWindow8::
verify_window_sizes(int numsizes, int *dimen) {
  // unfortunately this only works AFTER you make the window
  // initially, so its really mostly useful for resizes only
  nassertr(IS_VALID_PTR(_dxgsg), 0);

  int num_valid_modes = 0;

  wdxGraphicsPipe8 *dxpipe;
  DCAST_INTO_R(dxpipe, _pipe, 0);

  // not requesting same refresh rate since changing res might not
  // support same refresh rate at new size

  int *pCurDim = dimen;

  for (int i = 0; i < numsizes; i++, pCurDim += 2) {
    int x_size = pCurDim[0];
    int y_size = pCurDim[1];

    bool bIsGoodMode = false;
    bool CouldntFindAnyValidZBuf;
    D3DFORMAT newPixFmt = D3DFMT_UNKNOWN;

    if (dxpipe->special_check_fullscreen_resolution(_wcontext, x_size, y_size)) {
      // bypass the test below for certain cards we know have valid modes
      bIsGoodMode = true;

    } else {
      if (_wcontext._is_low_memory_card) {
        bIsGoodMode = ((x_size == 640) && (y_size == 480));
      } else  {
        dxpipe->search_for_valid_displaymode
          (_wcontext, x_size, y_size, _wcontext._presentation_params.EnableAutoDepthStencil != false,
           IS_STENCIL_FORMAT(_wcontext._presentation_params.AutoDepthStencilFormat),
           &_wcontext._supported_screen_depths_mask,
           &CouldntFindAnyValidZBuf, &newPixFmt, dx_force_16bpp_zbuffer);
        bIsGoodMode = (newPixFmt != D3DFMT_UNKNOWN);
      }
    }

    if (bIsGoodMode) {
      num_valid_modes++;
    } else {
      // tell caller the mode is invalid
      pCurDim[0] = 0;
      pCurDim[1] = 0;
    }

    if (wdxdisplay8_cat.is_spam()) {
      wdxdisplay8_cat.spam()
        << "Fullscrn Mode (" << x_size << ", " << y_size << ")\t"
        << (bIsGoodMode ? "V" : "Inv") << "alid\n";
    }
  }

  return num_valid_modes;
}

//////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow::close_window
//       Access: Public
//  Description: Some cleanup is necessary for directx closeup of window.
//               Handle close window events for this particular
//               window.
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
close_window() {
  if (wdxdisplay8_cat.is_debug()) {
    wdxdisplay8_cat.debug()
      << "wdxGraphicsWindow8::close_window() " << this << "\n";
  }

  if (_gsg != (GraphicsStateGuardian*)NULL) {
    _gsg.clear();
    _active = false;
  }

  _dxgsg->release_swap_chain(&_wcontext);
  WinGraphicsWindow::close_window();
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::open_window
//       Access: Protected, Virtual
//  Description: Opens the window right now.  Called from the window
//               thread.  Returns true if the window is successfully
//               opened, or false if there was a problem.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
open_window() {
  PT(DXGraphicsDevice8) dxdev;
  WindowProperties props;

  // For now, let's make this configurable.  If this is true, then you
  // can't open multiple different windows with the same GSG, but you
  // may have more luck opening different windows with different
  // GSG's.
  static ConfigVariableBool always_discard_device("always-discard-device", true);
  bool discard_device = always_discard_device;

  // GSG creation/initialization.
  if (_gsg == 0) {
    _dxgsg = new DXGraphicsStateGuardian8(_engine, _pipe);
    _gsg = _dxgsg;
  } else {
    DCAST_INTO_R(_dxgsg, _gsg, false);
  }
  
  if (!choose_device()) {
    return false;
  }

  // Ensure the window properties get set to the actual size of the
  // window.
  {
    WindowProperties resized_props;
    resized_props.set_size(_wcontext._display_mode.Width, 
                           _wcontext._display_mode.Height);
    _properties.add_properties(resized_props);
  }

  wdxdisplay8_cat.debug() << "_wcontext._window is " << _wcontext._window << "\n";
  if (!WinGraphicsWindow::open_window()) {
    return false;
  }
  _wcontext._window = _hWnd;

  wdxdisplay8_cat.debug() << "_wcontext._window is " << _wcontext._window << "\n";

  // Here check if a device already exists. If so, then this open_window
  // call may be an extension to create multiple windows on same device
  // In that case just create an additional swapchain for this window

  while (true) {
    if (_dxgsg->get_pipe()->get_device() == NULL || discard_device) {
      wdxdisplay8_cat.debug() << "device is null or fullscreen\n";

      // If device exists, free it
      if (_dxgsg->get_pipe()->get_device()) {
        _dxgsg->dx_cleanup();
      }

      wdxdisplay8_cat.debug() << "device width " << _wcontext._display_mode.Width << "\n";
      if (!create_screen_buffers_and_device(_wcontext, dx_force_16bpp_zbuffer)) {
        wdxdisplay8_cat.error() << "Unable to create window with specified parameters.\n";
        close_window();
        return false;
      }
      _dxgsg->get_pipe()->make_device((void*)(&_wcontext));
      _dxgsg->copy_pres_reset(&_wcontext);
      _dxgsg->create_swap_chain(&_wcontext);
      break;

    } else {
      // fill in the DXScreenData from dxdevice here and change the
      // reference to _window.
      wdxdisplay8_cat.debug() << "device is not null\n";

      dxdev = (DXGraphicsDevice8*)(_dxgsg->get_pipe()->get_device());
      props = get_properties();
      memcpy(&_wcontext, &dxdev->_Scrn, sizeof(DXScreenData));

      _wcontext._presentation_params.Windowed = !is_fullscreen();
      _wcontext._presentation_params.hDeviceWindow = _wcontext._window = _hWnd;
      _wcontext._presentation_params.BackBufferWidth = _wcontext._display_mode.Width = props.get_x_size();
      _wcontext._presentation_params.BackBufferHeight = _wcontext._display_mode.Height = props.get_y_size();

      wdxdisplay8_cat.debug() << "device width " << _wcontext._presentation_params.BackBufferWidth << "\n";
      if (!_dxgsg->create_swap_chain(&_wcontext)) {
        discard_device = true;
        continue; // try again
      }
      init_resized_window();
      break;
    }
  }
  wdxdisplay8_cat.debug() << "swapchain is " << _wcontext._swap_chain << "\n";
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::reset_window
//       Access: Public, Virtual
//  Description: Resets the window framebuffer right now.  Called
//               from graphicsEngine. It releases the current swap
//               chain / creates a new one. If this is the initial
//               window and swapchain is false, then it calls reset_
//               main_device to Reset the device.
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
reset_window(bool swapchain) {
  if (swapchain) {
    if (_wcontext._swap_chain) {
      _dxgsg->create_swap_chain(&_wcontext);
      wdxdisplay8_cat.debug() << "created swapchain " << _wcontext._swap_chain << "\n";
    }
  }
  else {
    if (_wcontext._swap_chain) {
      _dxgsg->release_swap_chain(&_wcontext);
      wdxdisplay8_cat.debug() << "released swapchain " << _wcontext._swap_chain << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::fullscreen_restored
//       Access: Protected, Virtual
//  Description: This is a hook for derived classes to do something
//               special, if necessary, when a fullscreen window has
//               been restored after being minimized.  The given
//               WindowProperties struct will be applied to this
//               window's properties after this function returns.
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
fullscreen_restored(WindowProperties &properties) {
  // In DX8, unlike DX7, for some reason we can't immediately start
  // rendering as soon as the window is restored, even though
  // BeginScene() says we can.  Instead, we have to wait until
  // TestCooperativeLevel() lets us in.  We need to set a flag so we
  // can handle this special case in begin_frame().
  if (_dxgsg != NULL) {
    _awaiting_restore = true;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::handle_reshape
//       Access: Protected, Virtual
//  Description: Called in the window thread when the window size or
//               location is changed, this updates the properties
//               structure accordingly.
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
handle_reshape() {
  GdiFlush();
  WinGraphicsWindow::handle_reshape();

  if (_dxgsg != NULL) {
    // create the new resized rendertargets
    WindowProperties props = get_properties();
    int x_size = props.get_x_size();
    int y_size = props.get_y_size();

    if (_wcontext._presentation_params.BackBufferWidth != x_size ||
        _wcontext._presentation_params.BackBufferHeight != y_size) {
      bool resize_succeeded = reset_device_resize_window(x_size, y_size);
      
      if (wdxdisplay8_cat.is_debug()) {
        if (!resize_succeeded) {
          wdxdisplay8_cat.debug()
            << "windowed_resize to size: (" << x_size << ", " << y_size
            << ") failed due to out-of-memory\n";
        } else {
          int x_origin = props.get_x_origin();
          int y_origin = props.get_y_origin();
          wdxdisplay8_cat.debug()
            << "windowed_resize to origin: (" << x_origin << ", "
            << y_origin << "), size: (" << x_size
            << ", " << y_size << ")\n";
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::do_fullscreen_resize
//       Access: Protected, Virtual
//  Description: Called in the window thread to resize a fullscreen
//               window.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
do_fullscreen_resize(int x_size, int y_size) {
  if (!WinGraphicsWindow::do_fullscreen_resize(x_size, y_size)) {
    return false;
  }

  bool bCouldntFindValidZBuf;
  D3DFORMAT pixFmt;
  bool bNeedZBuffer = (_wcontext._presentation_params.EnableAutoDepthStencil != false);
  bool bNeedStencilBuffer = IS_STENCIL_FORMAT(_wcontext._presentation_params.AutoDepthStencilFormat);

  wdxGraphicsPipe8 *dxpipe;
  DCAST_INTO_R(dxpipe, _pipe, false);

  bool bIsGoodMode = false;
  bool bResizeSucceeded = false;

  if (!dxpipe->special_check_fullscreen_resolution(_wcontext, x_size, y_size)) {
    // bypass the lowvidmem test below for certain "lowmem" cards we know have valid modes

    if (_wcontext._is_low_memory_card && (!((x_size == 640) && (y_size == 480)))) {
      wdxdisplay8_cat.error() << "resize() failed: will not try to resize low vidmem device #" << _wcontext._card_id << " to non-640x480!\n";
      return bResizeSucceeded;
    }
  }

  // must ALWAYS use search_for_valid_displaymode even if we know
  // a-priori that res is valid so we can get a valid pixfmt
  dxpipe->search_for_valid_displaymode(_wcontext, x_size, y_size,
                                       bNeedZBuffer, bNeedStencilBuffer,
                                       &_wcontext._supported_screen_depths_mask,
                                       &bCouldntFindValidZBuf,
                                       &pixFmt, dx_force_16bpp_zbuffer);
  bIsGoodMode = (pixFmt != D3DFMT_UNKNOWN);

  if (!bIsGoodMode) {
    wdxdisplay8_cat.error() << "resize() failed: "
                            << (bCouldntFindValidZBuf ? "Couldnt find valid zbuffer format to go with FullScreen mode" : "No supported FullScreen modes")
                            << " at " << x_size << "x" << y_size << " for device #" << _wcontext._card_id << endl;
    return bResizeSucceeded;
  }

  // reset_device_resize_window handles both windowed & fullscrn,
  // so need to set new displaymode manually here
  _wcontext._display_mode.Width = x_size;
  _wcontext._display_mode.Height = y_size;
  _wcontext._display_mode.Format = pixFmt;
  _wcontext._display_mode.RefreshRate = D3DPRESENT_RATE_DEFAULT;

  _wcontext._presentation_params.BackBufferFormat = pixFmt;   // make reset_device_resize use presparams or displaymode??

  bResizeSucceeded = reset_device_resize_window(x_size, y_size);

  if (!bResizeSucceeded) {
    wdxdisplay8_cat.error() << "resize() failed with OUT-OF-MEMORY error!\n";

    if ((!IS_16BPP_DISPLAY_FORMAT(_wcontext._presentation_params.BackBufferFormat)) &&
        (_wcontext._supported_screen_depths_mask & (R5G6B5_FLAG|X1R5G5B5_FLAG))) {
      // fallback strategy, if we trying >16bpp, fallback to 16bpp buffers
      _wcontext._display_mode.Format = ((_wcontext._supported_screen_depths_mask & R5G6B5_FLAG) ? D3DFMT_R5G6B5 : D3DFMT_X1R5G5B5);
      dx_force_16bpp_zbuffer = true;
      if (wdxdisplay8_cat.info())
        wdxdisplay8_cat.info() << "CreateDevice failed with out-of-vidmem, retrying w/16bpp buffers on device #" << _wcontext._card_id << endl;

      bResizeSucceeded = reset_device_resize_window(x_size, y_size);  // create the new resized rendertargets
    }
  }

  return bResizeSucceeded;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::create_screen_buffers_and_device
//       Access: Private
//  Description: Called whenever the window is resized, this recreates
//               the necessary buffers for rendering.
//
//               Sets _depth_buffer_bpp appropriately.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
create_screen_buffers_and_device(DXScreenData &display, bool force_16bpp_zbuffer) {
  wdxGraphicsPipe8 *dxpipe;
  DCAST_INTO_R(dxpipe, _pipe, false);

  DWORD dwRenderWidth = display._display_mode.Width;
  DWORD dwRenderHeight = display._display_mode.Height;
  DWORD dwBehaviorFlags = 0x0;
  LPDIRECT3D8 _d3d8 = display._d3d8;
  D3DCAPS8 *pD3DCaps = &display._d3dcaps;
  D3DPRESENT_PARAMETERS* presentation_params = &display._presentation_params;
  RECT view_rect;
  HRESULT hr;

  wdxdisplay8_cat.debug() << "Display Width " << dwRenderWidth << " and PresParam Width " << _wcontext._presentation_params.BackBufferWidth << "\n";

  bool bWantStencil = (_fb_properties.get_stencil_bits() > 0);

  PRINT_REFCNT(wdxdisplay8, _d3d8);

  nassertr(_d3d8 != NULL, false);
  nassertr(pD3DCaps->DevCaps & D3DDEVCAPS_HWRASTERIZATION, false);

  presentation_params->BackBufferFormat = display._display_mode.Format;  // dont need dest alpha, so just use adapter format
  cerr << "attempting " << D3DFormatStr(presentation_params->BackBufferFormat) << "\n";

  bool do_sync = sync_video;

  if (do_sync && !(pD3DCaps->Caps & D3DCAPS_READ_SCANLINE)) {
    wdxdisplay8_cat.info()
      << "HW doesnt support syncing to vertical refresh, ignoring sync-video\n";
    do_sync = false;
  }

  // verify the rendertarget fmt one last time
  if (FAILED(_d3d8->CheckDeviceFormat(display._card_id, D3DDEVTYPE_HAL, display._display_mode.Format, D3DUSAGE_RENDERTARGET,
                                      D3DRTYPE_SURFACE, presentation_params->BackBufferFormat))) {
    wdxdisplay8_cat.error() << "device #" << display._card_id << " CheckDeviceFmt failed for surface fmt " << D3DFormatStr(presentation_params->BackBufferFormat) << endl;
    goto Fallback_to_16bpp_buffers;
  }

  if (FAILED(_d3d8->CheckDeviceType(display._card_id, D3DDEVTYPE_HAL, display._display_mode.Format, presentation_params->BackBufferFormat,
                                    is_fullscreen()))) {
    wdxdisplay8_cat.error() << "device #" << display._card_id << " CheckDeviceType failed for surface fmt " << D3DFormatStr(presentation_params->BackBufferFormat) << endl;
    goto Fallback_to_16bpp_buffers;
  }

  if (display._presentation_params.EnableAutoDepthStencil) {
    if (!dxpipe->find_best_depth_format(display, display._display_mode,
                                        &display._presentation_params.AutoDepthStencilFormat,
                                        bWantStencil, false)) {
      wdxdisplay8_cat.error()
        << "find_best_depth_format failed in CreateScreenBuffers for device #"
        << display._card_id << endl;
      goto Fallback_to_16bpp_buffers;
    }
    _depth_buffer_bpp = D3DFMT_to_DepthBits(display._presentation_params.AutoDepthStencilFormat);
  } else {
    _depth_buffer_bpp = 0;
  }

  presentation_params->Windowed = !is_fullscreen();

  if (dx_multisample_antialiasing_level>1) {
    // need to check both rendertarget and zbuffer fmts
    hr = _d3d8->CheckDeviceMultiSampleType(display._card_id, D3DDEVTYPE_HAL, display._display_mode.Format,
                                           is_fullscreen(), D3DMULTISAMPLE_TYPE(dx_multisample_antialiasing_level.get_value()));
    if (FAILED(hr)) {
      wdxdisplay8_cat.fatal() << "device #" << display._card_id << " doesnt support multisample level " << dx_multisample_antialiasing_level << "surface fmt " << D3DFormatStr(display._display_mode.Format) << endl;
      return false;
    }

    if (display._presentation_params.EnableAutoDepthStencil) {
      hr = _d3d8->CheckDeviceMultiSampleType(display._card_id, D3DDEVTYPE_HAL, display._presentation_params.AutoDepthStencilFormat,
                                             is_fullscreen(), D3DMULTISAMPLE_TYPE(dx_multisample_antialiasing_level.get_value()));
      if (FAILED(hr)) {
        wdxdisplay8_cat.fatal() << "device #" << display._card_id << " doesnt support multisample level " << dx_multisample_antialiasing_level << "surface fmt " << D3DFormatStr(display._presentation_params.AutoDepthStencilFormat) << endl;
        return false;
      }
    }

    presentation_params->MultiSampleType = D3DMULTISAMPLE_TYPE(dx_multisample_antialiasing_level.get_value());

    if (wdxdisplay8_cat.is_info())
      wdxdisplay8_cat.info() << "device #" << display._card_id << " using multisample antialiasing level " << dx_multisample_antialiasing_level << endl;
  }

  presentation_params->BackBufferCount = 1;
  presentation_params->Flags = 0x0;
  presentation_params->hDeviceWindow = display._window;
  presentation_params->BackBufferWidth = display._display_mode.Width;
  presentation_params->BackBufferHeight = display._display_mode.Height;

  if (_wcontext._is_tnl_device) {
    dwBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    // note: we could create a pure device in this case if I
    // eliminated the GetRenderState calls in dxgsg

    // also, no software vertex processing available since I specify
    // D3DCREATE_HARDWARE_VERTEXPROCESSING and not
    // D3DCREATE_MIXED_VERTEXPROCESSING
  } else {
    dwBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  }

  if (dx_preserve_fpu_state)
    dwBehaviorFlags |= D3DCREATE_FPU_PRESERVE;

  // if window is not foreground in exclusive mode, ddraw thinks you
  // are 'not active', so it changes your WM_ACTIVATEAPP from true to
  // false, causing us to go into a 'wait-for WM_ACTIVATEAPP true'
  // loop, and the event never comes so we hang in fullscreen wait.
  // also doing this for windowed mode since it was requested.
  if (!SetForegroundWindow(display._window)) {
    wdxdisplay8_cat.warning() << "SetForegroundWindow() failed!\n";
  }

  if (is_fullscreen()) {
    // CREATE FULLSCREEN BUFFERS

    presentation_params->SwapEffect = D3DSWAPEFFECT_DISCARD;  // we dont care about preserving contents of old frame
    presentation_params->FullScreen_PresentationInterval = (do_sync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);
    presentation_params->FullScreen_RefreshRateInHz = display._display_mode.RefreshRate;

    ClearToBlack(display._window, get_properties());

    hr = _d3d8->CreateDevice(display._card_id, D3DDEVTYPE_HAL, _hWnd,
                             dwBehaviorFlags, presentation_params, &display._d3d_device);

    if (FAILED(hr)) {
      wdxdisplay8_cat.fatal() << "D3D CreateDevice failed for device #" << display._card_id << ", " << D3DERRORSTRING(hr);

      if (hr == D3DERR_OUTOFVIDEOMEMORY)
        goto Fallback_to_16bpp_buffers;
      else
        return false;
    }

    SetRect(&view_rect, 0, 0, dwRenderWidth, dwRenderHeight);

  } else {
    // CREATE WINDOWED BUFFERS

    D3DDISPLAYMODE dispmode;
    hr = display._d3d8->GetAdapterDisplayMode(display._card_id, &dispmode);

    if (FAILED(hr)) {
      wdxdisplay8_cat.fatal()
        << "GetAdapterDisplayMode failed" << D3DERRORSTRING(hr);
      return false;
    }

    if (dispmode.Format == D3DFMT_P8) {
      wdxdisplay8_cat.fatal()
        << "Can't run windowed in an 8-bit or less display mode" << endl;
      return false;
    }

    presentation_params->FullScreen_PresentationInterval = 0;

    if (dx_multisample_antialiasing_level<2) {
      if (do_sync) {
        // It turns out that COPY_VSYNC has real performance problems
        // on many nVidia cards--it syncs at some random interval,
        // possibly skipping over several video syncs.  Screw it,
        // we'll effectively disable sync-video with windowed mode
        // using DirectX8.
        //presentation_params->SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
        presentation_params->SwapEffect = D3DSWAPEFFECT_DISCARD;
      } else {
        presentation_params->SwapEffect = D3DSWAPEFFECT_DISCARD;
      }
    } else {
      presentation_params->SwapEffect = D3DSWAPEFFECT_DISCARD;
    }

    //nassertv((dwRenderWidth == presentation_params->BackBufferWidth)&&(dwRenderHeight == presentation_params->BackBufferHeight));

    hr = _d3d8->CreateDevice(display._card_id, D3DDEVTYPE_HAL, _hWnd,
                             dwBehaviorFlags, presentation_params, &display._d3d_device);

    if (FAILED(hr)) {
      wdxdisplay8_cat.warning() << "presentation_params->BackBufferWidth : " << presentation_params->BackBufferWidth << endl;
      wdxdisplay8_cat.warning() << "presentation_params->BackBufferHeight : " << presentation_params->BackBufferHeight << endl;
      wdxdisplay8_cat.warning() << "presentation_params->BackBufferFormat : " << presentation_params->BackBufferFormat << endl;
      wdxdisplay8_cat.warning() << "presentation_params->BackBufferCount : " << presentation_params->BackBufferCount << endl;
      wdxdisplay8_cat.warning() << "D3D CreateDevice failed for device #" << display._card_id << D3DERRORSTRING(hr);
      goto Fallback_to_16bpp_buffers;
    }
  }  // end create windowed buffers

  //  ========================================================

  PRINT_REFCNT(wdxdisplay8, _wcontext._d3d_device);

  if (presentation_params->EnableAutoDepthStencil) {
    int depth_bits;
    int stencil_bits;

    depth_bits = 1;
    stencil_bits = 0;
    switch (presentation_params->AutoDepthStencilFormat)
    {
      case D3DFMT_D16_LOCKABLE:
        depth_bits = 16;
        break;
      case D3DFMT_D32:
        depth_bits = 32;
        break;
      case D3DFMT_D15S1:
        depth_bits = 15;
        stencil_bits = 1;
        break;
      case D3DFMT_D24S8:
        depth_bits = 24;
        stencil_bits = 8;
        break;
      case D3DFMT_D24X8:
        depth_bits = 24;
        break;
      case D3DFMT_D24X4S4:
        depth_bits = 24;
        stencil_bits = 4;
        break;
      case D3DFMT_D16:
        depth_bits = 16;
        break;
      default:
        wdxdisplay8_cat.error() << "unknown depth stencil format  " << presentation_params->AutoDepthStencilFormat;
        break;
    }
      
    _fb_properties.set_stencil_bits(stencil_bits);
    _fb_properties.set_depth_bits(depth_bits);
  } else {
    _fb_properties.set_depth_bits(0);
    _fb_properties.set_stencil_bits(0);
  }

  init_resized_window();

  return true;

 Fallback_to_16bpp_buffers:
  if ((!IS_16BPP_DISPLAY_FORMAT(presentation_params->BackBufferFormat)) &&
      (display._supported_screen_depths_mask & (R5G6B5_FLAG|X1R5G5B5_FLAG))) {
    // fallback strategy, if we trying >16bpp, fallback to 16bpp buffers

    display._display_mode.Format = ((display._supported_screen_depths_mask & R5G6B5_FLAG) ? D3DFMT_R5G6B5 : D3DFMT_X1R5G5B5);

    if (wdxdisplay8_cat.info()) {
      wdxdisplay8_cat.info()
        << "CreateDevice failed with out-of-vidmem or invalid BackBufferFormat, retrying w/16bpp buffers on device #"
        << display._card_id << endl;
    }
    return create_screen_buffers_and_device(display, true);
    //return;

  } else if (!((dwRenderWidth == 640)&&(dwRenderHeight == 480))) {
    if (wdxdisplay8_cat.info())
      wdxdisplay8_cat.info() << "CreateDevice failed w/out-of-vidmem, retrying at 640x480 w/16bpp buffers on device #" << display._card_id << endl;
    // try final fallback to 640x480x16
    display._display_mode.Width = 640;
    display._display_mode.Height = 480;
    return create_screen_buffers_and_device(display, true);
    //return;

  } else {
    wdxdisplay8_cat.fatal()
      << "Can't create any screen buffers, bailing out.\n";
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::choose_device
//       Access: Private
//  Description: Looks at the list of available graphics adapters and
//               chooses a suitable one for the window.
//
//               Returns true if successful, false on failure.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
choose_device() {
  HRESULT hr;

  wdxGraphicsPipe8 *dxpipe;
  DCAST_INTO_R(dxpipe, _pipe, false);

  int num_adapters = dxpipe->__d3d8->GetAdapterCount();
  DXDeviceInfoVec device_infos;

  for (int i = 0; i < num_adapters; i++) {
    D3DADAPTER_IDENTIFIER8 adapter_info;
    ZeroMemory(&adapter_info, sizeof(D3DADAPTER_IDENTIFIER8));
    hr = dxpipe->__d3d8->GetAdapterIdentifier(i, D3DENUM_NO_WHQL_LEVEL, &adapter_info);
    if (FAILED(hr)) {
      wdxdisplay8_cat.fatal()
        << "D3D GetAdapterID(" << i << ") failed: "
        << D3DERRORSTRING(hr) << endl;
      continue;
    }

    LARGE_INTEGER *DrvVer = &adapter_info.DriverVersion;

    wdxdisplay8_cat.info()
      << "D3D8." << (dxpipe->__is_dx8_1 ?"1":"0") << " Adapter[" << i << "]: " << adapter_info.Description
      << ", Driver: " << adapter_info.Driver << ", DriverVersion: ("
      << HIWORD(DrvVer->HighPart) << "." << LOWORD(DrvVer->HighPart) << "."
      << HIWORD(DrvVer->LowPart) << "." << LOWORD(DrvVer->LowPart)
      << ")\nVendorID: 0x" << (void*) adapter_info.VendorId
      << " DeviceID: 0x" <<  (void*) adapter_info.DeviceId
      << " SubsysID: 0x" << (void*) adapter_info.SubSysId
      << " Revision: 0x" << (void*) adapter_info.Revision << endl;

    HMONITOR _monitor = dxpipe->__d3d8->GetAdapterMonitor(i);
    if (_monitor == NULL) {
      wdxdisplay8_cat.info()
        << "D3D8 Adapter[" << i << "]: seems to be disabled, skipping it\n";
      continue;
    }

    DXDeviceInfo devinfo;
    ZeroMemory(&devinfo, sizeof(devinfo));
    memcpy(&devinfo.guidDeviceIdentifier, &adapter_info.DeviceIdentifier,
           sizeof(GUID));
    strncpy(devinfo.szDescription, adapter_info.Description,
            MAX_DEVICE_IDENTIFIER_STRING);
    strncpy(devinfo.szDriver, adapter_info.Driver,
            MAX_DEVICE_IDENTIFIER_STRING);
    devinfo.VendorID = adapter_info.VendorId;
    devinfo.DeviceID = adapter_info.DeviceId;
    devinfo._monitor = _monitor;
    devinfo.cardID = i;

    device_infos.push_back(devinfo);
  }

  if (device_infos.empty()) {
    wdxdisplay8_cat.error()
      << "No available D3D8 devices found.\n";
    return false;
  }

  // Since some adapters may have been disabled, we should re-obtain
  // the number of available adapters.
  num_adapters = (int)device_infos.size();

  // Now choose a suitable adapter.

  int adapter_num = D3DADAPTER_DEFAULT;

  // Eventually, we should have some interface for specifying a device
  // index interactively, instead of only via Configrc.
  if (dx_preferred_device_id != -1) {
    if (dx_preferred_device_id < 0 || dx_preferred_device_id >= num_adapters) {
      wdxdisplay8_cat.error()
        << "invalid 'dx-preferred-device-id', valid values are 0-"
        << num_adapters - 1 << ", using default adapter instead.\n";
    } else {
      adapter_num = dx_preferred_device_id;
    }
  }

  UINT good_device_count = 0;
  for(UINT devnum = 0;devnum<device_infos.size() /*&& (good_device_count < num_windows)*/;devnum++) {
    if (search_for_device(dxpipe, &device_infos[devnum]))
      good_device_count++;
  }

  if (good_device_count == 0) {
    wdxdisplay8_cat.error() << "no usable display devices.\n";
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::search_for_device
//       Access: Private
//  Description: Searches for a suitable hardware device for
//               rendering.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
search_for_device(wdxGraphicsPipe8 *dxpipe, DXDeviceInfo *device_info) {

  nassertr(dxpipe != NULL, false);
  WindowProperties properties = get_properties();
  DWORD dwRenderWidth = properties.get_x_size();
  DWORD dwRenderHeight = properties.get_y_size();
  HRESULT hr;
  LPDIRECT3D8 _d3d8 = dxpipe->__d3d8;

  nassertr(_dxgsg != NULL, false);
  _wcontext._d3d8 = _d3d8;
  _wcontext._is_dx8_1 = dxpipe->__is_dx8_1;
  _wcontext._card_id = device_info->cardID;  // could this change by end?

  bool bWantStencil = (_fb_properties.get_stencil_bits() > 0);

  hr = _d3d8->GetAdapterIdentifier(device_info->cardID, D3DENUM_NO_WHQL_LEVEL,
                                   &_wcontext._dx_device_id);
  if (FAILED(hr)) {
    wdxdisplay8_cat.error()
      << "D3D GetAdapterID failed" << D3DERRORSTRING(hr);
    return false;
  }

  D3DCAPS8 _d3dcaps;
  hr = _d3d8->GetDeviceCaps(device_info->cardID, D3DDEVTYPE_HAL, &_d3dcaps);
  if (FAILED(hr)) {
    if ((hr == D3DERR_INVALIDDEVICE)||(hr == D3DERR_NOTAVAILABLE)) {
      wdxdisplay8_cat.error()
        << "No DirectX 8 D3D-capable 3D hardware detected for device # "
        << device_info->cardID << " (" << device_info->szDescription
        << ")!\n";
    } else {
      wdxdisplay8_cat.error()
        << "GetDeviceCaps failed: " << D3DERRORSTRING(hr) << endl;
    }
    return false;
  }

  //search_for_valid_displaymode needs these to be set
  memcpy(&_wcontext._d3dcaps, &_d3dcaps, sizeof(D3DCAPS8));
  _wcontext._card_id = device_info->cardID;

  _wcontext._max_available_video_memory = UNKNOWN_VIDMEM_SIZE;
  _wcontext._is_low_memory_card = false;

  // bugbug: wouldnt we like to do GetAVailVidMem so we can do
  // upper-limit memory computation for dx8 cards too?  otherwise
  // verify_window_sizes cant do much
  if (_d3dcaps.MaxStreams == 0) {
    if (wdxdisplay8_cat.is_debug()) {
      wdxdisplay8_cat.debug()
        << "checking vidmem size\n";
    }

    UINT IDnum;

    // simple linear search to match DX7 card info w/DX8 card ID
    for (IDnum = 0; IDnum < dxpipe->_card_ids.size(); IDnum++) {
      if ((device_info->VendorID == dxpipe->_card_ids[IDnum].VendorID) &&
          (device_info->DeviceID == dxpipe->_card_ids[IDnum].DeviceID) &&
          (device_info->_monitor == dxpipe->_card_ids[IDnum]._monitor))
        break;
    }

    if (IDnum < dxpipe->_card_ids.size()) {
      _wcontext._max_available_video_memory = dxpipe->_card_ids[IDnum]._max_available_video_memory;
      _wcontext._is_low_memory_card = dxpipe->_card_ids[IDnum]._is_low_memory_card;
    } else {
      wdxdisplay8_cat.error()
        << "Error: couldnt find a CardID match in DX7 info, assuming card is not a lowmem card\n";
    }
  }

  if ((bWantStencil) && (_d3dcaps.StencilCaps == 0x0)) {
    wdxdisplay8_cat.fatal()
      << "Stencil ability requested, but device #" << device_info->cardID
      << " (" << _wcontext._dx_device_id.Description
      << "), has no stencil capability!\n";
    return false;
  }

  // just because TNL is true, it doesnt mean vtx shaders are
  // supported in HW (see GF2) for this case, you probably want MIXED
  // processing to use HW for fixed-fn vertex processing and SW for
  // vtx shaders
  _wcontext._is_tnl_device =
    ((_d3dcaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0);
  _wcontext._can_use_hw_vertex_shaders =
    (_d3dcaps.VertexShaderVersion >= D3DVS_VERSION(1, 0));
  _wcontext._can_use_pixel_shaders =
    (_d3dcaps.PixelShaderVersion >= D3DPS_VERSION(1, 0));

  bool bNeedZBuffer =
    ((!(_d3dcaps.RasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR )) &&
     (_fb_properties.get_depth_bits() > 0));

  _wcontext._presentation_params.EnableAutoDepthStencil = bNeedZBuffer;

  D3DFORMAT pixFmt = D3DFMT_UNKNOWN;

  if (is_fullscreen()) {
    bool bCouldntFindValidZBuf;
    dxpipe->search_for_valid_displaymode(_wcontext, dwRenderWidth, dwRenderHeight,
                                         bNeedZBuffer, bWantStencil,
                                         &_wcontext._supported_screen_depths_mask,
                                         &bCouldntFindValidZBuf,
                                         &pixFmt, dx_force_16bpp_zbuffer);
    
    // note I'm not saving refresh rate, will just use adapter
    // default at given res for now
    
    if (pixFmt == D3DFMT_UNKNOWN) {
      wdxdisplay8_cat.error()
        << (bCouldntFindValidZBuf ? "Couldnt find valid zbuffer format to go with FullScreen mode" : "No supported FullScreen modes")
        << " at " << dwRenderWidth << "x" << dwRenderHeight << " for device #" << _wcontext._card_id << endl;
      
      // run it again in verbose mode to get more dbg info to log
      dxpipe->search_for_valid_displaymode(_wcontext, dwRenderWidth, dwRenderHeight,
                                           bNeedZBuffer, bWantStencil,
                                           &_wcontext._supported_screen_depths_mask,
                                           &bCouldntFindValidZBuf,
                                           &pixFmt, dx_force_16bpp_zbuffer, true);
      
      // if still D3DFMT_UNKNOWN return false
      if (pixFmt == D3DFMT_UNKNOWN)
        return false;
    }
  } else {
    // Windowed Mode

    D3DDISPLAYMODE dispmode;
    hr = _d3d8->GetAdapterDisplayMode(device_info->cardID, &dispmode);
    if (FAILED(hr)) {
      wdxdisplay8_cat.error()
        << "GetAdapterDisplayMode(" << device_info->cardID
        << ") failed" << D3DERRORSTRING(hr);
      return false;
    }
    pixFmt = dispmode.Format;
  }

  _wcontext._display_mode.Width = dwRenderWidth;
  _wcontext._display_mode.Height = dwRenderHeight;
  _wcontext._display_mode.Format = pixFmt;
  _wcontext._display_mode.RefreshRate = D3DPRESENT_RATE_DEFAULT;
  _wcontext._monitor = device_info->_monitor;

  if (dwRenderWidth != properties.get_x_size() ||
      dwRenderHeight != properties.get_y_size()) {
    // This is probably not the best place to put this; I'm just
    // putting it here for now because the code above might have
    // changed the size of the window unexpectedly.  This code gets
    // called when make_gsg() is called, which means it is called in
    // the draw thread, but this method should really be called from
    // the window thread.  In DirectX those may always be the same
    // threads anyway, so we may be all right.  Still, it's a little
    // strange that the window may change size after it has already
    // been opened, at the time we create the GSG for it; it would be
    // better if we could find a way to do this resolution-selection
    // logic earlier, say at the time the window is created.
    system_changed_size(dwRenderWidth, dwRenderHeight);
    WindowProperties resized_props;
    resized_props.set_size(dwRenderWidth, dwRenderHeight);
    _properties.add_properties(resized_props);
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::reset_device_resize_window
//       Access: Private
//  Description: Called after a window (either fullscreen or windowed)
//               has been resized, this recreates the D3D structures
//               to match the new size.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
reset_device_resize_window(UINT new_xsize, UINT new_ysize) {
  nassertr((new_xsize > 0) && (new_ysize > 0), false);
  bool retval = true;

  DXScreenData *screen = NULL;
  D3DPRESENT_PARAMETERS d3dpp;
  memcpy(&d3dpp, &_wcontext._presentation_params, sizeof(D3DPRESENT_PARAMETERS));
  _wcontext._presentation_params.BackBufferWidth = new_xsize;
  _wcontext._presentation_params.BackBufferHeight = new_ysize;
  make_current();
  HRESULT hr = _dxgsg->reset_d3d_device(&_wcontext._presentation_params, &screen);

  if (FAILED(hr)) {
    retval = false;
    wdxdisplay8_cat.error()
      << "reset_device_resize_window Reset() failed" << D3DERRORSTRING(hr);
    if (hr == D3DERR_OUTOFVIDEOMEMORY) {
      memcpy(&_wcontext._presentation_params, &d3dpp, sizeof(D3DPRESENT_PARAMETERS));
      hr = _dxgsg->reset_d3d_device(&_wcontext._presentation_params, &screen);
      if (FAILED(hr)) {
        wdxdisplay8_cat.error()
          << "reset_device_resize_window Reset() failed OutOfVidmem, then failed again doing Reset w/original params:" << D3DERRORSTRING(hr);
        throw_event("panda3d-render-error");
        return false;

      } else {
        if (wdxdisplay8_cat.is_info()) {
          wdxdisplay8_cat.info()
            << "reset of original size (" << _wcontext._presentation_params.BackBufferWidth
            << ", " << _wcontext._presentation_params.BackBufferHeight << ") succeeded\n";
        }
      }
    } else {
      wdxdisplay8_cat.fatal()
        << "Can't reset device, bailing out.\n";
      throw_event("panda3d-render-error");
      return false;
    }
  }
  // before you init_resized_window you need to copy certain changes to _wcontext
  if (screen) {
    _wcontext._swap_chain = screen->_swap_chain;
  }
  wdxdisplay8_cat.debug() << "swapchain is " << _wcontext._swap_chain << "\n";
  _gsg->mark_new();
  init_resized_window();
  return retval;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::init_resized_window
//       Access: Private
//  Description: Reinitializes the window after it has been resized,
//               or after it is first created.
//
//               Assumes CreateDevice or Device->Reset() has just been
//               called, and the new size is specified in
//               _wcontext._presentation_params.
////////////////////////////////////////////////////////////////////
void wdxGraphicsWindow8::
init_resized_window() {
  HRESULT hr;

  DWORD newWidth = _wcontext._presentation_params.BackBufferWidth;
  DWORD newHeight = _wcontext._presentation_params.BackBufferHeight;

  nassertv((newWidth != 0) && (newHeight != 0));
  nassertv(_wcontext._window != NULL);

  if (_wcontext._presentation_params.Windowed) {
    POINT ul, lr;
    RECT client_rect;

    // need to figure out x, y origin offset of window client area on screen
    // (we already know the client area size)

    GetClientRect(_wcontext._window, &client_rect);
    ul.x = client_rect.left;
    ul.y = client_rect.top;
    lr.x = client_rect.right;
    lr.y = client_rect.bottom;
    ClientToScreen(_wcontext._window, &ul);
    ClientToScreen(_wcontext._window, &lr);
    client_rect.left = ul.x;
    client_rect.top = ul.y;
    client_rect.right = lr.x;
    client_rect.bottom = lr.y;
  }

  // clear window to black ASAP
  nassertv(_wcontext._window != NULL);
  ClearToBlack(_wcontext._window, get_properties());

  // clear textures and VB's out of video&AGP mem, so cache is reset
  hr = _wcontext._d3d_device->ResourceManagerDiscardBytes(0);
  if (FAILED(hr)) {
    wdxdisplay8_cat.error()
      << "ResourceManagerDiscardBytes failed for device #"
      << _wcontext._card_id << D3DERRORSTRING(hr);
  }

  make_current();
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::D3DFMT_to_DepthBits
//       Access: Private, Static
//  Description: Returns the number of depth bits represented by the
//               indicated D3DFORMAT value.
////////////////////////////////////////////////////////////////////
int wdxGraphicsWindow8::
D3DFMT_to_DepthBits(D3DFORMAT fmt) {
  switch(fmt) {
  case D3DFMT_D16:
    return 16;

  case D3DFMT_D24X8:
  case D3DFMT_D24X4S4:
  case D3DFMT_D24S8:
    return 24;

  case D3DFMT_D32:
    return 32;

  case D3DFMT_D15S1:
    return 15;

  default:
    wdxdisplay8_cat.debug()
      << "D3DFMT_DepthBits: unhandled D3DFMT!\n";
    return 0;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsWindow8::is_badvidmem_card
//       Access: Private, Static
//  Description: Returns true if the indicated video adapter card is
//               known to report an inaccurate figure for available
//               video memory.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsWindow8::
is_badvidmem_card(D3DADAPTER_IDENTIFIER8 *pDevID) {
  // dont trust Intel cards since they often use regular memory as vidmem
  if (pDevID->VendorId == 0x00008086) {
    return true;
  }

  return false;
}
