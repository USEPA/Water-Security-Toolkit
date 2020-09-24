/*  Setuptools Script Launcher for Windows

    This is a stub executable for Windows that functions somewhat like
    Effbot's "exemaker", in that it runs a script with the same name but
    a .py extension, using information from a #! line.  It differs in that
    it spawns the actual Python executable, rather than attempting to
    hook into the Python DLL.  This means that the script will run with
    sys.executable set to the Python executable, where exemaker ends up with
    sys.executable pointing to itself.  (Which means it won't work if you try
    to run another Python process using sys.executable.)

    To build/rebuild with mingw32, do this in the setuptools project directory:

       gcc -DGUI=0           -mno-cygwin -O -s -o setuptools/cli.exe launcher.c
       gcc -DGUI=1 -mwindows -mno-cygwin -O -s -o setuptools/gui.exe launcher.c

    It links to msvcrt.dll, but this shouldn't be a problem since it doesn't
    actually run Python in the same process.  Note that using 'exec' instead
    of 'spawn' doesn't work, because on Windows this leads to the Python
    executable running in the *background*, attached to the same console
    window, meaning you get a command prompt back *before* Python even finishes
    starting.  So, we have to use spawnv() and wait for Python to exit before
    continuing.  :(
*/

/* This script was originally part of the Python setuptools 0.6c11
   package (http://pypi.python.org/pypi/setuptools).  While setuptools
   is dual-licensed under the PSF and ZPL licenses, the original source
   file did not bear any explicit copyright statement.  This program was
   subsequently modified by Sandia National Laboratories (John Siirola,
   jdsiiro@sandia.gov) to also search the system path and the windows
   registry when attempting to locate the python interpreter.  Added and
   modified code is indicated below in compliance with the PSF.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "windows.h"

int fail(char *format, char *data) {
    /* Print error message to stderr and return 2 */
    fprintf(stderr, format, data);
    return 2;
}

// BEGIN CODE ADDED BY J. SIIROLA
#include <tchar.h> 
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_LENGTH 16383

int find_python_on_path(LPTSTR buffer, DWORD len)
{
   DWORD ans;
   ans = SearchPath(NULL, "python.exe", NULL, len, buffer, NULL);
   if ( ans >= len )
   {
      fprintf(stderr, "Found python on the path, but the internal "
	      "buffer was not long enough (partial path=%s\n", buffer);
      return 0;
   }
   if ( ans > 0 )
      return 1;

   return 0;
}

int find_python_in_registry(LPTSTR buffer, DWORD len, int search_x64)
{
   HKEY key;
   LONG ans;
   DWORD numSubKeys = 0;
   DWORD maxKeyLen = 0;
   DWORD i = 0;
   DWORD readLen = len;
   DWORD keyLen = MAX_KEY_LENGTH;
   TCHAR subKey[MAX_KEY_LENGTH];
   TCHAR pathKey[MAX_KEY_LENGTH];
   TCHAR valueName[MAX_VALUE_LENGTH];
   DWORD valueLen = MAX_VALUE_LENGTH;
   int found_26 = 0;

   ans = RegOpenKeyEx
      ( HKEY_LOCAL_MACHINE,
        TEXT("SOFTWARE\\Python\\Pythoncore\\"),
        0,
	KEY_READ | ( search_x64 ? 0x0100 : 0x0200 ),
	&key );
   if ( ans != ERROR_SUCCESS )
   {
      RegCloseKey(key);
      return 0;
   }
   
   ans = RegQueryInfoKey
      ( key, NULL, NULL, NULL, &numSubKeys, &maxKeyLen, 
	NULL, NULL, NULL, NULL, NULL, NULL );
   if ( ans != ERROR_SUCCESS )
   {
      RegCloseKey(key);
      return 0;
   }

   if ( numSubKeys == 0 )
      return 0;

   // We need to pick a version of Python...  We hope for 2.7, settle
   // for 2.6, and after that, don't care.
   for (i=0; i<numSubKeys; i++)
   {
      keyLen = MAX_KEY_LENGTH;
      ans = RegEnumKeyEx(key, i, subKey, &keyLen, NULL, NULL, NULL, NULL);
      if ( ans != ERROR_SUCCESS )
	 continue;
      if ( strncmp("2.7", subKey, MAX_KEY_LENGTH) == 0 )
      {
	 found_26 = 0;
	 break;
      }
      else if ( strncmp("2.6", subKey, MAX_KEY_LENGTH) == 0 )
	 found_26 = 1;
   }
   if ( found_26 )
   {
      subKey[0] = '2';
      subKey[1] = '.';
      subKey[2] = '6';
      subKey[3] = 0;
   }

   sprintf(pathKey, "SOFTWARE\\Python\\PythonCore\\%s\\InstallPath\\", subKey);
   //printf("Reading key: %s\n", pathKey);
   ans = RegOpenKeyEx
      ( HKEY_LOCAL_MACHINE,
        TEXT(pathKey),
        0,
	KEY_READ | ( search_x64 ? 0x0100 : 0x0200 ),
	&key );
   if ( ans != ERROR_SUCCESS )
   {
      RegCloseKey(key);
      return 0;
   }

   readLen = len;
   ans = RegEnumValue( key, 0, valueName, &valueLen, NULL, NULL, 
		       buffer, &readLen );
   
   RegCloseKey(key);
   if ( readLen+strlen("\\python.exe") > len )
      fprintf(stderr, "Found python in the registry, but the internal "
	      "buffer was not long enough (partial path=%s\n", buffer);
   strncat(buffer, "\\python.exe", len-strlen(buffer)-1);
   return ( ans == ERROR_SUCCESS );
}
// END CODE ADDED BY J. SIIROLA


char *quoted(char *data) {
    int i, ln = strlen(data), nb;

    /* We allocate twice as much space as needed to deal with worse-case
       of having to escape everything. */
    char *result = calloc(ln*2+3, sizeof(char));
    char *presult = result;

    *presult++ = '"';
    for (nb=0, i=0; i < ln; i++)
      {
        if (data[i] == '\\')
          nb += 1;
        else if (data[i] == '"')
          {
            for (; nb > 0; nb--)
              *presult++ = '\\';
            *presult++ = '\\';
          }
        else
          nb = 0;
        *presult++ = data[i];
      }

    for (; nb > 0; nb--)        /* Deal w trailing slashes */
      *presult++ = '\\';

    *presult++ = '"';
    *presult++ = 0;
    return result;
}


char *loadable_exe(char *exename) {
    /* HINSTANCE hPython;  DLL handle for python executable */
    char *result;

    /* hPython = LoadLibraryEx(exename, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hPython) return NULL; */

    /* Return the absolute filename for spawnv */
    result = calloc(MAX_PATH, sizeof(char));
    strncpy(result, exename, MAX_PATH);
    /*if (result) GetModuleFileName(hPython, result, MAX_PATH);

    FreeLibrary(hPython); */
    return result;
}


char *find_exe(char *exename, char *script) {
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    char path[_MAX_PATH], c, *result;

    /* convert slashes to backslashes for uniform search below */
    result = exename;
    while (c = *result++) if (c=='/') result[-1] = '\\';

    _splitpath(exename, drive, dir, fname, ext);
    if (drive[0] || dir[0]=='\\') {
        return loadable_exe(exename);   /* absolute path, use directly */
    }

// BEGIN CODE ADDED BY J. SIIROLA
    // Look on the path
    if ( find_python_on_path(path, _MAX_PATH) )
    {
       //printf("Found Python on PATH: %s\n", path);
       return loadable_exe(path);
    }

    // Look for 64-bit Python
    if ( find_python_in_registry(path, _MAX_PATH, 1) )
    {
       //printf("Found 64-bit Python in Registry: %s\n", path);
       return loadable_exe(path);
    }
    // Look for 32-bit Python
    if ( find_python_in_registry(path, _MAX_PATH, 0) )
    {
       //printf("Found 32-bit Python in Registry: %s\n", path);
       return loadable_exe(path);
    }
// END CODE ADDED BY J. SIIROLA

    /* Use the script's parent directory, which should be the Python home
       (This should only be used for bdist_wininst-installed scripts, because
        easy_install-ed scripts use the absolute path to python[w].exe
    */
    _splitpath(script, drive, dir, fname, ext);
    result = dir + strlen(dir) -1;
    if (*result == '\\') result--;
    while (*result != '\\' && result>=dir) *result-- = 0;
    _makepath(path, drive, dir, exename, NULL);
    return loadable_exe(path);
}


char **parse_argv(char *cmdline, int *argc)
{
    /* Parse a command line in-place using MS C rules */

    char **result = calloc(strlen(cmdline), sizeof(char *));
    char *output = cmdline;
    char c;
    int nb = 0;
    int iq = 0;
    *argc = 0;

    result[0] = output;
    while (isspace(*cmdline)) cmdline++;   /* skip leading spaces */

    do {
        c = *cmdline++;
        if (!c || (isspace(c) && !iq)) {
            while (nb) {*output++ = '\\'; nb--; }
            *output++ = 0;
            result[++*argc] = output;
            if (!c) return result;
            while (isspace(*cmdline)) cmdline++;  /* skip leading spaces */
            if (!*cmdline) return result;  /* avoid empty arg if trailing ws */
            continue;
        }
        if (c == '\\')
            ++nb;   /* count \'s */
        else {
            if (c == '"') {
                if (!(nb & 1)) { iq = !iq; c = 0; }  /* skip " unless odd # of \ */
                nb = nb >> 1;   /* cut \'s in half */
            }
            while (nb) {*output++ = '\\'; nb--; }
            if (c) *output++ = c;
        }
    } while (1);
}




int run(int argc, char **argv, int is_gui) {

    char python[256];   /* python executable's filename*/
    char *pyopt;        /* Python option */
    char script[256];   /* the script's filename */

    int scriptf;        /* file descriptor for script file */

    char **newargs, **newargsp, **parsedargs; /* argument array for exec */
    char *ptr, *end;    /* working pointers for string manipulation */
    int i, parsedargc;              /* loop counter */


    /* compute script name from our .exe name*/
    GetModuleFileName(NULL, script, sizeof(script));
    end = script + strlen(script);
    while( end>script && *end != '.')
        *end-- = '\0';
    *end-- = '\0';
    strcat(script, (GUI ? "-script.pyw" : "-script.py"));

    /* figure out the target python executable */
    //printf("script = %s\n", script);

    scriptf = open(script, O_RDONLY);
    if (scriptf == -1) {
        return fail("Cannot open %s\n", script);
    }
    end = python + read(scriptf, python, sizeof(python));
    close(scriptf);

    ptr = python-1;
    while(++ptr < end && *ptr && *ptr!='\n' && *ptr!='\r') {;}

    *ptr-- = '\0';

    if (strncmp(python, "#!", 2)) {
        /* default to python.exe if no #! header */
        strcpy(python, "#!python.exe");
        //printf("Defaulting to %s\n", python);
    }

    parsedargs = parse_argv(python+2, &parsedargc);

    /* Using spawnv() can fail strangely if you e.g. find the Cygwin
       Python, so we'll make sure Windows can find and load it */

    ptr = find_exe(parsedargs[0], script);
    if (!ptr) {
        return fail("Cannot find Python executable %s\n", parsedargs[0]);
    }

    //printf("Python executable: %s\n", ptr);

    /* Argument array needs to be
       parsedargc + argc, plus 1 for null sentinel */

    newargs = (char **)calloc(parsedargc + argc + 1, sizeof(char *));
    newargsp = newargs;

    *newargsp++ = quoted(ptr);
    for (i = 1; i<parsedargc; i++) *newargsp++ = quoted(parsedargs[i]);

    *newargsp++ = quoted(script);
    for (i = 1; i < argc; i++)     *newargsp++ = quoted(argv[i]);

    *newargsp++ = NULL;

    /* printf("args 0: %s\nargs 1: %s\n", newargs[0], newargs[1]); */

    if (is_gui) {
        /* Use exec, we don't need to wait for the GUI to finish */
        execv(ptr, (const char * const *)(newargs));
        return fail("Could not exec %s", ptr);   /* shouldn't get here! */
    }

    /* We *do* need to wait for a CLI to finish, so use spawn */
    // CHANGED BY J. SIIROLA to return an error message if executing
    // python failed (usually because the interpreter path was wrong)
    int retcode = spawnv(P_WAIT, ptr, (const char * const *)(newargs));
    if ( retcode < 0 )
       return fail("Cound not exec %s", ptr);
    return retcode;
}


int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lpCmd, int nShow) {
    return run(__argc, __argv, GUI);
}

