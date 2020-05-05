/*
 * HELR.h
 *
 *  Created on: Jul 11, 2019
 *      Author: qlcql
 */

#ifndef HELR_H_
#define HELR_H_

#include <NTL/ZZ.h>
#include <NTL/BasicThreadPool.h>

#include "../src/HEAAN.h"


using namespace std;
using namespace NTL;

class Logistic{
public:
	Scheme scheme;
	Ciphertext Z;
	Ciphertext beta;

	int numc;
	int slots;
	long logp;
	long logq;
	long n;
	//constructor
	Logistic(Scheme scheme, double*x, double*y,int numc, int slots,long logn=4, long logp=30, long logq=1200);
	//void Encode(Plaintext& plain, double* vals, long n, long logp=30, long logq=120);
	void train(int degree=3,double alpha=0.01,long times=100,double eta=0.5);
	void sigmoid( int degree,Ciphertext& ct);

};

#endif /* HELR_H_ */
