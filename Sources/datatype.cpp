#include "datatype.h"
#include <stddef.h>

#ifdef _WIN32

#include <Windows.h>

Datatype loadDatatype(const char *name) {
	char path[MAX_PATH + 1];
	strcpy(path, "Datatypes\\");
	strcat(path, name);
	HMODULE lib = LoadLibraryA(path);

	Datatype datatype;
	memset(&datatype, 0, sizeof(datatype));

	if (lib != NULL) {
		datatype.imageEncodeformats = (FormatsFunction)GetProcAddress(lib, "imageEncodeformats");
		datatype.encodeImage = (ImageEncodeFunction)GetProcAddress(lib, "encodeImage");
		datatype.imageDecodeformats = (FormatsFunction)GetProcAddress(lib, "imageDecodeformats");
		datatype.decodeImage = (ImageDecodeFunction)GetProcAddress(lib, "decodeImage");

		datatype.audioEncodeformats = (FormatsFunction)GetProcAddress(lib, "audioEncodeformats");
		datatype.encodeAudio = (ImageEncodeFunction)GetProcAddress(lib, "encodeAudio");
		datatype.audioDecodeformats = (FormatsFunction)GetProcAddress(lib, "audioDecodeformats");
		datatype.decodeAudio = (ImageDecodeFunction)GetProcAddress(lib, "decodeAudio");
	}

	return datatype;
}

#else

Datatype loadDatatype(const char *name) {
	Datatype datatype;
	memset(&datatype, 0, sizeof(datatype));
	return datatype;
}

#endif
