// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 25.7.2011 (m,d,y)

#ifndef __H__UG__compol_subset__
#define __H__UG__compol_subset__
#include "pcl/pcl_communication_structs.h"

namespace ug
{


template <class TLayout>
class ComPol_Subset : public pcl::ICommunicationPolicy<TLayout>
{
	public:
		typedef TLayout							Layout;
		typedef typename Layout::Type			GeomObj;
		typedef typename Layout::Element		Element;
		typedef typename Layout::Interface		Interface;
		typedef typename Interface::iterator	InterfaceIter;

	///	Construct the communication policy with a ug::SubsetHandler.
	/**	Through the parameters select and deselect one may specify whether
	 * a process selects and/or deselects elements based on the received
	 * selection status.*/
		ComPol_Subset(ISubsetHandler& sel)
			 :	m_sh(sel)
		{}

		virtual int
		get_required_buffer_size(Interface& interface)		{return interface.size() * sizeof(int);}

	///	writes 1 for selected and 0 for unassigned interface entries
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	write the entry indices of marked elements.
			for(InterfaceIter iter = interface.begin();
				iter != interface.end(); ++iter)
			{
				Element elem = interface.get_element(iter);
				int si = m_sh.get_subset_index(elem);
				buff.write((char*)&si, sizeof(int));
			}

			return true;
		}

	///	reads marks from the given stream
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
			int nsi;
			bool retVal = true;
			for(InterfaceIter iter = interface.begin();
				iter != interface.end(); ++iter)
			{
				Element elem = interface.get_element(iter);
				buff.read((char*)&nsi, sizeof(int));
				if(m_sh.get_subset_index(elem) == -1){
					m_sh.assign_subset(elem, nsi);
				}
				else if(m_sh.get_subset_index(elem) != nsi){
				//	if the subset indices do not match, we have a problem here.
					retVal = false;
				}
			}
			return retVal;
		}

	protected:
		ISubsetHandler&	m_sh;
};

}//	end of namespace

#endif
