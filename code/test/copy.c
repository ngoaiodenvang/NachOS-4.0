#include "syscall.h"
#include "copyright.h"

#define MAX_LENGTH 255



int main(int argc, char* argv[])
{
	int openFileIdSrc;
    int openFileIdDes;
	int fileSize;
	char c; //Ky tu de in ra
	char fileNameSrc[MAX_LENGTH];
    char fileNameDes[MAX_LENGTH];
	int i; //Index for loop
    
	PrintString(" - Nhap vao ten file nguon: ");
	
	// //Goi ham ReadString de doc vao ten file
	// //Co the su dung Open(stdin), nhung de tiet kiem thoi gian test ta dung ReadString
	ReadString(fileNameSrc, MAX_LENGTH);
	PrintString(" - Nhap vao ten file dich: ");
    ReadString(fileNameDes, MAX_LENGTH);
	openFileIdSrc = Open(fileNameSrc, 1); // Goi ham Open de mo file 
	openFileIdDes = Open(fileNameDes, 0);


	if(openFileIdSrc != -1 && openFileIdDes != -1) //Kiem tra Open co loi khong
	{
		//Seek den cuoi file de lay duoc do dai noi dung (fileSize)
		fileSize = Seek(-1, openFileIdSrc);
		i = 0;
		// Seek den dau tap tin de tien hanh Read
		Seek(0, openFileIdSrc);
		
		PrintString(" -> Noi dung file:\n");
		for (; i < fileSize; i++) // Cho vong lap chay tu 0 - fileSize
		{
			Read(&c, 1, openFileIdSrc); // Goi ham Read de doc tung ki tu noi dung file
            Write(&c, 1, openFileIdDes);
			// PrintChar(c); // Goi ham PrintChar de in tung ki tu ra man hinh
		}
		Close(openFileIdSrc); // Goi ham Close de dong file
        Close(openFileIdDes);
    }
	else
	{
		PrintString(" -> Mo file khong thanh cong!!\n\n");
	}
	Halt();
}