#ifndef DSP_WAVSOURCE_H
#define DSP_WAVSOURCE_H

#include "WavHeader.h"
#include <cstdio>
#include <cstring>
#include <volk/volk.h>

class WavSource {
private:
	WavHeader header;
	uint8_t* byteBuff = nullptr;
	lv_32fc_t* sampleBuff = nullptr;
	size_t sampleBuffPos = 0;
	size_t buffSize = 0;
	float* re = nullptr;
	float* im = nullptr;
	FILE* file = nullptr;
	char* path = nullptr;
	int fileOpen = 0;
	size_t fileSize = 0;
	size_t samplesCount = 0;
	size_t numReadSamples = 0;

	size_t get_file_residue();
	void fill_buffer();

public:
	WavSource(const char* path, size_t buffSize = 512);
	~WavSource();
	uint32_t get_sampleRate();
	uint16_t get_bps();
	size_t get_samplesInFile();
	size_t get_fileSize();
	size_t read_samples(lv_32fc_t* arr, size_t numSamples);
};


#endif //DSP_WAVSOURCE_H
