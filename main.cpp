/*
 * main.cpp
 *
 *  Created on: Jul 11, 2019
 *      Author: qlcql
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "math.h"
#include "HELR.h"

using namespace std;

int string_to_int(string s){
	stringstream ss(s);
	int x=0;
	ss>>x;
	return x;
}
class Instance{
	public:
		int label;
		int* x;
		int size;
		Instance(int label, int *x, int size);
		~Instance(){
			delete x;
		}
	};
void readData(double*& x, double*&y, string filename,int&slots,int& numc=4){

		//reading data from a file
		//one y with 4 x (single digit)
		 vector<Instance> dataset;
		 ifstream input(filename);
		 int row=0;

		 if(input.is_open()){
			 while(input.peek()!=EOF){
				 string line;
				 std::getline(input,line);
				 if(line[0]=='#') continue;

				 int count=0;
				 int data[4];
				 int n=0;

				 while(count<4){
					 string column;
					 if(count==0) {
						 column=line.std::string::substr(0,line.std::string::find(' '));
						 n+=column.std::string::length()+2;
					 }
					 else {
						 column=line.std::string::substr(n,1);
						 n+=2;
					 }
					 data[count]=string_to_int(column);
					 count+=1;

				 }

				 string l=line.std::string::substr(n,1);
				 int label=string_to_int(l);

				 Instance instance=Instance(label,data,count);
				 dataset.push_back(instance);
				 line++;
			 }

		 }
		 else{
			 std::cerr<<"cannot open the file"<<std::endl;
			 return;
		 }
		 input.close();
		 x=new double[row*4];
		 y=new double[row];
		 for(long i=0;i<row;i++){
			 for(int j=0;j<4;j++){
				 x[4*i+j]=dataset[i].x[j];
			 }
			 y[i]=dataset[i];
		 }
		 slots=row;
	 }

double test(){

}
int main(){
	double*x;
	double*y;
	int numc;
	int slots;
	readData(x,y,"dataset.txt",slots);
	Ring ring;
	SecretKey secretKey(ring);
	Scheme scheme(secretKey, ring);
	Logistic(scheme,secretKey,x,y,numc,slots);
	test();
}
