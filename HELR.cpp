/*
 * HELR.cpp

 *
 *  Created on: Jul 11, 2019
 *      Author: qlcql
 */
#include "HELR.h"
#include "../src/HEAAN.h"

Logistic::Logistic(Scheme scheme, double*x, double*y,int numc, int slots,
		long logn, long logp, long logq):scheme(scheme),logp(logp),logq(logq),beta(logp,logq,n),numc(numc),slots(slots){

	std::cout<<"----constructor----"<<std::endl;
	Ciphertext encX;
	Ciphertext encY;
	n=(1<<logn);
	std::cout<<"----encryt x----"<<std::endl;
	scheme.encrypt(encX, x, n, logp, logq);

	std::cout<<"----repeat y----"<<std::endl;
	double repeatY[slots];
	//scale y
	int row=slots/numc;
	for(int i=0;i<row;++i){
		for(int n=0;n<numc;++n){
			repeatY[numc*i+n]=y[i];
		}
	}

	//delete y;
	scheme.encrypt(encY, repeatY, n, logp ,logq);
	std::cout<<"----calculate z----"<<std::endl;
	scheme.mult(Z,encX,encY);

}

void Logistic::train(int degree,double alpha,long times,double eta){

	Ciphertext& ct=Z;
	for(long l=0;l<times;l++){
		std::cout<<"iteration "<<l<<std::endl;
		Ciphertext temp;

		std::cout<<"----begin----"<<std::endl;
		//step1
		scheme.multAndEqual(ct,beta);
		scheme.reScaleByAndEqual(ct,logp);

		std::cout<<"step1 done"<<std::endl;

		//step2
		temp.copy(ct);
		for(long j=0;j<log(numc+1)-1;++j){
			scheme.leftRotateFastAndEqual(temp,pow(2,j));
			scheme.addAndEqual(ct,temp);
		}

		std::cout<<"step2 done"<<std::endl;

		//step3
		double C[slots];
		int row=slots/numc;
		for(int i=0;i<row;++i){
			for(int n=0;n<numc;++n){
				if(n==0) C[numc*i+n]=1;
				else C[numc*i+n]=0;
			}
		}
//		long logp-10;//what should be the value
		Ciphertext c;
		scheme.encrypt(c, C, n, logp ,logq);
		scheme.multAndEqual(ct,c);
		scheme.reScaleByAndEqual(ct,logp);

		std::cout<<"step3 done"<<std::endl;

		//step4
		temp.copy(ct);
		for(long j=0;j<log(numc+1)-1;++j){
			scheme.rightRotateFastAndEqual(temp,pow(2,j));
			scheme.addAndEqual(ct,temp);
		}

		std::cout<<"step4 done"<<std::endl;

		//step5:sigmoid
		sigmoid(degree,ct);

		std::cout<<"step5 done"<<std::endl;

		//step6
		scheme.multAndEqual(ct,Z);
		scheme.reScaleByAndEqual(ct,logp);//not sure about p or q, whether log here

		std::cout<<"step6 done"<<std::endl;

		//step7
		temp.copy(ct);
		for(long j=(int)log(numc+1);j<log(numc+1)-1+log(slots/numc);++j){
			scheme.leftRotateFastAndEqual(temp,pow(2,j));
			scheme.addAndEqual(ct,temp);
		}

		std::cout<<"step7 done"<<std::endl;

		//step8 and step9
		double delta=alpha*pow(2,logp);//p or q, log or not
		double delta1=eta*pow(2,logp);
		double delta2=pow(2,logp)-delta1;
		Ciphertext ctv;

		scheme.multByConst(temp,ct,delta1,logp);

		//updating beta
		scheme.multByConstAndEqual(ct,delta,logp);
		scheme.reScaleByAndEqual(ct,logp);
		scheme.addAndEqual(beta,ct);

		scheme.multByConst(ctv,ct,delta2,logp);
		//nesterov's gd
		scheme.addAndEqual(ctv,temp);
		scheme.reScaleByAndEqual(ctv,logp);


		std::cout<<"step 8 and 9 done"<<std::endl;
	}
}


void Logistic::sigmoid(int degree, Ciphertext& ct){
	if(degree!=3&&degree!=5&&degree!=7) {
		std::cout<<"invalid degree"<<std::endl;
		return;
	}
	std::cout<<"----sigmoid----"<<std::endl;
	scheme.multByConstAndEqual(ct,0.125,logp);
	//std::cout<<"problem is here"<<std::endl;
	Ciphertext ct3;
	ct3.copy(ct);
	if(degree==3){
		std::cout<<"entered degree 3"<<std::endl;
		for(int i=0;i<3;++i){
			scheme.multAndEqual(ct3,ct);
		}
		scheme.multByConstAndEqual(ct3,0.81562,logp);
		scheme.multByConstAndEqual(ct,-1.20096,logp);
		scheme.addConstAndEqual(ct,0.5,logp);
		scheme.addAndEqual(ct,ct3);
		return;
	}

	Ciphertext ct5;
	Ciphertext ct2;
	ct5.copy(ct);
	ct2.copy(ct);
	scheme.square(ct2,ct);
	if(degree==5){
		for(int i=0;i<3;++i){
			scheme.multAndEqual(ct3,ct);
		}
		scheme.mult(ct5,ct3,ct2);
		scheme.multByConstAndEqual(ct5,-1.3511295,logp);
		scheme.multByConstAndEqual(ct3,2.3533056,logp);
		scheme.multByConstAndEqual(ct,-1.53048,logp);
		scheme.addAndEqual(ct3,ct5);
		scheme.addAndEqual(ct,ct3);
		scheme.addConstAndEqual(ct,0.5,logp);
		return;
	}

	Ciphertext ct7;
	ct7.copy(ct);
	if(degree==7){
		for(int i=0;i<3;++i){
			scheme.multAndEqual(ct3,ct);
		}
		scheme.mult(ct5,ct3,ct2);
		scheme.mult(ct7,ct5,ct2);
		scheme.multByConstAndEqual(ct7,0.50739,logp);
		scheme.multByConstAndEqual(ct5,-5.43402,logp);
		scheme.multByConstAndEqual(ct3,4.19407,logp);
		scheme.multByConstAndEqual(ct,-1.73496,logp);
		scheme.addAndEqual(ct5,ct7);
		scheme.addAndEqual(ct3,ct5);
		scheme.addAndEqual(ct,ct3);
		scheme.addConstAndEqual(ct,0.5,logp);
	}

}




