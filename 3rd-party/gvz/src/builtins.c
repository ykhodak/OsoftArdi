#include "gvplugin.h"
#include "graphviz_version.h"


extern gvplugin_library_t gvplugin_dot_layout_LTX_library;
extern gvplugin_library_t gvplugin_neato_layout_LTX_library;
extern gvplugin_library_t gvplugin_core_LTX_library;
//extern gvplugin_library_t gvplugin_quartz_LTX_library;
//extern gvplugin_library_t gvplugin_visio_LTX_library;


lt_symlist_t lt_preloaded_symbols[] =
{
	{ "gvplugin_dot_layout_LTX_library", &gvplugin_dot_layout_LTX_library},
	{ "gvplugin_neato_layout_LTX_library", &gvplugin_neato_layout_LTX_library},
	{ "gvplugin_core_LTX_library", &gvplugin_core_LTX_library},
	//	{ "gvplugin_quartz_LTX_library", &gvplugin_quartz_LTX_library},
	//	{ "gvplugin_visio_LTX_library", &gvplugin_visio_LTX_library},
	{ 0, 0}
};

void ardDefineGraphvizPlugins(GVC_t* g)
{
  // for some reason it says "dot" is not available even after gvAddLibrary
  gvAddLibrary(g, &gvplugin_dot_layout_LTX_library);
  gvAddLibrary(g, &gvplugin_neato_layout_LTX_library);
}

const char* ardGraphvizVersion()
{
  static char ver[256] = "";
  static int firstCall = 1;
  if(firstCall)
    {
      sprintf(ver, "%s (custom build %s)", VERSION, __DATE__);
      firstCall = 0;
    }
  return ver;
}

#ifdef Q_OS_ANDROID
//this is for NDK 8re
void sincos(double x, double *sinV, double *cosV)
{
    *sinV = sin(x);
    *cosV = cos(x);
};

double log2(double x)
{
    static double conv = log(2.0);
    return log(x) / conv;
};

#endif

#undef HAVE_CONFIG_H
#include "cdt.h"
int ardGraphvizCheck()
{
  int rv = (Dttree != 0);
  return rv;
}
