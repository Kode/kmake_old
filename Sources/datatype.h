#pragma once

typedef const char* (*FormatsType)();
typedef void (*EncodeType)(int width, int height, int stride, const char* format, unsigned char* pixels, int* out_width, int* out_height, int* out_size, void** out_data);

struct Datatype {
	FormatsType formats;
	EncodeType encode;
};

Datatype loadDatatype(const char* name);
