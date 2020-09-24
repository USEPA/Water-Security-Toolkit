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
#ifndef _SSS_H_
#define _SSS_H_
#include "teva-assess.h"
#include "ta_input.h"
/*
**-------------------------------------------------------------
** Input data file section headers 
**-------------------------------------------------------------
*/
LIBEXPORT(PInputFileData) loadSSSFileData(const char *sssFile);
LIBEXPORT(void) processSensorSetData(PInputFileData sssIFD,PMem mem);

#endif
