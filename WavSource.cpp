#include "WavSource.h"

WavSource::WavSource(const char* path, size_t buffSize) {
	if (path == nullptr) {
		fputs("Null pointer to file path!\n", stderr);
		return;
	}
	this->path = (char*)malloc((strlen(path) + 1) * sizeof(char));
	strcpy(this->path, path);
	this->file = fopen(this->path, "rb");
	if (this->file == nullptr) {
		fputs("Invalid file path!\n", stderr);
		return;
	}

	fseek(this->file, 0, SEEK_END);
	this->fileSize = ftell(file);
	fseek(this->file, 0, SEEK_SET);
	if (this->fileSize < sizeof(this->header)) {
		fputs("Invalid file format!\n", stderr);
		fclose(file);
		return;
	}

	fread(&this->header, sizeof(this->header), 1, this->file);
	if (this->header.subchunk1Size != 16 or this->header.audioFormat != 1 or this->header.numChannels != 2 or
	    (this->header.bitsPerSample != 16 and this->header.bitsPerSample != 32)) {
		fputs("Invalid file format!\n", stderr);
		fclose(file);
		return;
	}
	this->samplesCount = (this->fileSize - sizeof(this->header)) / this->header.blockAlign;
	if (buffSize == 0) {
		this->buffSize = 512;
	}
	else {
		this->buffSize = buffSize;
	}

	size_t alignment = volk_get_alignment();
	this->byteBuff = (uint8_t*)volk_malloc((this->buffSize * this->header.blockAlign) * sizeof(uint8_t), alignment);
	this->sampleBuff = (lv_32fc_t*)volk_malloc(this->buffSize * sizeof(lv_32fc_t), alignment);
	this->re = (float*)volk_malloc(this->buffSize * sizeof(float), alignment);
	this->im = (float*)volk_malloc(this->buffSize * sizeof(float), alignment);
	if (this->byteBuff == nullptr or this->sampleBuff == nullptr or this->re == nullptr or this->im == nullptr) {
		fputs("Unable to allocate memory!\n", stderr);
		return;
	}
	this->fileOpen = 1;
	this->sampleBuffPos = this->buffSize;
}

WavSource::~WavSource() {
	volk_free(this->byteBuff);
	volk_free(this->sampleBuff);
	volk_free(this->re);
	volk_free(this->im);
	free(this->path);
	if (this->fileOpen == 1) {
		fclose(this->file);
		this->fileOpen = 0;
	}
}

uint32_t WavSource::get_sampleRate() {
	if (this->fileOpen == 0) {
		fputs("File closed!\n", stderr);
		return 0;
	}
	return this->header.sampleRate;
}

uint16_t WavSource::get_bps() {
	if (this->fileOpen == 0) {
		fputs("File closed!n\n", stderr);
		return 0;
	}
	return this->header.bitsPerSample;
}

size_t WavSource::get_samplesInFile() {
	if (this->fileOpen == 0) {
		fputs("File closed!\n", stderr);
		return 0;
	}
	return this->samplesCount;
}

size_t WavSource::get_file_residue() {
	if (this->fileOpen == 0) {
		fputs("File closed!\n", stderr);
		return 0;
	}
	return (this->fileSize - ftell(this->file)) / this->header.blockAlign;
}

size_t WavSource::read_samples(lv_32fc_t* arr, size_t numSamples) {
	if (this->fileOpen == 0) {
		fputs("File closed!\n", stderr);
		return 0;
	}

	size_t numReadSamples = 0;
	while (numReadSamples != numSamples) {
		if (this->buffSize - this->sampleBuffPos == 0) {
			fill_buffer();
			if (this->buffSize - this->sampleBuffPos == 0) {
				return numReadSamples;
			}
		}
		size_t residue = this->buffSize - this->sampleBuffPos;
		size_t numSamplesToRead = (residue > (numSamples - numReadSamples)) ? (numSamples - numReadSamples) : residue;
		memmove(arr + numReadSamples, this->sampleBuff + this->sampleBuffPos, numSamplesToRead * sizeof(lv_32fc_t));
		this->sampleBuffPos += numSamplesToRead;
		numReadSamples += numSamplesToRead;
	}
	return numReadSamples;
}

void WavSource::fill_buffer() {
	if (this->fileOpen == 0) {
		fputs("File closed!\n", stderr);
		return;
	}

	if (this->sampleBuffPos != 0) {
		size_t residue = this->buffSize - this->sampleBuffPos;
		size_t numSamplesToRead = this->sampleBuffPos;
		size_t numReadSamples = fread(this->byteBuff, this->header.blockAlign, numSamplesToRead, this->file);
		memmove(this->sampleBuff, this->sampleBuff + this->sampleBuffPos - numReadSamples, residue * sizeof(lv_32fc_t));
		this->sampleBuffPos -= numReadSamples;
		this->numReadSamples += numReadSamples;

		if (this->header.bitsPerSample == 16) {
			int16_t t = 0;
			for (size_t i = 0; i < numReadSamples; ++i) {
				memmove(&t, this->byteBuff + i * this->header.blockAlign, this->header.bitsPerSample / 8);
				this->re[i] = (float)t / (1 << (this->header.bitsPerSample - 1));
				memmove(&t, this->byteBuff + i * this->header.blockAlign + this->header.bitsPerSample / 8, this->header.bitsPerSample / 8);
				this->im[i] = (float)t / (1 << (this->header.bitsPerSample - 1));
			}
		}
		else {
			int32_t t = 0;
			for (size_t i = 0; i < numReadSamples; ++i) {
				memmove(&t, this->byteBuff + i * this->header.blockAlign, this->header.bitsPerSample / 8);
				this->re[i] = (float)t / (1 << (this->header.bitsPerSample - 1));
				memmove(&t, this->byteBuff + i * this->header.blockAlign + this->header.bitsPerSample / 8, this->header.bitsPerSample / 8);
				this->im[i] = (float)t / (1 << (this->header.bitsPerSample - 1));
			}
		}

		volk_32f_x2_interleave_32fc(this->sampleBuff + (this->buffSize - numSamplesToRead), this->re, this->im, numSamplesToRead);
	}
}

size_t WavSource::get_fileSize() {
	if (this->fileOpen == 0) {
		fputs("File closed!\n", stderr);
		return 0;
	}
	return this->fileSize;
}
