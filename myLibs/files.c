#include <windows.h>
#include <stdio.h>
#include "mystring.c"

void list_files_in_dir(const char *dir, char *filenames[]) {
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;
    int i = 0;

    if((hFind = FindFirstFile(dir, &fdFile)) == INVALID_HANDLE_VALUE)
    {
        printf("Path not found: [%s]\n", dir);
        return false;
    }

    do
    {
        //Find first file will always return "."
        //    and ".." as the first two directories.
        if(strcmp(fdFile.cFileName, ".") != 0
                && strcmp(fdFile.cFileName, "..") != 0)
        {
            //Build up our file path using the passed in
            //  [sDir] and the file/foldername we just found:

            //Is the entity a File or Folder? idgaf

            if(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
            {
                filenames[i++] = String(fdFile.cFileName).data;
            }
            else{
                filenames[i++] = String(fdFile.cFileName).data;
            }
        }
    }
    while(FindNextFile(hFind, &fdFile)); //Find the next file.

    FindClose(hFind); //Always, Always, clean things up!

    return true;
}


int main() {
    char *files[100] = {0};
    list_files_in_dir("..", files);

    for (int i = 0; i < 100; i++) {
        printf("%s\n", files[i]);
    }
}