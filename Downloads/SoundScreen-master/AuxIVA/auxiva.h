#ifndef AUXIVA_H
#define AUXIVA_H

// pass by value or reference?
// return time domain signal
float* unmix(int16_t* l_chan, int16_t* r_chan, int16_t* l_res, int16_t* r_res);

#endif	// AUXIVA_H
