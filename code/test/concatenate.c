#include "syscall.h"
#include "copyright.h"

#define MAX_LENGTH 255

int main(int argc, char* argv[]) {

    OpenFileId id1, id2, id3;
    char fileName1[MAX_LENGTH];
    char fileName2[MAX_LENGTH];
    int fileSize = 0;
    int i = 0;
    char c;
    
    PrintString("Nhap ten file 1: ");
    ReadString(fileName1, MAX_LENGTH);

    id1 = Open(fileName1, 1);
    if (id1 == -1) {
        PrintString("Loi!");
    } else {
        PrintString("Nhap ten file 2: ");
        ReadString(fileName2, MAX_LENGTH);
        id2 = Open(fileName2, 1);
        if (id2 == -1) {
            PrintString("Loi!");
            Close(id1);
        } else {
            if (Create("file3.txt") == 0) {
                id3 = Open("file3.txt", 0);
                if (id3 == -1) {
                    PrintString("Loi!");
                    Close(id2);
                } else {
                    fileSize = Seek(-1, id1);
                    Seek(0, id1);
                    for (i = 0; i < fileSize; i++) {
                        Read(&c, 1, id1);
                        Write(&c, 1, id3);
                    }
                    fileSize = Seek(-1, id2);
                    Seek(0, id2);
                    for (i = 0; i < fileSize; i++) {
                        Read(&c, 1, id2);
                        Write(&c, 1, id3);
                    }
                    Close(id3);
                    Close(id1);
                    Close(id2);
                }
            }
        }
    }
	
    Halt();
    return 0;
}