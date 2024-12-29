// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];

    // MP4 mod tag
    memset(table, 0, sizeof(DirectoryEntry) * size); // dummy operation to keep valgrind happy

    tableSize = size;
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::Find(char *name)
{
    int i = FindIndex(name);
    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector, bool isDirectory)
{
    if (FindIndex(name) != -1)
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse)
        {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen);
            table[i].sector = newSector;
            table[i].isDir = isDirectory;
            return TRUE;
        }
    return FALSE; // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name)
{
    int i = FindIndex(name);

    if (i == -1)
        return FALSE; // name not in directory
    table[i].inUse = FALSE;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------

void Directory::List()
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse){
            printf("%s\n", table[i].name+1);
        }  
}
void Directory::ListRecursive(int depth)
{
    OpenFile *temp;
    Directory *sub = new Directory(NumDirEntries);
    for (int i = 0; i < tableSize; i++){
        if (!table[i].inUse)
            continue;
        else {
            for(int j=0;j<depth;j++)
                printf("    ");
            printf("%s %s",table[i].isDir?"[D]":"[F]", table[i].name+1);
            printf("\n");
            if(table[i].isDir){
                temp = new OpenFile(table[i].sector);
                //cout << table[i].sector << '\n';
                sub->FetchFrom(temp);
                sub->ListRecursive(depth+1);
            }
        }
        
    }
}


//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        {
            printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}

void Split(char *name, char *Path, char *filename){
    int i, j;
	int split;
	// /t0/bb/f1
	for (i = 0; name[i] != '\0'; i++)
	{
		if (name[i] == '/')
		{
			split = i; // last '/'
		}
	}
	Path[0] = '/';
	for (i = 1; i < split; i++)
	{
		Path[i] = name[i];
	}
	Path[i] = '\0';
	for (i = split, j = 0; name[i] != '\0'; i++, j++)
	{
		filename[j] = name[i];
	}
	filename[j] = '\0';
}

int Directory::FindPath(char *name){
	char rootPath[256];
	char restPath[256];
	if (name[1] == '\0'){
		return 1;
	}
	else{
		bool LastLevel = true;
		int split = 0;
		for (int i = 1; name[i] != '\0'; i++)
		{
			if (name[i] == '/'){
				strncpy(rootPath, name, i); 
				rootPath[i] = '\0';			
				LastLevel = false;
				split = i;
				break;
			}
		}
		if (LastLevel){
			strcpy(rootPath, name);
			strcat(rootPath, "\0");
			return Find(rootPath);
		}
		else{
			int sector = Find(rootPath);
			if (sector == -1)
				return -1;
			Directory *directory = new Directory(sizeof(DirectoryEntry) * 64);
			OpenFile *directoryFile = new OpenFile(sector);
			directory->FetchFrom(directoryFile);
			for (int i = 1; i < strlen(name); i++){
				if (name[i] == '/')
				{
					for (int j = 0; j < strlen(name) - i + 1; j++)
						restPath[j] = name[j + i];
					break;
				}
			}
			sector = directory->FindPath(restPath);
			delete directory;
			delete directoryFile;
			return sector;
		}
	}
}