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
#ifndef MERA_TENSOREVAL_H
#define MERA_TENSOREVAL_H

#include "TensorSrep.h"
#include "Tensor.h"
#include <map>
#include "Tokenizer.h"
#include "SrepEquation.h"
#include "TensorBreakup.h"

namespace Mera {

template<typename ComplexOrRealType>
class TensorEval {

	typedef TensorSrep TensorSrepType;

	class TensorEvalHandle {

	public:

		enum Status {STATUS_IDLE, STATUS_IN_PROGRESS, STATUS_DONE};

		TensorEvalHandle(Status status = STATUS_IDLE)
		    : status_(status)
		{}

		bool done() const
		{
			return (status_ == STATUS_DONE);
		}

	private:

		Status status_;
	};

public:

	typedef SrepEquation<ComplexOrRealType> SrepEquationType;
	typedef typename PsimagLite::Vector<SrepEquationType*>::Type VectorSrepEquationType;
	typedef TensorBreakup::VectorStringType VectorStringType;
	typedef TensorEvalHandle HandleType;
	typedef Tensor<ComplexOrRealType> TensorType;
	typedef typename PsimagLite::Vector<TensorType*>::Type VectorTensorType;
	typedef typename SrepEquationType::PairStringSizeType PairStringSizeType;
	typedef typename PsimagLite::Vector<PairStringSizeType>::Type VectorPairStringSizeType;
	typedef std::map<PairStringSizeType,SizeType> MapPairStringSizeType;
	typedef typename PsimagLite::Vector<SizeType>::Type VectorSizeType;

	TensorEval(SrepEquationType& tSrep,
	           const VectorTensorType& vt,
	           const VectorPairStringSizeType& tensorNameIds,
	           MapPairStringSizeType& nameIdsTensor,
	           bool modify)
	    : srepEq_(tSrep),
	      data_(vt), // deep copy
	      tensorNameIds_(tensorNameIds), // deep copy
	      nameIdsTensor_(nameIdsTensor) // deep copy

	{
		indexOfOutputTensor_ = indexOfOutputTensor(tSrep, tensorNameIds, nameIdsTensor);

		if (!modify) return;

		TensorBreakup tensorBreakup(srepEq_.lhs(), srepEq_.rhs());
		// get t0, t1, etc definitions and result
		VectorStringType vstr;
		tensorBreakup(vstr);
		//		PsimagLite::String brokenResult = tensorBreakup.brokenResult();
		// loop over temporaries definitions
		assert(!(vstr.size() & 1));
		SizeType outputLocation = 1 + vstr.size();
		for (SizeType i = 0; i < vstr.size(); i += 2) {
			// add them to tensorNameIds nameIdsTensor
			PsimagLite::String temporaryName = vstr[i];
			if (temporaryName == tSrep.lhs().sRep()) {
				//				vstr[i + 1] = brokenResult;
				std::cout<<"Definition of "<<srepEq_.rhs().sRep()<<" is ";
				std::cout<<vstr[i + 1]<<"\n";
				srepEq_.rhs() = TensorSrep(vstr[i + 1]);
				outputLocation = i;
			}

			if (temporaryName[0] != 't') continue;
			PsimagLite::String str = temporaryName.substr(1,temporaryName.length());
			SizeType temporaryId = atoi(str.c_str());
			temporaryName = "t";
			PairStringSizeType tmpPair = PairStringSizeType(temporaryName,temporaryId);
			tensorNameIds_.push_back(tmpPair);
			nameIdsTensor_[tmpPair] = tensorNameIds_.size() - 1;

			// add this temporary, call setDimensions for output tensor later
			TensorStanza tmpStanza(vstr[i]);
			VectorSizeType args(1,1); // bogus
			TensorType* t = new TensorType(args, tmpStanza.ins());
			garbage_.push_back(t);
			data_.push_back(t);
		}

		VectorSrepEquationType veqs;
		TensorSrepType::VectorPairSizeType empty;
		for (SizeType i = 0; i < vstr.size(); i += 2) {
			veqs.push_back(new SrepEquationType(vstr[i] + "=" + vstr[i+1]));
			SizeType j = veqs.size() - 1;
			if (i != outputLocation)
				veqs[j]->canonicalize();
			else
				veqs[j]->rhs().simplify(empty);

			TensorEval tEval(*(veqs[j]),
			                 data_,
			                 tensorNameIds_,
			                 nameIdsTensor_,
			                 false);
			std::cerr<<"Evaluation of "<<veqs[j]->sRep()<<"\n";
			tEval(false); //handle the handle here
		}

		std::cout.flush();
		for (SizeType i = 0; i < veqs.size(); ++i) {
			delete veqs[i];
			veqs[i] = 0;
		}
	}

	~TensorEval()
	{
		for (SizeType i = 0; i < garbage_.size(); ++i) {
			delete garbage_[i];
			garbage_[i] = 0;
		}
	}

	HandleType operator()(bool cached)
	{
		HandleType handle(HandleType::STATUS_DONE);
		if (cached) return handle;

		SizeType total = srepEq_.lhs().maxTag('f') + 1;

		static VectorSizeType dimensions;
		if (total != dimensions.size()) dimensions.resize(total,0);
		else std::fill(dimensions.begin(), dimensions.end(), 0);
		prepare(dimensions,srepEq_.rhs(),TensorStanza::INDEX_TYPE_FREE);

		static VectorSizeType free;
		if (total != free.size()) free.resize(total,0);
		else std::fill(free.begin(), free.end(), 0);

		outputTensor().setSizes(dimensions);

		do {
			outputTensor()(free) = slowEvaluator(free,srepEq_.rhs());
		} while (nextIndex(free,dimensions,total));

		return handle;
	}

	void printResult(std::ostream& os) const
	{
		SizeType total = outputTensor().args();
		static VectorSizeType dimensions;
		if (total > dimensions.size()) dimensions.resize(total,0);

		for (SizeType i = 0; i < total; ++i)
			dimensions[i] = outputTensor().argSize(i);

		static VectorSizeType free;
		if (total > free.size()) free.resize(total,0);
		else std::fill(free.begin(), free.end(), 0);

		do {
			SizeType index = outputTensor().index(free);
			std::cout<<index<<" "<<outputTensor()(free)<<"\n";
		} while (nextIndex(free,dimensions,total));
	}

	static bool nextIndex(VectorSizeType& summed,
	                      const VectorSizeType& dimensions,
	                      SizeType total)
	{
		assert(total <= summed.size());
		for (SizeType i = 0; i < total; ++i)
			assert(dimensions[i] == 0 || summed[i] < dimensions[i]);

		for (SizeType i = 0; i < total; ++i) {
			summed[i]++;
			if (summed[i] < dimensions[i]) break;
			summed[i] = 0;
			if (i + 1 == total) return false;
		}

		for (SizeType i = 0; i < total; ++i)
			assert(dimensions[i] == 0 || summed[i] < dimensions[i]);

		return true;
	}

	static SizeType indexOfOutputTensor(const SrepEquationType& eq,
	                                    const VectorPairStringSizeType& tensorNameIds,
	                                    MapPairStringSizeType& nameIdsTensor)
	{
		SizeType ret = nameIdsTensor[eq.nameIdOfOutput()];
		if (tensorNameIds[ret] != eq.nameIdOfOutput()) {
			PsimagLite::String msg("SrepEquation: Could not find ");
			msg += "output tensor " + eq.nameIdOfOutput().first + "\n";
			throw PsimagLite::RuntimeError(msg);
		}

		return ret;
	}

private:

	ComplexOrRealType slowEvaluator(const VectorSizeType& free,
	                                const TensorSrepType& srep)
	{
		SizeType total = srep.maxTag('s') + 1;
		static VectorSizeType summed;
		if (summed.size() != total) summed.resize(total,0);
		else std::fill(summed.begin(), summed.end(), 0);

		static VectorSizeType dimensions;
		if (dimensions.size() != total) dimensions.resize(total,0);
		else std::fill(dimensions.begin(), dimensions.end(), 0);

		prepare(dimensions, srep, TensorStanza::INDEX_TYPE_SUMMED);

		ComplexOrRealType sum = 0.0;
		do {
			sum += evalInternal(summed,free,srep);
		} while (nextIndex(summed,dimensions,total));

		return sum;
	}

	void prepare(VectorSizeType& dimensions,
	             const TensorSrepType& tensorSrep,
	             TensorStanza::IndexTypeEnum type) const
	{
		SizeType ntensors = tensorSrep.size();
		for (SizeType i = 0; i < ntensors; ++i) {
			prepareStanza(dimensions, tensorSrep(i), type);
		}
	}

	void prepareStanza(VectorSizeType& dimensions,
	                   const TensorStanza& stanza,
	                   TensorStanza::IndexTypeEnum type) const
	{
		SizeType id = stanza.id();
		SizeType mid = idNameToIndex(stanza.name(),id);
		assert(mid < data_.size());
		SizeType ins = stanza.ins();
		for (SizeType j = 0; j < ins; ++j) {
			if (stanza.legType(j,TensorStanza::INDEX_DIR_IN) != type)
				continue;
			SizeType sIndex = stanza.legTag(j,TensorStanza::INDEX_DIR_IN);

			assert(j < data_[mid]->args());
			assert(sIndex < dimensions.size());
			dimensions[sIndex] = data_[mid]->argSize(j);
		}

		SizeType outs = stanza.outs();
		for (SizeType j = 0; j < outs; ++j) {
			if (stanza.legType(j,TensorStanza::INDEX_DIR_OUT) != type)
				continue;
			SizeType sIndex = stanza.legTag(j,TensorStanza::INDEX_DIR_OUT);

			assert(sIndex < dimensions.size());
			assert(j + ins < data_[mid]->args());
			dimensions[sIndex] = data_[mid]->argSize(j+ins);
		}
	}

	ComplexOrRealType evalInternal(const VectorSizeType& summed,
	                               const VectorSizeType& free,
	                               const TensorSrepType& tensorSrep)
	{
		ComplexOrRealType prod = 1.0;
		SizeType ntensors = tensorSrep.size();
		for (SizeType i = 0; i < ntensors; ++i) {
			prod *= evalThisTensor(tensorSrep(i),summed,free);
			if (prod == 0) break;
		}

		return prod;
	}

	ComplexOrRealType evalThisTensor(const TensorStanza& ts,
	                                 const VectorSizeType& summed,
	                                 const VectorSizeType& free)
	{
		SizeType id = ts.id();
		SizeType mid = idNameToIndex(ts.name(),id);
		SizeType ins = ts.ins();
		SizeType outs = ts.outs();
		assert(data_[mid]->args() == ins + outs);
		VectorSizeType args(data_[mid]->args(),0);

		for (SizeType j = 0; j < ins; ++j) {
			SizeType index = ts.legTag(j,TensorStanza::INDEX_DIR_IN);

			switch (ts.legType(j,TensorStanza::INDEX_DIR_IN)) {

			case TensorStanza::INDEX_TYPE_SUMMED:
				assert(index < summed.size());
				assert(j < args.size());
				args[j] = summed[index];
				break;

			case TensorStanza::INDEX_TYPE_FREE:
				assert(index < free.size());
				assert(j < args.size());
				args[j] = free[index];
				break;

			case  TensorStanza::INDEX_TYPE_DUMMY:
				assert(j < args.size());
				args[j] = 0;
				break;
			default:
				PsimagLite::RuntimeError("evalThisTensor: Wrong index type\n");
			}
		}

		for (SizeType j = 0; j < outs; ++j) {
			SizeType index = ts.legTag(j,TensorStanza::INDEX_DIR_OUT);

			switch (ts.legType(j,TensorStanza::INDEX_DIR_OUT)) {

			case TensorStanza::INDEX_TYPE_SUMMED:
				assert(index < summed.size());
				assert(j+ins < args.size());
				args[j+ins] = summed[index];
				break;

			case TensorStanza::INDEX_TYPE_FREE:
				assert(index < free.size());
				assert(j+ins < args.size());
				args[j+ins] = free[index];
				break;

			case  TensorStanza::INDEX_TYPE_DUMMY:
				assert(j+ins < args.size());
				args[j+ins] = 0;
				break;
			default:
				PsimagLite::RuntimeError("evalThisTensor: Wrong index type\n");
			}
		}

		return data_[mid]->operator()(args);
	}

	SizeType idNameToIndex(PsimagLite::String name, SizeType id) const
	{
		typename MapPairStringSizeType::iterator it =
		        nameIdsTensor_.find(PairStringSizeType(name,id));
		if (it == nameIdsTensor_.end())
			throw PsimagLite::RuntimeError("idNameToIndex: key not found\n");
		return it->second;
	}

	TensorType& outputTensor()
	{
		assert(indexOfOutputTensor_ < data_.size());
		return *(data_[indexOfOutputTensor_]);
	}

	const TensorType& outputTensor() const
	{
		assert(indexOfOutputTensor_ < data_.size());
		return *(data_[indexOfOutputTensor_]);
	}

	TensorEval(const TensorEval& other);

	TensorEval& operator=(const TensorEval& other);

	SrepEquationType srepEq_;
	VectorTensorType data_;
	VectorPairStringSizeType tensorNameIds_;
	mutable MapPairStringSizeType nameIdsTensor_;
	SizeType indexOfOutputTensor_;
	VectorTensorType garbage_;
};
}
#endif // MERA_TENSOREVAL_H
