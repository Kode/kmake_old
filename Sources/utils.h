#pragma once

#define addFunction(name, funcName, object)                                                                                                                    \
	JsPropertyIdRef name##Id;                                                                                                                                  \
	JsValueRef name##Func;                                                                                                                                     \
	JsCreateFunction(funcName, nullptr, &name##Func);                                                                                                          \
	JsCreatePropertyId(#name, strlen(#name), &name##Id);                                                                                                       \
	JsSetProperty(object, name##Id, name##Func, false)

typedef void *JsRef;
typedef JsRef JsPropertyIdRef;
JsPropertyIdRef getId(const char *name);