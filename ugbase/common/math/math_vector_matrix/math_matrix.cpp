/*
 * math_matrix.cpp
 *
 *  Created on: 08.07.2009
 *      Author: andreasvogel
 */

#include "math_matrix.h"
#include  <iostream>

namespace ug{

std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<2,2>& m)
{
	using namespace std;
	for(uint i = 0; i < 2; ++i)
	{
		cout << "|";
		for(uint j = 0; j < 2; ++j)
		{
			cout << scientific << setprecision(8) << setw(15) << m.entry(i, j);
		}
		cout << " |" << endl;
	}
	return outStream;
}

std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<2,3>& m)
{
	using namespace std;
	for(uint i = 0; i < 2; ++i)
	{
		cout << "|";
		for(uint j = 0; j < 3; ++j)
		{
			cout << scientific << setprecision(8) << setw(15) << m.entry(i, j);
		}
		cout << " |" << endl;
	}
	return outStream;
}

std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<3,2>& m)
{
	using namespace std;
	for(uint i = 0; i < 3; ++i)
	{
		cout << "|";
		for(uint j = 0; j < 2; ++j)
		{
			cout << scientific << setprecision(8) << setw(15) << m.entry(i, j);
		}
		cout << " |" << endl;
	}
	return outStream;
}


std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<3,3>& m)
{
	using namespace std;
	for(uint i = 0; i < 3; ++i)
	{
		cout << "|";
		for(uint j = 0; j < 3; ++j)
		{
			cout << scientific << setprecision(8) << setw(15) << m.entry(i, j);
		}
		cout << " |" << endl;
	}
	return outStream;
}


} //end of namespace: ug

