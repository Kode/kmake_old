#pragma once

#include <stdint.h>

typedef const char *(*FormatsFunction)();

typedef void (*ImageDecodeFunction)(const char *format, uint8_t *data, size_t data_size, int *out_width, int *out_height, uint8_t **out_pixels,
                                    size_t *out_size);
typedef void (*ImageEncodeFunction)(const char *format, int width, int height, int stride, uint8_t *pixels, int *out_width, int *out_height, void **out_data,
                                    int *out_size);

typedef void (*AudioDecodeFunction)(const char *format, uint8_t *data, size_t data_size, uint8_t **out_data, size_t *out_size);
typedef void (*AudioEncodeFunction)(const char *format, uint8_t *data, size_t data_size, uint8_t **out_data, size_t *out_size);

struct Datatype {
	FormatsFunction imageEncodeformats;
	ImageEncodeFunction encodeImage;
	FormatsFunction imageDecodeformats;
	ImageDecodeFunction decodeImage;

	FormatsFunction audioEncodeformats;
	ImageEncodeFunction encodeAudio;
	FormatsFunction audioDecodeformats;
	ImageDecodeFunction decodeAudio;
};

Datatype loadDatatype(const char *name);
