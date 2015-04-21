/* $RCSfile: $
 * $Revision: $ $Date: $
 * Auth: Jochen Fritz (jfritz@steptools.com)
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

#include <stp_schema.h>
#include <stix.h>
#include <stixmesh.h>

#include <RoseXMLWriter.h>
#include <ctype.h>

#include "stp2webgl.h"


// This function writes a lightweight XML description of the STEP
// product structure and faceted shapes using the format described
// below.
//
// http://www.steptools.com/support/stdev_docs/stixmesh/XMLFormat.html
//
// The product structure is written in a first pass and all of the
// shapes are submitted to the facetter.  Then the shell facets are
// written as they become available and then the facetted data is
// released.  This is a more complex arrangement than just facetting
// everything in one batch, but it may be more memory efficient.
//


extern int write_webxml (stp2webgl_opts * opts);


//======================================================================


static void append_double(RoseXMLWriter * xml, double val)
{
    char buff[64];
    sprintf (buff, "%.15g", val);
    xml->text(buff);
}

static void append_integer(RoseXMLWriter * xml, long val)
{
    char buff[64];
    sprintf (buff, "%ld", val);
    xml->text(buff);
}

static void append_color(RoseXMLWriter * xml, unsigned val)
{
    char buff[] = "rrggbb ";
    sprintf (buff, "%06x", val);
    xml->addAttribute("color", buff);
}


static void append_ref (RoseXMLWriter * xml, RoseObject * obj)
{
    if (obj->isa(ROSE_DOMAIN(RoseUnion))) {
	obj = rose_get_nested_object(ROSE_CAST(RoseUnion, obj));
    }
    
    if (!obj->entity_id()) {
	printf ("No entity id for %p: %s\n", obj, obj->domain()->name());
	exit (2);
    }
   
    char buff[20];
    sprintf (buff, "id%lu", obj->entity_id());
    xml->text(buff);
}


static void append_refatt (
    RoseXMLWriter * xml,
    const char * att,
    RoseObject * obj
    )
{
    if (obj) {
	xml->beginAttribute(att);
	append_ref(xml, obj);
	xml->endAttribute();
    }
}

static FILE * open_dir_file(const char * dir, const char * fname)
{
    RoseStringObject path = dir;
    path.cat("/");
    path.cat(fname);

    return fopen(path, "w");
}


//======================================================================
// STEP PMI Annotations -- GD&T and construction planes
// 



static void write_poly_point(RoseXMLWriter * xml, double vals[3])
{
    xml->beginElement("p");
    xml->beginAttribute ("l");
    append_double(xml, vals[0]);    xml->text(" ");
    append_double(xml, vals[1]);    xml->text(" ");
    append_double(xml, vals[2]);
    xml->endAttribute();
    xml->endElement("p");
}

static void write_poly_point(RoseXMLWriter * xml, ListOfDouble * vals)
{
    xml->beginElement("p");
    xml->beginAttribute ("l");
    append_double(xml, vals->get(0));    xml->text(" ");
    append_double(xml, vals->get(1));    xml->text(" ");
    append_double(xml, vals->get(2));
    xml->endAttribute();
    xml->endElement("p");
}


static void append_step_curve(RoseXMLWriter * xml, stp_curve * c)
{
    /* May also want to handle composite curves here in this routine, since
     * a composite curve may contain via points, and thus run into trouble.*/
    unsigned i,sz;

    if (c->isa(ROSE_DOMAIN(stp_polyline))) {
	
	stp_polyline * poly = ROSE_CAST(stp_polyline, c);
	ListOfstp_cartesian_point * pts = poly->points();
	
	if (!pts || pts->size() < 2) 
	    return;

	xml->beginElement("polyline");
	
	for (i=0, sz=pts->size(); i<sz; i++) {
	    stp_cartesian_point * pt = pts->get(i);
	    ListOfDouble * vals = pt->coordinates();
	    write_poly_point(xml, vals);
	}

	xml->endElement("polyline");
    }

}

static void append_annotation(
    RoseXMLWriter * xml,
    stp_geometric_set * gset
    )
{
    SetOfstp_geometric_set_select * elems = gset->elements();
    if (!elems) return;

    unsigned i,sz;
    for (i=0, sz=elems->size(); i<sz; i++) {
	stp_geometric_set_select * sel = elems->get(i);
	if (sel-> is_curve()) {
	    append_step_curve(xml, sel->_curve());
	}
	else if (sel-> is_point()) {
	}
	else if (sel-> is_surface()) {
	}
	
    }
}

static void append_annotation(
    RoseXMLWriter * xml,
    stp_representation_item * it
    )
{
    if (!it) return;

    if (it-> isa(ROSE_DOMAIN(stp_annotation_plane)))
    {
	// do we do anything special with the plane?
	unsigned i, sz;
	stp_annotation_plane * ap = ROSE_CAST(stp_annotation_plane,it);

	for (i=0, sz=ap->elements()->size(); i<sz; i++)
	{
	    // either draughting_callout or styled_item, both of which
	    // are rep items.
	    stp_representation_item * elem = 
		ROSE_CAST(stp_representation_item,
			  rose_get_nested_object(ap-> elements()-> get(i)));

	    append_annotation (xml, elem);
	}
    }
    
    else if (it-> isa(ROSE_DOMAIN(stp_annotation_occurrence)))
    {
	stp_annotation_occurrence * ao
	    = ROSE_CAST(stp_annotation_occurrence,it);
	
	if (!ao-> item()) return;

	if (ao-> item()-> isa(ROSE_DOMAIN(stp_geometric_set))) {
	    append_annotation(xml, ROSE_CAST(stp_geometric_set,ao->item()));
	}
    }

    else {
	printf("append_annotation unimplemented case: %s\n",
	       it->domain()->name());
    }
    
}


static void append_model_body(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    stp_representation * model)
{
    xml->beginElement("annotation");
    append_refatt (xml, "id", model);
    
    SetOfstp_representation_item * items = model->items();
    unsigned sz = items->size();
    for (unsigned i=0; i<sz; i++) {
	stp_representation_item * it = items->get(i);
	append_annotation(xml, it);
    }

    xml->endElement("annotation");
}


static void append_model(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    stp_representation * model
    )
{
    if (!model || rose_is_marked(model)) return;
    rose_mark_set(model);

    if (!opts->do_split)
	append_model_body(opts, xml, model);
    else
    {
	char fname[100];
	sprintf (fname, "annotation_id%lu.xml", model->entity_id());

	xml->beginElement("annotation");
	append_refatt (xml, "id", model);
	xml->addAttribute("href", fname);
	xml->endElement("annotation");

	FILE * fd = open_dir_file(opts->dstdir, fname);

	RoseOutputFile xmlfile (fd, fname);
	RoseXMLWriter part_xml(&xmlfile);
	part_xml.escape_dots = ROSE_FALSE;
	part_xml.writeHeader();

	append_model_body(opts, &part_xml, model);

	part_xml.close();
	xmlfile.flush();
	fclose(fd);
    }
}


static void append_step_curve(
    RoseXMLWriter * xml,
    stp_representation * rep,
    stp_bounded_curve * curve
    )
{
    StixMeshNurbs nurbs;
    stixmesh_create_bounded_curve(&nurbs, curve, rep);

    StixMeshBoundingBox bbox;
    
    if (!nurbs.getConvexHull(&bbox)) {
	printf ("Could not get convec hull of curve, skipping");
	return;
    }
    double tol = bbox.diagonal() / 100.;

    rose_real_vector u_vals;
    nurbs.extractTolerancedPoints(&u_vals, tol, 1);

    xml->beginElement("polyline");
    
    unsigned i,sz;
    for (i=0, sz=u_vals.size(); i<sz; i++) {
	double xyz[3];
	nurbs.eval(xyz, u_vals[i]);
	write_poly_point(xml, xyz);
    }

    xml->endElement("polyline");    
}

static void append_rep_item(
    RoseXMLWriter * xml,
    stp_representation * rep,
    stp_representation_item * it
    )
{
    if (!it)
	return;

    if (it->isa(ROSE_DOMAIN(stp_bounded_curve))) {
	append_step_curve(xml, rep, ROSE_CAST(stp_bounded_curve, it));
    }

}

static void append_constructive_geom_body(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    stp_constructive_geometry_representation * cgr
    )
{
    unsigned i,sz;
    xml->beginElement("annotation");
    append_refatt (xml, "id", cgr);
    
    SetOfstp_representation_item * items = cgr->items();
    for (i=0, sz=items->size(); i<sz; i++) {
	stp_representation_item * it = items->get(i);
	append_rep_item(xml, cgr, it);
    }

    xml->endElement("annotation");
}


static void append_constructive_geom(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    stp_constructive_geometry_representation * cg
    )
{
    if (!cg || rose_is_marked(cg)) return;
    rose_mark_set(cg);

    if (!opts->do_split)
	append_constructive_geom_body(opts, xml, cg);
    else
    {
	char fname[100];
	sprintf (fname, "constructive_id%lu.xml", cg->entity_id());

	xml->beginElement("annotation");
	append_refatt (xml, "id", cg);
	xml->addAttribute("href", fname);
	xml->endElement("annotation");

	FILE * fd = open_dir_file(opts->dstdir, fname);

	RoseOutputFile xmlfile (fd, fname);
	RoseXMLWriter part_xml(&xmlfile);
	part_xml.escape_dots = ROSE_FALSE;
	part_xml.writeHeader();

	append_constructive_geom_body(opts, &part_xml, cg);

	part_xml.close();
	xmlfile.flush();
	fclose(fd);
    }
}



static void append_annotation_refs(
    RoseXMLWriter * xml,
    stp_representation * rep
    )
{
    /* FIXME - this generates both constructive geometry and
     * annotations.  theses should be split out, but are are not
     * (currently) doing so since that would require updating the
     * webgl javascript.
     */
    unsigned i,sz;
    unsigned cnt = 0;
    
    if (!rep->isa(ROSE_DOMAIN(stp_shape_representation)))
	return;

    StixMeshRepresentationVec * models = stixmesh_get_draughting_models(
	ROSE_CAST(stp_shape_representation, rep)
	);

    StixMeshConstructiveGeomVec * cgeom =
	stixmesh_get_constructive_geometry(rep);
    
    if (!models && !cgeom)
	return;

    xml->beginAttribute("annotation");
    
    if (models) {
	for (i=0, sz=models->size(); i<sz; i++) {
	    if (cnt) xml->text (" ");
	    append_ref(xml, models->get(i));
	    cnt++;
	}
    }

    if (cgeom) {
	for (i=0, sz=cgeom->size(); i<sz; i++) {
	    if (cnt) xml->text (" ");
	    append_ref(xml, cgeom->get(i));
	    cnt++;
	}
    }
    
    xml->endAttribute();
}



static void append_annotations(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    stp_representation * rep
    )
{
    unsigned i, sz;
    
    if (!rep->isa(ROSE_DOMAIN(stp_shape_representation)))
	return;

    StixMeshRepresentationVec * models = stixmesh_get_draughting_models(
	ROSE_CAST(stp_shape_representation, rep));

    StixMeshConstructiveGeomVec * cgeom =
	stixmesh_get_constructive_geometry(rep);
        
    if (!models && !cgeom)
	return;

    if (models) {
	for (i=0, sz = models->size(); i<sz; i++) {
	    stp_representation * model = models->get(i);
	    append_model(opts, xml, model);
	}
    }

    if (cgeom) {
	for (i=0, sz = cgeom->size(); i<sz; i++) {
	    stp_constructive_geometry_representation * cg = cgeom->get(i);
	    append_constructive_geom(opts, xml, cg);
	}
    }
    
}


//======================================================================
// Queue STEP Representation -- Add shape data to facetter queue and
// write forward information about the step shell.
//
void append_shell_refs(
    RoseXMLWriter * xml,
    stp_representation * rep
    )
{
    unsigned i,sz;
    unsigned count=0;
    SetOfstp_representation_item * items = rep->items();

    for (i=0, sz=items->size(); i<sz; i++)
    {
	stp_representation_item * it = items->get(i);
	if (!StixMeshStpBuilder::isShell(rep, it))
	    continue;

	if (!count) xml->beginAttribute ("shell");
	if (count++) xml->text (" ");
	append_ref (xml, it);
    }

    if (count) xml->endAttribute();
}


static void append_asm_child(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    RoseObject * rel
    )
{
    StixMgrAsmRelation * mgr = StixMgrAsmRelation::find(rel);
    if (!mgr) return;

    stp_representation * child = mgr->child;

    xml->beginElement("child");
    append_refatt (xml, "ref", child);

    unsigned i,j;
    StixMtrx xform = stix_get_transform(mgr);
    xml->beginAttribute ("xform");
    for (i=0; i<4; i++) {
	for (j=0; j<4; j++) {
	    if (i || j) xml->text(" ");
	    append_double(xml, xform.get(j,i));
	}
    }
    xml->endAttribute();
    xml->endElement("child");
}


void queue_shapes(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    StixMeshStpAsyncMaker * mesher,
    stp_representation * rep
    )
{
    unsigned i, j, sz;

    if (!rep || rose_is_marked(rep)) return;
    rose_mark_set(rep);

    StixMgrAsmShapeRep * mgr = StixMgrAsmShapeRep::find(rep);
    if (!mgr) return;

    // Write facets by default, unless we have a list of reps.  In the
    // latter case only write facets if a rep is in the list.
    
    int do_facets = 1;
    if (opts->root_ids.size())
    {
	unsigned eid = rep->entity_id();
	do_facets = 0;
	
	for (i=0, sz=opts->root_ids.size(); i<sz; i++)
	{
	    if (opts->root_ids[i] == eid) {
		do_facets = 1;
		break;
	    }
	}	
    }    

    xml->beginElement("shape");
    append_refatt (xml, "id", rep);

    StixUnit unit = stix_get_context_length_unit(rep);
    if (unit != stixunit_unknown)
    {
	xml->beginAttribute("unit");
	xml->text (stix_get_unit_name(unit));
	char buff[20];
	sprintf (buff, " %f", stix_get_converted_measure(1., unit, stixunit_m));
	xml->text(buff);    
	xml->endAttribute();
    }
    
    if (do_facets)
	append_shell_refs(xml, rep);

    append_annotation_refs(xml, rep);
    
    for (j=0, sz=mgr->child_rels.size(); j<sz; j++) 
	append_asm_child(opts, xml, mgr->child_rels[j]);

    for (j=0, sz=mgr->child_mapped_items.size(); j<sz; j++) 
	append_asm_child(opts, xml, mgr->child_mapped_items[j]);

    xml->endElement("shape");

    for (j=0, sz=mgr->child_rels.size(); j<sz; j++) {
	StixMgrAsmRelation * rm = StixMgrAsmRelation::find(
	    mgr->child_rels[j]
	    );
	if (rm) queue_shapes(opts, xml, mesher, rm->child);
    }

    for (j=0, sz=mgr->child_mapped_items.size(); j<sz; j++) {
	StixMgrAsmRelation * rm = StixMgrAsmRelation::find(
	    mgr->child_mapped_items[j]
	    );
	if (rm) queue_shapes(opts, xml, mesher, rm->child);
    }

    
    if (do_facets)
    {
	SetOfstp_representation_item * items = rep->items();
	unsigned sz = items->size();

	for (unsigned i=0; i<sz; i++) {
	    stp_representation_item * ri = items->get(i);

	    if (StixMeshStpBuilder::canMake(rep, ri))
		mesher->startMesh(rep, ri, &opts->mesh);
	}    

	append_annotations(opts, xml, rep);
    }
}



//======================================================================
// Write Triangle Data for a Facetted Shell
//
static void append_facet(
    RoseXMLWriter * xml,
    const StixMeshFacetSet * fs,
    unsigned fidx,
    int write_normal
    )
{
    const StixMeshFacet * f = fs->getFacet(fidx);
    if (!f) return;
    
    xml->beginElement("f");		
    xml->beginAttribute("v");
    append_integer(xml, f->verts[0]);    xml->text(" ");
    append_integer(xml, f->verts[1]);    xml->text(" ");
    append_integer(xml, f->verts[2]);
    xml->endAttribute();    

    if (write_normal) {
	double normal[3];
	fs->getFacetNormal(normal, fidx);

	xml->beginAttribute("fn");
	append_double(xml, normal[0]);    xml->text(" ");
	append_double(xml, normal[1]);    xml->text(" ");
	append_double(xml, normal[2]);
	xml->endAttribute();    
    }
    
    for (unsigned j=0; j<3; j++) {
	const double * normal = fs->getNormal(f->normals[j]);
	if (normal)
	{
	    xml->beginElement("n");
	    xml->beginAttribute("d");
	    append_double(xml, normal[0]);    xml->text(" ");
	    append_double(xml, normal[1]);    xml->text(" ");
	    append_double(xml, normal[2]);
	    xml->endAttribute();    
	    xml->endElement("n");
	}
    }
    xml->endElement("f");
}


void append_shell_facets(
    RoseXMLWriter * xml,
    const StixMeshStp * shell
    )
{
    int WRITE_NORMAL = 0;
    unsigned i,sz;
    unsigned j,szz; 
    const StixMeshFacetSet * facets = shell->getFacetSet();
    
    xml->beginElement("shell");
    append_refatt(xml, "id", shell->getStepSolid());

    unsigned dflt_color = stixmesh_get_color (shell->getStepSolid());
    if (dflt_color != STIXMESH_NULL_COLOR) 
	append_color(xml, dflt_color);
    
    xml->beginElement("verts");
    for (i=0, sz=facets->getVertexCount(); i<sz; i++)
    {
	const double * pt = facets->getVertex(i);
	xml->beginElement("v");
	xml->beginAttribute("p");
	append_double(xml, pt[0]);    xml->text(" ");
	append_double(xml, pt[1]);    xml->text(" ");
	append_double(xml, pt[2]);
	xml->endAttribute();    
	xml->endElement("v");
    }
    xml->endElement("verts");


    // The facet set has all of the facets for the shell.  Break it up
    // into groups by step face.
   
    for (i=0, sz=shell->getFaceCount(); i<sz; i++)
    {
	const StixMeshStpFace * fi = shell->getFaceInfo(i);
	unsigned first = fi->getFirstFacet();
	unsigned color = stixmesh_get_color(fi->getFace());
	if (first == ROSE_NOTFOUND)
	    continue;
	
	xml->beginElement("facets");

	// Always tag the face with a color, unless everything is null. 
	if (color == STIXMESH_NULL_COLOR)
	    color = dflt_color;

	if (color != STIXMESH_NULL_COLOR) 
	    append_color(xml, color);

	for (j=0, szz=fi->getFacetCount(); j<szz; j++) {
	    append_facet(xml, facets, j+first, WRITE_NORMAL);
	}
	xml->endElement("facets");
    }
    
    xml->endElement("shell");
}


static void export_shell(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    const StixMeshStp * shell
    )
{
    if (!shell) return;

    if (!opts->do_split) {
	append_shell_facets(xml, shell);
    }
    else
    {
	unsigned i,sz;
	StixMeshBoundingBox bbox;
	const StixMeshFacetSet * facets = shell->getFacetSet();

	// compute the bounding box for the shell
	for (i=0, sz=facets->getVertexCount(); i<sz; i++)
	{
	    const double * pt = facets->getVertex(i);
	    bbox.update(pt);
	}

	char fname[100];
	sprintf (fname, "shell_id%lu.xml", shell->getStepSolid()->entity_id());

	xml->beginElement("shell");
	append_refatt(xml, "id", shell->getStepSolid());

	xml->beginAttribute("size");
	append_integer(xml, facets->getFacetCount());
	xml->endAttribute();
	
	xml->beginAttribute("bbox");
	append_double(xml, bbox.minx);    xml->text(" ");
	append_double(xml, bbox.miny);    xml->text(" ");
	append_double(xml, bbox.minz);    xml->text(" ");
	append_double(xml, bbox.maxx);    xml->text(" ");
	append_double(xml, bbox.maxy);    xml->text(" ");
	append_double(xml, bbox.maxz);
	xml->endAttribute();

	// append the area 
	double area = 0.;
	for (unsigned i=0, sz=shell->getFaceCount(); i<sz; i++) {
	    const StixMeshStpFace * face = shell->getFaceInfo(i);
	    area += face->getArea();
	}
	xml->beginAttribute("a");
	append_double (xml, area);
	xml->endAttribute();    

	xml->addAttribute("href", fname);
	xml->endElement("shell");

	/* Write the shell in its own XML file */
	FILE * fd = open_dir_file(opts->dstdir, fname);
	RoseOutputFile xmlfile (fd, fname);
	RoseXMLWriter shell_xml(&xmlfile);
	shell_xml.escape_dots = ROSE_FALSE;
	shell_xml.writeHeader();

	append_shell_facets(&shell_xml, shell);

	shell_xml.close();
	xmlfile.flush();
	fclose(fd);
    }
}




//======================================================================
// Write STEP Product Structure
//

static const char * get_product_name(stp_next_assembly_usage_occurrence* nauo)
{
    stp_product_definition * pd = stix_get_related_pdef(nauo);
    stp_product_definition_formation * pdf = pd? pd->formation(): 0;
    stp_product * prod = pdf? pdf->of_product(): 0;

    return prod? prod->name(): 0;
}

int nauo_product_cmp(const void* a, const void* b)
{
    const char * name_a = get_product_name(
	*(stp_next_assembly_usage_occurrence**) a
	);

    const char * name_b = get_product_name(
	*(stp_next_assembly_usage_occurrence**) b
	);

    if (name_a == name_b) return 0;
    if (!name_a) return +1;
    if (!name_b) return -1;

    return strcmp(name_a, name_b);
}

static void append_stplink (
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    stp_product_definition * pd
    )
{
    // write reference to a component stepfile.  Only used when
    // splitting a large assembly into a a collection of part files
    // and when a file exists for this particular product.
    // 
    if (!opts->do_split) return;

    RoseStringObject name ("part");

    stp_product_definition_formation * pdf = pd-> formation();
    stp_product * p = pdf? pdf-> of_product(): 0;

    char * pname = p? p-> name(): 0;
    if (!pname || !*pname) pname = (char *) "none";
	
    if (!name.is_empty()) name += "_";
    name += pname;

    // change whitespace and other non filesystem safe
    // characters to underscores
    //
    char * c = name;
    while (*c) { 
	if (isspace(*c)) *c = '_'; 
	if (*c == '?')   *c = '_'; 
	if (*c == '/')   *c = '_'; 
	if (*c == '\\')  *c = '_'; 
	if (*c == ':')   *c = '_'; 
	if (*c == '"')   *c = '_'; 
	if (*c == '\'')  *c = '_'; 
	c++; 
    }

    if (pd-> design()-> fileExtension()) {
	name += ".";
	name += pd-> design()-> fileExtension();
    }

    RoseStringObject path = opts->dstdir;
    path += "/";
    path += name;

    // expects the component files to be present already.  Do not
    // generate link if not there.
    if (rose_file_exists(path)) {
	xml->addAttribute("step", name);
    }
}


static void export_product(
    stp2webgl_opts * opts,
    RoseXMLWriter * xml,
    stp_product_definition * pd
    )
{
    unsigned i,sz;

    if (!pd || rose_is_marked(pd)) return;
    rose_mark_set(pd);
    
    StixMgrAsmProduct * mgr = StixMgrAsmProduct::find(pd);    
    
    xml->beginElement("product");
    append_refatt(xml, "id", pd);
    append_stplink (opts, xml, pd);
    
    stp_product_definition_formation * pdf = pd->formation();
    stp_product * prod = pdf->of_product();

    xml->addAttribute("name", prod->name());

    if (mgr->shapes.size()) {
	xml->beginAttribute("shape");
	for (i=0, sz=mgr->shapes.size(); i<sz; i++)
	{
	    if (i > 0) xml->text(" ");
	    append_ref(xml, mgr->shapes[i]);
	}
	xml->endAttribute();
    }

    if (mgr->child_nauos.size())
    {
	qsort(mgr->child_nauos._buffer(),
	      mgr->child_nauos.size(),
	      sizeof(stp_next_assembly_usage_occurrence*),
	      &nauo_product_cmp
	    );

	xml->beginAttribute("children");

	for (i=0, sz=mgr->child_nauos.size(); i<sz; i++)
	{
	    if (i > 0) xml->text(" ");
	    stp_next_assembly_usage_occurrence * nauo = mgr->child_nauos[i];
	    append_ref(xml, stix_get_related_pdef(mgr->child_nauos[i]));
	}
	xml->endAttribute();
    }
    xml->endElement("product");

    for (i=0, sz=mgr->child_nauos.size(); i<sz; i++)
    {
	stp_next_assembly_usage_occurrence * nauo = mgr->child_nauos[i];
	export_product(opts, xml, stix_get_related_pdef(nauo));
    }
}


// ======================================================================


int write_webxml (stp2webgl_opts * opts)
{    
    FILE * xmlout = 0;
    RoseStringObject index_file;
    unsigned i,sz;
    
    if (opts->do_split)
    {
	opts->dstdir = opts->dstfile;
	if (!opts->dstdir)
	    opts->dstdir = "step_data";

	index_file = opts->dstdir;
	index_file.cat("/index.xml");
	opts->dstfile = index_file;

	if (!rose_dir_exists (opts->dstdir) &&
	    (rose_mkdir(opts->dstdir) != 0)) {
	    printf ("Cannot create directory %s\n", opts->dstdir);
	    return 2;
	}
    }
    
    if (opts->dstfile)
    {
	xmlout = rose_fopen(opts->dstfile, "w");
	if (!xmlout) {
	    printf ("Could not open output file\n");
	    return 2;
	}
    }
	
    if (xmlout == 0) {
	xmlout = stdout;
    }

    // The XML writer class is a simple class that handles tag and
    // attribute writing.  The RoseOutputFile class is a data stream
    // class that the XML file writes to.
    // 
    RoseOutputFile xmlfile(xmlout, opts->dstfile? opts->dstfile: "xml file");
    RoseXMLWriter xml(&xmlfile);
    xml.escape_dots = ROSE_FALSE;
    xml.writeHeader();
    xml.beginElement("step-assembly");
    
    xml.beginAttribute("root");
    for (i=0, sz=opts->root_prods.size(); i<sz; i++)
    {
	stp_product_definition * pd = opts->root_prods[i];
	if (i > 0) xml.text(" ");
	append_ref(&xml, pd);
    }
    xml.endAttribute();

    rose_mark_begin();
    for (i=0, sz=opts->root_prods.size(); i<sz; i++)
    {
	export_product(opts, &xml, opts->root_prods[i]);
    }

    // Schedule each solid for faceting, which will happen in child
    // threads and then write each shell as it becomes available.
    StixMeshStpAsyncMaker mesher;
    StixMeshStp * mesh;

    for (i=0, sz=opts->root_prods.size(); i<sz; i++)
    {
	unsigned j, szz;
	StixMgrAsmProduct * mgr = StixMgrAsmProduct::find(
	    opts->root_prods[i]
	    );
	
	for (j=0, szz=mgr->shapes.size(); j<szz; j++) {
	    queue_shapes (opts, &xml, &mesher, mgr->shapes[j]);
	}
    }

    while ((mesh = mesher.getResult(1)) != 0)
    {
	export_shell(opts, &xml, mesh);
	delete mesh;
    }

    xml.endElement("step-assembly");
    xml.close();
    xmlfile.flush();
    rose_mark_end();

    return 0;
}

