/*
 *	source: https://ieeexplore.ieee.org/document/6843261/
 */

#include "auxiva.h"
#include "fft.h"
#include "mat.h"
#include <complex>
#include <math.h>
#include <array>

#define ALPHA 0.96
#define L_b 1	// block size
#define FFT_WINDOW 4096
#define WINDOW_SHIFT 1024
#define UPDATES_PER_FRAME 2	// N
#define NUM_INPUTS 2
#define FREQUENCY_BINS ((FFT_WINDOW / 2) + 1)

using namespace std;

// pass by value or reference?
void unmix(bool setup, int16_t* l_chan, int16_t* r_chan, int16_t* l_res = 0, int16_t* r_res = 0)
{
	// initialize structures & variables
	static Matrix<float> W(NUM_INPUTS, NUM_INPUTS)[FREQUENCY_BINS];				// demixing matrix, 1 per frequency bin
	Matrix<float> w_k(NUM_INPUTS, 1)[FREQUENCY_BINS]; 					// kth row of demixing matrix as column vector
	static Matrix<float> V(NUM_INPUTS, NUM_INPUTS)[NUM_INPUTS][FREQUENCY_BINS];		// each covariance V[k][freq] is a 2x2 matrix
	
	Matrix<float> x(NUM_INPUTS, 1)[FREQUENCY_BINS];			// observed input from mic
	float* r_k[NUM_INPUTS] = {0};					// auxiliary variables
	Matrix<float> y(NUM_INPUTS, 1)[FREQUENCY_BINS];			// estimated sources
	Matrix<int> e_k(NUM_INPUTS, 1);					// column vector with kth element = 1, else 0
	
	// pass l_chan & r_chan to FFT, multiply, & set to x[]
	//We're supposed to have one huge array for EACH time block tau, indexed by frequency bin omega
	x = fourier(l_chan, r_chan);
		
	// each invocation of this function is a tau so no need to loop over them
	// since W is static, every iteration uses the previous W
	for (int n = 0; n < UPDATES_PER_FRAME; ++n) 
	{
		for (int k = 0; k < NUM_INPUTS; ++k) 
		{
			e_k.data[k][0] = 1;
			
			// update auxiliary variable
			for (int freq = 0; freq < FREQUENCY_BINS; ++freq)
			{
				w_k[freq] = (W[freq].get_row(k)).transpose();
				
				r_k[k] += pow(abs((w_k[freq].transpose() * x[freq]).data[0][0]), 2);	// w_k * x should be 1x1
			}
			r_k[k] = sqrt(r_k[k]);

			for (int freq = 0; freq < FREQUENCY_BINS; ++freq)
			{
				// update covariance matrix
				// t = tau - L_b + 1 = tau  since L_b = 1 so no need to do the sum
				V[k][freq] = (ALPHA * V[k][freq]) + ((1 - ALPHA) * /*(1 / L_b) * */ ((x[freq] * x[freq].transpose()) / r_k[k] ));
				
				// update w_k
				w_k[freq] = (W[freq] * V[k][freq]).inverse() * e_k;
				w_k[freq] = w_k[freq] / sqrt(w_k[freq].transpose() * V[k][freq] * w_k[freq]);
				
				W[freq].set_row(k, w_k[freq].transpose());	// update the kth row of W
			}
			// resetting e_k, could also just do e_k.data[k][0] = 0;
			e_k.data[0][0] = 0;
			e_k.data[1][0] = 0;
		}
	}	
	
	if (!setup) 
	{
		for (int freq = 0; freq < FREQUENCY_BINS; ++freq)
		{
			y[freq] = W[freq] * x[freq];
		}
		
		inverse_fourier(y, l_res, r_res);
	}
	
	return;
}
