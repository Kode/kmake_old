#pragma once

#include <ChakraCore.h>

void fs_init();
JsValueRef CALLBACK fs_openSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_closeSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_writeSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_readFileSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_readDirSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_ensureDirSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_copySync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_statSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_existsSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
JsValueRef CALLBACK fs_mkdirSync(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
