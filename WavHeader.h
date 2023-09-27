#ifndef DSP_WAVHEADER_H
#define DSP_WAVHEADER_H

#include <cstdint>

struct WavHeader {
	uint8_t chunkId[4];
	uint32_t chunkSize;
	uint8_t format[4];
	uint8_t subchunk1Id[4];
	uint32_t subchunk1Size;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	uint8_t subchunk2Id[4];
	uint32_t subchunk2Size;
};

#endif //DSP_WAVHEADER_H
