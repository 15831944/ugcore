/*
 * local_shape_function_set_impl.h
 *
 *  Created on: 17.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISCRETIZATION__LOCAL_SHAPE_FUNCTION_SET_FACTORY_IMPL__
#define __H__UG__LIB_DISCRETIZATION__LOCAL_SHAPE_FUNCTION_SET_FACTORY_IMPL__

#include "common/common.h"
#include "local_shape_function_set_id.h"

namespace ug{

///////////////////////////////////////////
// LocalShapeFunctionSetProvider
///////////////////////////////////////////

template <typename TRefElem>
std::map<LSFSID, const LocalShapeFunctionSet<TRefElem>* >&
LocalShapeFunctionSetProvider::get_map()
{
//	get type of map
	typedef std::map<LSFSID, const LocalShapeFunctionSet<TRefElem>* > Map;

//	create static map
	static Map shape_function_set_map;

//	return map
	return shape_function_set_map;
};

template <typename TRefElem>
bool
LocalShapeFunctionSetProvider::
register_local_shape_function_set(LSFSID type, const LocalShapeFunctionSet<TRefElem>& set)
{
//	reference object id
	const ReferenceObjectID roid = TRefElem::REFERENCE_OBJECT_ID;

//	get vector of types
	std::vector<const LocalShapeFunctionSetBase*>& vBase = m_baseMap[type];

//	resize vector
	vBase.resize(NUM_REFERENCE_OBJECTS, NULL);

//	check that no space has been previously registered to this place
	if(vBase[roid])
	{
		UG_LOG("ERROR in 'LocalShapeFunctionSetProvider::"
				"register_local_shape_function_set()': "
				"Base type already registered for trial space: "<<type<<" and "
				" Reference element type "<<roid<<".\n");
		return false;
	}

//	if ok, add
	vBase[roid] = &set;

//	get type of map
	typedef std::map<LSFSID, const LocalShapeFunctionSet<TRefElem>* > Map;
	static Map& map = get_map<TRefElem>();
	typedef std::pair<LSFSID,const LocalShapeFunctionSet<TRefElem>*> MapPair;

//	insert into map
	if(map.insert(MapPair(type, &set)).second == false)
	{
		UG_LOG("ERROR in 'LocalShapeFunctionSetProvider::"
				"register_local_shape_function_set()': "
				"Reference type already registered for trial space: "<<type<<" and "
				" Reference element type "<<roid<<".\n");
		return false;
	}

//	all ok
	return true;
}


template <typename TRefElem>
bool
LocalShapeFunctionSetProvider::
unregister_local_shape_function_set(LSFSID id)
{
//	get type of map
	typedef std::map<LSFSID, const LocalShapeFunctionSet<TRefElem>* > Map;

//	init provider and get map
	static Map& map = inst().get_map<TRefElem>();

//	erase element
	bool bRet = true;
	bRet &= (map.erase(id) == 1);
	bRet &= (m_baseMap.erase(id) == 1);
	return bRet;
}

template <typename TRefElem>
const LocalShapeFunctionSet<TRefElem>&
LocalShapeFunctionSetProvider::
get(LSFSID id)
{
//	get type of map
	typedef std::map<LSFSID, const LocalShapeFunctionSet<TRefElem>* > Map;
	const static ReferenceObjectID roid = TRefElem::REFERENCE_OBJECT_ID;

//	init provider and get map
	static Map& map = inst().get_map<TRefElem>();

//	search for identifier
	typename Map::const_iterator iter = map.find(id);

//	if not found
	if(iter == map.end())
	{
		UG_LOG("ERROR in 'LocalShapeFunctionSetProvider::get': "
				"Unknown Trial Space Type "<<id<<" requested for Element"
				" type: "<<roid<<".\n");
		throw(UGFatalError("Trial Space type unknown"));
	}

//	return shape function set
	return *(iter->second);
}

}
#endif /* __H__UG__LIB_DISCRETIZATION__LOCAL_SHAPE_FUNCTION_SET_FACTORY_IMPL__ */
