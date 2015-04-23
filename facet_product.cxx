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

#include <stix.h>
#include <stixmesh.h>

#include <stp_representation.h>
#include <stp_product_definition.h>
#include <stp_shape_representation.h>

#include "stp2webgl.h"


// FACET THE SHAPE INFORMATION -- This follows the tree of shape
// information attached to a product and attempts to build a faceted
// version of each shape rep.  In an assembly, some reps only contain
// placements for subcomponents, so they will not have a mesh.
//
static void facet_shape_tree(
    stp2webgl_opts * opts, 
    StixMeshStpAsyncMaker * mesher,
    stp_representation * rep
    )
{
    unsigned i,sz;

    if (!rep || rose_is_marked(rep)) return;
    rose_mark_set(rep);

    // Find all of the solids and schedule them for mesh creation.
    // Later we will retrieve the completed meshes and attach them to
    // the STEP object.  Normally, we must delete the returned mesh
    // when finished, but the cache functions take care of that and
    // delete the cached mesh when the STEP data is deleted.
    //
    SetOfstp_representation_item * items = rep->items();
    for (i=0, sz=items->size(); i<sz; i++) 
    {
	stp_representation_item  * it = items->get(i);
	
	if (!StixMeshStpBuilder::canMake(rep, it)) 
	    continue;
    
	// Chance that it might have been previously faceted if it is
	// somehow reused by a different part.
	if (stixmesh_cache_find(it))
	    continue;

	if (mesher-> startMesh(rep, it, &opts->mesh)) {
	    // printf ("Solid #%lu started\n", it-> entity_id());
	}
    }


    // Now look for attached shapes
    StixMgrAsmShapeRep * rep_mgr = StixMgrAsmShapeRep::find(rep);
    if (!rep_mgr) return;


    // All shapes attached by a representation_relationship
    for (i=0, sz=rep_mgr->child_rels.size(); i<sz; i++) 
    {
	stp_shape_representation_relationship * rel = rep_mgr->child_rels[i];
	stp_representation * child = stix_get_shape_usage_child_rep (rel);

	facet_shape_tree(opts, mesher, child);
    }


    // All shapes attached by mapped_item
    for (i=0, sz=rep_mgr->child_mapped_items.size(); i<sz; i++) 
    {
	stp_mapped_item * rel = rep_mgr->child_mapped_items[i];
	stp_representation * child = stix_get_shape_usage_child_rep (rel);

	facet_shape_tree (opts, mesher, child);
    }
}



static void facet_product(
    stp2webgl_opts * opts, 
    StixMeshStpAsyncMaker * mesher,
    stp_product_definition * pd
    )
{
    if (!pd || rose_is_marked(pd)) return;
    rose_mark_set(pd);

    unsigned i,sz;
    StixMgrAsmProduct * pd_mgr = StixMgrAsmProduct::find(pd);
    if (!pd_mgr) return;  // not a proper part

    // facet all direct shapes and any related shapes.
    for (i=0, sz=pd_mgr->shapes.size(); i<sz; i++) 
    {
	stp_representation * rep = pd_mgr->shapes[i];
	facet_shape_tree (opts, mesher, rep);
    }
}


extern void facet_all_products (
    stp2webgl_opts * opts
    )
{
    // Mesh the geometry present in each assembly
    StixMeshStpAsyncMaker mesher;
    unsigned i,sz;

    rose_mark_begin();
    for (i=0, sz=opts->root_prods.size(); i<sz; i++)
    {
	facet_product (opts, &mesher, opts->root_prods[i]);
    }
    rose_mark_end();

    // Now collect the meshed representations as they are completed.
    // The getResults() function takes an argument that tells it
    // whether to block or poll.  Here we block, but you may want to
    // poll if your application is also servicing a UI.
    //
    StixMeshStp * mesh;
    while ((mesh = mesher.getResult(1)) != 0)
    {
	stp_representation * rep = mesh-> getRepresentation();
	stp_representation_item * it = mesh->getStepSolid();
	stixmesh_cache_add (it, mesh);

	// printf ("Rep #%lu, Solid #%lu completed\n", 
	//  rep-> entity_id(), it-> entity_id()
	// );
    }
}
