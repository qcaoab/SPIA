/*
 * lenet.cpp

 *
 *  Created on: Aug 5, 2019
 *      Author: qlcql
 */
#include "../src/HEAAN.h"
#include <math.h>

//for temporary use
Ring ring;
SecretKey secretKey(ring);
Scheme scheme(secretKey, ring);

long logq = 1200;
long logp = 30;
long logn = 2;
long n= (1<<logn);

//
double activation(Ciphertext ct){
	std::cout<<"----sigmoid----"<<std::endl;
	std::complex<double>* result=scheme.decrypt(secretKey,ct);
	double val=result[0].real();
	return 1.0 / (1.0 + exp(-val));
}

//
const int length=1024;//32x32
double output[10];

void fully_connected(Ciphertext* layer, double* W3){

	std::cout<<"----fully connected----"<<std::endl;
	for(int i=0;i<10;i++){
		Ciphertext sum;
		for(int t=0;t<12;t++){
			double weight[length];
			int r=0;
			int c=0;
			int value=0;
			int count=0;
			for(int j=0;j<length;j++){
			//add 0 at garbage slots
				//0 4 8 12
				//128 ....
				//...
				//.....396
				//mistake: should not be 4
				if(r<=12&&c<=12&&count<16){
					if(c==12) c=0;
					value=r+128*c;
				}
				if(j==value){
					weight[j]=W3[12*16*i+16*t+count];
					count++;
					r+=4;
					c+=4;
				}
				else weight[j]=0;
			}

			Ciphertext mr;
			scheme.encrypt(w,weight,n,logp,logq);
			std::cout<<"----parameter prepared----"<<std::endl;
			scheme.multAndEqual(mr,layer[t]);
			scheme.reScaleByAndEqual(mr,logp);
			scheme.addAndEqual(sum,mr);
		}

		Ciphertext temp;
		std::cout<<"----rotate and add----"<<std::endl;
		for(int j=0;j<log(length+1);j++){
			scheme.leftRotateFast(temp,sum,pow(2,j));
			scheme.addAndEqual(sum,temp);
		}
		output[i]=activation(sum);
	}
}
//
int main(){
	double input[length];
	Ciphertext layer[12];
	//12 input ciphertexts of size 32x32
	//init to all 1
	for(int i=0;i<12;i++){
		for(int n=0;n<length;n++){
			input[n]=1;
		}
		scheme.encrypt(layer[i],input,n,logp,logq);
	}
	double weight[12*10*length];
	for(int n=0;n<12;n++){
		for (int i=0;i<10;i++){
			for(int j=0;j<length;j++){
				weight[n*10*length+length*i+j]=(n+1.0)/10.0;
			}
		}
	}
	scheme.addLeftRotKeys(secretKey);


	fully_connected(layer,weight);
	for(int i=0;i<10;i++){
		std::cout<<"output "<<i<<": "<<output[i]<<endl;
	}
}


