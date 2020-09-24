/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

#if defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#include <limits.h>
char *getfullpath(const char *pathname, char *resolved_path, size_t maxLength)
{
  return _fullpath(resolved_path,pathname,_MAX_PATH);
}
#else
#include <limits.h>
#include <string.h>
#include <stdlib.h>
char *getfullpath(const char *pathname, char *resolved_path, size_t maxLength)
{
  char rpath[PATH_MAX];
  char *rv=realpath(pathname,rpath);
  if(rv==NULL) return NULL;
  if(strlen(rpath) > maxLength-1) return NULL;
  else {
    strcpy(resolved_path,rv);
    return resolved_path;
  }
}
#endif

