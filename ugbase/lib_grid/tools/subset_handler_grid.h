//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m11 d24   (reworked y09 m12 d15)

#ifndef __H__LIBGRID__SUBSET_HANDLER_GRID__
#define __H__LIBGRID__SUBSET_HANDLER_GRID__

#include <vector>
#include <cassert>
#include "lib_grid/grid/grid.h"
#include "common/util/section_container.h"
#include "subset_handler_interface.h"

namespace ug
{
////////////////////////////////////////////////////////////////////////
//	GridSubsetHandler
/// \ingroup lib_grid
class GridSubsetHandler : public ISubsetHandler
{
	public:
		using ISubsetHandler::assign_subset;
		
	public:
		GridSubsetHandler(uint supportedElements = SHE_ALL);
		GridSubsetHandler(Grid& grid, uint supportedElements = SHE_ALL);
	/**	WARNING: Don't call the copy-constructor from derived classes,
	  *	Since it calls virtual methods.*/
		GridSubsetHandler(const GridSubsetHandler& sh);
		~GridSubsetHandler();
		
		GridSubsetHandler& operator = (const ISubsetHandler& sh);

		void assign_grid(Grid& grid);
		
	///	Makes sure that the subset with the given index exists.
	/**	If required the subsets between num_subsets() and index will be created.
	 *	ISubsetHandler::subset_info_required is called automatically.*/
		inline void subset_required(int index);
		
	///	The const version of subset_required throws an error if the subset does not exist.
		inline void subset_required(int index) const;

	////////////////////////////////////////////////
	//	implementation of public virtual methdos of ISubsetHandler.
	///	assigns a vertex to a subset.
	/**	If the subset doesn't already exist, it will be created.*/
		void assign_subset(VertexBase* elem, int subsetIndex);

	///	assigns an edge to a subset.
	/**	If the subset doesn't already exist, it will be created.*/
		void assign_subset(EdgeBase* elem, int subsetIndex);

	///	assigns a face to a subset.
	/**	If the subset doesn't already exist, it will be created.*/
		void assign_subset(Face* elem, int subsetIndex);

	///	assigns a volume to a subset.
	/**	If the subset doesn't already exist, it will be created.*/
		void assign_subset(Volume* elem, int subsetIndex);

	////////////////////////////////////////////////
	//	element-access
	///	returns the begin-iterator for the elements of type TElem in the given subset.
	/**	e.g. begin<Triangle>(0)*/
		template <class TElem>
		typename geometry_traits<TElem>::iterator
		begin(int subsetIndex);

	///	returns the end-iterator for the elements of type TElem in the given subset.
	/**	e.g. end<Triangle>(0)*/
		template <class TElem>
		typename geometry_traits<TElem>::iterator
		end(int subsetIndex);

	///	returns the begin-iterator for the elements of type TElem in the given subset.
	/**	e.g. begin<Triangle>(0)*/
		template <class TElem>
		typename geometry_traits<TElem>::const_iterator
		begin(int subsetIndex) const;

	///	returns the end-iterator for the elements of type TElem in the given subset.
	/**	e.g. end<Triangle>(0)*/
		template <class TElem>
		typename geometry_traits<TElem>::const_iterator
		end(int subsetIndex) const;
		
	///	returns the number of elements in the given subset
		template <class TElem>
		uint num_elements(int subsetIndex) const;

	///	returns the total number of elements
		template <class TElem>
		uint num() const;

	///	returns the number of elements in the given subset
		template <class TElem>
		uint num(int subsetIndex) const;

	///	returns true if the subset-handler contains no elements of the given type.
		template <class TElem> inline
		bool empty() const;
		
	///	returns true if the subset-handler contains no elements at all.
		inline bool empty() const;

		template <class TElem> inline
		bool empty(int subsetIndex) const;
		
	///	returns true if the subset-handler contains no elements at all.
		inline bool empty(int subsetIndex) const;
		
	///	removes all elements of type TElem from the specified subset.
		template <class TElem>
		void clear_subset_elements(int subsetIndex);

	//	geometric-object-collection
		virtual GeometricObjectCollection
		get_geometric_objects_in_subset(int subsetIndex) const;
		
	//	multi-level-geometric-object-collection
		GeometricObjectCollection
		get_geometric_objects() const;

	///	collects all vertices that are in the given subset.
	/**	Please consider using begin and end methods instead.
	 *	If subset -1 is specified, the method has compexity O(n), where n is the number
	 *	of vertices in the underlying grid.
	 *	\returns number of collected elements.
	 *	\sa begin, end*/
		//virtual size_t collect_subset_elements(std::vector<VertexBase*>& vrtsOut, int subsetIndex) const;

	///	collects all edges that are in the given subset.
	/**	Please consider using begin and end methods instead.
	 *	If subset -1 is specified, the method has compexity O(n), where n is the number
	 *	of edges in the underlying grid.
	 *	\returns number of collected elements.
	 *	\sa begin, end*/
		//virtual size_t collect_subset_elements(std::vector<EdgeBase*>& edgesOut, int subsetIndex) const;

	///	collects all faces that are in the given subset.
	/**	Please consider using begin and end methods instead.
	 *	If subset -1 is specified, the method has compexity O(n), where n is the number
	 *	of faces in the underlying grid.
	 *	\returns number of collected elements.
	 *	\sa begin, end*/
		//virtual size_t collect_subset_elements(std::vector<Face*>& facesOut, int subsetIndex) const;

	///	collects all volumes that are in the given subset.
	/**	Please consider using begin and end methods instead.
	 *	If subset -1 is specified, the method has compexity O(n), where n is the number
	 *	of volumes in the underlying grid.
	 *	\returns number of collected elements.
	 *	\sa begin, end*/
		//virtual size_t collect_subset_elements(std::vector<Volume*>& volsOut, int subsetIndex) const;

	///	returns true if the subset contains vertices
		virtual bool contains_vertices(int subsetIndex) const	{return num<VertexBase>(subsetIndex) > 0;}

	///	returns true if the subset contains edges
		virtual bool contains_edges(int subsetIndex) const		{return num<EdgeBase>(subsetIndex) > 0;}
		
	///	returns true if the subset contains faces
		virtual bool contains_faces(int subsetIndex) const		{return num<Face>(subsetIndex) > 0;}
		
	///	returns true if the subset contains volumes
		virtual bool contains_volumes(int subsetIndex) const	{return num<Volume>(subsetIndex) > 0;}

	///	only for debug purposes
		template <class TElem>
		bool perform_self_tests();
		
	////////////////////////////////////////////////
	//	for compatibility with MGSubsetHandler
	///	returns number of levels (always 1)
	/**	only for compatibility reasons with MGSubsetHandler.*/
		uint num_levels() const						{return 1;}
		
	///	returns the begin-iterator for the elements of type TElem in the given subset.
	/**	only for compatibility reasons with MGSubsetHandler.
	 *	second argument is ignored.
	 *	use i.e. as follows: begin<Triangle>(0, 0)*/
		template <class TElem>
		typename geometry_traits<TElem>::const_iterator
		begin(int subsetIndex, size_t) const		{return begin<TElem>(subsetIndex);}

	///	returns the end-iterator for the elements of type TElem in the given subset.
	/**	only for compatibility reasons with MGSubsetHandler.
	 *	second argument is ignored.
	 *	use i.e. as follows: end<Triangle>(0, 0)*/
		template <class TElem>
		typename geometry_traits<TElem>::const_iterator
		end(int subsetIndex, size_t) const			{return end<TElem>(subsetIndex);}

	///	returns the number of elements in the given subset
	/**	only for compatibility reasons with MGSubsetHandler.
	 *	second argument is ignored.*/
		template <class TElem>
		uint num_elements(int subsetIndex, size_t) const	{return num_elements<TElem>();}

	///	returns the number of elements in the given subset
	/**	only for compatibility reasons with MGSubsetHandler.
	 *	second argument is ignored.*/
		template <class TElem>
		uint num(int subsetIndex, size_t) const				{return num<TElem>();}
		

	///	perform cleanup
		virtual void grid_to_be_destroyed(Grid* grid);

	protected:
	///	returns the number of subsets in the local list
		inline uint num_subsets_in_list() const	{return m_subsets.size();}
		
	////////////////////////////////////////////////
	//	implementation of protected virtual methdos of ISubsetHandler.
	///	erases the subsets. Doesn't alter any indices.
		void erase_subset_lists();
		
	///	clears the element lists in the given subset. Does not alter any indices.
		void clear_subset_lists(int index);

	///	changes the subset-indices of all elements int the subset.
	/**	WARNING: subsets are not automatically changed accordingly.
	 *	After termination Subset-Indices and Subset-Infos/iterators are asynchronous.
	 *	Make sure to change subset-infos and iterators accordingly.*/		
		void change_subset_indices(int indOld, int indNew);		

		
	///	add a subset
		void add_required_subset_lists(int maxIndex);
		
	///	erases the subset but does not touch the subset-indices.
		void erase_subset_lists(int index);

	///	swaps the subsets but does not touch the subset-indices.
		void swap_subset_lists(int ind1, int ind2);

	///	moves the subset but does not touch the subset-indices.
		void move_subset_lists(int indexFrom, int indexTo);

	///	this method is called by ISubsetHandler when attachment_support has been enabled.
		void register_subset_elements_at_pipe();

	////////////////////////////////////////////////
	//	protected helper methods
	///	a helper method for the public assign_subset methods.
		template<class TElemPtr>
		void assign_subset(TElemPtr elem, int subsetIndex, int elemType);
							
	///	helper for change_subset_indices
		template<class TElem>
		void change_elem_subset_indices(int indOld, int indNew);

	///	helper for collect_subset_elements
		//template <class TElem>
		//size_t collect_subset_elements_impl(std::vector<TElem*>& elemsOut, int subsetIndex) const;
		
	///	removes attachments
		void cleanup();

	///	returns the iterator at which the given element lies in the section container
	/**	This method may only be called if the element is in a subset != -1.
	 * \{
	 */
		inline ISubsetHandler::SectionContainer::iterator
		get_list_iterator(VertexBase* o)
		{
			assert((get_subset_index(o) >= 0) && "invalid subset.");
			return m_subsets[get_subset_index(o)]->m_elements[VERTEX].
				get_container().get_iterator(o);
		}

		inline ISubsetHandler::SectionContainer::iterator
		get_list_iterator(EdgeBase* o)
		{
			assert((get_subset_index(o) >= 0) && "invalid subset.");
			return m_subsets[get_subset_index(o)]->m_elements[EDGE].
				get_container().get_iterator(o);
		}

		inline ISubsetHandler::SectionContainer::iterator
		get_list_iterator(Face* o)
		{
			assert((get_subset_index(o) >= 0) && "invalid subset.");
			return m_subsets[get_subset_index(o)]->m_elements[FACE].
				get_container().get_iterator(o);
		}

		inline ISubsetHandler::SectionContainer::iterator
		get_list_iterator(Volume* o)
		{
			assert((get_subset_index(o) >= 0) && "invalid subset.");
			return m_subsets[get_subset_index(o)]->m_elements[VOLUME].
				get_container().get_iterator(o);
		}
	/**	\}	*/

	protected:
		typedef ISubsetHandler::SectionContainer SectionContainer;
		typedef ISubsetHandler::AttachedElemList AttachedElemList;
		
		struct Subset
		{
			SectionContainer 	m_elements[NUM_GEOMETRIC_BASE_OBJECTS];	/// holds pointers to elements.
		};

		typedef std::vector<Subset*>	SubsetVec;

	protected:
		SubsetVec			m_subsets;
	//	we use a shared attachment for the entry-lists of all section containers
		AttachedElemList::AEntry	m_aSharedEntry;
};


/// \ingroup lib_grid
typedef GridSubsetHandler SubsetHandler;

}//	end of namespace

//	include implementation
#include "subset_handler_grid_impl.hpp"

#endif
