//	traverser.h
//	created by Sebastian Reiter 15. November 2007
//	s.b.reiter@googlemail.com

#ifndef __H__UG__NODE_TREE__TRAVERSER__
#define __H__UG__NODE_TREE__TRAVERSER__

#include <vector>
#include "object.h"
#include "node.h"
#include "group_node.h"

namespace ug{
namespace node_tree
{

class Traverser;
class GroupNode;
class BoxedGroupNode;
class CollisionTreeRootNode;
class CollisionEdgesNode;

////////////////////////////////////////////////////////////////////////
//	Traverser
///	a Traverser can be used to traverse a scenegraph.
/**
*/
class Traverser
{
	public:
		Traverser();
		virtual ~Traverser();

		void apply(SPNode& node);

		template<typename HandlerType>
		void register_handler_function(unsigned int oc, HandlerType func);

	protected:
		void traverse_object(Object* obj);

		virtual void handle_group(GroupNode* group);
		virtual void handle_boxed_group(BoxedGroupNode* boxedGroup);
		virtual void handle_collision_tree_root(CollisionTreeRootNode* collisionTreeRoot);
		virtual void handle_collision_edges(CollisionEdgesNode* collisionEdges);

	private:
		bool handler_function_registered(unsigned int oc);

	private:
		typedef void (Traverser::*HandlerFunc)(Object* obj);
		std::vector<HandlerFunc>	m_vHandlerFuncs;

};


template<typename HandlerType>
void Traverser::register_handler_function(unsigned int oc, HandlerType func)
{
//	make sure that there is enough space
	if(oc >= m_vHandlerFuncs.size())
		m_vHandlerFuncs.resize(oc+1, 0);

	m_vHandlerFuncs[oc] = (HandlerFunc)func;
}


}//	end of namespace node_tree
}//	end of namespace ug

#endif