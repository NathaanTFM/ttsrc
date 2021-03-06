#ifndef	VRPN_CONFIGURE_H

//--------------------------------------------------------------
/* This file contains configuration options for VRPN.  The first
   section has definition lines that can be commented in or out
   at build time.  The second session has automaticly-generated
   directives and should not be edited. */
//--------------------------------------------------------------

//--------------------------------------------------------//
// EDIT BELOW THIS LINE FOR NORMAL CONFIGURATION SETTING. //
//--------------------------------------------------------//

//-----------------------
// Default port to listen on for a server.  It used to be 4500
// up through version 6.03, but then all sorts of VPNs started
// using this, as did Microsoft.  Port 3883 was assigned to VRPN
// by the Internet Assigned Numbers Authority (IANA) October, 2003.
// Change this to make a location-specific default if you like.
// The parentheses are to keep it from being expanded into something
// unexpected if the code has a dot after it.
#define	vrpn_DEFAULT_LISTEN_PORT_NO (3883)

//-----------------------
// Instructs VRPN to expose the vrpn_gettimeofday() function also
// as gettimeofday() so that external programs can use it.  This
// has no effect on any system that already has gettimeofday()
// defined, and is put here for Windows.  This function should
// not really be implemented within VRPN, but it was expedient to
// include it when porting applications to Windows.  Turn this
// off if you have another implementation, or if you want to call
// vrpn_gettimeofday() directly.
#define	VRPN_EXPORT_GETTIMEOFDAY

//-----------------------
// Instructs VRPN to use phantom library to construct a unified
// server, using phantom as a common device, and phantom
// configuration in .cfg file.
// PLEASE SPECIFY PATH TO GHOSTLIB IN NEXT SECTION IF YOU USE THIS
//#define     VRPN_USE_PHANTOM_SERVER

//------------------------
// Instructs vrpn to use SensAble's HDAPI rather than GHOST library.
// Only used in conjuntion with VRPN_USE_PHANTOM_SERVER.
// PLEASE SPECIFY PATH TO HDAPI IN NEXT SECTION IF YOU USE THIS.
// Also, you need to go to the vrpn_phantom and vrpn_server projects
// and remove the GHOST include directories from the include paths.
// Yes, HDAPI fails if it even has them in the path (as so many other
// things also fail).  At least we're rid of them now.  When you
// uncomment it (to use GHOST), add the following to the include
// directories for the vrpn_phantom project: $(SYSTEMDRIVE)\Program Files\SensAble\GHOST\v4.0\include,$(SYSTEMDRIVE)\Program Files\SensAble\GHOST\v4.0\external\stl,
#define VRPN_USE_HDAPI

//------------------------
// Instructs vrpn to use Ghost 3.1 instead of Ghost 3.4.
// Only used in conjuntion with VRPN_USE_PHANTOM_SERVER.
// PLEASE SPECIFY PATH TO GHOSTLIB IN NEXT SECTION IF YOU USE THIS
// (This is expected to be used on systems where Ghost 4.0 is not
// available, such as the SGI platform.  If you are using this on
// a Windows PC with Visual Studio, you will need to alter
// server_src/vrpn_phantom.dsp to reference the Ghost 3.1 include
// paths.)
// #define VRPN_USE_GHOST_31

//-----------------------
// Instructs VRPN to use the high-performance timer code on
// Windows, rather than the default clock which has an infrequent
// update.  At one point in the past, an implementation of this
// would only work correctly on some flavors of Windows and with
// some types of CPUs.
// There are actually two implementations
// of the faster windows clock.  The original one, made by Hans
// Weber, checks the clock rate to see how fast the performance
// clock runs (it takes a second to do this when the program
// first calls vrpn_gettimeofday()).  The second version by Haris
// Fretzagias relies on the timing supplied by Windows.  To use
// the second version, also define VRPN_WINDOWS_CLOCK_V2.
#define	VRPN_UNSAFE_WINDOWS_CLOCK
#define	VRPN_WINDOWS_CLOCK_V2

//-----------------------
// Instructs VRPN library and server to include code that uses
// the DirectX SDK (from its standard installation in C:\DXSDK).
// Later in this file, we also instruct the compiler to link with
// the DirectX library if this is defined.
//#define	VRPN_USE_DIRECTINPUT

//-----------------------
// Instructs the VRPN server to create an entry for the Adrienne
// time-code generator.  This is a device that produces time values
// from an analog video stream so that events in the virtual world
// can be synchronized with events on a movie.  The Adrienne folder
// should be located at the same level as the VRPN folder for the
// code to find it.
//#define	VRPN_INCLUDE_TIMECODE_SERVER

//-----------------------
// Compiles the InterSense Tracker using the
// InterSense Interface Libraries SDK (tested for version
// 3.45) on windows.  This should work with all Intersense trackers,
// both the USB and the serial port versions.  The files isense.h,
// types.h and isense.c should be put in a directory called 'isense'
// at the same level as the vrpn directory.  The isense.dll should
// be put either in Windows/system32 or in the location where the
// executable lives or somewhere on the path.
//#define VRPN_INCLUDE_INTERSENSE

//-----------------------
// Instructs VRPN library and server to include code that uses
// the National Instruments Nidaq libary to control analog outputa.
// Later in this file, we also instruct the compiler to link with
// the National Instruments libraries if this is defined.
//#define	VRPN_USE_NATIONAL_INSTRUMENTS

//-----------------------
// Instructs VRPN to use the default room space transforms for
// the Desktop Phantom as used in the nanoManipulator application
// rather than the default world-origin with identity rotation.
// Please don't anyone new use the room space transforms built
// into VRPN -- they are a hack pulled forward from Trackerlib.
#define	DESKTOP_PHANTOM_DEFAULTS

//------------------------
// Instructs VRPN to use microscribe3D library to construct a unified
// server
//#define VRPN_USE_MICROSCRIBE

//-----------------------
// Instructs VRPN to use a DLL interface on Windows systems.
// When using this, link with VRPNDLL.LIB (and VRPN.DLL) rather
// than VRPN.LIB in user code.  This is experimental and is
// under development to enable C# and other languages to pull in
// VRPN.  This is only needed when trying to link VRPN with
// languages other than C++ (and not even for Java).  If you don't
// have a good reason to, don't define it.
// Not implemented for .so-based Unix systems.
//#define VRPN_USE_SHARED_LIBRARY

//------------------------------------------------------------------//
// EDIT THESE DEFINITIONS IF YOUR SYSTEM IS NON-STANDARD.  THEY ARE //
// USED BELOW TO LOCATE LIBRARIES AND INCLUDE FILES.                //
//------------------------------------------------------------------//

#define VRPN_SYSTEMDRIVE "D:"

#define VRPN_HDAPI_PATH         VRPN_SYSTEMDRIVE "/Program Files/SensAble/3DTouch/lib/"
#define VRPN_HDAPI_UTIL_PATH    VRPN_SYSTEMDRIVE "/Program Files/SensAble/3DTouch/utilities/lib/"
#define VRPN_GHOST_31_PATH      VRPN_SYSTEMDRIVE "/Program Files/SensAble/GHOST/v3.1/lib/"
#define VRPN_GHOST_40_PATH      VRPN_SYSTEMDRIVE "/Program Files/SensAble/GHOST/v4.0/lib/"

#define VRPN_DIRECT_X_PATH      VRPN_SYSTEMDRIVE "/DXSDK/lib/"

#define VRPN_NIDAQ_PATH         VRPN_SYSTEMDRIVE "/Program Files/National Instruments/NI-DAQ/Lib/"

//---------------------------------------------------------------//
// DO NOT EDIT BELOW THIS LINE FOR NORMAL CONFIGURATION SETTING. //
//---------------------------------------------------------------//

// Load VRPN Phantom library if we are using phantom server as unified server
// Load SensAble Technologies GHOST library to run the Phantom
// NOTE: The paths to these libraries are set in the Settings/Link tab of
// the various project files.  The paths to the include files are in the
// Settings/C++/preprocessor tab.
#ifdef VRPN_USE_PHANTOM_SERVER
  #ifdef VRPN_USE_HDAPI
    #pragma comment (lib, VRPN_HDAPI_PATH "hd.lib")
    #ifdef	_DEBUG
      #pragma comment (lib,VRPN_HDAPI_UTIL_PATH "hdud.lib")
    #else
      #pragma comment (lib,VRPN_HDAPI_UTIL_PATH "hdu.lib")
    #endif
  #else
    #ifdef VRPN_USE_GHOST_31
      #pragma comment (lib,VRPN_GHOST_31_PATH "GHOST31.lib")
    #else
      #pragma comment (lib,VRPN_GHOST_40_PATH "GHOST40.lib")
    #endif
  #endif
#endif

// Load DirectX SDK libraries and tell which version we need if we are using it.
// If this doesn't match where you have installed these libraries,
// edit the following lines to point at the correct libraries.  Do
// this here rather than in the project settings so that it can be
// turned on and off using the definition above.
#ifdef	VRPN_USE_DIRECTINPUT
#define	DIRECTINPUT_VERSION 0x0800
#pragma comment (lib, VRPN_DIRECT_X_PATH "dxguid.lib")
#pragma comment (lib, VRPN_DIRECT_X_PATH "dxerr8.lib")
#pragma comment (lib, VRPN_DIRECT_X_PATH "dinput8.lib")
#endif

// Load Adrienne libraries if we are using the timecode generator.
// If this doesn't match where you have installed these libraries,
// edit the following lines to point at the correct libraries.  Do
// this here rather than in the project settings so that it can be
// turned on and off using the definition above.
#ifdef	VRPN_INCLUDE_TIMECODE_SERVER
#pragma comment (lib, "../../Adrienne/AEC_DLL/AEC_NTTC.lib")
#endif

// Load National Instruments libraries if we are using them.
// If this doesn't match where you have installed these libraries,
// edit the following lines to point at the correct libraries.  Do
// this here rather than in the project settings so that it can be
// turned on and off using the definition above.
// NOTE: The paths to these libraries are set in the Settings/Link tab of
// the various project files.  The paths to the include files are in the
// Settings/C++/preprocessor tab.
#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
#pragma comment (lib, VRPN_NIDAQ_PATH "nidaq32.lib")
#pragma comment (lib, VRPN_NIDAQ_PATH "nidex32.lib")
#endif

// Load Microscribe-3D SDK libraries 
// If this doesn't match where you have installed these libraries,
// edit the following lines to point at the correct libraries.  Do
// this here rather than in the project settings so that it can be
// turned on and off using the definition above.
#ifdef        VRPN_USE_MICROSCRIBE
#pragma comment (lib, "armdll32.lib")
#endif

// This will be defined in the VRPN (non-DLL) project and nothing else
// Overrides USE_SHARED_LIBRARY to get rid of "inconsistent DLL linkage" warnings.
#ifdef VRPNDLL_NOEXPORTS
#undef VRPN_USE_SHARED_LIBRARY
#endif

// This will be defined in the VRPN (DLL) project and nothing else
// Forces "USE_SHARED_LIBRARY independent of definition above so that the
// DLL will build
#if defined(VRPNDLL_EXPORTS) && !defined(VRPN_USE_SHARED_LIBRARY)
#define VRPN_USE_SHARED_LIBRARY
#endif

// For client code, make sure we add the proper library dependency to the linker
#ifdef _WIN32
#pragma comment (lib, "wsock32.lib")  // VRPN requires the Windows Sockets library.
#ifdef VRPN_USE_SHARED_LIBRARY
#ifdef VRPNDLL_EXPORTS
#define  VRPN_API		 __declspec(dllexport) 
#else
#define  VRPN_API		 __declspec(dllimport)
#pragma comment (lib, "vrpndll.lib")
#endif
#else
#ifndef VRPNDLL_NOEXPORTS
#pragma comment (lib, "vrpn.lib")     // We'll need the VRPN library
#endif
#define  VRPN_API
#endif
#define	 VRPN_CALLBACK	 __stdcall
#else
// In the future, other architectures may need their own sections
#define  VRPN_API
#define  VRPN_CALLBACK
#endif

#define	VRPN_CONFIGURE_H
#endif

