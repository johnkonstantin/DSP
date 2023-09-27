#ifndef DSP_WAVSINK_H
#define DSP_WAVSINK_H

#include "WavHeader.h"
#include <cstdio>
#include <cstring>
#include <volk/volk.h>

class WavSink {
private:
	WavHeader header;
	uint8_t* byteBuff = nullptr;
	size_t buffSize = 0;
	FILE* file = nullptr;
	char* path = nullptr;
	int fileOpen = 0;
	lv_32fc_t* sampleBuff = nullptr;
	size_t sampleBufPos = 0;
	float* re = nullptr;
	float* im = nullptr;

	void flush();

public:
	WavSink(const char* path, uint32_t sampleRate, uint16_t bitsPerSample, size_t buffSize = 512);
	~WavSink();
	int write_samples(lv_32fc_t* arr, size_t numSamples);
};


#endif //DSP_WAVSINK_H
