/*
Copyright (c) 2016, UT-Battelle, LLC

MERA++, Version 0.

This file is part of MERA++.
MERA++ is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
MERA++ is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with MERA++. If not, see <http://www.gnu.org/licenses/>.
*/
#include "TensorEval.h"
#include "Vector.h"

int main()
{
	PsimagLite::String str = "u0(s0)u1(s0)";

	SizeType dim0 = 5;
	typedef Mera::TensorEval<double> TensorEvalType;
	typedef TensorEvalType::TensorType TensorType;
	TensorEvalType::VectorTensorType vt(2);

	for (SizeType i = 0; i < vt.size(); ++i) {
		vt[i] = new TensorType(dim0,1);
		vt[i]->setToIdentity(1.0);
	}

	TensorEvalType::VectorPairStringSizeType idNames;
	idNames.push_back(TensorEvalType::PairStringSizeType("u",0));
	idNames.push_back(TensorEvalType::PairStringSizeType("u",1));
	TensorEvalType::MapPairStringSizeType nameIdTensor;
	nameIdTensor[idNames[0]] = 0;
	nameIdTensor[idNames[1]] = 1;
	TensorEvalType tensorEval(str,vt,idNames,nameIdTensor);
	TensorEvalType::VectorSizeType freeIndices;
	std::cout<<tensorEval(freeIndices)<<"\n";

	for (SizeType i = 0; i < vt.size(); ++i) {
		delete vt[i];
		vt[i] = 0;
	}
}
