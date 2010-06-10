// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// y10 m02 d15

#include "lib_grid/algorithms/attachment_util.h"
#include "selector_interface.h"

namespace ug
{
	
ISelector::ISelector(uint supportedElements) :
	m_aIterator(false)
{
	m_pGrid = NULL;
	m_supportedElements = supportedElements;
	m_bAutoselectionEnabled = false;
	m_bSelectionInheritanceEnabled = true;
	m_invalidContainer.push_back(NULL);
	m_invalidIterator = m_invalidContainer.begin();
}

ISelector::ISelector(Grid& grid, uint supportedElements) :
	m_aIterator(false)
{
	m_pGrid = &grid;
	m_supportedElements = SE_NONE;
	m_bAutoselectionEnabled = false;
	m_bSelectionInheritanceEnabled = true;
	m_invalidContainer.push_back(NULL);
	m_invalidIterator = m_invalidContainer.begin();

//	register at grid. Don't use set_grid here since it invokes virtual methods.
	if(m_pGrid){
		m_pGrid->register_observer(this, OT_GRID_OBSERVER | OT_VERTEX_OBSERVER | OT_EDGE_OBSERVER |
									OT_FACE_OBSERVER | OT_VOLUME_OBSERVER);
									
	//	initialise attachments and accessors.
		enable_element_support(supportedElements);
	}
}

ISelector::~ISelector()
{
//	unregister from grid
//	don't use set_grid here, since it invokes virtual methods.
	if(m_pGrid){
	//	disable all currently supported elements (this will remove any attachments)
		disable_element_support(m_supportedElements);
		m_pGrid->unregister_observer(this);
		m_pGrid = NULL;
	}
}

void ISelector::
set_supported_elements(uint shElements)
{
//	do this in two steps:
//	1: disable the element-support that is no longer required.
//	2: enable the element-support that was not already enabled.
//	disable the elements that shall be disabled.

//	(the ones which shall not be set, but are currently active.)
	disable_element_support((~shElements) & m_supportedElements);

//	enable the elements that are not already enabled
	enable_element_support(shElements & (~m_supportedElements));
}

void ISelector::enable_element_support(uint shElements)
{
//	if no grid is assigned, we can't do anything.
	if(m_pGrid){
	//	check for each option whether it should be enabled.
	//	to reduce unnecessary operations, we have to make sure that
	//	that option hasn't already been enabled.

		if((shElements & SE_VERTEX) &&
			(!elements_are_supported(SE_VERTEX))){
//LOG("enabling vertex support\n");
		//	enable vertex-support.
			m_pGrid->attach_to_vertices(m_aIterator);
			m_aaIterVRT.access(*m_pGrid, m_aIterator);
			SetAttachmentValues(m_aaIterVRT, m_pGrid->begin<VertexBase>(),
								m_pGrid->end<VertexBase>(), m_invalidIterator);
			m_supportedElements |= SE_VERTEX;
		}

		if((shElements & SE_EDGE) &&
			(!elements_are_supported(SE_EDGE))){
//LOG("enabling edge support\n");
		//	enable edge support
			m_pGrid->attach_to_edges(m_aIterator);
			m_aaIterEDGE.access(*m_pGrid, m_aIterator);
			SetAttachmentValues(m_aaIterEDGE, m_pGrid->begin<EdgeBase>(),
								m_pGrid->end<EdgeBase>(), m_invalidIterator);
			m_supportedElements |= SE_EDGE;
		}

		if((shElements & SE_FACE) &&
			(!elements_are_supported(SE_FACE))){
//LOG("enabling face support\n");
		//	enable face support
			m_pGrid->attach_to_faces(m_aIterator);
			m_aaIterFACE.access(*m_pGrid, m_aIterator);
			SetAttachmentValues(m_aaIterFACE, m_pGrid->begin<Face>(),
								m_pGrid->end<Face>(), m_invalidIterator);
			m_supportedElements |= SE_FACE;
		}

		if((shElements & SE_VOLUME) &&
			(!elements_are_supported(SE_VOLUME))){
//LOG("enabling volume support\n");
		//	enable volume support
			m_pGrid->attach_to_volumes(m_aIterator);
			m_aaIterVOL.access(*m_pGrid, m_aIterator);
			SetAttachmentValues(m_aaIterVOL, m_pGrid->begin<Volume>(),
								m_pGrid->end<Volume>(), m_invalidIterator);
			m_supportedElements |= SE_VOLUME;
		}
	}
}

void ISelector::disable_element_support(uint shElements)
{
//	if no grid is assigned, we can't do anything.
	if(m_pGrid){
	//	check for each option whether it should be disabled.
	//	to reduce unnecessary operations, we have to make sure that
	//	that option hasn't already been disabled.

		if((shElements & SE_VERTEX) && elements_are_supported(SE_VERTEX)){
//LOG("disabling vertex support\n");
			m_pGrid->detach_from_vertices(m_aIterator);
		}

		if((shElements & SE_EDGE) && elements_are_supported(SE_EDGE)){
//LOG("disabling edge support\n");
			m_pGrid->detach_from_edges(m_aIterator);
		}

		if((shElements & SE_FACE) && elements_are_supported(SE_FACE)){
//LOG("disabling face support\n");
			m_pGrid->detach_from_faces(m_aIterator);
		}

		if((shElements & SE_VOLUME) && elements_are_supported(SE_VOLUME)){
//LOG("disabling volume support\n");
			m_pGrid->detach_from_volumes(m_aIterator);
		}
	}

//	remove the disabled elements from the set of currently supported elements.
	m_supportedElements &= (~shElements);
}

void ISelector::enable_autoselection(bool bEnable)
{
	m_bAutoselectionEnabled = bEnable;
}

void ISelector::enable_selection_inheritance(bool bEnable)
{
	m_bSelectionInheritanceEnabled = bEnable;
}



void ISelector::set_grid(Grid* grid)
{
//	if we're already registered at this grid then return
	if(m_pGrid == grid)
		return;
		
//	if we're already registered at a grid unregister first.
	if(m_pGrid){
	//	disable all currently supported elements (this will remove any attachments)
		clear();
		disable_element_support(m_supportedElements);
		m_pGrid->unregister_observer(this);
		m_pGrid = NULL;
	}

//	if the new grid is not empty, we'll initialise and register
	if(grid){
		grid->register_observer(this, OT_GRID_OBSERVER | OT_VERTEX_OBSERVER | OT_EDGE_OBSERVER |
									OT_FACE_OBSERVER | OT_VOLUME_OBSERVER);
		m_pGrid = grid;

	//	initialise attachments and accessors.
	//	do this whith a little trick:
	//	set the supported-element-options to SE_NONE,
	//	then call enable for all element-types that should be supported.
		uint tmpOpts = m_supportedElements;
		m_supportedElements = SE_NONE;
		enable_element_support(tmpOpts);
	}
}

////////////////////////////////////////////////////////////////////////
//	grid callbacks
/*
void ISelector::registered_at_grid(Grid* grid)
{
//	if we're already registered at this grid then return
	if(m_pGrid == grid)
		return;

//	if we're already registered at a grid, then unregister first
	if(m_pGrid)
		m_pGrid->unregister_observer(this);

//	assign grid
	m_pGrid = grid;

//	initialise attachments and accessors.
//	do this whith a little trick:
//	set the supported-element-options to SE_NONE,
//	then call enable for all element-types that should be supported.
	uint tmpOpts = m_supportedElements;
	m_supportedElements = SE_NONE;
	enable_element_support(tmpOpts);
}

void ISelector::unregistered_from_grid(Grid* grid)
{
	assert(m_pGrid == grid && "grids do not match!");

	if(m_pGrid == grid){
	//	disable all currently supported elements (this will remove any attachments)
		disable_element_support(m_supportedElements);
		m_pGrid = NULL;
	}
}
*/
void ISelector::grid_to_be_destroyed(Grid* grid)
{
	assert(m_pGrid == grid && "grids do not match!");

	if(m_pGrid == grid){
		set_grid(NULL);
	}
}

void ISelector::elements_to_be_cleared(Grid* grid)
{
	clear();
}

//	vertex callbacks
void ISelector::vertex_created(Grid* grid, VertexBase* vrt,
									GeometricObject* pParent)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_VERTEX)){
	//	init the element
		mark_deselected(vrt);
		if(autoselection_enabled())
			select(vrt);
		else if((pParent != NULL) && selection_inheritance_enabled()){
			if(is_selected(pParent))
				select(vrt);
		}
	}
}

void ISelector::vertex_to_be_erased(Grid* grid, VertexBase* vrt)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_VERTEX)){
		deselect(vrt);
	}
}

//	edge callbacks
void ISelector::edge_created(Grid* grid, EdgeBase* edge,
								  GeometricObject* pParent)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_EDGE)){
	//	init the element
		mark_deselected(edge);
		if(autoselection_enabled())
			select(edge);
		else if((pParent != NULL) && selection_inheritance_enabled()){
			if(is_selected(pParent))
				select(edge);
		}
	}
}

void ISelector::edge_to_be_erased(Grid* grid, EdgeBase* edge)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_EDGE)){
		deselect(edge);
	}
}

//	face callbacks
void ISelector::face_created(Grid* grid, Face* face,
								  GeometricObject* pParent)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_FACE)){
	//	init the element
		mark_deselected(face);
		if(autoselection_enabled())
			select(face);
		else if((pParent != NULL) && selection_inheritance_enabled()){
			if(is_selected(pParent))
				select(face);
		}
	}
}

void ISelector::face_to_be_erased(Grid* grid, Face* face)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_FACE)){
		deselect(face);
	}
}

//	volume callbacks
void ISelector::volume_created(Grid* grid, Volume* vol,
									GeometricObject* pParent)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_VOLUME)){
	//	init the element
		mark_deselected(vol);
		if(autoselection_enabled())
			select(vol);
		else if((pParent != NULL) && selection_inheritance_enabled()){
			if(is_selected(pParent))
				select(vol);
		}
	}
}

void ISelector::volume_to_be_erased(Grid* grid, Volume* vol)
{
	assert((m_pGrid == grid) && "grids do not match.");
	
//TODO: this if could be removed if the subset-handler was only registered for
//		the elements that it supports. Note that a dynamic register/unregister
//		would be required...
	if(elements_are_supported(SE_VOLUME)){
		deselect(vol);
	}
}

}//	end of namespace
