//	Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m07 d29

#ifndef __H__PCL_UTIL__
#define __H__PCL_UTIL__

#include <iostream>
#include "pcl_base.h"
#include "pcl_communication_structs.h"
#include "pcl_communicator.h"
#include "pcl_process_communicator.h"

namespace pcl
{
////////////////////////////////////////////////////////////////////////
/**
 * Removes unselected entries from the interfaces in the given layout.
 * Empty interfaces are removed from the layout, too.
 *
 * TLayout has to be compatible with pcl::Layout or pcl::MultiLevelLayout.
 *
 * \code
 * bool TSelector::is_selected(TType* t);
 * \endcode
 *
 * \returns:	true, if the layout has changed, false if not.
 */
template <class TLayout, class TSelector>
bool RemoveUnselectedInterfaceEntries(TLayout& layout, TSelector& sel)
{
//	iterate over all interfaces of the layout.
//	for each we'll create a new one, into which elements selected
//	elements will be inserted.
//	Finally we'll swap the content of the those interfaces.
//	if the interface is empty at the end of the operation, it will be
//	removed from the layout.
	bool retVal = false;
	
//	some typedefs first
	typedef typename TLayout::Interface Interface;
	typedef typename TLayout::iterator InterfaceIter;
	typedef typename Interface::Element	Elem;
	typedef typename Interface::iterator ElemIter;
	
//	iterate over all levels
	for(size_t level = 0; level < layout.num_levels(); ++level)
	{
	//	iterate over all interfaces
		for(InterfaceIter iiter = layout.begin(level);
			iiter != layout.end(level);)
		{
			bool interfaceChanged = false;
			Interface& interface = layout.interface(iiter);
		
		//	create a temporary interface and fill it with the selected entries
			Interface tInterface;
			
			for(ElemIter iter = interface.begin();
				iter != interface.end(); ++iter)
			{
				Elem& e = interface.get_element(iter);
				if(sel.is_selected(e))
					tInterface.push_back(e);
				else
					interfaceChanged = true;
			}
			
		//	now swap the interface contents.
			if(interfaceChanged){
				interface.swap(tInterface);
			
			//	if the interface is empty, erase it.
			//	if not, simply increase the iterator
				if(interface.size() == 0){
					iiter = layout.erase(iiter, level);
				}
				else{
					++iiter;
				}
				
				retVal = true;
			}
			else{
				++iiter;
			}
		}
	}
	
	return retVal;
}

////////////////////////////////////////////////////////////////////////
/**
 * Removes interface-entries, empty interfaces and empty layouts from
 * the given layoutMap for the given type.
 *
 * TLayoutMap has to be compatible with pcl::LayoutMap.
 *
 * TSelector has to feature a method
 * \code
 * bool TSelector::is_selected(TType* t);
 * \endcode
 *
 * \returns:	true, if the layout-map has changed, false if not.
 */
template <class TType, class TLayoutMap, class TSelector>
bool RemoveUnselectedInterfaceEntries(TLayoutMap& lm, TSelector& sel)
{
	typedef typename TLayoutMap::template Types<TType>::Map::iterator iterator;
	typedef typename TLayoutMap::template Types<TType>::Layout		Layout;

	bool retVal = false;
	for(iterator iter = lm.template layouts_begin<TType>();
		iter != lm.template layouts_end<TType>();)
	{
	//	get the layout
		Layout& layout = iter->second;
	//	remove unnecessary interface entries and interfaces
		retVal |= RemoveUnselectedInterfaceEntries(layout, sel);
	//	if the layout is empty, it can be removed from the map
	//	if not we'll simply increase the iterator
		if(layout.empty()){
			iter = lm.template erase_layout<TType>(iter);
		}
		else{
			++iter;
		}
	}
	return retVal;
}

////////////////////////////////////////////////////////////////////////
///	communicates selection-status of interface elements
/**
 *	TLayout has to be compatible with the pcl::layout_tags.
 *
 *	TSelectorIn has to feature a method
 *	\code
 *	bool TSelectorIn::is_selected(TLayout::Element e);
 *	\endcode
 *
 *	TSelectorOut has to feature methods
 *	\code
 *	void TSelectorOut::select(TLayout::Element e);
 *	void TSelectorOut::deselect(TLayout::Element e);
 *	\endcode
 */
template <class TLayout, class TSelectorIn, class TSelectorOut>
class SelectionCommPol : public ICommunicationPolicy<TLayout>
{
	public:
		typedef typename ICommunicationPolicy<TLayout>::Interface Interface;
		
	public:
		SelectionCommPol(TSelectorIn& selIn, TSelectorOut& selOut) :
			m_selIn(selIn), m_selOut(selOut)	{}
			
	///	iterates over the interface entries. Writes 1 for selected, 0 for unselected.
		virtual bool
		collect(std::ostream& buff, Interface& interface)
		{
			char zero = 0;
			char one = 1;
			
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
				if(m_selIn.is_selected(interface.get_element(iter)))
					buff.write(&one, sizeof(char));
				else
					buff.write(&zero, sizeof(char));
			}
			
			return true;
		}
		
	///	iterates over the interface entries. selects for 1, deselects for 0.
		virtual bool
		extract(std::istream& buff, Interface& interface)
		{
			char tmp;
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
				buff.read(&tmp, sizeof(char));
				if(tmp == 0)
					m_selOut.deselect(interface.get_element(iter));
				else
					m_selOut.select(interface.get_element(iter));
			}
			
			return true;
		}
		
	protected:
		TSelectorIn&	m_selIn;
		TSelectorOut&	m_selOut;
};

}//	end of namespace

#endif
