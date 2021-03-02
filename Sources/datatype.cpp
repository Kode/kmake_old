#include "datatype.h"
#include <stddef.h>

#ifdef _WIN32

#include <Windows.h>

Datatype loadDatatype(const char* name) {
	char path[MAX_PATH + 1];
	strcpy(path, "Datatypes\\");
	strcat(path, name);
	HMODULE lib = LoadLibraryA(path);
	Datatype datatype;
	datatype.formats = (FormatsType)GetProcAddress(lib, "formats");
	datatype.encode = (EncodeType)GetProcAddress(lib, "encode");
	return datatype;
}

#else

Datatype loadDatatype(const char* name) {
	Datatype datatype;
	datatype.formats = NULL;
	datatype.encode = NULL;
	return datatype;
}

#endif
