// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 22.06.2011 (m,d,y)

#ifndef __H__UG__page_container_impl__
#define __H__UG__page_container_impl__

#include <algorithm>
#include "page_container.h"

//	a temporary include
#include "common/log.h"

namespace ug
{

template <class T, int MAX_PAGE_SIZE, class Allocator>
PageContainer<T, MAX_PAGE_SIZE, Allocator>::
PageContainer() :
	m_numPageEntries((size_t)(MAX_PAGE_SIZE / sizeof(T))),
	m_size(0),
	m_capacity(0)
{
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
PageContainer<T, MAX_PAGE_SIZE, Allocator>::
PageContainer(const PageContainer& pc)
{
	assign_container(pc);
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
PageContainer<T, MAX_PAGE_SIZE, Allocator>::
~PageContainer()
{
	clear();
	for(size_t i = 0; i < m_pages.size(); ++i){
		m_alloc.deallocate(m_pages[i], m_numPageEntries);
	}
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
PageContainer<T, MAX_PAGE_SIZE, Allocator>&
PageContainer<T, MAX_PAGE_SIZE, Allocator>::
operator=(const PageContainer& pc)
{
	assign_container(pc);
	return *this;
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
size_t PageContainer<T, MAX_PAGE_SIZE, Allocator>::
size() const
{
	return m_size;
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
size_t PageContainer<T, MAX_PAGE_SIZE, Allocator>::
capacity() const
{
	return m_capacity;
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
void PageContainer<T, MAX_PAGE_SIZE, Allocator>::
resize(size_t size, const T& val)
{
	using namespace std;

//	allocate memory
	reserve(size);

//	call constructor on new objects
//	to do this with optimal performance,
//	we'll iterate over the pages directly
	while(m_size < size){
		T* page = get_page(m_size);
		size_t offset = get_page_offset(m_size);
		const size_t maxI = min(m_numPageEntries, offset + size - m_size);

		for(size_t i = offset; i < maxI; ++i)
			m_alloc.construct(page + i, val);

		m_size += maxI;
	}

//	if resize shrinks the data array, we have to call the destructors of
//	deleted objects. At this point size <= m_size.
	while(m_size > size){
		T* page = get_page(m_size - 1);
		size_t maxI = get_page_offset(m_size - 1) + 1;
		size_t minI = 0;
		const size_t diff = m_size - size;
		if(maxI > diff)
			minI = (maxI) - diff;

		for(size_t i = minI; i < maxI; ++i)
			m_alloc.destroy(page + i);

		m_size -= (maxI - minI);
	}
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
void PageContainer<T, MAX_PAGE_SIZE, Allocator>::
reserve(size_t size)
{
	using namespace std;

	while(m_pages.size() * m_numPageEntries < size){
		UG_LOG("PC: aquiring new page\n");
		T* buf = m_alloc.allocate(m_numPageEntries);
		m_pages.push_back(buf);
	}

	m_capacity = max(m_capacity, size);
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
void PageContainer<T, MAX_PAGE_SIZE, Allocator>::
clear()
{
	resize(0);
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
T& PageContainer<T, MAX_PAGE_SIZE, Allocator>::
operator[](size_t ind)
{
	assert(ind < m_size);
	return get_page(ind)[get_page_offset(ind)];
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
const T& PageContainer<T, MAX_PAGE_SIZE, Allocator>::
operator[](size_t ind) const
{
	assert(ind < m_size);
	return get_page(ind)[get_page_offset(ind)];
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
void PageContainer<T, MAX_PAGE_SIZE, Allocator>::
swap(PageContainer& pc)
{
	using namespace std;
	m_pages.swap(pc.m_pages);
	swap(m_numPageEntries, pc.m_numPageEntries);
	swap(m_size, pc.m_size);
	swap(m_capacity, pc.m_capacity);
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
void PageContainer<T, MAX_PAGE_SIZE, Allocator>::
assign_container(const PageContainer& pc)
{
	clear();

	m_numPageEntries = pc.m_numPageEntries;

	reserve(pc.m_size);

//	copy all entries with optimal performance
	while(m_size < pc.m_size){
		T* page = get_page(m_size);
		T* srcPage = pc.get_page(m_size);
		size_t offset = get_page_offset(m_size);
		const size_t maxI = min(m_numPageEntries, offset + pc.m_size - m_size);

		for(size_t i = offset; i < maxI; ++i)
			m_alloc.construct(page + i, srcPage[i]);

		m_size += maxI;
	}
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
inline T* PageContainer<T, MAX_PAGE_SIZE, Allocator>::
get_page(size_t ind) const
{
	assert(get_page_index(ind) < m_pages.size());
	return m_pages[get_page_index(ind)];
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
inline size_t PageContainer<T, MAX_PAGE_SIZE, Allocator>::
get_page_index(size_t ind) const
{
	return ind / m_numPageEntries;
}

template <class T, int MAX_PAGE_SIZE, class Allocator>
inline size_t PageContainer<T, MAX_PAGE_SIZE, Allocator>::
get_page_offset(size_t ind) const
{
	return ind % m_numPageEntries;
}

}//	end of namespace

#endif
