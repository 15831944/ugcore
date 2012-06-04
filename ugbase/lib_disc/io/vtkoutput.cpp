/*
 * vtkoutput.cpp
 *
 *  Created on: 25.04.2012
 *      Author: andreasvogel
 */

#include "vtkoutput.h"

namespace ug{

////////////////////////////////////////////////////////////////////////////////
//	Domain Output
////////////////////////////////////////////////////////////////////////////////
template <int TDim>
void VTKOutput<TDim>::
print(const char* filename, Domain<TDim>& domain)
{
//	get the grid associated to the solution
	MultiGrid& grid = *domain.grid();
	MGSubsetHandler& sh = *domain.subset_handler();

// 	attach help indices
	typedef ug::Attachment<int> AVrtIndex;
	AVrtIndex aVrtIndex;
	Grid::VertexAttachmentAccessor<AVrtIndex> aaVrtIndex;
	grid.attach_to_vertices(aVrtIndex);
	aaVrtIndex.access(grid, aVrtIndex);

//	get rank of process
	int rank = 0;
#ifdef UG_PARALLEL
	rank = pcl::GetProcRank();
#endif

	const int si = -1;

//	get name for *.vtu file
	std::string name;
	try{
		vtu_filename(name, filename, rank, si, sh.num_subsets()-1, -1);
	}
	UG_CATCH_THROW("VTK::print_subset: Can not write vtu - file.");


//	open the file
	try
	{
	VTKFileWriter File(name.c_str());

//	header
	File.write("<?xml version=\"1.0\"?>\n");
	File.write("<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"");
#ifdef __SWAPBYTES__
	File.write("LittleEndian");
#else
	File.write("BigEndian");
#endif
	File.write("\">\n");

//	opening the grid
	File.write("  <UnstructuredGrid>\n");

// 	get dimension of grid-piece
	int dim = DimensionOfSubsets(sh);

//	write piece of grid
	if(dim >= 0)
	{
		try{
			write_grid_piece<MGSubsetHandler>
			(File, aaVrtIndex, domain.position_accessor(), grid,
			 sh, si, dim);
		}
		UG_CATCH_THROW("VTK::print: Can not write Subset: "<<si);
	}
	else
	{
	//	if dim < 0, some is wrong with grid, except no element is in the grid
		if( ((si < 0) && grid.num<VertexBase>() != 0) ||
			((si >=0) && sh.num<VertexBase>(si) != 0))
		{
			UG_THROW("VTK::print: Dimension of grid/subset not"
					" detected correctly although grid objects present.");
		}

		write_empty_grid_piece(File);
	}

//	write closing xml tags
	File.write("  </UnstructuredGrid>\n");
	File.write("</VTKFile>\n");

// 	detach help indices
	grid.detach_from_vertices(aVrtIndex);

	}
	UG_CATCH_THROW("VTK::print: Can not open Output File: "<< filename);
}

template <int TDim>
void VTKOutput<TDim>::
write_empty_grid_piece(VTKFileWriter& File)
{
//	write that no elements are in the grid
	int n = 0;
	File.write("    <Piece NumberOfPoints=\"0\" NumberOfCells=\"0\">\n");
	File.write("      <Points>\n");
	File.write("        <DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"binary\">\n");
	File.write_base64(n);
	File.write("\n        </DataArray>\n");
	File.write("      </Points>\n");
	File.write("      <Cells>\n");
	File.write("        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"binary\">\n");
	File.write_base64(n);
	File.write("\n        </DataArray>\n");
	File.write("        <DataArray type=\"Int32\" Name=\"offsets\" format=\"binary\">\n");
	File.write_base64(n);
	File.write("\n        </DataArray>\n");
	File.write("        <DataArray type=\"Int8\" Name=\"types\" format=\"binary\">\n");
	File.write_base64(n);
	File.write("\n        </DataArray>\n");
	File.write("      </Cells>\n");
	File.write("    </Piece>\n");
}

////////////////////////////////////////////////////////////////////////////////
// FileNames
////////////////////////////////////////////////////////////////////////////////


template <int TDim>
void VTKOutput<TDim>::
vtu_filename(std::string& nameOut, std::string nameIn, int rank,
             int si, int maxSi, int step)
{
//	copy name
	nameOut = nameIn.substr(0, nameIn.find_first_of('.'));

#ifdef UG_PARALLEL
// 	process index
	if(pcl::GetNumProcesses() > 1)
		AppendCounterToString(nameOut, "_p", rank, pcl::GetNumProcesses() - 1);
#endif

// 	subset index
	if(si >= 0)
		AppendCounterToString(nameOut, "_s", si, maxSi);

// 	time index
	if(step >= 0)
		AppendCounterToString(nameOut, "_t", (int) step);

// 	add file extension
	nameOut.append(".vtu");
}


template <int TDim>
void VTKOutput<TDim>::
pvtu_filename(std::string& nameOut, std::string nameIn,
              int si, int maxSi, int step)
{
//	copy name
	nameOut = nameIn.substr(0, nameIn.find_first_of('.'));

// 	subset index
	if(si >= 0)
		AppendCounterToString(nameOut, "_s", si, maxSi);

// 	time index
	if(step >= 0)
		AppendCounterToString(nameOut, "_t", (int) step);

// 	add file extension
	nameOut.append(".pvtu");
}


template <int TDim>
void VTKOutput<TDim>::
pvd_filename(std::string& nameOut, std::string nameIn)
{
//	copy name
	nameOut = nameIn.substr(0, nameIn.find_first_of('.'));

// 	add file extension
	nameOut.append(".pvd");
}


template <int TDim>
void VTKOutput<TDim>::
pvd_time_filename(std::string& nameOut, std::string nameIn, int step)
{
//	copy name
	nameOut = nameIn.substr(0, nameIn.find_first_of('.'));

// 	time index
	if(step >= 0)
		AppendCounterToString(nameOut, "_t", (int) step);

// 	add file extension
	nameOut.append(".pvd");
}

template <int TDim>
void VTKOutput<TDim>::
write_subset_pvd(int numSubset, const std::string& filename, int step, number time)
{
//	file pointer
	FILE* file;

//	to store name of file
	std::string name;

//	get rank, outproc bool and number of processes
	bool isOutputProc = GetLogAssistant().is_output_process();
	int rank = 0;
	int numProcs = 1;

#ifdef UG_PARALLEL
	rank = pcl::GetProcRank();
	numProcs = pcl::GetNumProcesses();
#endif

//	only output proc writes this file
	if (isOutputProc)
	{
	//	get file name
		if(step >= 0) pvd_time_filename(name, filename, step);
		else pvd_filename(name, filename);

	//	open file
		file = fopen(name.c_str(), "w");
		if (file == NULL)
			UG_THROW("VTKOutput: Cannot print to file.");

	// 	Write beginning of file
		fprintf(file, "<?xml version=\"1.0\"?>\n");
		fprintf(file, "<VTKFile type=\"Collection\" version=\"0.1\">\n");
		fprintf(file, "  <Collection>\n");

	// 	Include files for all subsets
		for(int si = 0; si < numSubset; ++si)
		{
			vtu_filename(name, filename, rank, si, numSubset-1, step);
			if(numProcs > 1) pvtu_filename(name, filename, si, numSubset-1, step);

			name = FilenameWithoutPath(name);
			fprintf(file, "  <DataSet timestep=\"%g\" part=\"%d\" file=\"%s\"/>\n",
			        		time, si, name.c_str());
		}

	//	write closing tag
		fprintf(file, "  </Collection>\n");
		fprintf(file, "</VTKFile>\n");
		fclose(file);
	}

	if (isOutputProc && numProcs > 1)
	{
		std::string procName(filename);
		procName.append("_processwise");

	//	get file name
		if(step >= 0) pvd_time_filename(name, filename, step);
		else pvd_filename(name, filename);

	//	open File
		file = fopen(name.c_str(), "w");
		if (file == NULL)
			UG_THROW("VTKOutput: Cannot print to file.");

	// 	Write to file
		fprintf(file, "<?xml version=\"1.0\"?>\n");
		fprintf(file, "<VTKFile type=\"Collection\" version=\"0.1\">\n");
		fprintf(file, "  <Collection>\n");

	// 	include files from all procs
		for (int r = 0; r < numProcs; r++)
			for(int si = 0; si < numSubset; ++si)
			{
				vtu_filename(name, filename, rank, si, numSubset-1, step);
				if(numProcs > 1) pvtu_filename(name, filename, si, numSubset-1, step);

				name = FilenameWithoutPath(name);
				fprintf(file, "  <DataSet timestep=\"%g\" part=\"%d\" file=\"%s\"/>\n",
				        	time, r, name.c_str());
			}

	//	end file
		fprintf(file, "  </Collection>\n");
		fprintf(file, "</VTKFile>\n");
		fclose(file);
	}
}

template <int TDim>
void VTKOutput<TDim>::
select_nodal(const char* fctNames, const char* name)
{
	TrimString(name);
	std::vector<std::string> tokens;
	std::string fctString(fctNames);
	TokenizeString(fctString, tokens, ',');
	for(size_t i = 0; i < tokens.size(); ++i) TrimString(tokens[i]);

//	check that admissible number of components passed
	if(tokens.size() != 1 && tokens.size() != (size_t)TDim)
		UG_THROW("VTK:select_nodal: In order to select"
				" a nodal data of a grid function for output to vtk,"
				" 1 or "<<TDim<<" function components must be chosen, but passed are "
				<<tokens.size()<<" components in '"<<fctNames<<"'. Please select 1 or "<<
			        TDim<<" components in world dimension "<<TDim);

//	check if name is not in use
	if(vtk_name_used(name))
		UG_THROW("VTK:select_nodal: Using name " << name <<
			       " that is already used by other data is not allowed.");

	m_vSymbFct.push_back(std::pair<std::string, std::string>(fctNames, name));
}

template <int TDim>
bool VTKOutput<TDim>::
vtk_name_used(const char* name) const
{
	for(size_t j = 0; j < m_vSymbFct.size(); ++j)
		if(m_vSymbFct[j].second == name)
			return true;

	for(size_t j = 0; j < m_vScalarData.size(); ++j)
		if(m_vScalarData[j].second == name)
			return true;

	for(size_t j = 0; j < m_vVectorData.size(); ++j)
		if(m_vVectorData[j].second == name)
			return true;

	return false;
}



template <int TDim>
void VTKOutput<TDim>::
select_nodal(SmartPtr<IDirectIPData<number, TDim> > spData, const char* name)
{
	TrimString(name);
	if(vtk_name_used(name))
		UG_THROW("VTK:select_nodal: Using name " << name <<
			       " that is already used by other data is not allowed.");

	if(!spData->is_continuous())
		UG_THROW("VTK:select_nodal: data with vtk-name " << name <<
			       " is not continuous. Cannot write it as nodal data.");

	m_vScalarData.push_back(std::pair<SmartPtr<IDirectIPData<number, TDim> >,std::string>(spData, name));
}

template <int TDim>
void VTKOutput<TDim>::
select_nodal(SmartPtr<IDirectIPData<MathVector<TDim>, TDim> > spData, const char* name)
{
	TrimString(name);
	if(vtk_name_used(name))
		UG_THROW("VTK:select_nodal: Using name " << name <<
			       " that is already used by other data is not allowed.");

	if(!spData->is_continuous())
		UG_THROW("VTK:select_nodal: data with vtk-name " << name <<
			       " is not continuous. Cannot write it as nodal data.");

	m_vVectorData.push_back(std::pair<SmartPtr<IDirectIPData<MathVector<TDim>, TDim> >,std::string>(spData, name));
}

#ifdef UG_DIM_1
template class VTKOutput<1>;
#endif
#ifdef UG_DIM_2
template class VTKOutput<2>;
#endif
#ifdef UG_DIM_3
template class VTKOutput<3>;
#endif

} // end namespace ug
