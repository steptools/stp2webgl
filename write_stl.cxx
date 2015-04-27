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

// write_stl() -- write a single STL file for a STEP model.  This
// facets everything in one pass, and then work on the cached data.
// It then recursively walks down through any assemblies, applying
// transforms to the facet data and writing ASCII STL.
//

extern void facet_all_products (stp2webgl_opts * opts);
extern int write_ascii_stl (stp2webgl_opts * opts);

static void print_mesh_for_product (
    FILE * stlfile,
    stp_product_definition * pd,
    StixMtrx &starting_placement
    );

// ======================================================================

extern int write_ascii_stl (stp2webgl_opts * opts)
{    
    FILE * stlfile = stdout;
    unsigned i,sz;
    
    if (opts->do_split)
    {
	printf ("Only single STL file output currently implemented\n");
	return 2;
    }
    
    if (opts->dstfile)
    {
	stlfile = rose_fopen(opts->dstfile, "w");
	if (!stlfile) {
	    printf ("Could not open output file\n");
	    return 2;
	}
    }

    // Recursively facet all of the products in the root assemblies
    // and attach each resulting mesh to the representation item for
    // each solid.
    //
    facet_all_products(opts);

    
    fputs ("solid ", stlfile);
    if (opts-> dstfile) fputs (opts-> dstfile, stlfile);
    fputs ("\n", stlfile);

    // Now print the mesh details along with placement info
    for (i=0, sz=opts->root_prods.size(); i<sz; i++)
    {
	// The root placement is usually the identity matrix but some
	// systems put a standalone AP3D at the top to place the whole
	// thing in the global space.
	StixMtrx root_placement; 

	print_mesh_for_product (stlfile, opts->root_prods[i], root_placement);
    }

    fputs ("endsolid ", stlfile);
    if (opts-> dstfile) fputs (opts-> dstfile, stlfile);
    fputs ("\n", stlfile);

    return 0;
}



//------------------------------------------------------------
//------------------------------------------------------------
// PRINT THE FACET INFORMATION -- This follows the shape information
// attached to a single product or assembly and prints it to the STL
// file.  This is adapted from the stixmesh facet assembly sample.
//
// Since the shapes are in a tree that parallels the product tree, we
// look for attached next_assembly_usage_occurrences (NAUO) that tell
// us when we are moving into the shape of another product.
//------------------------------------------------------------
//------------------------------------------------------------



static void print_triangle (
    FILE * stlfile,
    const StixMeshFacetSet * fs,
    StixMtrx &xform,
    unsigned facet_num
    )
{
    double v[3];
    double n[3];
    const StixMeshFacet * f = fs-> getFacet(facet_num);
    const char * vertexfmt = "        vertex %.15g %.15g %.15g\n";

    if (!f) return;

    // The components of the triangle verticies and vertex normals are
    // given by an index into internal tables.  Apply the transform so
    // that the facet is placed correctly in the part space.
    //
// facet_normal_now_computed_in_latest_versions
#ifdef LATEST_STDEV
    fs->getFacetNormal(n, f);
    stixmesh_transform_dir (n, xform, n); 
#else
    stixmesh_transform_dir (n, xform, fs-> getNormal(f-> facet_normal));
#endif
    fprintf(stlfile, "facet normal %.15g %.15g %.15g\n", n[0], n[1], n[2]);
    
    fputs("    outer loop\n", stlfile);

    stixmesh_transform (v, xform, fs-> getVertex(f-> verts[0]));
    fprintf(stlfile, vertexfmt, v[0], v[1], v[2]);

    stixmesh_transform (v, xform, fs-> getVertex(f-> verts[1]));
    fprintf(stlfile, vertexfmt, v[0], v[1], v[2]);

    stixmesh_transform (v, xform, fs-> getVertex(f-> verts[2]));
    fprintf(stlfile, vertexfmt, v[0], v[1], v[2]);
    fputs("    endloop\n", stlfile);
    fputs("endfacet\n", stlfile);
}




static void print_mesh_for_shape (
    FILE * stlfile,
    stp_representation * rep,
    StixMtrx &rep_xform
    )
{
    unsigned i, sz;
    unsigned j, szz;

    if (!rep) return;
    
    // Does the rep have any meshed items?  In an assembly, some reps
    // just contain placements for transforming components. If there
    // are solids, we should have previously generated meshes.
    //
    SetOfstp_representation_item * items = rep->items();
    for (i=0, sz=items->size(); i<sz; i++) 
    {
	stp_representation_item  * it = items->get(i);
	StixMeshStp * mesh = stixmesh_cache_find (it);
	if (!mesh) continue;

	const StixMeshFacetSet * fs = mesh-> getFacetSet();

	for (j=0, szz=fs->getFacetCount(); j< szz; j++) {
	    print_triangle (stlfile, fs, rep_xform, j);
	}
    }


    // Go through all of the child shapes which can be attached by a
    // shape_reprepresentation_relationship or a mapped_item.  If the
    // relation has a NAUO associated with it, then it is the start of
    // a different product, otherwise it is still part of the shape of
    // this one.
    //
    StixMgrAsmShapeRep * rep_mgr = StixMgrAsmShapeRep::find(rep);
    if (!rep_mgr) return;

    for (i=0, sz=rep_mgr->child_rels.size(); i<sz; i++) 
    {
	stp_shape_representation_relationship * rel = rep_mgr->child_rels[i];
	stp_representation * child = stix_get_shape_usage_child_rep (rel);

	// Move to location in enclosing asm
	StixMtrx child_xform = stix_get_shape_usage_xform (rel);
	child_xform = child_xform * rep_xform;

	print_mesh_for_shape (stlfile, child, child_xform);
    }


    for (i=0, sz=rep_mgr->child_mapped_items.size(); i<sz; i++) 
    {
	stp_mapped_item * rel = rep_mgr->child_mapped_items[i];
	stp_representation * child = stix_get_shape_usage_child_rep (rel);

	// Move to location in enclosing asm
	StixMtrx child_xform = stix_get_shape_usage_xform (rel);
	child_xform = child_xform * rep_xform;

	print_mesh_for_shape (stlfile, child, child_xform);
    }
}


static void print_mesh_for_product (
    FILE * stlfile,
    stp_product_definition * pd,
    StixMtrx &starting_placement
    ) 
{
    // Print the shape tree for each shape associated with a product,
    // and then follow the shape tree downward.  At each level we
    // check the shape relationship for a link to product relations
    // because shape side because there can be relationships there
    // that are not linked to products.
    //
    unsigned i, sz;
    StixMgrAsmProduct * pm = StixMgrAsmProduct::find(pd);
    if (!pm) return;

    for (i=0, sz=pm->shapes.size(); i<sz; i++) 
    {
	stp_shape_representation * rep = pm->shapes[i];
	print_mesh_for_shape (stlfile, rep, starting_placement);
    }
}



