//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m04 d27

#ifndef __H__UG__NODE_TREE__TRAVERSER_COLLISION_TREE__
#define __H__UG__NODE_TREE__TRAVERSER_COLLISION_TREE__

#include "../traverser.h"

namespace ug{
namespace node_tree
{

class CollisionTreeRootNode;
class CollisionEdgesNode;
class CollisionTrianglesNode;

////////////////////////////////////////////////////////////////////////
//	Traverser_CollisionTree
///	Enhances the Traverser base-class by methods to traverse a collision tree.
/**
 * Derive from this class if you want to create a traverser that traverses
 * a collision-tree.
 *
 * This class does nothing more than registering its callback-methods at the
 * base-traverser.
 *
 * The methods here do not do much. handle_collision_tree_root calls its
 * handle_boxed_group on its node. The other methods are empty.
 */
class Traverser_CollisionTree : public Traverser
{
	public:
		Traverser_CollisionTree();
		virtual ~Traverser_CollisionTree();

	protected:
		virtual void handle_collision_tree_root(CollisionTreeRootNode* colTreeRootNode) = 0;
		virtual void handle_collision_edges(CollisionEdgesNode* colEdgesNode) = 0;
		virtual void handle_collision_triangles(CollisionTrianglesNode* colTrisNode) = 0;
};

}//	end of namespace node_tree
}//	end of namespace ug

#endif
