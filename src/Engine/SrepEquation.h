#ifndef SREPEQUATION_H
#define SREPEQUATION_H
#include "Vector.h"
#include "TensorSrep.h"
#include <map>

namespace  Mera {

template<typename ComplexOrRealType>
class SrepEquation {

public:

	typedef std::pair<SizeType, SizeType> PairSizeType;
	typedef PsimagLite::Vector<PairSizeType>::Type VectorPairSizeType;
	typedef typename PsimagLite::Vector<ComplexOrRealType>::Type VectorType;
	typedef std::pair<PsimagLite::String,SizeType> PairStringSizeType;
	typedef typename PsimagLite::Vector<PairStringSizeType>::Type VectorPairStringSizeType;
	typedef std::map<PairStringSizeType,SizeType> MapPairStringSizeType;
	typedef PsimagLite::Vector<PsimagLite::String>::Type VectorStringType;
	typedef TensorSrep TensorSrepType;

	SrepEquation(PsimagLite::String str)
	    : lhs_(0),rhs_(0)
	{
		VectorStringType vstr;
		PsimagLite::tokenizer(str,vstr,"=");
		if (vstr.size() != 2)
			throw PsimagLite::RuntimeError("SrepEquation:: syntax error " + str + "\n");
		lhs_ = new TensorStanza(vstr[0]);
		rhs_ = new TensorSrepType(vstr[1]);

		nameIdOfOutput_ = PairStringSizeType(lhs_->name(), lhs_->id());
	}

	SrepEquation(const SrepEquation& other)
	    : lhs_(0),rhs_(0)
	{
		lhs_ = new TensorStanza(*(other.lhs_));
		rhs_ = new TensorSrepType(*(other.rhs_));
		nameIdOfOutput_ = PairStringSizeType(lhs_->name(), lhs_->id());
	}

	~SrepEquation()
	{
		delete lhs_;
		delete rhs_;
	}

	void canonicalize() const
	{
		static SizeType counter = 0;
		VectorPairSizeType frees;
		computeFrees(frees);
		// check replacements
		std::cerr<<"COUNTER="<<counter++<<" ";
		std::cerr<<"FREES "<<frees<<"\n";
		if (frees.size() == 0) return;
		assert(frees[0].second == 0);
		rhs_->simplifyFrees(frees);
		lhs_->replaceSummedOrFrees(frees,TensorStanza::INDEX_TYPE_FREE);
		lhs_->refresh();
	}

	PsimagLite::String sRep() const
	{
		return lhs_->sRep() + "=" + rhs_->sRep();
	}

	const TensorStanza& lhs() const
	{
		assert(lhs_);
		return *lhs_;
	}

	const TensorSrepType& rhs() const
	{
		assert(rhs_);
		return *rhs_;
	}

	SizeType indexOfOutputTensor(const VectorPairStringSizeType& tensorNameIds,
	                             MapPairStringSizeType& nameIdsTensor) const
	{
		SizeType ret = nameIdsTensor[nameIdOfOutput_];
		if (tensorNameIds[ret] != nameIdOfOutput_) {
			PsimagLite::String msg("SrepEquation: Could not find ");
			msg += "output tensor " + nameIdOfOutput_.first + "\n";
			throw PsimagLite::RuntimeError(msg);
		}

		return ret;
	}

	// FIXME: Gives away internals!
	TensorSrepType& rhs()
	{
		assert(rhs_);
		return *rhs_;
	}

private:

	void computeFrees(VectorPairSizeType& replacements) const
	{
		TensorStanza::IndexDirectionEnum in = TensorStanza::INDEX_DIR_IN;
		TensorStanza::IndexDirectionEnum out = TensorStanza::INDEX_DIR_OUT;

		SizeType legs = lhs_->ins();
		SizeType counter = 0;
		for (SizeType j = 0; j < legs; ++j) {
			if (lhs_->legType(j,in) != TensorStanza::INDEX_TYPE_FREE)
				continue;
			replacements.push_back(PairSizeType(lhs_->legTag(j,in),counter++));
		}

		legs = lhs_->outs();
		for (SizeType j = 0; j < legs; ++j) {
			if (lhs_->legType(j,out) != TensorStanza::INDEX_TYPE_FREE)
				continue;
			replacements.push_back(PairSizeType(lhs_->legTag(j,out),counter++));
		}
	}

	//	SrepEquation(const SrepEquation&);

	SrepEquation& operator=(const SrepEquation&);

	TensorStanza* lhs_;
	TensorSrepType* rhs_;
	PairStringSizeType nameIdOfOutput_;
}; // class SrepEquation

} // namespace Mera

#endif // SREPEQUATION_H
