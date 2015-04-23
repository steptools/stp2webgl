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

#include <stp_schema.h>
#include <stix.h>
#include <stixmesh.h>

#include "stp2webgl.h"

enum FileFormat { FmtWebXML, FmtTxtSTL, FmtBinSTL };

extern int write_webxml (stp2webgl_opts * opts);
extern int write_ascii_stl (stp2webgl_opts * opts);
extern int write_binary_stl (stp2webgl_opts * opts);


const char * tool_name 	= "Facet STEP for Lightweight Viewing";
const char * tool_trace	= "stp2webgl";

const char * usage_msg 	= "Usage: %s  [options] <stpfile> [shape_eids]\n";
const char * opts_short =
    " -help for a list of available options.\n\n";

const char * opts_long = 
    "\n"
    " This tool converts STEP CAD data into triangular meshes, and then\n"
    " writes the result into lightweight files for use in WebGL and other\n"
    " visualization applications.\n"
    "\n"
    " -help\t\t - Print this help message. \n"
    " -stl\t\t - Write STL data in ascii text format. \n"
    " -stlbin\t\t - Write STL data in binary format. \n"
    " -webxml\t\t - Write XML for WebGl client (default). \n"
    "\n"
    " -tol <dist>\t - Absolute linearization tolerance.  When linearizing\n"
    "\t\t   curves, this is the maximum that a line segment can\n"
    "\t\t   deviate from the original.  Distance given in native\n"
    "\t\t   units of the part.  (eg 0.001)\n"
    "\n"
    " -ftol <frac>\t - Fractional linearization tolerance.  As above, but\n"
    "       \t\t   the value is given as a fraction of the bounding\n"
    " \t\t   box of the curve or surface. (eg 0.1 for 10%)\n"
    "\n"
    " -min <sz>\t - Absolute minimum face size to facet.  Faces with a\n"
    "      \t\t   bounding box smaller than this size are collapsed into\n"
    "\t\t   a point.  Size given in native units of the part.\n"
    "\n"
    " -fmin <frac>\t - Fractional minimum face size to facet. As above, but\n"
    "       \t\t   the value is given as a fraction of the bounding box\n"
    "\t\t   of the face. (eg 0.1 for 10%%)\n"
    "\n"
    " -root <eid>\t - Write the subassembly rooted at the #eid instance,\n"
    "       \t\t   which should be a product_definition\n"
    "\n"
    " -o <outname>\t - Write output to given file\n"
    " -d\t\t - Write multiple files (-o is a directory)\n"
    "\n" 
    ;

static void usage (const char * name) 
{  
    fprintf (stderr, usage_msg, name);
    fputs (opts_short, stderr);
    exit (1);
}

static void long_usage (const char * name) 
{
    printf (usage_msg, name);
    puts (opts_long);
    exit (0);
}


#define NEXT_ARG(i,argc,argv) ((i<argc)? argv[i++]: 0)

int main(int argc, char ** argv)
{
    /* Disable buffering */
    //setvbuf(stdout, 0, _IONBF, 2);
    ROSE.quiet(1);
    stplib_init();
    stixmesh_init();
    
    stp2webgl_opts opts;
    FileFormat fmt = FmtWebXML;
    
    int idx = 1;

    /* must have at least one arg */
    if (argc < 2) usage(argv[0]);

    while ( idx < argc )
    {
	const char * arg = NEXT_ARG(idx,argc,argv);
    
	/* command line options */
	if (!strcmp (arg, "-h") ||
	    !strcmp (arg, "-help") ||
	    !strcmp (arg, "--help"))
	{
	    long_usage(argv[0]);
	}

	else if (!strcmp(arg, "-stl"))		{ fmt = FmtTxtSTL; }
	else if (!strcmp(arg, "-stlbin"))	{ fmt = FmtBinSTL; }
	else if (!strcmp(arg, "-webxml"))	{ fmt = FmtWebXML; }

	else if (!strcmp(arg, "-tol"))
	{
	    double tmp;
	    const char * val = NEXT_ARG(idx,argc,argv);
	    if (!val || (sscanf (val, "%lf", &tmp) != 1)) {
		fprintf (stderr, "option: -tol <num>\n");
		exit (1);
	    }
  	    opts.mesh.setToleranceAbsolute(tmp);
	}
       	
	else if (!strcmp(arg, "-ftol"))
	{
	    double tmp;
	    const char * val = NEXT_ARG(idx,argc,argv);
	    if (!val || (sscanf (val, "%lf", &tmp) != 1)) {
		fprintf (stderr, "option: -ftol <frac>\n");
		exit (1);
	    }
	    opts.mesh.setToleranceFraction(tmp);
	}
		
	else if (!strcmp(arg, "-min"))
	{
	    double tmp;
	    const char * val = NEXT_ARG(idx,argc,argv);
	    if (!val || (sscanf (val, "%lf", &tmp) != 1)) {
		fprintf (stderr, "option: -min <num>\n");
		exit (1);
	    }
	    opts.mesh.setMinFaceAbsolute(tmp);
	}
	else if (!strcmp(arg, "-fmin"))
	{
	    double tmp;
	    const char * val = NEXT_ARG(idx,argc,argv);
	    if (!val || (sscanf (val, "%lf", &tmp) != 1)) {
		fprintf (stderr, "option: -fmin <frac>\n");
		exit (1);
	    }
	    opts.mesh.setMinFaceFraction(tmp);
	}
	else if (!strcmp(arg, "-root"))
	{
	    unsigned tmp;
	    const char * val = NEXT_ARG(idx,argc,argv);

	    if (!val || (tmp=atol(val)) == 0) {
		fprintf (stderr, "invalid entity id for -root\n");
		exit (1);
	    }
	    opts.root_ids.append(tmp);
		    
	}
	else if (!strcmp(arg, "-o"))
	{
	    const char * val = NEXT_ARG(idx,argc,argv);	    
	    if (!val) {
		fprintf (stderr, "option: -o <name>\n");
		exit (1);
	    }
	    opts.dstfile = val;
	}
		
	else if (!strcmp(arg, "-d"))
	{
	    opts.do_split = 1;
	}

	else if (*arg == '-')
	{
	    fprintf (stderr, "unknown option: %s\n", arg);
	    exit (1);
	}		
	else if (!opts.srcfile)
	{
	    opts.srcfile = arg;
	}
	else
	{
	    // already seen a filename, so this must be
	    // a shape rep id
	    unsigned eid = atol(arg);
	    if (!eid) {
		printf ("Bad EID: %s\n", arg);
		exit (1);
	    }
	    opts.shape_ids.append(eid);		
	}
    }

    if (!opts.srcfile)
	usage(argv[0]);


    // Read the step file
    opts.design = ROSE.findDesign(opts.srcfile);
    if (!opts.design) {
	printf ("Could not open design %s\n", opts.srcfile);
	exit (2);
    }

    // prepare for working with assemblies
    rose_compute_backptrs (opts.design);
    stix_tag_asms (opts.design);
    stix_tag_units (opts.design);    
    stixmesh_resolve_presentation (opts.design);

    // Find the assembly roots to export.  Given as a list of product
    // definition #IDs or all roots by default.
    //
    unsigned i,sz;
    for (i=0, sz=opts.root_ids.size(); i<sz; i++)
    {
	unsigned long eid = opts.root_ids[i];
	stp_product_definition * pd = ROSE_CAST(
	    stp_product_definition, 
	    opts.design->findByEntityId(eid)
	    );

	if (!pd) {
	    printf ("Could not find product definition #%d\n", eid);
	    exit (2);
	}
	opts.root_prods.append(pd);
    }

    // default to all of the assembly roots if none given 
    if (!opts.root_prods.size()) 
	stix_find_root_products(&opts.root_prods, opts.design);


    // Recursively traverse the root assemblies and write out the
    // faceted data.
    switch (fmt) {
    case FmtTxtSTL:
	return write_ascii_stl(&opts);

    case FmtBinSTL:
	return write_binary_stl(&opts);

    case FmtWebXML:
	return write_webxml(&opts);

	//------------------------------
	// Other lightweight visualization formats can be added here
	// by creating your own write_foo() driver.  You can use the
	// existing drivers as a template.  The webxml driver emits
	// product structure as well as facets, while the STL driver
	// just emits a single collection of facets for everything.
	//
	
    default:
	printf ("No support for format %d\n", (int)fmt);
	return 1;
    }
}
