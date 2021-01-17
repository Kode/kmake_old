#pragma once

int LZ4_compressBound(int inputSize);
int LZ4_compress_default(const char* source, char* dest, int sourceSize, int maxDestSize);
