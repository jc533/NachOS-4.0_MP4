/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt()
{
	kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}

#ifdef FILESYS_STUB
int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}
OpenFileId SysOpen(char *name){
    return kernel->fileSystem->OpenAFile(name);
}

OpenFileId SysRead(char *buffer, int size, OpenFileId id){
    return kernel->fileSystem->ReadFile(buffer, size, id);
}
OpenFileId SysWrite(char *buffer, int size, OpenFileId id){
    return kernel->fileSystem->WriteFile(buffer, size, id);
}
OpenFileId SysClose(OpenFileId id){
    return kernel->fileSystem->CloseFile(id);
}
#endif

#endif /* ! __USERPROG_KSYSCALL_H__ */
