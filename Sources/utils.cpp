#include <ChakraCore.h>
#include <assert.h>

JsPropertyIdRef getId(const char *name) {
	JsPropertyIdRef id;
	JsErrorCode err = JsCreatePropertyId(name, strlen(name), &id);
	assert(err == JsNoError);
	return id;
}