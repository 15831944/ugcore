/*
 * parallel_matrix_overlap_impl.h
 *
 *  Created on: 17.01.2010
 *      Author: Martin Rupp / Sebastian Reiter
 */

#ifndef __H__LIB_ALGEBRA__PARALLELIZATION__PARALLEL_MATRIX_OVERLAP_IMPL__
#define __H__LIB_ALGEBRA__PARALLELIZATION__PARALLEL_MATRIX_OVERLAP_IMPL__

#include <vector>
#include <set>
#include <map>

#include "parallelization_util.h"
//#include "test_layout.h"
#include "common/util/sort_util.h"
#include "common/util/binary_buffer.h"

#include "parallel_nodes.h"

#include "row_sending_scheme.h"
#include "new_layout_creator.h"

namespace ug
{

// neuer algorithmus:
// overlap 0
// 1. slave-knoten verschicken ihre Zeile an den Master.
// 2. Master nimmt diese entgegen, zeile wird f�r vorhandene Knoten addiert
// 3. fertig.

// overlap 1
// 1. slave-knoten verschicken ihre Zeile an den Master.
//    /!\ werden verkn�pfungen zu anderen prozessoren verschickt, werden die prozessoren informiert
//    /!\ unter umst�nden wird so ein prozessor "von 2 seiten" informiert. dann muss es eine
// 2. verschicke die matrixzeilen und benachrichtungen
// 3. nehme matrixzeilen und benachrichtungen entgegen
// 4. verarbeite benachrichtungen: erzeuge u.U. neue Master Knoten
// 5. verarbeite matrixzeilen und erzeugt u.U. neue Knoten (Slave). Diese werden als "neu" markiert
//
// \param mat matrix to take values from
// \param layout : for all interfaces I in layout, send matrix rows of nodes in I with com
// \param create_new_nodes : if true, do not add nodes to masterOLLayout
// \param masterOLLayout : for nodes which will be new on the other processors, add them to this layout
// \param pids ... unclear if this is needed




template<typename matrix_type>
class GenerateOverlapClass
{
private:
	typedef IndexLayout::Interface Interface;
	typedef std::map<int, BinaryBuffer>	BufferMap;

	pcl::ParallelCommunicator<IndexLayout> &m_com; ///< communicator


	matrix_type &m_mat;		///< the original matrix (should be const)
	matrix_type &m_newMat;	///< the new to create matrix

	IndexLayout &m_totalMasterLayout;	///< layout combining all master layouts from overlap 0 to overlap_depth-1
	IndexLayout &m_totalSlaveLayout;	///< layout combining all slave layouts from overlap 0 to overlap_depth-1

	std::vector<IndexLayout> &m_vMasterLayouts; 	///< m_vMasterLayout[i] is the master layout from overlap i without others
	std::vector<IndexLayout> &m_vSlaveLayouts;	///< m_vSlaveLayout[i] is the slave layout from overlap i without others
	ParallelNodes &PN;
		//size_t m_overlapDepth;						///< overlap depth to be achieved

public:
	std::vector<size_t> m_overlapSize;
	size_t m_overlapDepthMaster;
	size_t m_overlapDepthSlave;
	bool m_masterDirichletLast;
	bool m_slaveDirichletLast;
private:

	/// communicate
	/**
	 * \brief creates one overlap level one-sided
	 * \param sendingNodesLayout Layout of which nodes send their matrix rows
	 * \param receivingNodesLayout Layout of which nodes receive matrix rows
	 * \param bCreateNewNodes if true, create new indices/interfaces for globalIndices which are not yet on this or the other processor
	 * \param newSlavesLayout new slaves are added to this layout
	 * \param newMastersLayout new masters are added to this layout
	 * \param pids used to send/receive notifications
	 * \param bSet if true, use overwrite (set) for received matrix rows. else add matrix rows.
	 * \param level current overlap level
	 */

	void communicate(IndexLayout &sendingLayout, IndexLayout &receivingLayout,
			bool bCreateNewNodes,
			IndexLayout &newSlavesLayout, IndexLayout &newMastersLayout,
			std::set<int> &pids, bool bSet, size_t level)
	{
		PROFILE_FUNC();

		UG_DLOG(LIB_ALG_MATRIX, 4, "\n\n*** GenerateOverlapClass::communicate ***\n\n")
		IF_DEBUG(LIB_ALG_MATRIX, 4)
		{
			UG_LOG("\n\nnew sending Layout:\n")
			PrintLayout(sendingLayout);
			UG_LOG("\n\nnew receiving Layout:\n")
			PrintLayout(receivingLayout);
			UG_LOG("\n\n");

			UG_ASSERT(TestLayout(m_com, sendingLayout, receivingLayout), "layout corrupted");
		}

		RowSendingScheme<matrix_type> rowSendingScheme(m_newMat, PN);

		rowSendingScheme.set_create_new_nodes(bCreateNewNodes);
		rowSendingScheme.issue_send(m_com, sendingLayout, receivingLayout);
		m_com.communicate();

		rowSendingScheme.process(receivingLayout);

		if(bCreateNewNodes)
		{
			m_newMat.resize(PN.local_size(), PN.local_size());
			PN.add_new_layouts_to(newMastersLayout, newSlavesLayout);

			AddLayout(m_totalMasterLayout, newMastersLayout);
			AddLayout(m_totalSlaveLayout, newSlavesLayout);

			IF_DEBUG(LIB_ALG_MATRIX, 4)
			{
				UG_LOG("\n\nnew Master Layout:\n")
				PrintLayout(newMastersLayout);
				UG_LOG("\n\nnew Slave Layout:\n")
				PrintLayout(newSlavesLayout);
				UG_LOG("\n\n");

				UG_ASSERT(TestLayout(m_com, newMastersLayout, newSlavesLayout), "layout corrupted");
			}


			AddLayout(m_vMasterLayouts[level+1], newMastersLayout);
			AddLayout(m_vSlaveLayouts[level+1], newSlavesLayout);
		}
		if(bSet)
			rowSendingScheme.set_rows_in_matrix(m_newMat);
		else
			rowSendingScheme.add_rows_to_matrix(m_newMat);
	}

public:
	// GenerateOverlap
		//--------------------------------------------------
		/**
		 * \brief Generates a new matrix with overlap from another matrix
		 * \param _mat				matrix to create overlap from
		 * \param newMat			matrix to store overlap matrix in
		 * \param masterOLLayout	Layout
		 * \param masterOLLayout
		 * \param overlap_depth
		 *
		 */
	GenerateOverlapClass(matrix_type &mat, matrix_type &newMat, IndexLayout &totalMasterLayout, IndexLayout &totalSlaveLayout,
			std::vector<IndexLayout> &vMasterLayouts, std::vector<IndexLayout> &vSlaveLayouts,
			ParallelNodes &pn) :
		m_com(mat.get_communicator()), m_mat(mat), m_newMat(newMat), m_totalMasterLayout(totalMasterLayout), m_totalSlaveLayout(totalSlaveLayout),
		m_vMasterLayouts(vMasterLayouts), m_vSlaveLayouts(vSlaveLayouts),
		PN(pn)
	{

	}

	/// calculate
	/**
	 * calculates overlap
	 */
	bool calculate()
	{
		PROFILE_FUNC();
		IF_DEBUG(LIB_ALG_MATRIX, 4)
		{
			UG_DLOG(LIB_ALG_MATRIX, 4, "GENERATE OVERLAP START\n");

			UG_DLOG(LIB_ALG_MATRIX, 4, "\n\nmatrix is " << m_mat.num_rows() << " x " << m_mat.num_cols() << "\n");
			m_mat.print();

			UG_LOG("\nmaster layout:\n");
			PrintLayout(m_mat.get_master_layout());
			UG_LOG("slave layout:\n");
			PrintLayout(m_mat.get_slave_layout());
			UG_LOG("\n\n");
		}
		UG_ASSERT(m_mat.num_rows() == m_mat.num_cols(), "atm only for square matrices");

		PROFILE_END(); PROFILE_BEGIN(calculate1);

		IndexLayout &masterLayout = m_mat.get_master_layout();
		IndexLayout &slaveLayout = m_mat.get_slave_layout();

		PROFILE_END(); PROFILE_BEGIN(calculate_TestLayout);
		IF_DEBUG(LIB_ALG_MATRIX, 1)
		{
			UG_ASSERT(TestLayout(m_com, masterLayout, slaveLayout), "layout corrupted");
		}

		PROFILE_END(); PROFILE_BEGIN(calculate1_2);

		// generate global algebra indices
		UG_DLOG(LIB_ALG_MATRIX, 4, "generate " << m_mat.num_rows() << " m_globalIDs\n");
		//GenerateGlobalAlgebraIDs(m_globalIDs, m_mat.num_rows(), masterLayout, slaveLayout);

		PROFILE_END(); PROFILE_BEGIN(calculate1_3);

		IF_DEBUG(LIB_ALG_MATRIX, 4)
		{
			for(size_t i=0; i<PN.local_size(); i++)
				UG_DLOG(LIB_ALG_MATRIX, 4, "  " << i << ": global id " << PN.local_to_global(i) << "\n")
		}


		m_newMat.set_as_copy_of(m_mat);

		PROFILE_END(); PROFILE_BEGIN(calculate1_4);

		// collect data
		//-----------------

		PROFILE_END(); PROFILE_BEGIN(calculate2);
		size_t maxOverlap = std::max(m_overlapDepthMaster, m_overlapDepthSlave);

		std::vector<IndexLayout> masterOLLayouts, slaveOLLayouts;
		masterOLLayouts.clear();
		masterOLLayouts.resize(maxOverlap+1);
		slaveOLLayouts.clear();
		slaveOLLayouts.resize(maxOverlap+1);

		std::vector<IndexLayout> backward_masterOLLayouts, backward_slaveOLLayouts;
		backward_masterOLLayouts.clear();
		backward_masterOLLayouts.resize(maxOverlap+1);
		backward_slaveOLLayouts.clear();
		backward_slaveOLLayouts.resize(maxOverlap+1);

		// TODO: try to remove these pid numbers or reduce them by introducing receivePIDs, sendPIDs
		// these are necessary because notifications can occur from a processor not in the current layout
		std::set<int> pids;
		for(IndexLayout::iterator iter = slaveLayout.begin(); iter != slaveLayout.end(); ++iter)
			pids.insert(iter->first);
		for(IndexLayout::iterator iter = masterLayout.begin(); iter != masterLayout.end(); ++iter)
			pids.insert(iter->first);


		//create_mark_map(masterLayout);
		m_vMasterLayouts.resize(maxOverlap+1);
		m_vSlaveLayouts.resize(maxOverlap+1);
		AddLayout(m_vMasterLayouts[0], masterLayout);
		AddLayout(m_vSlaveLayouts[0], slaveLayout);
		m_overlapSize.clear();
		PROFILE_END(); PROFILE_BEGIN(calculate3);
		for(size_t current_overlap=0; current_overlap <= maxOverlap; current_overlap++)
		{
			m_overlapSize.push_back(m_newMat.num_rows());

			IF_DEBUG(LIB_ALG_MATRIX, 4)
			{
				UG_DLOG(LIB_ALG_MATRIX, 4, "\n---------------------\ncurrentOL: " << current_overlap << "\n");
				UG_DLOG(LIB_ALG_MATRIX, 4, "---------------------\n\n");
			}

			if(current_overlap <= m_overlapDepthMaster)
			{
				UG_DLOG(LIB_ALG_MATRIX, 4, "\n--FORWARDS--\n\n");
				IndexLayout *send_layout;
				if(current_overlap == 0)
					send_layout = &slaveLayout;
				else
					send_layout = &masterOLLayouts[current_overlap-1];

				IndexLayout *receive_layout;
				if(current_overlap == 0)
					receive_layout = &masterLayout;
				else
					receive_layout = &slaveOLLayouts[current_overlap-1];

				if(current_overlap == m_overlapDepthMaster && m_masterDirichletLast)
				{
					PROFILE_BEGIN(calculate3_1a);
					std::vector<IndexLayout::Element> vIndex;
					CollectUniqueElements(vIndex,  *receive_layout);
					SetDirichletRow(m_newMat, vIndex);
				}
				else
				{
					PROFILE_BEGIN(calculate3_1b);
					bool bCreateNewNodes = (current_overlap == m_overlapDepthMaster ? false : true);
					communicate(*send_layout, *receive_layout, bCreateNewNodes,
						slaveOLLayouts[current_overlap], masterOLLayouts[current_overlap], pids, false, current_overlap);
				}
			}

			// backwards
			if(current_overlap <= m_overlapDepthSlave)
			{
				UG_DLOG(LIB_ALG_MATRIX, 4, "\n--BACKWARDS--\n\n");
				IndexLayout *backward_send_layout;
				if(current_overlap == 0)
					backward_send_layout = &masterLayout;
				else
					backward_send_layout = &backward_masterOLLayouts[current_overlap-1];

				IndexLayout *backward_receive_layout;
				if(current_overlap == 0)
					backward_receive_layout = &slaveLayout;
				else
					backward_receive_layout = &backward_slaveOLLayouts[current_overlap-1];

				if(current_overlap == m_overlapDepthSlave && m_slaveDirichletLast)
				{
					PROFILE_BEGIN(calculate3_2a);
					std::vector<IndexLayout::Element> vIndex;
					CollectUniqueElements(vIndex,  *backward_receive_layout);
					SetDirichletRow(m_newMat, vIndex);
				}
				else
				{
					PROFILE_BEGIN(calculate3_2b);
					bool bCreateNewNodes = (current_overlap == m_overlapDepthSlave ? false : true);
					communicate(*backward_send_layout, *backward_receive_layout, bCreateNewNodes,
						backward_slaveOLLayouts[current_overlap], backward_masterOLLayouts[current_overlap], pids, true, current_overlap+1);
				}
			}

			// done!
		}



		PROFILE_END(); PROFILE_BEGIN(calculate4);
		m_newMat.set_layouts(m_totalMasterLayout, m_totalSlaveLayout);
		m_newMat.set_communicator(m_mat.get_communicator());
		m_newMat.set_process_communicator(m_mat.get_process_communicator());
		m_newMat.copy_storage_type(m_mat);



		IF_DEBUG(LIB_ALG_MATRIX, 4)
		{
			UG_DLOG(LIB_ALG_MATRIX, 4, "new matrix\n\n");
			m_newMat.print();

			UG_DLOG(LIB_ALG_MATRIX, 4, "master OL Layout:\n");
			PrintLayout(m_totalMasterLayout);
			UG_DLOG(LIB_ALG_MATRIX, 4, "slave OL Layout:\n");
			PrintLayout(m_totalSlaveLayout);

			UG_DLOG(LIB_ALG_MATRIX, 4, "OL Layout:\n");
			PrintLayout(m_com, m_totalMasterLayout, m_totalSlaveLayout);
		}

		return true;
	}

};
/*
// TODO: one "bug" remains: dirichlet nodes, which have only connection to themselfs = 1.0, get afterwards 2.0 (because rows are not additive there)
template<typename matrix_type>
bool GenerateOverlap(const ParallelMatrix<matrix_type> &_mat, ParallelMatrix<matrix_type> &newMat,
		IndexLayout &totalMasterLayout, IndexLayout &totalSlaveLayout, std::vector<IndexLayout> &vMasterLayouts, std::vector<IndexLayout> &vSlaveLayouts,
		std::vector<size_t> &overlapSize,
		size_t overlapDepth=1)
{
	PROFILE_FUNC();
	// pcl does not use const much
	//UG_ASSERT(overlap_depth > 0, "overlap_depth has to be > 0");
	ParallelMatrix<matrix_type> &mat = const_cast<ParallelMatrix<matrix_type> &> (_mat);

	GenerateOverlapClass<ParallelMatrix<matrix_type> > c(mat, newMat, totalMasterLayout, totalSlaveLayout, vMasterLayouts, vSlaveLayouts);
	c.m_overlapDepthMaster = overlapDepth;
	c.m_overlapDepthSlave = overlapDepth;
	c.m_masterDirichletLast = false;
	c.m_slaveDirichletLast = false;
	bool b = c.calculate();
	overlapSize = c.m_overlapSize;
	return b;
}

// TODO: one "bug" remains: dirichlet nodes, which have only connection to themselfs = 1.0, get afterwards 2.0 (because rows are not additive there)
template<typename matrix_type>
bool GenerateOverlap2(const ParallelMatrix<matrix_type> &_mat, ParallelMatrix<matrix_type> &newMat,
		IndexLayout &totalMasterLayout, IndexLayout &totalSlaveLayout, std::vector<IndexLayout> &vMasterLayouts, std::vector<IndexLayout> &vSlaveLayouts,
		size_t overlapDepthMaster, size_t overlapDepthSlave, bool masterDirichletLast, bool slaveDirichletLast)
{
	PROFILE_FUNC();
	// pcl does not use const much
	//UG_ASSERT(overlap_depth > 0, "overlap_depth has to be > 0");
	ParallelMatrix<matrix_type> &mat = const_cast<ParallelMatrix<matrix_type> &> (_mat);
	PN(mat.get_communicator(), mat.get_master_layout(), mat.get_slave_layout(), mat.num_rows())

	GenerateOverlapClass<ParallelMatrix<matrix_type> > c(mat, newMat, totalMasterLayout, totalSlaveLayout, vMasterLayouts, vSlaveLayouts,
		PN);
	c.m_overlapDepthMaster = overlapDepthMaster;
	c.m_overlapDepthSlave = overlapDepthSlave;
	c.m_masterDirichletLast = masterDirichletLast;
	c.m_slaveDirichletLast = slaveDirichletLast;
	return c.calculate();
}*/

// TODO: one "bug" remains: dirichlet nodes, which have only connection to themselfs = 1.0, get afterwards 2.0 (because rows are not additive there)
template<typename matrix_type>
bool MakeConsistent(const ParallelMatrix<matrix_type> &_mat, ParallelMatrix<matrix_type> &newMat)
{
	PROFILE_FUNC();
	IndexLayout totalMasterLayout, totalSlaveLayout;
	std::vector<IndexLayout> vMasterLayouts;
	std::vector<IndexLayout> vSlaveLayouts;
	// pcl does not use const much
	//UG_ASSERT(overlap_depth > 0, "overlap_depth has to be > 0");
	ParallelMatrix<matrix_type> &mat = const_cast<ParallelMatrix<matrix_type> &> (_mat);
	ParallelNodes PN(mat.get_communicator(), mat.get_master_layout(), mat.get_slave_layout(), mat.num_rows());
	GenerateOverlapClass<ParallelMatrix<matrix_type> > c(mat, newMat, totalMasterLayout, totalSlaveLayout, vMasterLayouts, vSlaveLayouts,
			PN);
	c.m_overlapDepthMaster = 0;
	c.m_overlapDepthSlave = 0;
	c.m_masterDirichletLast = false;
	c.m_slaveDirichletLast = false;
	bool b = c.calculate();
	newMat.set_layouts(mat.get_master_layout(), mat.get_slave_layout());
	return b;
}

/*
template<typename matrix_type>
bool MakeFullRowsMatrix(const ParallelMatrix<matrix_type> &mat, ParallelMatrix<matrix_type> &newMat)
{
	// GenerateOverlap2(mat, newMat, totalMasterLayout, totalSlaveLayout, masterLayouts, slaveLayouts, 1, 0, true, true);
	return true;
}*/


}
#endif
