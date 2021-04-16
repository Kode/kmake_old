#include "fs.h"

#include "utils.h"

#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define PATHNAME_MAX 256
#define MAX_FILE_COUNT 128
#if _WIN32
#include <io.h>
#define F_OK 0
#define access _access

#include <malloc.h>
#define alloca _malloca

#include <direct.h>
#define S_IRWXU	0
#define mkdir(dirname,mode) _mkdir(dirname)
#else
#include <sys/stat.h>
#include <unistd.h>
#include <alloca.h>
#include <sys/types.h>
#include <dirent.h>
#endif


static bool directoryExists(const char* path) {
	#if _WIN32
	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(path, &ffd);
	return hFind != INVALID_HANDLE_VALUE && (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	#else
	return opendir(path) != NULL
	#endif
}

static int getExtension(char* filename,char* extension) {
	size_t strLength = strlen(filename);
	for (int i = strLength - 1; i > 0 ; --i) {
		if (filename[i] == '.' && i != (strLength - 1)) {
			strcpy(extension, &filename[i]);
			return 1;
		}
		else if (strLength - i > 8) {
			return 0;
		}
	}
	return 0;
}
static uint32_t readDir(const char *path, bool with_extensions, char **out) {
	if (!directoryExists(path)) goto EXIT;
	#if _WIN32
	int len = strlen(path) + 4;
	char *fixedPath = (char *)alloca(sizeof(char) * (len));
	assert(fixedPath != NULL);
	strcpy(fixedPath, path);
	strcat(fixedPath, "\\*");
	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(fixedPath, &ffd);
	if (hFind == INVALID_HANDLE_VALUE) goto EXIT;
	uint32_t count = 0;
	while (FindNextFileA(hFind, &ffd))
	{
		if (strcmp(ffd.cFileName, "..") == 0) continue;
		char ext[8];
		if (!with_extensions && getExtension(ffd.cFileName,ext)) {
			size_t size = strlen(ffd.cFileName);
			char *endName = out[count];
			strcpy(endName, ffd.cFileName);
			endName[size - strlen(ext)] = 0;
		}
		else {
			strcpy(out[count], ffd.cFileName);	
		}
		count++;
	}
	return count;
	#else
	return opendir(path) != NULL
	#endif
EXIT:
	kinc_log(KINC_LOG_LEVEL_ERROR, "The path %s isn't a directory or it couldn't be found", path);
	exit(1);

}
static int recursiveMkdir(char *path, int mode) {
	int success = 0;// 0 Is true because mkdir reasons...
	size_t strLen = strlen(path);
	char *token = strtok(path, "/");
	char* out = (char*)alloca(sizeof(char) * strLen);
	assert(out != NULL);
	memset(out, 0, sizeof(char) * strLen);
	while (token != NULL && strlen(token) != strLen) {
		if (token != NULL) {
			strcat(out, token);
			if (!directoryExists(out)) {
				success = mkdir(out, mode);
				if (success != 0) {
					kinc_log(KINC_LOG_LEVEL_ERROR, "Path %s was not found.", out);
					return success;
				}
			}
			strcat(out, "/");
		}
		token = strtok(NULL, "/");
		
	}
	return success;
}

static bool copyFile(char *from, char *to) {
	FILE *srcFile;
	FILE *destFile;

	srcFile = fopen(from, "rb");
	assert(srcFile != NULL);
	fseek(srcFile, 0, SEEK_END);
	size_t size = ftell(srcFile);
	fseek(srcFile, 0, SEEK_SET);

	char *txt = (char *)alloca(size + 1);
	assert(txt != NULL);
	fread(txt, 1, size, srcFile);
	txt[size] = 0;

	destFile = fopen(to, "wb");
	assert(destFile != NULL);
	size_t writeAmount = 0;
	writeAmount = fwrite(txt, sizeof(char), size, destFile);

	fclose(srcFile);
	fclose(destFile);

	return writeAmount == size;
}
static bool copyDir(char *from, char *to) {
	char *out[MAX_FILE_COUNT];
	for (int i = 0; i < MAX_FILE_COUNT; ++i) {
		out[i] = (char *)alloca(sizeof(char) * PATHNAME_MAX);
		assert(out[i] != NULL);
	}
	uint32_t size = readDir(from, true, out);
	bool succeded = true;
	char srcPath[PATHNAME_MAX];
	char destPath[PATHNAME_MAX];
	for (int i = 0; i < size && succeded; ++i) {
		memset(srcPath, 0, PATHNAME_MAX);
		memset(destPath, 0, PATHNAME_MAX);
		strcpy(srcPath, from);
		strcat(srcPath, "/");
		strcat(srcPath, out[i]);

		strcpy(destPath, to);
		strcat(destPath, "/");
		strcat(destPath, out[i]);
		bool isSrcDir = directoryExists(srcPath);

		if (isSrcDir && !directoryExists(destPath)) {
			int success = mkdir(destPath, S_IRWXU);
			if (success != 0) {
				kinc_log(KINC_LOG_LEVEL_ERROR, "Couldn't create folder at path %s", destPath);
				exit(1);
			}
		}

		if (isSrcDir) {
			succeded = copyDir(srcPath, destPath);
		}
		else {
			succeded = copyFile(srcPath, destPath);
		}
	}
	return succeded;
}

void fs_init() {

	JsPropertyIdRef fsPropId;
	JsValueRef global, fs;
	JsGetGlobalObject(&global);

	JsCreateObject(&fs);
	addFunction(open, fs_openSync, fs);
	addFunction(close, fs_closeSync, fs);
	addFunction(write, fs_writeSync, fs);
	addFunction(writeFile, fs_writeSync, fs);
	addFunction(readFile, fs_readFileSync, fs);
	addFunction(readdir, fs_readDirSync, fs);
	addFunction(ensureDir, fs_ensureDirSync, fs);
	addFunction(copy, fs_copySync, fs);
	addFunction(exists, fs_existsSync, fs);
	addFunction(mkdir, fs_mkdirSync, fs);

	JsCreatePropertyId("fs", strlen("fs"), &fsPropId);
	JsSetProperty(global, fsPropId, fs, true);
}


JsValueRef fs_openSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {

	JsValueRef stringValue;
	JsConvertValueToString(arguments[1], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);
	
	char file_path[512];
	for (int i = 0; i < strLength; ++i) {
		file_path[i] = str[i];
	}
	file_path[strLength] = 0;

	JsValueRef flagsValue;
	JsConvertValueToString(arguments[2], &flagsValue);
	JsStringToPointer(flagsValue, &str, &strLength);

	char flags[8];
	for (int i = 0; i < strLength; ++i) {
		flags[i] = str[i];
	}
	flags[strLength] = 0;
	
	JsValueRef value;
	FILE *file = fopen(file_path, flags);
	if (file != NULL) {
		JsCreateExternalObject(file, nullptr, &value);
		return value;
	}
	
	return JsValueRef();
}

JsValueRef fs_closeSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	
	FILE *file;
	JsGetExternalData(arguments[1], (void **)&file);
	int failed = 1;
	if (file != NULL) {
		failed = fclose(file);
	}
	JsValueRef value;
	JsBoolToBoolean(failed == 0, &value);
	return value;
}

JsValueRef fs_writeSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	
	FILE *file;

	JsValueType type;
	JsGetValueType(arguments[1], &type);
	if (type == JsString) {
		JsValueRef stringValue;
		JsConvertValueToString(arguments[1], &stringValue);

		const wchar_t *str = nullptr;
		size_t strLength = 0;
		JsStringToPointer(stringValue, &str, &strLength);

		char file_path[512];
		for (int i = 0; i < strLength; ++i) {
			file_path[i] = str[i];
		}
		file_path[strLength] = 0;

		file = fopen(file_path, "wb+");
	}
	else {
		JsGetExternalData(arguments[1], (void **)&file);
	}

	JsValueRef stringValue;
	JsConvertValueToString(arguments[2], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);

	char* txt = (char*)alloca(sizeof(char) * strLength);
	assert(txt != NULL);
	for (int i = 0; i < strLength; ++i) {
		txt[i] = str[i];
	}
	txt[strLength] = 0;

	size_t writeAmount = 0;
	if (file != NULL) {
		writeAmount = fwrite(txt, sizeof(char), strLength, file);
	}

	if (type == JsString) {
		fclose(file);
	}

	JsValueRef value;
	JsBoolToBoolean(writeAmount == strLength, &value);
	return value;
}

JsValueRef fs_readFileSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	JsValueRef stringValue;
	JsConvertValueToString(arguments[1], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);

	char file_path[512];
	for (int i = 0; i < strLength; ++i) {
		file_path[i] = str[i];
	}
	file_path[strLength] = 0;

	JsValueRef value;
	FILE *file = fopen(file_path, "rb");
	if (file != NULL) {
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);

		char *contents = (char *)alloca(size + 1);
		assert(contents != NULL);
		fread(contents, 1, size, file);
		contents[size] = 0;
		JsCreateString(contents, size, &value);
		return value;
	}

	return JsValueRef();
}

JsValueRef fs_readDirSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	JsValueRef stringValue;
	JsConvertValueToString(arguments[1], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);

	char file_path[512];
	for (int i = 0; i < strLength; ++i) {
		file_path[i] = str[i];
	}
	file_path[strLength] = 0;

	bool with_extensions = false;
	char encoding[8] = "utf8";
	JsValueType type;
	JsGetValueType(arguments[2], &type);
	if (type == JsObject) {
		JsValueRef v;
		if (JsGetProperty(arguments[2], getId("withFileTypes"), &v) == JsNoError) {
			JsBooleanToBool(v, &with_extensions);
		}
		if (JsGetProperty(arguments[2], getId("encoding"), &v) == JsNoError) {
			JsGetValueType(v, &type);
			if (type == JsString) {
				JsValueRef stringValue;
				JsConvertValueToString(arguments[2], &stringValue);
				JsStringToPointer(stringValue, &str, &strLength);
				for (int i = 0; i < strLength; ++i) {
					encoding[i] = str[i];
				}
				encoding[strLength] = 0;
			}
		}
	}
	else if (type == JsString) {
		JsValueRef stringValue;
		JsConvertValueToString(arguments[2], &stringValue);
		JsStringToPointer(stringValue, &str, &strLength);
		for (int i = 0; i < strLength; ++i) {
			encoding[i] = str[i];
		}
		encoding[strLength] = 0;
	}

	JsValueRef value;
	char *out[MAX_FILE_COUNT];
	for (int i = 0; i < MAX_FILE_COUNT; ++i) {
		out[i] = (char *)alloca(sizeof(char) * PATHNAME_MAX);
		assert(out[i] != NULL);
	}
	uint32_t size = readDir(file_path, with_extensions, out);
	if (strcmp(encoding, "utf8") == 0) {
		JsCreateArray(size, &value);
	}
	else {
		JsCreateArrayBuffer(size,&value);
	}
	for (int i = 0; i < size; ++i) {
		JsValueRef index, element;
		JsIntToNumber(i, &index);
		JsCreateString(out[i], strlen(out[i]), &element);
		JsSetIndexedProperty(value, index, element);
	}

	return value;
}

JsValueRef fs_ensureDirSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	JsValueRef stringValue;
	JsConvertValueToString(arguments[1], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);

	char file_path[512];
	for (int i = 0; i < strLength; ++i) {
		file_path[i] = str[i];
	}
	file_path[strLength] = 0;

	int success = 0;
	if (!directoryExists(file_path)) {
		JsValueType type;
		JsGetValueType(arguments[2], &type);

		int mode = S_IRWXU;
		bool recursive = false;
		if (type == JsObject) {
			bool hasProp;
			JsHasProperty(arguments[2], getId("recursive"), &hasProp);
			if (hasProp) {
				JsValueRef v;
				JsGetProperty(arguments[2], getId("recursive"), &v);
				JsBooleanToBool(v, &recursive);
			}
			JsHasProperty(arguments[2], getId("mode"), &hasProp);
			if (hasProp) {
				JsValueRef v;
				JsGetProperty(arguments[2], getId("mode"), &v);
				JsNumberToInt(v, &mode);
			}
		}
		else if (type == JsNumber) {
			JsNumberToInt(arguments[2], &mode);
		}

		if (!recursive) {
			success = mkdir(file_path, mode);
		}
		else {
			success = recursiveMkdir(file_path, mode);
		}
	}

	JsValueRef value;
	JsBoolToBoolean(success == 0, &value);
	return value;
}

JsValueRef fs_copySync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	
	JsValueRef stringValue;
	JsConvertValueToString(arguments[1], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);

	char* src = (char*)alloca(sizeof(char)* strLength);
	assert(src != NULL);
	for (int i = 0; i < strLength; ++i) {
		src[i] = str[i];
	}
	src[strLength] = 0;

	JsConvertValueToString(arguments[2], &stringValue);
	JsStringToPointer(stringValue, &str, &strLength);

	char *dest = (char *) alloca(sizeof(char) * strLength);
	assert(dest != NULL);
	for (int i = 0; i < strLength; ++i) {
		dest[i] = str[i];
	}
	dest[strLength] = 0;

	bool isSrcDir = directoryExists(src);
	bool isDestDir = directoryExists(dest);
	if (!isSrcDir && access(src, F_OK) != -1 && !isDestDir) {
		copyFile(src, dest);
	}
	else if (isSrcDir && isDestDir) {
		copyDir(src, dest);
	}
	else {
		if (isSrcDir && !isDestDir) {
			kinc_log(KINC_LOG_LEVEL_ERROR, "Destination directory doesn't exist, at path: %s",dest);
		}
		else if (!isSrcDir && isDestDir) {
			kinc_log(KINC_LOG_LEVEL_ERROR, "Destination path can't be a directory when source path is a file\n src: %s\n dest: %s",src,dest);
		}
		exit(1);
	}
	return JsValueRef();
}

JsValueRef fs_statSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	return JsValueRef();
}

JsValueRef fs_existsSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	JsValueRef stringValue;
	JsConvertValueToString(arguments[1], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);

	char file_path[512];
	for (int i = 0; i < strLength; ++i) {
		file_path[i] = str[i];
	}
	file_path[strLength] = 0;

	JsValueRef value;
	JsBoolToBoolean(access(file_path, F_OK) != -1,&value);

	return value;
}

JsValueRef fs_mkdirSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {

	JsValueRef stringValue;
	JsConvertValueToString(arguments[1], &stringValue);

	const wchar_t *str = nullptr;
	size_t strLength = 0;
	JsStringToPointer(stringValue, &str, &strLength);

	char file_path[512];
	for (int i = 0; i < strLength; ++i) {
		file_path[i] = str[i];
	}
	file_path[strLength] = 0;

	JsValueType type;
	JsGetValueType(arguments[2], &type);

	int mode = S_IRWXU;
	bool recursive = false;
	if (type == JsObject) {
		bool hasProp;
		JsHasProperty(arguments[2], getId("recursive"), &hasProp);
		if (hasProp) {
			JsValueRef v;
			JsGetProperty(arguments[2], getId("recursive"), &v);
			JsBooleanToBool(v, &recursive);
		}
		JsHasProperty(arguments[2], getId("mode"), &hasProp);
		if (hasProp) {
			JsValueRef v;
			JsGetProperty(arguments[2], getId("mode"), &v);
			JsNumberToInt(v, &mode);
		}
	}
	else if (type == JsNumber) {
		JsNumberToInt(arguments[2], &mode);
	}

	int success; 
	if (!recursive) {
		success = mkdir(file_path, mode);	
	}
	else {
		success = recursiveMkdir(file_path, mode);
	}

	JsValueRef value;
	JsBoolToBoolean(success == 0, &value);
	return value;
}
