#include <HEAAN.h>
#include "math.h"
#include <iostream>
#include <string>

using namespace std;

void loadWeights(double* weights, string& path) {
	ifstream openFile(path.data());
	if(openFile.is_open()) {
		string line, temp;
		long i;
		long idx = 0;
		while(getline(openFile, line)){
			weights[idx++] = atof(line.c_str());
		}
	} else {
		cout << "Error: cannot read file" << endl;
	}
}

void sigmoid(Scheme &scheme, Ciphertext &cipher) {

}

void conv1(vector<Ciphertext> &output, Scheme &scheme, long logp, long logq,
		Ciphertext &enc_data, double* kernel) {
	Ciphertext tVec1, tVec2;
	for (int k = 0; k < output.size(); k++) {
		int kernel_start_index = 25 * k;
		output[k].copy(enc_data);
		scheme.multByConstAndEqual(output[k], kernel[kernel_start_index], logp);
		scheme.reScaleByAndEqual(output[k], logp);
		for (int i = 1; i < 25; i++) {
			scheme.leftRotateFast(tVec1, enc_data, 32 * (i / 5) + (i % 5));
			scheme.multByConstAndEqual(tVec1, kernel[kernel_start_index + i],
					logp);
			scheme.reScaleByAndEqual(tVec1, logp);
			scheme.addAndEqual(output[k], tVec1);
		}
		sigmoid(scheme, output[k]);
	}

}

/*
 input data is 32 * 32 * 4 = 1024 * 4 array (four elements which point 1024 elements)
 Each element is of form as below:
 <----- 24 ------> <-- 8 -->
 [* 0 * 0 .... * 0 0 0 ... 0, ^
 0 0 0 0 .... 0 0 0 0 ... 0, |
 * 0 * 0 .... * 0 0 0 ... 0, 2
 0 0 0 0 .... 0 0 0 0 ... 0, 4
 ...						 |
 * 0 * 0 .... * 0 0 0 ... 0, |
 0 0 0 0 .... 0 0 0 0 ... 0, v
 0 0 0 0 .... 0 0 0 0 ... 0, ^
 0 0 0 0 .... 0 0 0 0 ... 0, |
 .							 8
 .							 |
 0 0 0 0 .... 0 0 0 0 ... 0] v

 Kernel is the 1-D array containing (width, height, depth, kernum_num) = (5, 5, 4, 12) shape
 This is equivalent 4-D array[12][4][5][5]
 */
void conv2(vector<Ciphertext> &convolved, Scheme scheme, long logp, long logq,
		vector<Ciphertext> &enc_data, double *kernel) {
	Ciphertext rotated, multiplied;

	for (int each_kernel = 0; each_kernel < 12; each_kernel++) {
		for (int depth = 0; depth < 4; depth++) {
			for (int k = 0; k < 25; k++) {
				int rotate_amount = 64 * (k / 5) + 2 * (k % 5);
				if(rotate_amount == 0) {
					rotated.copy(enc_data[depth]);
				} else {
					scheme.leftRotateFast(rotated, enc_data[depth], rotate_amount);
				}
				double kernel_value = kernel[100 * each_kernel + 25 * depth + k];
				scheme.multByConst(multiplied, enc_data[depth], kernel_value,logp);
				scheme.reScaleByAndEqual(multiplied, logp);

				if (depth == 0 && k == 0) {
					convolved[each_kernel].copy(multiplied);
				} else {
					scheme.addAndEqual(convolved[each_kernel], multiplied);
				}
			}
		}

		sigmoid(scheme, convolved[each_kernel]);
	}
}

void avg_pool1(Ciphertext &output, Scheme scheme, long logp, long logq,
		Ciphertext &prev, long n, long tl) {
	//n=24 l=28 tl=32

	output.copy(prev);
	Ciphertext tmp;

	scheme.leftRotateFast(tmp, output, 1);
	scheme.addAndEqual(output, tmp);
	scheme.leftRotateFast(tmp, output, tl);
	scheme.addAndEqual(output, tmp);

	scheme.multByConstAndEqual(output, 0.25, logp);
	scheme.reScaleByAndEqual(output, logp);

	complex<double> *mul = new complex<double> [tl * tl];

	for (int i = 0; i < n; i += 2) {
		for (int j = 0; j < n; j += 2) {
			mul[i * tl + j].real(1.0);
		}
	}

	Ciphertext MUL;
	scheme.encrypt(MUL, mul, tl * tl, logp, logq);
	scheme.modDownToAndEqual(MUL, output.logq);

	scheme.multAndEqual(output, MUL);
	scheme.reScaleByAndEqual(output, logp);
}

void avg_pool2(Ciphertext &output, Scheme scheme, long logp, long logq,
		Ciphertext &prev, long n, long tl) {
	//n=8 l=12 tl=32

	output.copy(prev);
	Ciphertext tmp;

	scheme.leftRotateFast(tmp, output, 2);
	scheme.addAndEqual(output, tmp);
	scheme.leftRotateFast(tmp, output, 2 * tl);
	scheme.addAndEqual(output, tmp);

	scheme.multByConstAndEqual(output, 0.25, logp);
	scheme.reScaleByAndEqual(output, logp);

	complex<double> *mul = new complex<double> [tl * tl];

	for (int i = 0; i < n; i += 4) {
		for (int j = 0; j < n; j += 4) {
			mul[i * tl + j].real(1.0);
		}
	}

	Ciphertext MUL;
	scheme.encrypt(MUL, mul, tl * tl, logp, logq);
	scheme.modDownToAndEqual(MUL, output.logq);
	scheme.multAndEqual(output, MUL);
	scheme.reScaleByAndEqual(output, logp);
}

void fully_connected(vector<Ciphertext> &output, Scheme scheme, long logp,
		long logq, vector<Ciphertext> &layer, double* W3) {
	for (int i = 0; i < output.size(); i++) {
		for (int t = 0; t < layer.size(); t++) {
			double weight[32 * 32];
			int r = 0, c = 0, value = 0, count = 0;
			for (int j = 0; j < 32; j++) {
				//add 0 at garbage slots
				//0 4 8 12
				//128 ....
				//...
				//.....396
				if (r < 4 && c <= 12 && count < 16) {
					if (c == 12)
						c = 0;
					value = r + 128 * c;
				}
				if (j == value) {
					weight[j] = W3[12 * 16 * i + 16 * t + count];
					count++;
					r += 4;
					c += 1;
				} else
					weight[j] = 0;
			}

			Ciphertext mr;
			scheme.encrypt(mr, weight, 32 * 32, logp, logq);
			std::cout << "----parameter prepared----" << std::endl;
			scheme.modDownToAndEqual(mr, layer[t].logq);
			scheme.multAndEqual(mr, layer[t]);
			scheme.reScaleByAndEqual(mr, logp);

			if (t == 0) {
				output[i].copy(mr);
			} else {
				scheme.addAndEqual(output[i], mr);
			}
		}

		Ciphertext temp;
		std::cout << "----rotate and add----" << std::endl;

		scheme.leftRotateFast(temp, output[i], 4);
		scheme.addAndEqual(output[i], temp);
		scheme.leftRotateFast(temp, output[i], 8);
		scheme.addAndEqual(output[i], temp);
		scheme.leftRotateFast(temp, output[i], 4*32);
		scheme.addAndEqual(output[i], temp);
		scheme.leftRotateFast(temp, output[i], 8*32);
		scheme.addAndEqual(output[i], temp);

	}
}

int main() {
	srand(time(NULL));
	SetNumThreads(8);

	Ring ring;
	SecretKey secretKey(ring);
	Scheme scheme(secretKey, ring);
	scheme.addLeftRotKeys(secretKey);
	TimeUtils timeutils;

	for (int i = 1; i < 25; i++) {
		int idx =32 * (i / 5) + (i % 5);
		if(scheme.leftRotKeyMap.find(idx) == scheme.leftRotKeyMap.end()) {
			scheme.addLeftRotKey(secretKey, idx);
		}
	}

	for (int k = 1; k < 25; k++) {
		int idx = 64 * (k / 5) + 2 * (k % 5);
		if(scheme.leftRotKeyMap.find(idx) == scheme.leftRotKeyMap.end()) {
			scheme.addLeftRotKey(secretKey, idx);
		}
	}

	long logn = 5;
	long logm = 5;

	long logq = 1200;
	long logp = 30;
	long bootstrap_logq = 45;
	long one_side_num = 32;
	long feature_num = one_side_num * one_side_num;
	scheme.addBootKey(secretKey, logn + logm, bootstrap_logq + 4);

	double* data = EvaluatorUtils::randomRealArray(feature_num, 1.0);

	Ciphertext enc_data;
	scheme.encrypt(enc_data, data, feature_num, logp, logq);

	double *kernel1, *kernel2, *fully_connected_weight;
	kernel1 = new double[5 * 5 * 1 * 4];
	kernel2 = new double[5 * 5 * 4 * 12];
	fully_connected_weight = new double[16*12*10];

	string path1 = "data/W1.txt";
	string path2 = "data/W2.txt";
	string path3 = "data/W3.txt";

	cout << "loading data" << endl;
	loadWeights(kernel1, path1);
	loadWeights(kernel2, path2);
	loadWeights(fully_connected_weight, path3);
	cout << "data loaded" << endl;

	vector<Ciphertext> enc_conv1(4), enc_avg_pool1(4);
	vector<Ciphertext> enc_conv2(12), enc_avg_pool2(12);
	vector<Ciphertext> enc_output;

	timeutils.start("convolution 1");
	conv1(enc_conv1, scheme, logp, logq, enc_data, kernel1);
	timeutils.stop("convolution 1");

	timeutils.start("average pooling 1");
	for (int i = 0; i < enc_conv1.size(); ++i) {
		avg_pool1(enc_avg_pool1[i], scheme, logp, logq, enc_conv1[i], 24, 32);
	}
	timeutils.stop("average pooling 1");

	timeutils.start("convolution 2");
	conv2(enc_conv2, scheme, logp, logq, enc_avg_pool1, kernel2);
	timeutils.stop("convolution 2");

	timeutils.start("average pooling 2");
	for (int i = 0; i < enc_conv2.size(); ++i) {
		avg_pool2(enc_avg_pool2[i], scheme, logp, logq, enc_conv2[i], 8, 32);
	}
	timeutils.stop("average pooling 2");

	timeutils.start("fully connected");
	fully_connected(enc_output, scheme, logp, logq, enc_avg_pool2,fully_connected_weight);
	timeutils.stop("fully connected");

	double* plain_output = new double[10];
	for (int i = 0; i < enc_output.size(); ++i) {
		complex<double>* outi = scheme.decrypt(secretKey, enc_output[i]);
		plain_output[i] = outi[0].real();
	}
	for (int i = 0; i < 10; ++i) {
		cout << plain_output[i] << ", ";
	}
	cout << endl;

	return 0;
}
