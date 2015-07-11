// created by Sebastian Reiter
// s.b.reiter@gmail.com
// Sep 4, 2013

#ifndef __H__UG__ntree_traverser__
#define __H__UG__ntree_traverser__

#include <algorithm>
#include <utility>
#include <vector>
#include "ntree_traversal.h"

namespace ug{

template <class tree_t>
class Traverser_FindLowestLeafNodeLevel
{
	public:
		Traverser_FindLowestLeafNodeLevel() :
			m_lowestLeafNodeLvl(0)	{}

		void begin_traversal(const tree_t& tree)
		{
			m_lowestLeafNodeLvl = 0;
		}

		int visit_up(const tree_t& tree, size_t node)
		{
			if(tree.num_child_nodes(node) == 0){
				m_lowestLeafNodeLvl = tree.level(node);
				return ABORT_TRAVERSAL;
			}
			return TRAVERSE_CHILDREN;
		}

		void visit_down(const tree_t&, size_t)	{}

		void end_traversal(const tree_t&)	{}

		size_t result() const {return m_lowestLeafNodeLvl;}

	private:
		size_t m_lowestLeafNodeLvl;
};

template <class tree_t>
size_t FindLowestLeafNodeLevel(const tree_t& tree)
{
	Traverser_FindLowestLeafNodeLevel<tree_t> trav;
	TraverseBreadthFirst(tree, trav);
	return trav.result();
}


///	returns the minimum and maximum number of elements in all subtrees of nodes of the given level
template <class tree_t>
class Traverser_MinMaxNumElements
{
	public:
		Traverser_MinMaxNumElements(size_t lvl) :
			m_lvl(lvl), m_minNumElements(0), m_maxNumElements(0),
			m_elemCount(0), m_firstEval(true)	{}

		void begin_traversal(const tree_t& tree)
		{
			m_minNumElements = m_maxNumElements = 0;
			m_elemCount = 0;
			m_firstEval = true;
		}

		int visit_up(const tree_t& tree, size_t node)
		{
			if(tree.level(node) == m_lvl)
				m_elemCount = 0;

			if(tree.level(node) >= m_lvl)
				m_elemCount += tree.num_elements(node);

			return TRAVERSE_CHILDREN;
		}

		void visit_down(const tree_t& tree, size_t node)
		{
			if(tree.level(node) == m_lvl){
				if(m_firstEval){
					m_minNumElements = m_maxNumElements= m_elemCount;
					m_firstEval = false;
				}
				else{
					m_minNumElements = std::min(m_minNumElements, m_elemCount);
					m_maxNumElements = std::max(m_maxNumElements, m_elemCount);
				}
			}
		}

		void end_traversal(const tree_t&)	{}

		size_t min_num_elements() const {return m_minNumElements;}
		size_t max_num_elements() const {return m_maxNumElements;}

	private:
		size_t m_lvl;
		size_t m_minNumElements;
		size_t m_maxNumElements;
		size_t m_elemCount;
		bool m_firstEval;
};

template <class tree_t>
std::pair<size_t, size_t> GetMinMaxNumElements(const tree_t& tree, size_t lvl)
{
	Traverser_MinMaxNumElements<tree_t> trav(lvl);
	TraverseDepthFirst(tree, trav);
	return std::make_pair(trav.min_num_elements(), trav.max_num_elements());
}


template <class tree_t>
class Traverser_FindContainingElement
{
	public:
		typedef typename tree_t::elem_t		elem_t;
		typedef typename tree_t::vector_t	vector_t;

		Traverser_FindContainingElement(const vector_t& point) :
			m_point(point),
			m_foundElem(false)
		{}

		void begin_traversal(const tree_t& tree)
		{
			m_foundElem = false;
			m_numElemsChecked = 0;
		}

		int visit_up(const tree_t& tree, size_t node)
		{
		//	if the point doesn't lie in the node's bounding box, we don't have
		//	to check it's elements at all.
			if(!tree_t::traits::box_contains_point(tree.bounding_box(node), m_point))
				return DONT_TRAVERSE_CHILDREN;

		//	first check whether the nodes box contains the given point
			if(tree.num_child_nodes(node) == 0){
			//	iterate over all elements. If an element contains the given point,
			//	we're done and we may return.
				for(typename tree_t::elem_iterator_t iter = tree.elems_begin(node);
					iter != tree.elems_end(node); ++iter)
				{
					++m_numElemsChecked;
					if(tree_t::traits::contains_point(*iter, m_point, tree.common_data())){
						m_foundElem = true;
						m_elem = *iter;
						return ABORT_TRAVERSAL;
					}
				}
			}
			return TRAVERSE_CHILDREN;
		}

		void visit_down(const tree_t&, size_t)	{}

		void end_traversal(const tree_t&)	{}

		bool result(elem_t& foundElemOut) const
		{
			if(m_foundElem)
				foundElemOut = m_elem;
			return m_foundElem;
		}

		size_t num_elems_checked() const	{return m_numElemsChecked;}

	private:
		vector_t	m_point;
		elem_t		m_elem;
		size_t		m_numElemsChecked;
		bool		m_foundElem;
};

template <class tree_t>
bool FindContainingElement(typename tree_t::elem_t& elemOut, const tree_t& tree,
						   const typename tree_t::vector_t& point)
{
	Traverser_FindContainingElement<tree_t> trav(point);
	TraverseDepthFirst(tree, trav);
	//UG_LOG("num elems checked for one pick: " << trav.num_elems_checked() << "\n");
	typename tree_t::elem_t tmpElem;
	if(trav.result(tmpElem)){
		elemOut = tmpElem;
		return true;
	}
	return false;
}


template <class tree_t>
class Traverser_FindElementsInIntersectingNodes
{
	public:
		typedef typename tree_t::elem_t		elem_t;
		typedef typename tree_t::vector_t	vector_t;
		typedef typename tree_t::box_t		box_t;

		Traverser_FindElementsInIntersectingNodes(const box_t& bbox) :
			m_bbox(bbox)
		{}

		void begin_traversal(const tree_t& tree)
		{
			m_foundElems.clear();
		}

		int visit_up(const tree_t& tree, size_t node)
		{
		//	if our bounding box doesn't intersect the nodes bounding box,
		//	then there's nothing to do
			if(!tree_t::traits::box_box_intersection(tree.bounding_box(node), m_bbox))
				return DONT_TRAVERSE_CHILDREN;

		//	first check whether the nodes box contains the given point
			if(tree.num_child_nodes(node) == 0){
			//	iterate over all elements. If an element contains the given point,
			//	we're done and we may return.
				for(typename tree_t::elem_iterator_t iter = tree.elems_begin(node);
					iter != tree.elems_end(node); ++iter)
				{
					m_foundElems.push_back(*iter);
				}
			}
			return TRAVERSE_CHILDREN;
		}

		void visit_down(const tree_t&, size_t)	{}

		void end_traversal(const tree_t&)	{}

		const std::vector<elem_t>& result() const	{return m_foundElems;}

	private:
		box_t					m_bbox;
		std::vector<elem_t>		m_foundElems;
};

template <class tree_t>
bool FindElementsInIntersectingNodes(std::vector<typename tree_t::elem_t>& elemsOut,
									 const tree_t& tree,
									 const typename tree_t::box_t& bbox)
{
	Traverser_FindElementsInIntersectingNodes<tree_t> trav(bbox);
	TraverseDepthFirst(tree, trav);
	//UG_LOG("num elems checked for one pick: " << trav.num_elems_checked() << "\n");
	elemsOut = trav.result();
	return !elemsOut.empty();
}



// template <class TVector, class TData>
// class TraceRecorder {
// 	public:
// 		typedef TVector		vector_t;
// 		typedef TData		data_t;

// 		void clear		();
// 		void set_ray	(const vector_t& from, const vector_t& dir);
// 		void add_point	(const vector_t& p, const data_t& d);
// 		void add_point	(number t, const data_t& d);

// 		size_t 				num_points 				() const;
// 		const vector_t&		point					(size_t i) const;
// 		const point_data&	point_data				(size_t i) const;
// 		number 				local_point_coordinate	(size_t i) const;

// 		size_t	closest_point_index				() const;
// 		size_t	closest_positive_point_index	() const;
// 		size_t	closest_negative_point_index	() const;
// };

template <class TElem>
struct RayElemIntersectionRecord
{
	RayElemIntersectionRecord()	{}
	RayElemIntersectionRecord(number _smin, number _smax, TElem _elem) :
		smin(_smin), smax(_smax), elem(_elem)	{}

	number	smin;	///< relative coordinate where the ray enters the element
	number	smax;	///< relative coordinate where the ray leaves the element. May be equal to smin.
	TElem	elem; ///< the element that was intersected
};

template <class tree_t>
class Traverser_RayElementIntersection
{
	public:
		typedef typename tree_t::elem_t				elem_t;
		typedef typename tree_t::vector_t			vector_t;
		typedef typename tree_t::box_t				box_t;
		typedef RayElemIntersectionRecord<elem_t>	intersection_record_t;

		Traverser_RayElementIntersection(const vector_t& rayFrom,
										 const vector_t rayDir,
										 const number small = 1.e-12) :
			m_rayFrom(rayFrom),
			m_rayDir(rayDir),
			m_small(small)
		{}

		void begin_traversal(const tree_t& tree)
		{
			m_intersections.clear();
		}

		int visit_up(const tree_t& tree, size_t node)
		{
			const box_t& box = tree.bounding_box(node);
			if(!tree_t::traits::ray_box_intersection(m_rayFrom, m_rayDir, box))
				return DONT_TRAVERSE_CHILDREN;

			if(tree.num_child_nodes(node) == 0){
			//	iterate over all elements. If an element intersects the given ray,
			//	we'll record the intersection point
				vector_t v;
				number smin = 0, smax = 0, t0 = 0, t1 = 0;
				for(typename tree_t::elem_iterator_t iter = tree.elems_begin(node);
					iter != tree.elems_end(node); ++iter)
				{
					if(tree_t::traits::intersects_ray(
							*iter, m_rayFrom, m_rayDir, tree.common_data(),
							smin, smax, t0, t1, m_small))
					{
						m_intersections.push_back(intersection_record_t(smin, smax, *iter));
					}
				}
			}
			return TRAVERSE_CHILDREN;
		}

		void visit_down(const tree_t&, size_t)	{}

		void end_traversal(const tree_t&)	{}

		const std::vector<intersection_record_t>& result() const	{return m_intersections;}

	private:
		vector_t							m_rayFrom;
		vector_t							m_rayDir;
		const number						m_small;
		std::vector<intersection_record_t>	m_intersections;
};

template <class tree_t>
bool RayElementIntersections(
	std::vector<RayElemIntersectionRecord<typename tree_t::elem_t> >& intersectionsOut,
	const tree_t& tree,
	const typename tree_t::vector_t& rayFrom,
	const typename tree_t::vector_t& rayDir,
	const number small = 1.e-12)
{
	Traverser_RayElementIntersection<tree_t> trav(rayFrom, rayDir, small);
	TraverseDepthFirst(tree, trav);
	//UG_LOG("num elems checked for one pick: " << trav.num_elems_checked() << "\n");
	intersectionsOut = trav.result();
	return !intersectionsOut.empty();
}

}// end of namespace

#endif
