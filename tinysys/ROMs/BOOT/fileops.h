// File operations
#pragma once

#include <inttypes.h>

void MountDrive();
void UnmountDrive();
void ListFiles(const char *path);
uint32_t LoadExecutable(const char *filename, const bool reportError);
void RunExecutable(uint32_t startAddress);
