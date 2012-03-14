/*
 * communication_policies.h
 *
 *  Created on: 14.6.2010
 *      Author: A. Vogel, S.Reiter
 */

#ifndef __H__LIB_ALGEBRA__PARALLELIZATION__COMMUNICATION_POLICIES__
#define __H__LIB_ALGEBRA__PARALLELIZATION__COMMUNICATION_POLICIES__

#include "pcl/pcl.h"
#include "common/serialization.h"
#include "parallel_index_layout.h"
#include "algebra_id.h"


namespace ug{

// predeclaration of block_traits
/**	The block-traits are required by communication policies to determine whether buffers
 *	have a fixed size or whether they have to be treated in a more flexible way.
 *	such traits could look like this (default implementation):
 *	\begincode
 *	template <> struct block_traits<double>
 *	{
 *		enum{
 *			is_static = 1
 *		};
 *	};
 *	\endcode
 */
template <typename t> struct block_traits
{
	enum{
		is_static = 1
	};
};

/**
 * \brief Communication Policies for parallel Algebra
 *
 * Algebra Communication Policies are the basic building blocks for the
 * parallel communication between Vectors and Matrices.
 *
 * \defgroup lib_algebra_parallelization_policies Parallel Algebra Communication Policies
 * \ingroup lib_algebra_parallelization
 */

/// \addtogroup lib_algebra_parallelization_policies
/// @{

/// Communication Policy to copy values of a vector
/**
 * This class is used as a policy to copy values on the interfaces of a
 * parallel vector. The collecting interfaces pack the values on the interface
 * into a stream. The extracting interfaces receive the stream and overwrites
 * the values of the vector with the sent data.
 *
 * \tparam	TVector		Vector type
 */
template <class TVector>
class ComPol_VecCopy : public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Default constructor
		ComPol_VecCopy() : m_pVecDest(NULL), m_pVecSrc(NULL) {}

	///	Constructor setting the vector
		ComPol_VecCopy(TVector* pVec): m_pVecDest(pVec), m_pVecSrc(pVec)	{}

	///	Constructor setting the vector
		ComPol_VecCopy(TVector* pVecDest, const TVector* pVecSrc)
			: m_pVecDest(pVecDest), m_pVecSrc(pVecSrc)	{}

	///	set the vector to work on
		void set_vector(TVector* pVec)	{m_pVecDest = pVec; m_pVecSrc = pVec;}

	///	set the vector to work on
		void set_vector(TVector* pVecDest, const TVector* pVecSrc)
			{m_pVecDest = pVecDest; m_pVecSrc = pVecSrc;}

	/// returns the buffer size
	/**
	 * This function returns the size of the buffer needed for the communication
	 * of passed interface. If the vector has fixed size entries this is just
	 * the number of interface entries times the size of the entry. In case
	 * of a variable size entry type a negative value is returned to indicate
	 * that no buffer size can be determined in advanced.
	 *
	 * \param[in]	interface	Interface that will communicate
	 */
		virtual int
		get_required_buffer_size(Interface& interface)
		{
			if(block_traits<typename TVector::value_type>::is_static)
				return interface.size() * sizeof(typename TVector::value_type);
			else
				return -1;
		}

	///	writes the interface values into a buffer that will be sent
	/**
	 * This function collects all entries of the vector into a buffer that
	 * are part of the interface.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that will communicate
	 */
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVecSrc == NULL) return false;

		//	rename for convenience
			const TVector& v = *m_pVecSrc;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	write entry into buffer
				Serialize(buff, v[index]);
			}
			return true;
		}

	///	writes values from a buffer into the interface
	/**
	 * This function writes the buffer values into the vector.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that communicates
	 */
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVecDest == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVecDest;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	write entry into vector
				Deserialize(buff, v[index]);
			}
			return true;
		}

	private:
	//	pointer to current vector
		TVector* m_pVecDest;
		const TVector* m_pVecSrc;
};

/// Communication Policy to copy scaled values of a vector
/**
 * This class is used as a policy to copy scaled values on the interfaces of a
 * parallel vector. The collecting interfaces pack the values on the interface
 * into a stream. The extracting interfaces receive the stream and overwrites
 * the values of the vector with the sent data times the scaling.
 *
 * \tparam	TVector		Vector type
 */
template <class TVector>
class ComPol_VecScaleCopy : public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Default Constructor
		ComPol_VecScaleCopy() : m_pVec(NULL), m_scale(1.0)	{}

	///	Constructor setting vector and scaling factor
		ComPol_VecScaleCopy(TVector* pVec, number scale)
			: m_pVec(pVec), m_scale(scale)	{}

	///	sets the vector that we be used for communication
		void set_vector(TVector* pVec)	{m_pVec = pVec;}

	///	sets the scaling factor
		void set_scale(number scale)	{m_scale = scale;}

	/// returns the buffer size
	/**
	 * This function returns the size of the buffer needed for the communication
	 * of passed interface. If the vector has fixed size entries this is just
	 * the number of interface entries times the size of the entry. In case
	 * of a variable size entry type a negative value is returned to indicate
	 * that no buffer size can be determined in advanced.
	 *
	 * \param[in]	interface	Interface that will communicate
	 */
		virtual int
		get_required_buffer_size(Interface& interface)
		{
			if(block_traits<typename TVector::value_type>::is_static)
				return interface.size() * sizeof(typename TVector::value_type);
			else
				return -1;
		}

	///	writes the interface values into a buffer that will be sent
	/**
	 * This function collects all entries of the vector into a buffer that
	 * are part of the interface.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that will communicate
	 */
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	copy value into buffer
				Serialize(buff, v[index]);
			}
			return true;
		}

	///	scales values of a buffer and writes the into the interface
	/**
	 * This function writes the scaled buffer values into the vector.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that communicates
	 */
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	copy value from buffer
				Deserialize(buff, v[index]);

			//	scale value
				v[index] *= m_scale;
			}
			return true;
		}

	private:
		TVector* m_pVec;

		number m_scale;
};

/// Communication Policy to add values of a vector
/**
 * This class is used as a policy to add values on the interfaces of a
 * parallel vector. The collecting interfaces pack the values on the interface
 * into a stream. The extracting interfaces receive the stream and adds
 * the values of the vector.
 *
 * \tparam	TVector		Vector type
 */
template <class TVector>
class ComPol_VecAdd : public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Default constructor
		ComPol_VecAdd() : m_pVecDest(NULL), m_pVecSrc(NULL)	{}

	///	Constructor setting the values
		ComPol_VecAdd(TVector* pVec) : m_pVecDest(pVec),  m_pVecSrc(pVec)	{}

	///	Constructor setting the values
		ComPol_VecAdd(TVector* pVecDest, const TVector* pVecSrc)
			: m_pVecDest(pVecDest),  m_pVecSrc(pVecSrc)	{}

	///	sets the vector used in communication
		void set_vector(TVector* pVec)	{m_pVecDest = pVec; m_pVecSrc = pVec;}

	///	sets the vector used in communication
		void set_vector(TVector* pVecDest, const TVector* pVecSrc)
			{m_pVecDest = pVecDest; m_pVecSrc = pVecSrc;}

	/// returns the buffer size
	/**
	 * This function returns the size of the buffer needed for the communication
	 * of passed interface. If the vector has fixed size entries this is just
	 * the number of interface entries times the size of the entry. In case
	 * of a variable size entry type a negative value is returned to indicate
	 * that no buffer size can be determined in advanced.
	 *
	 * \param[in]	interface	Interface that will communicate
	 */
		virtual int
		get_required_buffer_size(Interface& interface)
		{
			if(block_traits<typename TVector::value_type>::is_static)
				return interface.size() * sizeof(typename TVector::value_type);
			else
				return -1;
		}

	///	writes the interface values into a buffer that will be sent
	/**
	 * This function collects all entries of the vector into a buffer that
	 * are part of the interface.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that will communicate
	 */
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVecSrc == NULL) return false;

		//	rename for convenience
			const TVector& v = *m_pVecSrc;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			// copy entry
				Serialize(buff, v[index]);
			}
			return true;
		}

	///	adds values of a buffer to the interface values
	/**
	 * This function adds the buffer values to the vector values.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that communicates
	 */
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVecDest == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVecDest;

		//	entry
			typename TVector::value_type entry;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	copy entry
				Deserialize(buff, entry);

			//	add entry
				v[index] += entry;
			}
			return true;
		}

	private:
		TVector* m_pVecDest;
		const TVector* m_pVecSrc;
};

/// Communication Policy to add values of a vector
/**
 * This class is used as a policy to add values on the interfaces of a
 * parallel vector. The collecting interfaces pack the values on the interface
 * into a stream. The extracting interfaces receive the stream and adds
 * the values of the vector.
 *
 * \tparam	TVector		Vector type
 */
template <class TVector>
class ComPol_VecScaleAdd : public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Default constructor
		ComPol_VecScaleAdd() : m_pVec(NULL), m_scale(1.0) {}

	///	Constructor setting the values
		ComPol_VecScaleAdd(TVector* pVec) : m_pVec(pVec)	{}

	///	sets the vector used in communication
		void set_vector(TVector* pVec)	{m_pVec = pVec;}

	///	sets the scaling factor
		void set_scale(number scale)	{m_scale = scale;}

	/// returns the buffer size
	/**
	 * This function returns the size of the buffer needed for the communication
	 * of passed interface. If the vector has fixed size entries this is just
	 * the number of interface entries times the size of the entry. In case
	 * of a variable size entry type a negative value is returned to indicate
	 * that no buffer size can be determined in advanced.
	 *
	 * \param[in]	interface	Interface that will communicate
	 */
		virtual int
		get_required_buffer_size(Interface& interface)
		{
			if(block_traits<typename TVector::value_type>::is_static)
				return interface.size() * sizeof(typename TVector::value_type);
			else
				return -1;
		}

	///	writes the interface values into a buffer that will be sent
	/**
	 * This function collects all entries of the vector into a buffer that
	 * are part of the interface.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that will communicate
	 */
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			// copy entry
				Serialize(buff, v[index]);
			}
			return true;
		}

	///	scales and adds values of a buffer to the interface values
	/**
	 * This function sclaes and adds the buffer values to the vector values.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that communicates
	 */
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	entry
			typename TVector::value_type entry;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	copy entry
				Deserialize(buff, entry);

			//	add entry
				v[index] += entry * m_scale;
			}
			return true;
		}

	private:
		TVector* m_pVec;

		number m_scale;
};


/// Communication Policy to add values of a vector and reset value to zero on sending interface
/**
 * This class is used as a policy to add values on the interfaces of a
 * parallel vector and set the values to zero on the sending interface.
 * The collecting interfaces pack the values on the interface and sets them to
 * zero into a stream. The extracting interfaces receive the stream and adds
 * the values to those of the vector.
 *
 * \tparam	TVector		Vector type
 */
template <class TVector>
class ComPol_VecAddSetZero : public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Default Constructor
		ComPol_VecAddSetZero() : m_pVec(NULL)	{}

	///	Constructor setting vector
		ComPol_VecAddSetZero(TVector* pVec) : m_pVec(pVec)	{}

	///	sets the vector that we be used for communication
		void set_vector(TVector* pVec)	{m_pVec = pVec;}

	/// returns the buffer size
	/**
	 * This function returns the size of the buffer needed for the communication
	 * of passed interface. If the vector has fixed size entries this is just
	 * the number of interface entries times the size of the entry. In case
	 * of a variable size entry type a negative value is returned to indicate
	 * that no buffer size can be determined in advanced.
	 *
	 * \param[in]	interface	Interface that will communicate
	 */
		virtual int
		get_required_buffer_size(Interface& interface)
		{
			if(block_traits<typename TVector::value_type>::is_static)
				return interface.size() * sizeof(typename TVector::value_type);
			else
				return -1;
		}

	///	writes the interface values into a buffer that will be sent and then sets
	/// the value to zero on the interface
	/**
	 * This function collects all entries of the vector into a buffer that
	 * are part of the interface. The values on the interface are then set to
	 * zero.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that will communicate
	 */
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			// copy values
				Serialize(buff, v[index]);

			// set to zero on this process
				v[index] = 0.0;
			}
			return true;
		}

	///	adds the values of a buffer to the values on the interface
	/**
	 * This function adds the values of the buffer to the interface values.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that communicates
	 */
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	entry
			typename TVector::value_type entry;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	copy values
				Deserialize(buff, entry);

			//	add entry
				v[index] += entry;
			}
			return true;
		}

	private:
		TVector* m_pVec;
};

/// Communication Policy to subtract values of a vector
/**
 * This class is used as a policy to subtract values on the interfaces of a
 * parallel vector. The collecting interfaces pack the values on the interface
 * into a stream. The extracting interfaces receive the stream and subtracts
 * the values of the vector.
 *
 * \tparam	TVector		Vector type
 */
template <class TVector>
class ComPol_VecSubtract : public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Default constructor
		ComPol_VecSubtract() : m_pVec(NULL)	{}

	///	Constructor setting the values
		ComPol_VecSubtract(TVector* pVec) : m_pVec(pVec)	{}

	///	sets the vector used in communication
		void set_vector(TVector* pVec)	{m_pVec = pVec;}

	/// returns the buffer size
	/**
	 * This function returns the size of the buffer needed for the communication
	 * of passed interface. If the vector has fixed size entries this is just
	 * the number of interface entries times the size of the entry. In case
	 * of a variable size entry type a negative value is returned to indicate
	 * that no buffer size can be determined in advanced.
	 *
	 * \param[in]	interface	Interface that will communicate
	 */
		virtual int
		get_required_buffer_size(Interface& interface)
		{
			if(block_traits<typename TVector::value_type>::is_static)
				return interface.size() * sizeof(typename TVector::value_type);
			else
				return -1;
		}

	///	writes the interface values into a buffer that will be sent
	/**
	 * This function collects all entries of the vector into a buffer that
	 * are part of the interface.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that will communicate
	 */
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			// get index
				const size_t index = interface.get_element(iter);

			// copy value
				Serialize(buff, v[index]);
			}
			return true;
		}

	///	subtracts values of a buffer to the interface values
	/**
	 * This function subtracts the buffer values to the vector values.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that communicates
	 */
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		// entry
			typename TVector::value_type entry;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	copy vector
				Deserialize(buff, entry);

			//	subtract entry
				v[index] -= entry;
			}
			return true;
		}

	private:
		TVector* m_pVec;
};

/// Communication Policy to subtract only one slave value per master of a vector
/**
 * This class is used as a policy to subtract values on the interfaces of a
 * parallel vector. The collecting interfaces - supposed to consist of slave
 * dofs - pack the values on the interface into a stream. 
 * The extracting interfaces - supposed to consist of master dofs - receive
 * the stream and subtracts only one slave value (the first) per master.
 *
 * \tparam	TVector		Vector type
 */
template <class TVector>
class ComPol_VecSubtractOnlyOneSlave : public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Default constructor
		ComPol_VecSubtractOnlyOneSlave() : m_pVec(NULL)	{}

	///	Constructor setting the values
		ComPol_VecSubtractOnlyOneSlave(TVector* pVec) 
	  	{
			set_vector(pVec);
		}

	///	sets the vector used in communication
		void set_vector(TVector* pVec) 
		{
			m_pVec = pVec;
			m_vProcessed.resize(m_pVec->size(), false);
		}

	/// clear processed flag
		void clear()
		{
			m_vProcessed.clear();
			m_vProcessed.resize(m_pVec->size(), false);
		}

	/// returns the buffer size
	/**
	 * This function returns the size of the buffer needed for the communication
	 * of passed interface. If the vector has fixed size entries this is just
	 * the number of interface entries times the size of the entry. In case
	 * of a variable size entry type a negative value is returned to indicate
	 * that no buffer size can be determined in advanced.
	 *
	 * \param[in]	interface	Interface that will communicate
	 */
		virtual int
		get_required_buffer_size(Interface& interface)
		{
			if(block_traits<typename TVector::value_type>::is_static)
				return interface.size() * sizeof(typename TVector::value_type);
			else
				return -1;
		}

	///	writes the interface values into a buffer that will be sent
	/**
	 * This function collects all entries of the vector into a buffer that
	 * are part of the interface.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that will communicate
	 */
		virtual bool
		collect(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			// get index
				const size_t index = interface.get_element(iter);

			// copy value
				Serialize(buff, v[index]);
			}
			return true;
		}

	///	subtracts values of a buffer to the interface values
	/**
	 * This function subtracts the buffer values to the vector values.
	 *
	 * \param[out]		buff		Buffer
	 * \param[in]		interface	Interface that communicates
	 */
		virtual bool
		extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	check that vector has been set
			if(m_pVec == NULL) return false;

		//	rename for convenience
			TVector& v = *m_pVec;

		// entry
			typename TVector::value_type entry;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	copy vector
				Deserialize(buff, entry);

			//	subtract entry
				if(m_vProcessed[index] == false)
				{
					v[index] -= entry;
					m_vProcessed[index] = true;
				}
			}
			return true;
		}

	private:
		TVector* m_pVec;
		std::vector<bool> m_vProcessed;
};

///	Generates a set of global algebra ids.
/**	Make sure that masterLayout and slaveLayout do not reference
 * indices >= numIDs.
 */
template <class TLayout>
void GenerateGlobalAlgebraIDs(pcl::InterfaceCommunicator<TLayout>& communicator,
		std::vector<AlgebraID>& idsOut,
		size_t numIDs,
		TLayout& masterLayout,
		TLayout& slaveLayout)
{
//	generate an id for each entry.
	idsOut.resize(numIDs);
	int localProc = pcl::GetProcRank();
	for(size_t i = 0; i < numIDs; ++i)
		idsOut[i] = AlgebraID(localProc, i);

//	copy all ids from master to slave interfaces
	ComPol_VecCopy<std::vector<AlgebraID> >	copyPol(&idsOut);

	communicator.send_data(masterLayout, copyPol);
	communicator.receive_data(slaveLayout, copyPol);
	communicator.communicate();

//	a set of global ids has now been generated.
}

/// Communication Policy to copy slave couplings to master row
/**
 * This Communication policy assumes that the matrix is stored additively with
 * no overlap. It then copies the row entries of the slaves indices to the
 * corresponding master rows. Only those values are copied that have if the index
 * the slave couples to has also a representation on the other process. This means
 * that couplings to inner indices or to other slave/master, that have only
 * connections to other processes, are not taken into account.
 *
 * \tparam TMatrix	matrix type
 */
template <class TMatrix>
class ComPol_MatAddSlaveRowsToMasterOverlap0
	: public pcl::ICommunicationPolicy<IndexLayout>
{
	public:
	///	Constructor setting the vector
	/**
	 * vGlobalID must have size >= mat.num_rows()
	 */
		ComPol_MatAddSlaveRowsToMasterOverlap0(TMatrix& rMat, AlgebraIDVec& vGlobalID)
			: m_rMat(rMat), m_vGlobalID(vGlobalID)
		{
			UG_ASSERT(vGlobalID.size() >= m_rMat.num_rows(), "too few Global ids");
		}

	///	writes the interface values into a buffer that will be sent
		virtual bool collect(ug::BinaryBuffer& buff, Interface& interface)
		{
			typedef typename TMatrix::row_iterator row_iterator;
			typedef typename TMatrix::value_type block_type;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	count number of row entries
				const row_iterator rowEnd = m_rMat.end_row(index);
				size_t numRowEntry = 0;
				for(row_iterator it_k = m_rMat.begin_row(index); it_k != rowEnd; ++it_k)
					numRowEntry++;

			//	write number of row entries to stream
				Serialize(buff, numRowEntry);

			//	write entries and global id to stream
				for(row_iterator it_k = m_rMat.begin_row(index); it_k != rowEnd; ++it_k)
				{
					const size_t k = it_k.index();
					block_type& a_ik = it_k.value();

				//	write global entry to buffer
					Serialize(buff, m_vGlobalID[k]);

				//	write entry into buffer
					Serialize(buff, a_ik);
				}
			}

		///	done
			return true;
		}

		virtual bool
		begin_layout_extraction(Layout* pLayout)
		{
		//	fill the map global->local
			GenerateAlgebraIDHashList(m_algIDHash, m_vGlobalID);
			return true;
		}

	///	writes values from a buffer into the interface
		virtual bool extract(ug::BinaryBuffer& buff, Interface& interface)
		{
		//	block type of associated matrix
			typedef typename TMatrix::value_type block_type;

		//	we'll read global ids into this variable
			AlgebraID gID;

		//	we'll read blocks into this var
			block_type block;

		//	loop interface
			for(typename Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
			{
			//	get index
				const size_t index = interface.get_element(iter);

			//	read the number of connections
				size_t numConnections = 0;
				Deserialize(buff, numConnections);

			//	read each connection
				for(size_t i_conn = 0; i_conn < numConnections; ++i_conn){
					Deserialize(buff, gID);
					Deserialize(buff, block);

				//	if gID exists on this process, then add the connection to
				//	the matrix.
					AlgebraIDHashList::Iterator ibegin, iend;
					m_algIDHash.get_iterators(ibegin, iend, gID);
					if(ibegin != iend){
					//	add connection between index and *ibegin to matrix
						m_rMat(index, *ibegin) += block;
					}
				}
			}

		///	done
			return true;
		}

	private:
	//	pointer to current vector
		TMatrix& m_rMat;

	//	map localID->globalID
		AlgebraIDVec& m_vGlobalID;

	//	map globalID->localID
		AlgebraIDHashList	m_algIDHash;

};


/// @}

}//	end of namespace


#endif /* __H__LIB_ALGEBRA__PARALLELIZATION__COMMUNICATION_POLICIES__ */
