/* $RCSfile: $
 * $Revision: $ $Date: $
 * Auth: David Loffredo (loffredo@steptools.com)
 * 
 * Copyright (c) 1991-2015 by STEP Tools Inc. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
