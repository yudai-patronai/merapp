#ifndef ParametersForMera_H
#define ParametersForMera_H
#include "Vector.h"
#include "IoSimple.h"

namespace Mera {

template<typename ComplexOrRealType_>
struct ParametersForMera {

	typedef ComplexOrRealType_ ComplexOrRealType;
	typedef typename PsimagLite::Vector<ComplexOrRealType>::Type VectorType;

	ParametersForMera(VectorType& hTerms,
	                  SizeType h1,
	                  SizeType m1,
	                  PsimagLite::String eval)
	    : hamiltonianConnection(hTerms),h(h1),m(m1),verbose(false),evaluator(eval)
	{}

	ParametersForMera(PsimagLite::String filename)
	{
		PsimagLite::IoSimple::In io(filename);
		io.read(hamiltonianConnection, "hamiltonianConnection");
		io.readline(h, "h=");
		io.readline(m, "m=");
		int x = 0;
		io.readline(x, "verbose=");
		verbose = (x > 0);
		io.readline(evaluator, "evaluator=");
	}

	VectorType hamiltonianConnection;
	SizeType h;
	SizeType m;
	bool verbose;
	PsimagLite::String evaluator;
}; // struct ParametersForMera

template<typename T>
std::ostream& operator<<(std::ostream& os, const ParametersForMera<T>& p)
{
	os<<"hamiltonianConnection ";
	os<<p.hamiltonianConnection;
	os<<"h="<<p.h<<"\n";
	os<<"m="<<p.m<<"\n";
	os<<"verbose="<<((p.verbose) ? 1 : 0)<<"\n";
	os<<"evaluator="<<p.evaluator<<"\n";
	return os;
}

} // namespace Mera
#endif // ParametersForMera_H