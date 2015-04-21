/* $RCSfile: $
 * $Revision: $ $Date: $
 * Auth: David Loffredo (loffredo@steptools.com)
 * 
 * Copyright (c) 1991-2015 by STEP Tools Inc. 
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted, provided that this copyright
 * notice and license appear on all copies of the software.
 * 
 * STEP TOOLS MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. STEP TOOLS
 * SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A
 * RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS
 * DERIVATIVES.
 */


class stp2webgl_opts {
public:
    StpAsmProductDefVec root_prods;

    rose_uint_vector root_ids;
    rose_uint_vector shape_ids;

    StixMeshOptions mesh;

    RoseDesign * design;

    const char * srcfile;
    const char * dstfile;
    const char * dstdir;

    int	do_split;
    

    stp2webgl_opts()
	: design(0),
	  srcfile(0),
	  dstfile(0),
	  dstdir(0),
	  do_split(0)
    {
    }
};
