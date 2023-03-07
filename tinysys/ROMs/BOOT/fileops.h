// File operations
#pragma once

void MountDrive();
void UnmountDrive();
void ListFiles(const char *path);
void RunExecutable(const int hartID, const char *filename, const bool reportError);
