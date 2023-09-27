#include "WavSink.h"

WavSink::WavSink(const char* path, uint32_t sampleRate, uint16_t bitsPerSample, size_t buffSize) {
	if (path == nullptr) {
		fputs("Null pointer to file path!\n", stderr);
		return;
	}
	this->path = (char*)malloc((strlen(path) + 1) * sizeof(char));
	strcpy(this->path, path);
	if (sampleRate == 0) {
		fputs("sample rate > 0!\n", stderr);
		return;
	}
	if (bitsPerSample != 16 and bitsPerSample != 32) {
		fputs("bits per sample 16 or 32!\n", stderr);
		return;
	}

	this->file = fopen(this->path, "wb");
	if (this->file == nullptr) {
		fputs("Invalid file path!\n", stderr);
		return;
	}

	if (buffSize == 0) {
		this->buffSize = 512;
	}
	else {
		this->buffSize = buffSize;
	}

	this->header.chunkId[0] = 'R';
	this->header.chunkId[1] = 'I';
	this->header.chunkId[2] = 'F';
	this->header.chunkId[3] = 'F';
	this->header.chunkSize = 36;
	this->header.format[0] = 'W';
	this->header.format[1] = 'A';
	this->header.format[2] = 'V';
	this->header.format[3] = 'E';
	this->header.subchunk1Id[0] = 'f';
	this->header.subchunk1Id[1] = 'm';
	this->header.subchunk1Id[2] = 't';
	this->header.subchunk1Id[3] = 0x20;
	this->header.subchunk1Size = 16;
	this->header.audioFormat = 1;
	this->header.numChannels = 2;
	this->header.sampleRate = sampleRate;
	this->header.byteRate = sampleRate * bitsPerSample / 4;
	this->header.blockAlign = bitsPerSample / 4;
	this->header.bitsPerSample = bitsPerSample;
	this->header.subchunk2Id[0] = 'd';
	this->header.subchunk2Id[1] = 'a';
	this->header.subchunk2Id[2] = 't';
	this->header.subchunk2Id[3] = 'a';
	this->header.subchunk2Size = 0;

	size_t alignment = volk_get_alignment();
	this->byteBuff = (uint8_t*)volk_malloc((this->buffSize * this->header.blockAlign) * sizeof(uint8_t), alignment);
	this->sampleBuff = (lv_32fc_t*)volk_malloc(this->buffSize * sizeof(lv_32fc_t), alignment);
	this->re = (float*)volk_malloc(this->buffSize * sizeof(float), alignment);
	this->im = (float*)volk_malloc(this->buffSize * sizeof(float), alignment);
	if (this->byteBuff == nullptr or this->sampleBuff == nullptr or this->re == nullptr or this->im == nullptr) {
		fputs("Unable to allocate memory!\n", stderr);
		return;
	}
	if (fwrite(&this->header, sizeof(header), 1, file) != 1) {
		fputs("Unable to write to file!\n", stderr);
		return;
	}
	this->fileOpen = 1;
}

WavSink::~WavSink() {
	if (this->fileOpen == 1) {
		flush();
		fclose(this->file);
		this->fileOpen = 0;
	}
	volk_free(this->byteBuff);
	volk_free(this->sampleBuff);
	volk_free(this->re);
	volk_free(this->im);
	free(this->path);
}

int WavSink::write_samples(lv_32fc_t* arr, size_t numSamples) {
	if (this->fileOpen == 0) {
		fputs("File closed!n\n", stderr);
		return -1;
	}
	size_t numWroteSamples = 0;
	while (numWroteSamples != numSamples) {
		size_t numSamplesToPush = ((this->buffSize - this->sampleBufPos) > (numSamples - numWroteSamples)) ? (
				numSamples - numWroteSamples) : (this->buffSize - this->sampleBufPos);
		memmove(this->sampleBuff + this->sampleBufPos, arr + numWroteSamples, numSamplesToPush * sizeof(lv_32fc_t));
		this->sampleBufPos += numSamplesToPush;
		numWroteSamples += numSamplesToPush;
		if (this->sampleBufPos == this->buffSize) {
			flush();
		}
	}
	return 0;
}

void WavSink::flush() {
	if (this->fileOpen == 0) {
		fputs("File closed!n\n", stderr);
		return;
	}
	if (this->sampleBufPos != 0) {
		volk_32fc_deinterleave_32f_x2(this->re, this->im, this->sampleBuff, this->sampleBufPos);

		if (this->header.bitsPerSample == 16) {
			int16_t t = 0;
			for (size_t i = 0; i < this->sampleBufPos; ++i) {
				t = static_cast<int16_t>(this->sampleBuff[i].real() * (1 << (this->header.bitsPerSample - 1)));
				memmove(this->byteBuff + i * this->header.blockAlign, &t, this->header.bitsPerSample / 8);
				t = static_cast<int16_t>(this->sampleBuff[i].imag() * (1 << (this->header.bitsPerSample - 1)));
				memmove(this->byteBuff + i * this->header.blockAlign + this->header.bitsPerSample / 8, &t,
				       this->header.bitsPerSample / 8);
			}
		}
		else {
			int32_t t = 0;
			for (size_t i = 0; i < this->sampleBufPos; ++i) {
				t = static_cast<int32_t>(this->sampleBuff[i].real() * (1 << (this->header.bitsPerSample - 1)));
				memmove(this->byteBuff + i * this->header.blockAlign, &t, this->header.bitsPerSample / 8);
				t = static_cast<int32_t>(this->sampleBuff[i].imag() * (1 << (this->header.bitsPerSample - 1)));
				memmove(this->byteBuff + i * this->header.blockAlign + this->header.bitsPerSample / 8, &t,
				       this->header.bitsPerSample / 8);
			}
		}

		size_t curPos = ftell(this->file);
		fseek(this->file, 0, SEEK_SET);
		this->header.chunkSize += this->sampleBufPos * this->header.blockAlign;
		this->header.subchunk2Size += this->sampleBufPos * this->header.blockAlign;
		fwrite(&this->header, sizeof(header), 1, this->file);
		fseek(this->file, curPos, SEEK_SET);
		fwrite(this->byteBuff, sizeof(uint8_t), this->sampleBufPos * this->header.blockAlign, this->file);
		this->sampleBufPos = 0;
	}
}
