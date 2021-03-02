#include "log.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

static void kinc_microsoft_format(const char *format, va_list args, wchar_t *buffer) {
	wchar_t formatw[4096];
	MultiByteToWideChar(CP_UTF8, 0, format, -1, formatw, 4096);

	size_t bufferIndex = 0;
	buffer[bufferIndex] = 0;
	printf("");
	for (int i = 0; formatw[i] != 0; ++i) {
		if (formatw[i] == L'%') {
			++i;
			switch (formatw[i]) {
			case L's':
			case L'S': {
				char *arg = va_arg(args, char *);
				wchar_t argw[1024];
				MultiByteToWideChar(CP_UTF8, 0, arg, -1, argw, 1024);
				wcscat(buffer, argw);
				bufferIndex += wcslen(argw);
				break;
			}
			case L'd':
			case L'i':
			case L'u':
			case L'o':
			case L'x': {
				int arg = va_arg(args, int);
				wchar_t argformat[3];
				argformat[0] = L'%';
				argformat[1] = formatw[i];
				argformat[2] = 0;
				bufferIndex += swprintf(&buffer[bufferIndex], 4096 - bufferIndex - 1, argformat, arg);
				break;
			}
			case 'f':
			case 'e':
			case 'g':
			case 'a': {
				double arg = va_arg(args, double);
				wchar_t argformat[3];
				argformat[0] = L'%';
				argformat[1] = formatw[i];
				argformat[2] = 0;
				bufferIndex += swprintf(&buffer[bufferIndex], 4096 - bufferIndex - 1, argformat, arg);
				break;
			}
			case 'c': {
				char arg = va_arg(args, char);
				wchar_t argformat[3];
				argformat[0] = L'%';
				argformat[1] = formatw[i];
				argformat[2] = 0;
				bufferIndex += swprintf(&buffer[bufferIndex], 4096 - bufferIndex - 1, argformat, arg);
				break;
			}
			case 'p':
			case 'n': {
				void *arg = va_arg(args, void *);
				wchar_t argformat[3];
				argformat[0] = L'%';
				argformat[1] = formatw[i];
				argformat[2] = 0;
				bufferIndex += swprintf(&buffer[bufferIndex], 4096 - bufferIndex - 1, argformat, arg);
				break;
			}
			case '%': {
				bufferIndex += swprintf(&buffer[bufferIndex], 4096 - bufferIndex - 1, L"%%");
				break;
			}
			}
		}
		else {
			buffer[bufferIndex++] = formatw[i];
			buffer[bufferIndex] = 0;
		}
	}
}

void kinc_log(kinc_log_level_t level, const char *format, ...) {
	va_list args;
	va_start(args, format);
	kinc_log_args(level, format, args);
	va_end(args);
}

#define UTF8

void kinc_log_args(kinc_log_level_t level, const char *format, va_list args) {
#ifdef _WIN32
#ifdef UTF8
	wchar_t buffer[4096];
	kinc_microsoft_format(format, args, buffer);
	wcscat(buffer, L"\r\n");
	OutputDebugString(buffer);
#ifdef _WIN32
	DWORD written;
	WriteConsole(GetStdHandle(level == KINC_LOG_LEVEL_INFO ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE), buffer, (DWORD)wcslen(buffer), &written, NULL);
#endif
#else
	char buffer[4096];
	vsnprintf(buffer, 4090, format, args);
	strcat(buffer, "\r\n");
	OutputDebugStringA(buffer);
#ifdef _WIN32
	DWORD written;
	WriteConsoleA(GetStdHandle(level == KINC_LOG_LEVEL_INFO ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE), buffer, (DWORD)strlen(buffer), &written, NULL);
#endif
#endif
#else
	char buffer[4096];
	vsnprintf(buffer, 4090, format, args);
	strcat(buffer, "\n");
	fprintf(level == KINC_LOG_LEVEL_INFO ? stdout : stderr, "%s", buffer);
#endif

#ifdef __ANDROID__
	switch (level) {
	case KINC_LOG_LEVEL_INFO:
		__android_log_vprint(ANDROID_LOG_INFO, "Kinc", format, args);
		break;
	case KINC_LOG_LEVEL_WARNING:
		__android_log_vprint(ANDROID_LOG_WARN, "Kinc", format, args);
		break;
	case KINC_LOG_LEVEL_ERROR:
		__android_log_vprint(ANDROID_LOG_ERROR, "Kinc", format, args);
		break;
	}
#endif
}
