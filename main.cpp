#include <iostream>
#include "WavSource.h"
#include "WavSink.h"
#include "FIRFilter.h"
#include <ctime>

int main() {
	size_t bufSize = 8192;
	WavSource source("C:/Users/DNAPC/Desktop/DSP/Wav files/in.wav", bufSize);
	WavSink sink("C:/Users/DNAPC/Desktop/DSP/Wav files/out.wav", source.get_sampleRate(), source.get_bps(), bufSize);
	FIRFilter filter(LOWPASS, WIN_BLACKMAN_NUTTALL, source.get_sampleRate(), 1000, 1500);
	lv_32fc_t* arr = (lv_32fc_t*)volk_malloc(bufSize * sizeof(lv_32fc_t), 32);
	lv_32fc_t* arr2 = (lv_32fc_t*)volk_malloc(bufSize * sizeof(lv_32fc_t), 32);
	clock_t begin, end;
	begin = clock();
	while (source.read_samples(arr, bufSize) != 0) {
		filter.filter(arr, arr2, bufSize);
		sink.write_samples(arr2, bufSize);
	}
	end = clock();
	float time = (float)(end - begin) / CLOCKS_PER_SEC;
	float speed = ((float)source.get_fileSize() / 1048576) / time;
	printf("%f MB/s", speed);
	volk_free(arr);
	volk_free(arr2);
	return 0;
}