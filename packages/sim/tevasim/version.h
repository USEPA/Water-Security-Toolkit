/*
 * Copyright © 2008 UChicago Argonne, LLC
 * NOTICE: This computer software, TEVA-SPOT, was prepared for UChicago Argonne, LLC
 * as the operator of Argonne National Laboratory under Contract No. DE-AC02-06CH11357
 * with the Department of Energy (DOE). All rights in the computer software are reserved
 * by DOE on behalf of the United States Government and the Contractor as provided in
 * the Contract.
 * NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
 *
 * This software is distributed under the BSD License.
 */

#include <string>

#define TEVA_SPOT_TOOLKIT_VERSION "2.1" 

inline std::string create_version(const char* command, const char* date, const char* time)
{
std::string version;
version += command;
version += " (TEVA-SPOT Toolkit) ";
version += TEVA_SPOT_TOOLKIT_VERSION;
version += "\n";
version += "  Compiled ";
version += date;
version += " ";
version += time;
version += "\n\n\
Copyright (2008) Sandia Corporation. Under the terms of Contract DE-AC04-       94AL85000 with Sandia Corporation, the U.S. Government retains certain rights   in this software.";
return version;
}
