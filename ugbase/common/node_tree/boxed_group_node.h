/*
 * Copyright (c) 2007-2015:  Sebastian Reiter
 * Author: Sebastian Reiter
 * 
 * This file is part of UG4.
 * 
 * UG4 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (2) The following notice must be displayed at a prominent place in the
 * terminal output of covered works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (3) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S., Vogel, A., Heppner, I., Rupp, M., and Wittum, G. A massively
 *   parallel geometric multigrid solver on hierarchically distributed grids.
 *   Computing and visualization in science 16, 4 (2013), 151-164"
 * "Vogel, A., Reiter, S., Rupp, M., Nägel, A., and Wittum, G. UG4 -- a novel
 *   flexible software system for simulating pde based models on high performance
 *   computers. Computing and visualization in science 16, 4 (2013), 165-179"
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#ifndef __H__UG__NODE_TREE__BOXED_GROUP_NODE__
#define __H__UG__NODE_TREE__BOXED_GROUP_NODE__

#include "group_node.h"
#include "common/math/ugmath.h"

namespace ug{
namespace node_tree
{

class BoxedGroupNode;

////////////////////////////////////////////////////////////////////////
///	the smartpointer used to encapsulate the node
typedef SmartPtr<BoxedGroupNode> SPBoxedGroupNode;

////////////////////////////////////////////////////////////////////////
//	BoxedGroupNode
///	A group node featuring a bounding box
/**
...
*/
class BoxedGroupNode : public GroupNode
{
	public:
		static SPBoxedGroupNode create();

		virtual ~BoxedGroupNode();

		virtual void set_box(const vector3& minCorner,
							 const vector3& maxCorner);

		virtual const vector3& min_corner() const;
		virtual const vector3& max_corner() const;

	protected:
		BoxedGroupNode();

	protected:
		vector3 m_vMin;
		vector3 m_vMax;
};

}//	end of namespace node_tree
}//	end of namespace ug

#endif
