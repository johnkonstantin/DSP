#ifndef DSP_FIRFILTER_H
#define DSP_FIRFILTER_H

#include "volk/volk.h"

enum WindowType {
	WIN_HAMMING,
	WIN_BLACKMAN,
	WIN_BLACKMAN_HARRIS,
	WIN_NUTTALL,
	WIN_BLACKMAN_NUTTALL,
	WIN_FLATTOP,
	WIN_RECTANGULAR
};

enum FilterType {
	LOWPASS
};

class FIRFilter {
private:
	float* taps = nullptr;
	size_t numTaps = 0;
	lv_32fc_t* workspace = nullptr;

	float maxAttenuation(WindowType winType);
	size_t computeNumTaps(float fs, float transitionWidth, WindowType winType);
	void computeWindow(WindowType type, float* winTaps, size_t numTaps);
	void computeFirdes(float* firTaps, float* winTaps, size_t numTaps, FilterType type, float fs, float fc1 = 0, float fc2 = 0);


public:
	FIRFilter(FilterType firType, WindowType winType, float fs, float transitionWidth = 0, float fc1 = 0, float fc2 = 0);
	~FIRFilter();
	void filter(lv_32fc_t* inSamples, lv_32fc_t* outSamples, size_t numSamples);
};


#endif //DSP_FIRFILTER_H
