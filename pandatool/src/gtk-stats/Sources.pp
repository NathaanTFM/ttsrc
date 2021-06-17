#define BUILD_DIRECTORY $[and $[HAVE_GTK],$[HAVE_NET]]
#define USE_PACKAGES net gtk

#begin bin_target
  // We suspect gtk will not be built universal on OSX.  Don't try.
  #define UNIVERSAL_BINARIES

  // We rename TARGET to pstats-gtk on Windows, so it won't compete
  // with Windows-native pstats.
  #define TARGET $[if $[WINDOWS_PLATFORM],pstats-gtk,pstats]
  #define LOCAL_LIBS \
    progbase pstatserver
  #define OTHER_LIBS \
    $[if $[HAVE_NET],net:c] $[if $[WANT_NATIVE_NET],nativenet:c] \
    pandabase:c pnmimage:c event:c pstatclient:c \
    linmath:c putil:c pipeline:c express:c pandaexpress:m panda:m \
    interrogatedb:c dtoolutil:c dtoolbase:c prc:c dconfig:c dtoolconfig:m dtool:m \
    pystub

  #define SOURCES \
    gtkStats.cxx \
    gtkStatsChartMenu.cxx gtkStatsChartMenu.h \
    gtkStatsGraph.cxx gtkStatsGraph.h \
    gtkStatsLabel.cxx gtkStatsLabel.h \
    gtkStatsLabelStack.cxx gtkStatsLabelStack.h \
    gtkStatsMenuId.h \
    gtkStatsMonitor.cxx gtkStatsMonitor.h gtkStatsMonitor.I \
    gtkStatsPianoRoll.cxx gtkStatsPianoRoll.h \
    gtkStatsServer.cxx gtkStatsServer.h \
    gtkStatsStripChart.cxx gtkStatsStripChart.h

  #if $[DEVELOP_GTKSTATS]
    #define EXTRA_CDEFS $[EXTRA_CDEFS] DEVELOP_GTKSTATS
  #endif

#end bin_target

