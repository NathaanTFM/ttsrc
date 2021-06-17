NVIDIA Cg 3.0 February 2011 README  Copyright (C) 2002-2011 NVIDIA Corp.
==================================

This distribution contains
--------------------------

- NVIDIA Cg toolkit documentation
  in the docs directory

- NVIDIA Cg compiler (cgc)
  in the bin directory

- NVIDIA Cg runtime libraries
  in the lib directory

- Example Cg applications
  in the examples directory

- Under Microsoft Windows, a Cg language syntax highlighter
  for Microsoft Visual Studio is provided in the
  msdev_syntax_highlighting directory

- Under Microsoft Windows, if selected at install time, 64-bit
  binaries and libraries are in the bin.x64 and lib.x64 directories.

See the release notes (docs/CgReleaseNotes.pdf) for detailed
information about this release.

The Cg toolkit is available for a number of different hardware and
OS platforms.  As of this writing, supported platforms include:

  - Windows (XP, Vista, Win 7) on x86/x86-64
  - Linux on x86/x86-64
  - Mac OS X (Tiger, Leopard, Snow Leopard) on ppc/i386/x86_64
  - Solaris 10 on x86/x86-64

Visit the NVIDIA Cg website at http://developer.nvidia.com/page/cg_main.html
for updates and complete compatibility information.

Changes since Cg 3.0 November 2010
----------------------------------
- Bug fixes
  - Fixed some bugs in DX11 tessellation support
  - Fixed a crash when effects in a have contained nearly identical user defined types
  - Fixed a crash when getting default values from unreferenced uniform parameters
  - Fixed problem setting matrices beyond 96 float constants in the vp30 profile
  - Application supplied options now override any set by cgGLSetOptimalOptions
  - Fixed a problem with 'const' variables in the GLSL profiles
- New documentation
  - Added reference manual pages for sampler states

Changes since Cg 3.0 July 2010
------------------------------
- New features
  - Vertex shaders aren't disabled when also using tessellation shaders
  - DirectX feature levels 9 and 10 now work with cgD3D11
  - cgCombinePrograms respects the auto compile flag
  - The hlsl10g profile now supports structures in AttribArray 
- Bug fixes
  - cgCopyProgram works once again!
  - Fixed a bug when compound-assignment operators (+=, *=, etc.) were used on column major matrices
  - tex2Dsize now works for shadow samplers
  - Resolved problem with geometry shaders on OSX
  - Corrected our OSX Framework executable path
  - Fixed crash when connecting API created parameters to precompiled (CG_OBJECT) program parameters
  - No longer try to use typedef keyword in generated GLSL code
  - Fixed a problem in the modf routine
  - Various documentation updates

Changes since Cg 3.0 beta May 2010
----------------------------------
- New features
  - Debian .deb packages for Debian and Ubuntu users
- New examples
  - Direct3D10/advanced/custom_state_assignments
  - Direct3D10/advanced/include_string
  - Direct3D10/advanced/interpolation_modifier
  - Direct3D11/advanced/cgfx_bumpdemo
  - Direct3D11/advanced/cgfx_bumpdemo_array
  - Direct3D11/advanced/cgfx_latest
  - Direct3D11/advanced/custom_state_assignments
  - Direct3D11/advanced/include_string
  - Direct3D11/advanced/interpolation_modifier
- New documentation
  - Improved documentation for the GP5 profiles
  - Additional reference manual for states
- Bug fixes
  - D3D11 profiles work now
  - cgIsParameterUsed now returns true for parameters that are assigned to fixed function pipeline texture units
  - Fixed a crash when calling cgGetEffectParameterBySemantic on an undefined interface
  - Fixed handling of ENV uniform semantic in GLSL profiles
  - Support centroid semantic modifier in DX10 profile
  - Vertex or tessellation state assignments now disable potentially conflicting profiles
  - cgGetIntAnnotationValues now returns data from unsigned int annotations
  - Added support for texBUF and texRBUF to GLSL profiles
  - Added support for GLSL1.30 texelFetch() routines
  - Add support for cgGetFirstProgramAnnotation for non-CgFX programs
  - Add support for program parameter annotations for entry point function
  - Support the use of ARB_vertex_array_bgra for OpenGL profiles
  - Silently ignore GenerateMipmap state assignment for samplerRECT

Changes since Cg 2.2 February 2010
----------------------------------
- New features
  - OpenGL GPU Program5 profiles
  - DirectX11 Shader Model 5 profiles
  - Support for tessellation programs
  - Support for up to 32 texture units
  - Unbind routines for D3D programs
  - CgFX buffer routines
  - Dependent parameter routines for CgFX shader arrays
  - Shadow versions of texBLAHproj functions in the hlsl10f profile 
  - Improved evaluation engine for expressions in CgFX files
- New examples
  - OpenGL/advanced/cgfx_buffer_lighting
  - OpenGL/advanced/cgfx_tessellation
  - OpenGL/advanced/tess_bezier
  - OpenGL/advanced/tess_simple
  - Direct3D11/basic/02_vertex_and_fragment_program
  - Direct3D11/basic/03_uniform_parameter
  - Direct3D11/basic/04_varying_parameter
  - Direct3D11/basic/05_texture_sampling
  - Direct3D11/basic/06_vertex_twisting
  - Direct3D11/basic/07_two_texture_accesses
  - Direct3D11/basic/cgfx_buffer
  - Direct3D11/basic/cgfx_simple
  - Direct3D11/basic/cgfx_texture
  - Direct3D11/advanced/cgfx_buffer_lighting
  - Direct3D11/advanced/combine_programs
  - Direct3D11/advanced/gs_shrinky
  - Direct3D11/advanced/gs_simple
  - Direct3D11/advanced/tess_bezier
  - Direct3D11/advanced/tess_simple
- New documentation
  - Updated reference manual for new profiles and entry points
- Bug fixes
  - fixed crash when invalid options strings are passed to the compiler
  - cgGetParameterBufferOffset now correctly returns -1 for parameters not in buffers
  - fixed crash in cgGetParameterBufferOffset and cgGetParameterBufferIndex
  - handle user-defined constants with same name as an overloaded stdlib function
  - fix issues with C regs and texture lookups in D3D10 translation profile
  - allow UTF-8 byte-order-marker (BOM) at beginning of effects
  - remove spurious warnings from the GLSL profiles about uninitialized variables
  - better error message when trying to access an undefined struct member
