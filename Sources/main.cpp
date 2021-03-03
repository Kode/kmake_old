#include <ChakraCore.h>

#if 0
#include "Base/ThreadBoundThreadContextManager.h"
#include "Base/ThreadContextTlsEntry.h"
#include "Core/AtomLockGuids.h"
#include "Core/ConfigParser.h"
#include "Runtime.h"
#ifdef DYNAMIC_PROFILE_STORAGE
#include "Language/DynamicProfileStorage.h"
#endif
#include "JsrtContext.h"
#include "TestHooks.h"
#ifdef VTUNE_PROFILING
#include "Base/VTuneChakraProfile.h"
#endif
#ifdef ENABLE_JS_ETW
#include "Base/EtwTrace.h"
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#include "debug.h"
#include "debug_server.h"

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <map>
#include <sstream>
#include <stdarg.h>
#include <vector>

#ifdef _WIN32
#include <Windows.h> // AttachConsole
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

CHAKRA_API
JsStringToPointer(_In_ JsValueRef value, _Outptr_result_buffer_(*stringLength) const wchar_t **stringValue, _Out_ size_t *stringLength);

bool AttachProcess(HANDLE hmod);

const char *getExeDir();

JsRuntimeHandle runtime;
JsContextRef context;

#ifdef _WIN32
#define CALLBACK __stdcall
#else
#define CALLBACK
#endif

static int _argc;
static char **_argv;
static bool debugMode = false;
static bool watch = false;

static const int tempStringSize = 1024 * 1024 - 1;
static char tempString[tempStringSize + 1];
static char tempStringVS[tempStringSize + 1];
static char tempStringFS[tempStringSize + 1];

static JsPropertyIdRef buffer_id;

static void sendLogMessageArgs(const char *format, va_list args) {
	char msg[4096];
	vsnprintf(msg, sizeof(msg) - 2, format, args);
	kinc_log(KINC_LOG_LEVEL_INFO, "%s", msg);

	if (debugMode) {
		std::vector<int> message;
		message.push_back(IDE_MESSAGE_LOG);
		size_t messageLength = strlen(msg);
		message.push_back((int)messageLength);
		for (size_t i = 0; i < messageLength; ++i) {
			message.push_back(msg[i]);
		}
		//**sendMessage(message.data(), message.size());
	}
}

static void sendLogMessage(const char *format, ...) {
	va_list args;
	va_start(args, format);
	sendLogMessageArgs(format, args);
	va_end(args);
}

static std::vector<std::string> code;

static std::vector<std::string> split(std::string pattern) {
	std::vector<std::string> parts;
	size_t lastOffset = 0;
	for (size_t i = 0; i < pattern.length(); ++i) {
		if (pattern[i] == '/' || pattern[i] == '\\') {
			parts.push_back(pattern.substr(lastOffset, i - lastOffset));
			lastOffset = ++i;
		}
	}
	parts.push_back(pattern.substr(lastOffset));
	return parts;
}

static void findCode(const char *pattern, std::vector<std::string> parts, std::string current) {
	if (parts.size() > 1) {
		if (parts[0] == "**") {
		}
		else if (parts[0] == "*") {
		}
		else {
			std::string subdir = current + "/" + parts[0];
			DWORD dwAttrib = GetFileAttributesA(subdir.c_str());
			bool exists = dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
			if (exists) {
				parts.erase(parts.begin());
				findCode(pattern, parts, subdir);
			}
		}
	}
	else if (parts.size() == 1) {
		if (parts[0] == "**") {
			WIN32_FIND_DATAA ffd;
			LARGE_INTEGER filesize;
			std::string search = current + "\\*";
			size_t length_of_arg;
			HANDLE hFind = INVALID_HANDLE_VALUE;
			DWORD dwError = 0;

			hFind = FindFirstFileA(search.c_str(), &ffd);

			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
							findCode(pattern, parts, current + "/" + ffd.cFileName);
						}
					}
					else {
						code.push_back(current + "/" + ffd.cFileName);
					}
				} while (FindNextFileA(hFind, &ffd) != 0);
			}
		}
		else if (parts[0] == "*") {
		}
		else {
		}
	}
}

static void findCode(const char *pattern) {
	std::vector<std::string> parts = split(pattern);

	char szDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, szDir);

	findCode(pattern, parts, szDir);
}

static char kmake_dir[256];
static JsSourceContext next_cookie = 1234;

static void run_exporter() {
	char file_path[256];
	strcpy(file_path, kmake_dir);
	strcat(file_path, "/Library/out/linuxexporter.js");

	FILE *file = fopen(file_path, "rb");
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	const char *extra = "\nconst exporter = new LinuxExporter(); write(exporter.exportSolution(project));";

	char *code = (char *)malloc(size + 1 + strlen(extra));
	assert(code != NULL);
	fread(code, 1, size, file);
	code[size] = 0;
	strcat(code, extra);

	fclose(file);

	JsValueRef source;
	JsCreateString("exporter", strlen("exporter"), &source);

	JsSourceContext cookie = next_cookie;
	next_cookie += 1;

	JsModuleRecord record;
	JsValueRef specifier;
	JsCreateString("exporter", strlen("exporter"), &specifier);
	JsErrorCode err = JsInitializeModuleRecord(nullptr, specifier, &record);
	assert(err == JsErrorCode::JsNoError);

	JsValueRef exception;
	err = JsParseModuleSource(record, next_cookie, (BYTE *)code, size + strlen(extra), JsParseModuleSourceFlags_DataIsUTF8, &exception);
	assert(err == JsErrorCode::JsNoError);
	free(code);

	JsValueRef result;
	err = JsModuleEvaluation(record, &result);
	assert(err == JsErrorCode::JsNoError);
}

static JsValueRef CALLBACK kmake_write(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	char *string = (char *)malloc(1024 * 1024 * 16);
	size_t length;
	JsCopyString(arguments[1], string, 1024 * 1024 * 16, &length);

	FILE *file;
	fopen_s(&file, "projectfile", "wb");
	assert(file != NULL);
	fwrite(string, 1, length, file);
	fclose(file);

	return JS_INVALID_REFERENCE;
}

static JsValueRef CALLBACK kmake_resolve(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState) {
	{
		JsPropertyIdRef codeId;
		JsCreatePropertyId("code", strlen("code"), &codeId);
		JsValueRef code;
		JsGetProperty(arguments[1], codeId, &code);
		for (int i = 0;; ++i) {
			bool result;
			JsValueRef index;
			JsIntToNumber(i, &index);
			JsHasIndexedProperty(code, index, &result);
			if (!result) {
				break;
			}
			JsValueRef value;
			JsGetIndexedProperty(code, index, &value);
			char string[256];
			size_t length;
			JsCopyString(value, string, 256, &length);
			string[length] = 0;
			kinc_log(KINC_LOG_LEVEL_INFO, "Code: %s", string);
			findCode(string);
		}
	}

	JsValueRef codeFiles;
	JsCreateArray(code.size(), &codeFiles);
	for (size_t i = 0; i < code.size(); ++i) {
		JsValueRef index;
		JsIntToNumber(i, &index);

		JsValueRef string;
		JsCreateString(code[i].c_str(), code[i].size(), &string);

		JsSetIndexedProperty(codeFiles, index, string);
	}

	JsPropertyIdRef codeFilesId;
	JsCreatePropertyId("codeFiles", strlen("codeFiles"), &codeFilesId);
	JsSetProperty(arguments[1], codeFilesId, codeFiles, true);

	JsValueRef global;
	JsGetGlobalObject(&global);

	JsPropertyIdRef project;
	JsCreatePropertyId("project", strlen("project"), &project);
	JsSetProperty(global, project, arguments[1], false);

	return JS_INVALID_REFERENCE;
}

#define addFunction(name, funcName)                                                                                                                            \
	JsPropertyIdRef name##Id;                                                                                                                                  \
	JsValueRef name##Func;                                                                                                                                     \
	JsCreateFunction(funcName, nullptr, &name##Func);                                                                                                          \
	JsCreatePropertyId(#name, strlen(#name), &name##Id);                                                                                                       \
	JsSetProperty(global, name##Id, name##Func, false)

#define createId(name) JsCreatePropertyId(#name, strlen(#name), &name##_id)

static void bindFunctions() {
	createId(buffer);

	JsValueRef global;
	JsGetGlobalObject(&global);

	addFunction(resolve, kmake_resolve);
	addFunction(write, kmake_write);
}

JsValueRef script, source;

static void runJS() {
#if 0
	if (debugMode) {
		Message message = receiveMessage();
		handleDebugMessage(message, false);
	}
#endif

	JsValueRef undef;
	JsGetUndefinedValue(&undef);
	JsValueRef result;
	// JsCallFunction(updateFunction, &undef, 1, &result);

	bool except;
	JsHasException(&except);
	if (except) {
		JsValueRef meta;
		JsValueRef exceptionObj;
		JsGetAndClearExceptionWithMetadata(&meta);
		JsGetProperty(meta, getId("exception"), &exceptionObj);
		char buf[2048];
		size_t length;

		sendLogMessage("Uncaught exception:");
		JsValueRef sourceObj;
		JsGetProperty(meta, getId("source"), &sourceObj);
		JsCopyString(sourceObj, nullptr, 0, &length);
		if (length < 2048) {
			JsCopyString(sourceObj, buf, 2047, &length);
			buf[length] = 0;
			sendLogMessage("%s", buf);

			JsValueRef columnObj;
			JsGetProperty(meta, getId("column"), &columnObj);
			int column;
			JsNumberToInt(columnObj, &column);
			for (int i = 0; i < column; i++)
				if (buf[i] != '\t') buf[i] = ' ';
			buf[column] = '^';
			buf[column + 1] = 0;
			sendLogMessage("%s", buf);
		}

		JsValueRef stackObj;
		JsGetProperty(exceptionObj, getId("stack"), &stackObj);
		JsCopyString(stackObj, nullptr, 0, &length);
		if (length < 2048) {
			JsCopyString(stackObj, buf, 2047, &length);
			buf[length] = 0;
			sendLogMessage("%s\n", buf);
		}
	}
}

static bool startsWith(std::string str, std::string start) {
	return str.substr(0, start.size()) == start;
}

static bool endsWith(std::string str, std::string end) {
	if (str.size() < end.size()) return false;
	for (size_t i = str.size() - end.size(); i < str.size(); ++i) {
		if (str[i] != end[i - (str.size() - end.size())]) return false;
	}
	return true;
}

static std::string replaceAll(std::string str, const std::string &from, const std::string &to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
	return str;
}

extern "C" void watchDirectories(char *path1, char *path2);

extern "C" void filechanged(char *path) {
	std::string strpath = path;
	if (endsWith(strpath, ".png")) {
		std::string name = strpath.substr(strpath.find_last_of('/') + 1);
		// imageChanges[name] = true;
	}
	else if (endsWith(strpath, ".essl") || endsWith(strpath, ".glsl") || endsWith(strpath, ".d3d11")) {
		std::string name = strpath.substr(strpath.find_last_of('/') + 1);
		name = name.substr(0, name.find_last_of('.'));
		/*name = replace(name, '.', '_');
		name = replace(name, '-', '_');
		sendLogMessage("Shader changed: %s.", name.c_str());
		shaderFileNames[name] = strpath;
		shaderChanges[name] = true;*/
	}
	else if (endsWith(strpath, "krom.js")) {
		sendLogMessage("Code changed.");
		// codechanged = true;
	}
}

static void find_kmake_dir(char *exe_path, char *kmake_dir) {
	size_t length = strlen(exe_path);
	const int max_path_count = 4;
	int path_count = 0;

	for (size_t i = length - 1; i > 0; --i) {
		if (exe_path[i] == '\\') {
			++path_count;
			if (path_count == max_path_count) {
				strncpy(kmake_dir, exe_path, i);
				kmake_dir[i] = 0;
				return;
			}
		}
	}

	return;
}

static JsErrorCode CHAKRA_CALLBACK fetch_imported_module(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier,
                                                         _Outptr_result_maybenull_ JsModuleRecord *dependentModuleRecord) {
	char filename[256];
	size_t length;
	JsCopyString(specifier, filename, 255, &length);
	filename[length] = 0;

	char file_path[256];
	strcpy(file_path, kmake_dir);
	strcat(file_path, "/Library/out/");
	if (filename[0] == '.') {
		strcat(file_path, &filename[1]);
	}
	else {
		strcat(file_path, filename);
	}
	strcat(file_path, ".js");

	FILE *file = fopen(file_path, "rb");
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *code = (char *)malloc(size + 1);
	assert(code != NULL);
	fread(code, 1, size, file);
	code[size] = 0;

	JsSourceContext cookie = next_cookie;
	next_cookie += 1;

	JsModuleRecord record;
	JsErrorCode err = JsInitializeModuleRecord(nullptr, specifier, &record);
	assert(err == JsErrorCode::JsNoError);

	JsValueRef exception;
	err = JsParseModuleSource(record, next_cookie, (BYTE *)code, size, JsParseModuleSourceFlags_DataIsUTF8, &exception);
	assert(err == JsErrorCode::JsNoError);

	*dependentModuleRecord = record;

	return JsErrorCode::JsNoError;
}

static JsErrorCode CHAKRA_CALLBACK notify_module_ready(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar) {
	return JsErrorCode::JsNoError;
}

static void CHAKRA_CALLBACK promise_continuation_callback(_In_ JsValueRef task, _In_opt_ void *callbackState) {
	*(void **)callbackState = task;
}

static void run_file(const char *file_path, const char *name) {
	FILE *file = fopen(file_path, "rb");
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char import_path[256];
	strcpy(import_path, kmake_dir);
	strcat(import_path, "/Library/out/import.js");

	FILE *import_file = fopen(import_path, "rb");
	fseek(import_file, 0, SEEK_END);
	size_t import_size = ftell(import_file);
	fseek(import_file, 0, SEEK_SET);

	char *code = (char *)malloc(import_size + 1 + size + 1);
	assert(code != NULL);
	fread(code, 1, import_size, import_file);
	code[import_size] = '\n';
	fread(&code[import_size + 1], 1, size, file);
	code[import_size + 1 + size] = 0;

	fclose(import_file);
	fclose(file);

	JsValueRef source;
	JsCreateString(name, strlen(name), &source);

	JsSourceContext cookie = next_cookie;
	next_cookie += 1;

	JsModuleRecord record;
	JsValueRef specifier;
	JsCreateString(name, strlen(name), &specifier);
	JsErrorCode err = JsInitializeModuleRecord(nullptr, specifier, &record);
	assert(err == JsErrorCode::JsNoError);

	JsSetModuleHostInfo(record, JsModuleHostInfo_FetchImportedModuleCallback, fetch_imported_module);
	JsSetModuleHostInfo(record, JsModuleHostInfo_NotifyModuleReadyCallback, notify_module_ready);

	JsValueRef exception;
	err = JsParseModuleSource(record, next_cookie, (BYTE *)code, import_size + 1 + size, JsParseModuleSourceFlags_DataIsUTF8, &exception);
	assert(err == JsErrorCode::JsNoError);
	free(code);

	JsValueRef result;
	err = JsModuleEvaluation(record, &result);
	assert(err == JsErrorCode::JsNoError);
}

int main(int argc, char **argv) {
#ifdef _WIN32
	AttachProcess(GetModuleHandle(nullptr));
#else
	AttachProcess(nullptr);
#endif

	_argc = argc;
	_argv = argv;

#ifdef NDEBUG
	JsCreateRuntime(JsRuntimeAttributeEnableIdleProcessing, nullptr, &runtime);
#else
	JsCreateRuntime(JsRuntimeAttributeAllowScriptInterrupt, nullptr, &runtime);
#endif

	JsCreateContext(runtime, &context);
	JsAddRef(context, nullptr);

	JsSetCurrentContext(context);

	bindFunctions();

	JsValueRef callback = JS_INVALID_REFERENCE;
	JsSetPromiseContinuationCallback(promise_continuation_callback, &callback);

	find_kmake_dir(argv[0], kmake_dir);

	run_file("kfile.js", "kfile.js");

	JsValueRef task = JS_INVALID_REFERENCE;
	while (callback != JS_INVALID_REFERENCE) {
		task = callback;
		callback = JS_INVALID_REFERENCE;
		JsValueRef result;
		JsCallFunction(task, nullptr, 0, &result);
	}

	run_exporter();

	return 0;
}
