#include "model.h"

#include <float.h>

double binomialProbability(double successProbability, int numTrials, int numSuccess)
{
	if (numTrials - numSuccess < numSuccess)
	{
		successProbability = 1 - successProbability;
		numSuccess = numTrials - numSuccess;
	}
	double probability = 1;
	int qExp = numTrials - numSuccess;
	for (int i = 0; i < numSuccess; i++)
	{
		if (i == 200)
		{
			int a = 1;
		}
		probability *= successProbability * (numTrials - i) / (numSuccess - i);
		while (qExp && probability > 1)
		{
			probability *= 1 - successProbability;
			qExp--;
		}
	}
	double temp = 1 - successProbability;
	while (qExp > 0)
	{
		if (qExp & 1)
		{
			probability *= temp;
		}
		qExp >>= 1;
		temp *= temp;
	}
	return probability;
}

double binomialCumulativeProbability(double successProbability, int numTrials, int numSuccess)
{
	double curProbability = binomialProbability(successProbability, numTrials, numSuccess);
	double cumulativeProbability;
	if (numSuccess < numTrials * successProbability)
	{
		cumulativeProbability = 1;
		for (int i = numSuccess; i > 0 && curProbability > DBL_MIN; i--)
		{
			curProbability *= (1 - successProbability) * i / (successProbability * (numTrials - i + 1));
			cumulativeProbability -= curProbability;
		}

	}
	else
	{
		cumulativeProbability = 0;
		for (int i = numSuccess; i <= numTrials && curProbability > DBL_MIN; i++)
		{
			cumulativeProbability += curProbability;
			curProbability *= successProbability * (numTrials - i) / ((1 - successProbability) * (i + 1));
		}
	}
	return cumulativeProbability;
}