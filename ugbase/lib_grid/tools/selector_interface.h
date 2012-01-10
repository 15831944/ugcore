// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// y10 m02 d15

#ifndef __H__LIBGRID__SELECTOR_INTERFACE__
#define __H__LIBGRID__SELECTOR_INTERFACE__

#include <cassert>
#include "lib_grid/grid/grid.h"
#include "lib_grid/grid/geometric_object_callbacks.h"

namespace ug
{
///	\addtogroup lib_grid
/// \{

////////////////////////////////////////////////////////////////////////
//	SelectorElements
///	Use these constants to specify which elements shall be supported by a Selector.
/**
 * You may combine the constants using or-operations.
 */
enum SelectorElements
{
	SE_NONE = 0,
	SE_VERTEX = 1,
	SE_EDGE = 1<<1,
	SE_FACE = 1<<2,
	SE_VOLUME = 1 << 3,
	SE_ALL = SE_VERTEX | SE_EDGE | SE_FACE | SE_VOLUME
};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	ISelector
///	base-implementation of a selector
/**
 * A selector is a useful class, that allows the user to mark
 * elements of a grid as selected or deselected.
 * The selection status is maintained even if new elements
 * are created or old ones deleted. Features like
 * autoselection and selection_inheritance allow users to
 * follow the creation and removal of elements in all kind of
 * algorithms.
 *
 * Derived classes like Selector and MGSelector or
 * VertexSelector, EdgeSelector, FaceSelector and VolumeSelector
 * keep selected elements in linked lists and allow the user
 * to iterate over them. If a selected element is removed
 * from the underlying grid, it is automatically removed
 * from the element-lists of all associated selectors.
 *
 * Please note that the selector has to be registered at a
 * grid before it may be used. Derived classes may use the
 * protected method set_grid to do so, or pass a grid directly to
 * the constructor.
 *
 * This is a base implementation that can not be instatiated.
 * It features most cruical methods like is_selected,
 * select, deselect etc.
 * 
 * autoselection and selection_inheritance can be enabled here.
 * If autoselection is enabled, then all elements that are created
 * while the selector is registered at a grid will automatically
 * be selected.
 *
 * If selection_inheritance is enabled, then all elements that are
 * created while the selector is registered at a grid, and whose
 * parents (from the same grid) are selected will be automatically
 * selected, too. selection_inheritance is ignored if autoselection
 * is enabled.
 *
 * Support for different element-types can be enabled / disabled
 * by derived classes using set_supported_elements,
 * enable_element_support or disable_element_support.
 *
 * Derived classes have to store the selected elements in linked lists
 * of type ISelector::SectionContainer and should feature begin and
 * end itertors to the sections of those lists.
 *
 * ISelector communicates with derived classes via the methods
 * add_to_list and erase_from_list.
 *
 * Especially for the implementation of erase_from_list the protected
 * member function get_iterator may be useful, which returns the
 * iterator that is associated with the selected element.
 *
 * If a derived class wants to change the selection-status of an
 * element behind the scenes (avoids select, deselect), then it
 * may use the protected methods mark_selected, mark_deselected.
 */
class ISelector : public GridObserver
{
	public:
		typedef byte	status_t;

		enum{
			DESELECTED = 0,
			SELECTED = 1
		};

	public:
		ISelector(uint supportedElements = SE_ALL);
		ISelector(Grid& grid, uint supportedElements = SE_ALL);
		virtual ~ISelector();

	//	virtual methods
		virtual void clear() = 0;

	//	selection
	///	selects an element
	/**	You may optionally pass a status-flag. Note that 0 is reserved for
	 * non-selected elements. status thus has to be bigger then 0.
	 * \{
	 */
		inline void select(GeometricObject* elem, byte status = 1);

		template <class TElem>
		inline void select(TElem* elem, byte status = 1);

		template <class TIterator>
		inline void select(TIterator iterBegin, TIterator iterEnd, byte status = 1);
	/**	\} */
		
	//	deselection
		inline void deselect(GeometricObject* elem);
		
		template <class TElem>
		inline void deselect(TElem* elem);
		
		template <class TIterator>
		inline void deselect(TIterator iterBegin, TIterator iterEnd);

	//	selection status
		inline byte get_selection_status(GeometricObject* elem) const;
		inline byte get_selection_status(VertexBase* vrt) const	{if(!elements_are_supported(SE_VERTEX)) return 0; return m_aaSelVRT[vrt];}
		inline byte get_selection_status(EdgeBase* edge) const	{if(!elements_are_supported(SE_EDGE)) return 0; return m_aaSelEDGE[edge];}
		inline byte get_selection_status(Face* face) const		{if(!elements_are_supported(SE_FACE)) return 0; return m_aaSelFACE[face];}
		inline byte get_selection_status(Volume* vol) const		{if(!elements_are_supported(SE_VOLUME)) return 0; return m_aaSelVOL[vol];}

		template <class TElem>
		inline bool is_selected(TElem* elem) const		{return get_selection_status(elem) != 0;}

	//	non-virtual methods.
		inline Grid* grid() const		{return m_pGrid;}

	///	returns a geometric object collection, containing all selected objects
		virtual GeometricObjectCollection get_geometric_objects() = 0;

	///	returns true if the given element-types are supported.
	/**	pass an or-combination of constants enumerated in SubsetHandlerElements.*/
		inline bool elements_are_supported(uint shElements) const;

	//	if enabled, all new elements will be automatically enabled. Disabled by default.
		void enable_autoselection(bool bEnable);
		inline bool autoselection_enabled()		{return m_bAutoselectionEnabled;}

	//	if enabled, newly created elements derive their selection status from their parents. Enabled by default.
		void enable_selection_inheritance(bool bEnable);
		inline bool selection_inheritance_enabled()		{return m_bSelectionInheritanceEnabled;}

	/**	restricts subset inheritance so that new elements derive their
	 * 	selection status only from parents with the same base-type.
	 * 	Disabled by default.
	 * 	NOTE: strict inheritance only has an effect if
	 * 	selection inheritance is enabled.*/
		void enable_strict_inheritance(bool bEnable);
		inline bool strict_inheritance_enabled()	{return m_bStrictInheritanceEnabled;}
	//	grid callbacks
	/*
		virtual void registered_at_grid(Grid* grid);
		virtual void unregistered_from_grid(Grid* grid);
	*/
		virtual void grid_to_be_destroyed(Grid* grid);
		virtual void elements_to_be_cleared(Grid* grid);

	//	element callbacks
		virtual void vertex_created(Grid* grid, VertexBase* vrt,
									GeometricObject* pParent = NULL,
									bool replacesParent = false);

		virtual void edge_created(Grid* grid, EdgeBase* e,
									GeometricObject* pParent = NULL,
									bool replacesParent = false);

		virtual void face_created(Grid* grid, Face* f,
									GeometricObject* pParent = NULL,
									bool replacesParent = false);

		virtual void volume_created(Grid* grid, Volume* vol,
									GeometricObject* pParent = NULL,
									bool replacesParent = false);

		virtual void vertex_to_be_erased(Grid* grid, VertexBase* vrt,
										 VertexBase* replacedBy = NULL);

		virtual void edge_to_be_erased(Grid* grid, EdgeBase* e,
										 EdgeBase* replacedBy = NULL);

		virtual void face_to_be_erased(Grid* grid, Face* f,
										 Face* replacedBy = NULL);

		virtual void volume_to_be_erased(Grid* grid, Volume* vol,
										 Volume* replacedBy = NULL);

		virtual void vertices_to_be_merged(Grid* grid, VertexBase* target,
										 VertexBase* elem1, VertexBase* elem2);

		virtual void edges_to_be_merged(Grid* grid, EdgeBase* target,
										 EdgeBase* elem1, EdgeBase* elem2);

		virtual void faces_to_be_merged(Grid* grid, Face* target,
										 Face* elem1, Face* elem2);

		virtual void volumes_to_be_merged(Grid* grid, Volume* target,
										 Volume* elem1, Volume* elem2);

	protected:
		typedef Grid::traits<VertexBase>::AttachedElementList	AttachedVertexList;
		typedef Grid::traits<EdgeBase>::AttachedElementList		AttachedEdgeList;
		typedef Grid::traits<Face>::AttachedElementList			AttachedFaceList;
		typedef Grid::traits<Volume>::AttachedElementList		AttachedVolumeList;

		typedef Grid::traits<VertexBase>::SectionContainer		VertexSectionContainer;
		typedef Grid::traits<EdgeBase>::SectionContainer		EdgeSectionContainer;
		typedef Grid::traits<Face>::SectionContainer			FaceSectionContainer;
		typedef Grid::traits<Volume>::SectionContainer			VolumeSectionContainer;

	protected:
		virtual void add_to_list(VertexBase* elem) = 0;
		virtual void add_to_list(EdgeBase* elem) = 0;
		virtual void add_to_list(Face* elem) = 0;
		virtual void add_to_list(Volume* elem) = 0;

		virtual void erase_from_list(VertexBase* elem) = 0;
		virtual void erase_from_list(EdgeBase* elem) = 0;
		virtual void erase_from_list(Face* elem) = 0;
		virtual void erase_from_list(Volume* elem) = 0;

	protected:
	///	performs grid registration / deregistration and initialisation of the observer.
	/**	If you call this method with NULL, deregistration and cleanup is performed.
	 *
	 *	Please note: sine set_grid calls virtual methods it shouldn't
	 *	be invoked from any constructors / destructors.*/
		void set_grid(Grid* grid);

	///	set the type of elements that shall be handled by the SubsetHandler.
	/**	Pass an or-combination of constants enumerated in SubsetHandlerElements.
	 *	\sa SubsetHandler::enable_element_support*/
		void set_supported_elements(uint shElements);

	///	enable support for element-types. Does not invalidate previous settings.
	/**	pass an or-combination of constants enumerated in SubsetHandlerElements.*/
		void enable_element_support(uint shElements);

	///	disable support for element-types.
	/**	pass an or-combination of constants enumerated in SubsetHandlerElements.*/
		void disable_element_support(uint shElements);

		inline void mark_selected(VertexBase* elem, byte status)	{assert(elements_are_supported(SE_VERTEX)); m_aaSelVRT[elem] = status;}
		inline void mark_selected(EdgeBase* elem, byte status)		{assert(elements_are_supported(SE_EDGE)); m_aaSelEDGE[elem] = status;}
		inline void mark_selected(Face* elem, byte status)			{assert(elements_are_supported(SE_FACE)); m_aaSelFACE[elem] = status;}
		inline void mark_selected(Volume* elem, byte status)		{assert(elements_are_supported(SE_VOLUME)); m_aaSelVOL[elem] = status;}

		inline void mark_deselected(VertexBase* elem)	{assert(elements_are_supported(SE_VERTEX)); m_aaSelVRT[elem] = 0;}
		inline void mark_deselected(EdgeBase* elem)		{assert(elements_are_supported(SE_EDGE)); m_aaSelEDGE[elem] = 0;}
		inline void mark_deselected(Face* elem)			{assert(elements_are_supported(SE_FACE)); m_aaSelFACE[elem] = 0;}
		inline void mark_deselected(Volume* elem)		{assert(elements_are_supported(SE_VOLUME)); m_aaSelVOL[elem] = 0;}

	///	helper for GridObserver callbacks.
		template <class TElem>
		void elems_to_be_merged(Grid* grid, TElem* target,
								TElem* elem1, TElem* elem2);

	private:
		ISelector(const ISelector& sel){};///<	Copy Constructor not yet implemented!

	protected:
		Grid*	m_pGrid;
		uint	m_supportedElements;
		bool	m_bAutoselectionEnabled;
		bool	m_bSelectionInheritanceEnabled;
		bool	m_bStrictInheritanceEnabled;
		
	//	will use a default constructor
		typedef Attachment<unsigned char> 	AUChar;
		AUChar								m_aSelected;

		Grid::AttachmentAccessor<VertexBase, AUChar>	m_aaSelVRT;
		Grid::AttachmentAccessor<EdgeBase, AUChar>		m_aaSelEDGE;
		Grid::AttachmentAccessor<Face, AUChar>			m_aaSelFACE;
		Grid::AttachmentAccessor<Volume, AUChar>		m_aaSelVOL;
};

/// \}



/**	A wrapper that returns whether an object is selected. Instances can
 * be used as callbacks CB_ConsiderVertex, ..., CB_ConsiderVolume.
 *
 * \ingroup lib_grid_callbacks
 */
class IsSelected
{
	public:
		IsSelected(const ISelector& sel) :
			m_sel(sel)	{}

		bool operator() (VertexBase* v)	{return m_sel.is_selected(v);}
		bool operator() (EdgeBase* e)	{return m_sel.is_selected(e);}
		bool operator() (Face* f)		{return m_sel.is_selected(f);}
		bool operator() (Volume* v)		{return m_sel.is_selected(v);}

	private:
		const ISelector&	m_sel;
};

/**	A wrapper that returns whether an object is not selected. Instances can
 * be used as callbacks CB_ConsiderVertex, ..., CB_ConsiderVolume.
 *
 * \ingroup lib_grid_callbacks
 */
class IsNotSelected
{
	public:
		IsNotSelected(const ISelector& sel) :
			m_sel(sel)	{}

		bool operator() (VertexBase* v)	{return !m_sel.is_selected(v);}
		bool operator() (EdgeBase* e)	{return !m_sel.is_selected(e);}
		bool operator() (Face* f)		{return !m_sel.is_selected(f);}
		bool operator() (Volume* v)		{return !m_sel.is_selected(v);}

	private:
		const ISelector&	m_sel;
};

}//	end of namespace

////////////////////////////////
//	include implementation
#include "selector_interface_impl.hpp"

#endif
