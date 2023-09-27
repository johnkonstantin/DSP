#include "FIRFilter.h"

FIRFilter::FIRFilter(FilterType firType, WindowType winType, float fs, float transitionWidth, float fc1, float fc2) {
	if (fs <= 0) {
		fputs("fs must be > 0!", stderr);
		return;
	}
	this->numTaps = this->computeNumTaps(fs, transitionWidth, winType);
	size_t alignment = volk_get_alignment();
	float* winTaps = (float*)volk_malloc(this->numTaps * sizeof(float), alignment);
	this->taps = (float*)volk_malloc(this->numTaps * sizeof(float), alignment);
	computeWindow(winType, winTaps, this->numTaps);
	computeFirdes(this->taps, winTaps, this->numTaps, firType, fs, fc1, fc2);
	volk_free(winTaps);

	this->workspace = (lv_32fc_t*)volk_malloc(this->numTaps * sizeof(lv_32fc_t), alignment);
	for (size_t i = 0; i < this->numTaps; ++i) {
		this->workspace[i] = 0.0;
	}
}

FIRFilter::~FIRFilter() {
	volk_free(this->workspace);
	volk_free(this->taps);
}

float FIRFilter::maxAttenuation(WindowType winType) {
	switch (winType) {
		case WIN_HAMMING:
			return 53;
		case WIN_BLACKMAN:
			return 74;
		case WIN_RECTANGULAR:
			return 21;
		case WIN_BLACKMAN_HARRIS:
			return 92;
		case WIN_NUTTALL:
			return 93;
		case WIN_BLACKMAN_NUTTALL:
			return 98;
		case WIN_FLATTOP:
			return 93;
		default:
			return 0;
	}
}

size_t FIRFilter::computeNumTaps(float fs, float transitionWidth, WindowType winType) {
	float a = this->maxAttenuation(winType);
	size_t num = a * fs / (22.0 * transitionWidth);
	if (!(num & 1)) {
		num++;
	}
	return num;
}

void FIRFilter::computeWindow(WindowType type, float* winTaps, size_t numTaps) {
	switch (type) {
		case WIN_HAMMING:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 0.54 - 0.46 * cos(2 * M_PI * i / (numTaps - 1));
			}
			break;
		case WIN_BLACKMAN:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 0.42 - 0.5 * cos(2 * M_PI * i / (numTaps - 1)) +
				             0.08 * cos(4 * M_PI * i / (numTaps - 1));
			}
			break;
		case WIN_BLACKMAN_HARRIS:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 0.35875 - 0.48829 * cos(2 * M_PI * i / (numTaps - 1)) +
				             0.14128 * cos(4 * M_PI * i / (numTaps - 1)) -
				             0.01168 * cos(6 * M_PI * i / (numTaps - 1));
			}
			break;
		case WIN_NUTTALL:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 0.355768 - 0.487396 * cos(2 * M_PI * i / (numTaps - 1)) +
				             0.144232 * cos(4 * M_PI * i / (numTaps - 1)) -
				             0.012604 * cos(6 * M_PI * i / (numTaps - 1));
			}
			break;
		case WIN_BLACKMAN_NUTTALL:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 0.3635819 - 0.4891775 * cos(2 * M_PI * i / (numTaps - 1)) +
				             0.1365995 * cos(4 * M_PI * i / (numTaps - 1)) -
				             0.0106411 * cos(6 * M_PI * i / (numTaps - 1));
			}
			break;
		case WIN_FLATTOP:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 1.0 - 1.93 * cos(2 * M_PI * i / (numTaps - 1)) +
				             1.29 * cos(4 * M_PI * i / (numTaps - 1)) -
				             0.388 * cos(6 * M_PI * i / (numTaps - 1)) +
				             0.032 * cos(8 * M_PI * i / (numTaps - 1));
			}
			break;
		case WIN_RECTANGULAR:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 1.0;
			}
			break;
		default:
			for (size_t i = 0; i < numTaps; ++i) {
				winTaps[i] = 0.0;
			}
			break;
	}
}

void FIRFilter::computeFirdes(float* firTaps, float* winTaps, size_t numTaps, FilterType type, float fs, float fc1, float fc2) {
	ssize_t m;
	float fwT0;
	float fmax;
	switch (type) {
		case LOWPASS:
			m = (numTaps - 1) / 2;
			fwT0 = 2.0 * M_PI * fc1 / fs;
			for(ssize_t n = -m; n <= m; n++) {
				if (n == 0)
					firTaps[n + m] = fwT0 / M_PI * winTaps[n + m];
				else {
					firTaps[n + m] = sin(n * fwT0) / (n * M_PI) * winTaps[n + m];
				}
			}
			fmax = firTaps[0 + m];
			for(int n = 1; n <= m; n++) {
				fmax += 2 * firTaps[n + m];
			}
			for(ssize_t i = 0; i < numTaps; i++) {
				firTaps[i] /= fmax;
			}
			break;
		default:
			for (size_t i = 0; i < numTaps; ++i) {
				firTaps[i] = 0.0;
			}
			break;
	}
}

void FIRFilter::filter(lv_32fc_t* inSamples, lv_32fc_t* outSamples, size_t numSamples) {
	lv_32fc_t t = 0.0;
	for (size_t i = 0; i < numSamples; ++i) {
		memmove(this->workspace + 1, this->workspace, (this->numTaps - 1) * sizeof(lv_32fc_t));
		this->workspace[0] = inSamples[i];
		volk_32fc_32f_dot_prod_32fc(&t, this->workspace, this->taps, this->numTaps);
		outSamples[i] = t;
	}
}
