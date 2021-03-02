#include "debug.h"
#include "debug_server.h"

#include <ChakraCore.h>
#include <ChakraDebug.h>

#include "log.h"

#include <assert.h>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace {
	void CHAKRA_CALLBACK debugCallback(JsDiagDebugEvent debugEvent, JsValueRef eventData, void *callbackState) {
		if (debugEvent == JsDiagDebugEventBreakpoint || debugEvent == JsDiagDebugEventAsyncBreak || debugEvent == JsDiagDebugEventStepComplete ||
		    debugEvent == JsDiagDebugEventDebuggerStatement || debugEvent == JsDiagDebugEventRuntimeException) {
			kinc_log(KINC_LOG_LEVEL_INFO, "Debug callback: %i", debugEvent);

#if 0
			int message = IDE_MESSAGE_BREAK;
			sendMessage(&message, 1);

			for (;;) {
				Message message = receiveMessage();
				if (handleDebugMessage(message, true)) {
					break;
				}
#ifdef _WIN32
				Sleep(100);
#else
				usleep(100 * 1000);
#endif
		}
#endif
		}
		else if (debugEvent == JsDiagDebugEventCompileError) {
			kinc_log(KINC_LOG_LEVEL_ERROR, "Script compile error.");
		}
	}

	class Script {
	public:
		int scriptId;
		char fileName[1024];
		int lineCount;
		int sourceLength;
	};

	std::vector<Script> scripts;
}

int scriptId() {
#if 0
	if (::scripts.size() == 0) {
		JsValueRef scripts;
		JsDiagGetScripts(&scripts);
		JsValueRef lengthObj;
		JsGetProperty(scripts, getId("length"), &lengthObj);
		int length;
		JsNumberToInt(lengthObj, &length);
		for (int i = 0; i < length; ++i) {
			JsValueRef scriptId, fileName, lineCount, sourceLength;
			JsValueRef iObj;
			JsIntToNumber(i, &iObj);
			JsValueRef obj;
			JsGetIndexedProperty(scripts, iObj, &obj);
			JsGetProperty(obj, getId("scriptId"), &scriptId);
			JsGetProperty(obj, getId("fileName"), &fileName);
			JsGetProperty(obj, getId("lineCount"), &lineCount);
			JsGetProperty(obj, getId("sourceLength"), &sourceLength);

			Script script;
			JsNumberToInt(scriptId, &script.scriptId);
			JsNumberToInt(lineCount, &script.lineCount);
			JsNumberToInt(sourceLength, &script.sourceLength);
			size_t length;
			JsCopyString(fileName, script.fileName, 1023, &length);
			script.fileName[length] = 0;

			::scripts.push_back(script);
		}
	}
#endif
	return ::scripts[0].scriptId;
}

void startDebugger(JsRuntimeHandle runtimeHandle, int port) {
	JsDiagStartDebugging(runtimeHandle, debugCallback, nullptr);
	//** startServer(port);
}
