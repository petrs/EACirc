#ifndef PREDICT_BYTES_PARITY_CIRCUIT_EVALUATOR_H
#define PREDICT_BYTES_PARITY_CIRCUIT_EVALUATOR_H

#include "ICircuitEvaluator.h"

class PredictBytesParityCircuitEvaluator: public ICircuitEvaluator {
	public:
		PredictBytesParityCircuitEvaluator();
		void evaluateCircuit(unsigned char*, unsigned char*, unsigned char*, int*, int*, int*);
};

#endif