#include "a-db-utils.h"

#ifdef Q_OS_UNIX
#ifndef Q_OS_ANDROID
#include <cxxabi.h>
#include <execinfo.h>

void get_stacktrace(STRING_LIST& bt)
{
  static unsigned int max_frames = 16;
  void* addrlist[max_frames+1];
  auto addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
  if (addrlen == 0) 
    {
      // corrupted..corr is comming
      return;
    }
  char** symbollist = backtrace_symbols(addrlist, addrlen);
  size_t funcnamesize = 256;
  char* funcname = (char*)malloc(funcnamesize);
  QString fname_res;

  for (int i = 2; i < addrlen; i++)
    {
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p)
        {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset
            && begin_name < begin_offset)
        {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            int status;
            char* ret = abi::__cxa_demangle(begin_name,
                                            funcname, &funcnamesize, &status);
            if (status == 0) {
                funcname = ret; // use possibly realloc()-ed string
        fname_res = funcname;
        int ppos = fname_res.indexOf("(");
        if(ppos != -1)
          {
            fname_res = fname_res.remove(ppos, fname_res.length());
          }
        bt.push_back(fname_res);
            }
            else {
          // demangling failed. Output function name as a C function with
          fname_res = begin_name;
          bt.push_back(fname_res);
            }
        }
        else
        {
      // couldn't parse the line? print the whole line.
      fname_res = symbollist[i];
      // bt.push_back(fname_res);
        }
    }

    free(funcname);
    free(symbollist);
}
#endif// Q_OS_ANDROID
#endif //Q_OS_UNIX

#ifdef Q_OS_ANDROID
void get_stacktrace(STRING_LIST&)
{
  //it's better to implement..
}
#endif

#ifdef Q_OS_WIN32
void get_stacktrace(STRING_LIST&)
{
  //it's better to implement..
}
#endif // Q_OS_WIN32

QString getLastStackFrames(int frames)
{
#ifdef _SQL_PROFILER
  QString rv = "";
  STRING_LIST bt;
  get_stacktrace(bt);
  if(!bt.empty())
    {
      STRING_LIST::iterator k = bt.begin();
      // if(frames == 1)k++;
      while(k != bt.end() && frames != 0)
    {     
      rv += *k;
      frames--;
      if(frames != 0)rv += "/";
      k++;
    }
    }
  return rv;
#else
Q_UNUSED(frames);
  return "---";
#endif
};


