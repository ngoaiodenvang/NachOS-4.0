// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "machine.h"
#include "synchconsole.h"

void handleHelp(const char *buffer);

#define MaxFileLength 32
//--------------------------------------------------------------------
// Input: - User space address (int)
// - Limit of buffer (int)
// Output:- Buffer (char*)
// Purpose: Copy buffer from User memory space to System memory space
char *User2System(int virtAddr, int limit)
{
	int i; // index
	int oneChar;
	char *kernelBuf = NULL;
	kernelBuf = new char[limit + 1]; // need for terminal string
	if (kernelBuf == NULL)
		return kernelBuf;
	memset(kernelBuf, 0, limit + 1);
	// printf("\n Filename u2s:");
	for (i = 0; i < limit; i++)
	{
		kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
		kernelBuf[i] = (char)oneChar;
		// printf("%c",kernelBuf[i]);
		if (oneChar == 0)
			break;
	}
	return kernelBuf;
}

//----------------------------------------------------------
// Input: - User space address (int)
// - Limit of buffer (int)
// - Buffer (char[])
// Output:- Number of bytes copied (int)
// Purpose: Copy buffer from System memory space to User memory space
int System2User(int virtAddr, int len, char *buffer)
{
	if (len < 0)
		return -1;
	if (len == 0)
		return len;
	int i = 0;
	int oneChar = 0;
	do
	{
		oneChar = (int)buffer[i];
		kernel->machine->WriteMem(virtAddr + i, 1, oneChar);
		i++;
	} while (i < len && oneChar != 0);
	return i;
}

void IncreasePC()
{
	// set previous program counter to current programcounter
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

	/* set next programm counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which)
	{
	case SyscallException:
		DEBUG('a', "\n SC_Create call ...");
		switch (type)
		{
		case SC_Halt:
		{

			SysHalt();

			ASSERTNOTREACHED();
			break;
		}

		case SC_Add:
		{
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;
		}
		case SC_Create:
		{
			int virtAddr;
			char *filename;

			virtAddr = kernel->machine->ReadRegister(4);
			filename = User2System(virtAddr, MaxFileLength + 1);

			if (strlen(filename) == 0)
			{
				printf("\nFile name is not valid");
				kernel->machine->WriteRegister(2, -1);
				delete[] filename;
				break;
			}

			if (filename == NULL)
			{
				printf("\n Not enough memory in system");
				kernel->machine->WriteRegister(2, -1);
				delete[] filename;
				break;
			}

			if (!kernel->fileSystem->Create(filename))
			{
				printf("\nError create file '%s'", filename);
				kernel->machine->WriteRegister(2, -1);
				delete[] filename;
				break;
			}

			printf("\nCreate file '%s' success", filename);
			kernel->machine->WriteRegister(2, 0);
			delete[] filename;
			IncreasePC();
			return;
		}
		case SC_Read:
		{
			int virtAddr = kernel->machine->ReadRegister(4);  // Lay dia chi cua tham so buffer tu thanh ghi so 4
			int charcount = kernel->machine->ReadRegister(5); // Lay charcount tu thanh ghi so 5
			int id = kernel->machine->ReadRegister(6);        // Lay id cua file tu thanh ghi so 6
			int OldPos;
			int NewPos;
			char *buf;
			// Kiem tra id cua file truyen vao co nam ngoai bang mo ta file khong
			if (id < 0 || id > 14)
			{
				printf("\nKhong the read vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}
			// Kiem tra file co ton tai khong
			if (kernel->fileSystem->openf[id] == NULL)
			{
				printf("\nKhong the read vi file nay khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}
			if (kernel->fileSystem->openf[id]->type == 3) // Xet truong hop doc file stdout (type quy uoc la 3) thi tra ve -1
			{
				printf("\nKhong the read file stdout.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}
			OldPos = kernel->fileSystem->openf[id]->GetCurrentPos(); // Kiem tra thanh cong thi lay vi tri OldPos
			buf = User2System(virtAddr, charcount);                  // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai charcount
			// Xet truong hop doc file stdin (type quy uoc la 2)
			if (kernel->fileSystem->openf[id]->type == 2)
			{
				// Su dung ham Read cua lop SynchConsole de tra ve so byte thuc su doc duoc
				int size = 0;
				for (int i = 0; i < charcount; ++i)
				{
				size = size + 1;
				buf[i] = kernel->synchConsoleIn->GetChar();
				//Quy uoc chuoi ket thuc la \n
				if (buf[i] == '\n')
				{
					buf[i + 1] = '\0';
					break;
				}
				}
				buf[size] = '\0';
				System2User(virtAddr, size, buf);        // Copy chuoi tu vung nho System Space sang User Space voi bo dem buffer co do dai la so byte thuc su
				kernel->machine->WriteRegister(2, size); // Tra ve so byte thuc su doc duoc
				delete buf;
				IncreasePC();
				return;
			}
			// Xet truong hop doc file binh thuong thi tra ve so byte thuc su
			if ((kernel->fileSystem->openf[id]->Read(buf, charcount)) > 0)
			{
				// So byte thuc su = NewPos - OldPos
				NewPos = kernel->fileSystem->openf[id]->GetCurrentPos();
				// Copy chuoi tu vung nho System Space sang User Space voi bo dem buffer co do dai la so byte thuc su
				System2User(virtAddr, NewPos - OldPos, buf);
				kernel->machine->WriteRegister(2, NewPos - OldPos);
			}
			else
			{
				// Truong hop con lai la doc file co noi dung la NULL tra ve -2
				//printf("\nDoc file rong.");
				kernel->machine->WriteRegister(2, -2);
			}
			delete buf;
			IncreasePC();
			return;
		}
		case SC_Write:
		{
			int virtAddr = kernel->machine->ReadRegister(4);
			int charcount = kernel->machine->ReadRegister(5);
			int openf_id = kernel->machine->ReadRegister(6);
			int i = kernel->fileSystem->index;

			if (openf_id > i || openf_id < 0 || openf_id == 0) // `out of domain` filesys + try to write to stdin
			{
				printf("\nKhong the write vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			if (kernel->fileSystem->openf[openf_id] == NULL)
			{
				printf("\nKhong the write vi file nay khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			// read-only file
			if (kernel->fileSystem->openf[openf_id]->type == 1)
			{
				printf("\nKhong the write file stdin hoac file only read.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			// write to console
			char *buf = User2System(virtAddr, charcount);
			if (openf_id == 1)
			{
				int i = 0;
				while (buf[i] != '\0' && buf[i] != '\n')
				{
					kernel->synchConsoleOut->PutChar(buf[i]);
					i++;
				}
				buf[i] = '\n';
				kernel->synchConsoleOut->PutChar(buf[i]); // write last character

				kernel->machine->WriteRegister(2, i - 1);
				delete[] buf;
				IncreasePC();
				return;
			}

			// write into file
			int before = kernel->fileSystem->openf[openf_id]->GetCurrentPos();
			if ((kernel->fileSystem->openf[openf_id]->Write(buf, charcount)) != 0)
			{
				int after = kernel->fileSystem->openf[openf_id]->GetCurrentPos();
				System2User(virtAddr, after - before, buf);
				kernel->machine->WriteRegister(2, after - before + 1);
				delete[] buf;
				IncreasePC();
				return;
			}

			IncreasePC();
			return;
		}
		case SC_ReadChar:
		{
			char buffer;

			DEBUG(dbgSys, "\n SC_ReadChar call ...");

			// read char from keyboard
			buffer = kernel->synchConsoleIn->GetChar();

			// write value to register 2
			kernel->machine->WriteRegister(2, buffer);

			IncreasePC();
			return;
			ASSERTNOTREACHED();
			break;
		}

		case SC_PrintString:
		{
			DEBUG(dbgSys, "\n SC_PrintString call ...");
			int virtAddr;
			char *buffer;
			virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so buffer tu thanh ghi so 4
			buffer = User2System(virtAddr, 255);		 // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai 255 ki tu
			int length = 0;
			while (buffer[length] != 0)
				length++; // Dem do dai that cua chuoi
			for (int i = 0; i < length; i++)
			{
				kernel->synchConsoleOut->PutChar(buffer[i]);
			}
			delete buffer;
			DEBUG(dbgSys, "\n SC_PrintString call ...");
			IncreasePC();
			return;
		}

		case SC_PrintChar:
		{
			int ch;
			ch = kernel->machine->ReadRegister(4);
			kernel->synchConsoleOut->PutChar((char)ch);
			IncreasePC();
			return;
		}
		case SC_ReadInt:
		case SC_ReadNum:
		{
			int result = SysReadNum(); // <-- ksyscall.h
			kernel->machine->WriteRegister(2, result);
			IncreasePC();
			return;
		}

		case SC_PrintInt:
		case SC_PrintNum:
		{
			// Input: mot so integer
			// Output: khong co
			// Chuc nang: In so nguyen len man hinh console
			int number = kernel->machine->ReadRegister(4);
			if (number == 0)
			{
				kernel->synchConsoleOut->PutChar('0'); // In ra man hinh so 0
				IncreasePC();
				return;
			}

			/*Qua trinh chuyen so thanh chuoi de in ra man hinh*/
			bool isNegative = false; // gia su la so duong
			int numberOfNum = 0;	 // Bien de luu so chu so cua number
			int firstNumIndex = 0;

			if (number < 0)
			{
				isNegative = true;
				number = number * -1; // Nham chuyen so am thanh so duong de tinh so chu so
				firstNumIndex = 1;
			}

			int t_number = number; // bien tam cho number
			while (t_number)
			{
				numberOfNum++;
				t_number /= 10;
			}

			// Tao buffer chuoi de in ra man hinh
			char *buffer;
			int MAX_BUFFER = 255;
			buffer = new char[MAX_BUFFER + 1];
			for (int i = firstNumIndex + numberOfNum - 1; i >= firstNumIndex; i--)
			{
				buffer[i] = (char)((number % 10) + 48);
				number /= 10;
			}
			if (isNegative)
			{
				buffer[0] = '-';
				buffer[numberOfNum + 1] = 0;
				for (int i = 0; i < numberOfNum + 1; i++)
				{
					kernel->synchConsoleOut->PutChar(buffer[i]);
				}
				delete buffer;
				IncreasePC();
				return;
			}
			buffer[numberOfNum] = 0;
			for (int i = 0; i < numberOfNum; i++)
			{
				kernel->synchConsoleOut->PutChar(buffer[i]);
			}
			delete buffer;
			IncreasePC();
			return;
		}
		case SC_ReadString:
		{
			DEBUG(dbgSys, "\n SC_ReadString call ...");
			int addr, lenght;
			char *buffer;

			// read parameter from register 4 and 5
			addr = kernel->machine->ReadRegister(4);
			lenght = kernel->machine->ReadRegister(5);

			char ch;

			// copy string from user space to kernel space
			buffer = User2System(addr, lenght);
			int i = 0;

			// read string from keyboard
			while (true)
			{
				ch = kernel->synchConsoleIn->GetChar();
				if (ch == '\n')
					break;
				buffer[i] = ch;
				i++;
			}
			buffer += '\0';

			// copy string from kernel space to user space
			System2User(addr, lenght, buffer);

			IncreasePC();
			return;
			break;
		}
		case SC_RandomNum:
		{
			kernel->machine->WriteRegister(2, SysRandomNumber());
			IncreasePC();
			return;
			break;
		}
		case SC_Help:
		{
			handleHelp("Do an He dieu hanh\n");
			handleHelp("Cac thanh vien:\n");
			handleHelp("20120011 - Nguyen Hoang Huy\n");
			handleHelp("20120017 - Phan Quoc Ky\n");
			handleHelp("20120345 - Luu Nguyen Tien Anh\n");
			handleHelp("---\n");
			handleHelp("Gioi thieu chuong trinh sort:\n");
			handleHelp("\tChuong trinh sort giup sort mang n phan tu voi thuat toan bubble sort\n");
			handleHelp("Gioi thieu chuong trinh ascii:\n");
			handleHelp("\tChuong trinh ascii in ra cac ky tu ascii doc duoc\n");
			IncreasePC();
			return;
			break;
		}

		case SC_Seek:
		{
			// Input: Vi tri(int), id cua file(OpenFileID)
			// Output: -1: Loi, Vi tri thuc su: Thanh cong
			// Cong dung: Di chuyen con tro den vi tri thich hop trong file voi tham so la vi tri can chuyen va id cua file
			int pos = kernel->machine->ReadRegister(4); // Lay vi tri can chuyen con tro den trong file
			int id = kernel->machine->ReadRegister(5);	// Lay id cua file
			// Kiem tra id cua file truyen vao co nam ngoai bang mo ta file khong
			if (id < 0 || id > 14)
			{
				printf("\nKhong the seek vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}
			// Kiem tra file co ton tai khong
			if (kernel->fileSystem->openf[id] == NULL)
			{
				printf("\nKhong the seek vi file nay khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}
			// Kiem tra co goi Seek tren console khong
			if (id == 0 || id == 1)
			{
				printf("\nKhong the seek tren file console.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}
			// Neu pos = -1 thi gan pos = Length nguoc lai thi giu nguyen pos
			pos = (pos == -1) ? kernel->fileSystem->openf[id]->Length() : pos;
			if (pos > kernel->fileSystem->openf[id]->Length() || pos < 0) // Kiem tra lai vi tri pos co hop le khong
			{
				printf("\nKhong the seek file den vi tri nay.");
				kernel->machine->WriteRegister(2, -1);
			}
			else
			{
				// Neu hop le thi tra ve vi tri di chuyen thuc su trong file
				kernel->fileSystem->openf[id]->Seek(pos);
				kernel->machine->WriteRegister(2, pos);
			}
			IncreasePC();
			return;
			break;
		}

		case SC_Open:
		{
			DEBUG(dbgSys, "\n SC_Open calling ...");
			int virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so name tu thanh ghi so 4
			int type = kernel->machine->ReadRegister(5);	 // Lay tham so type tu thanh ghi so 5
			char *filename;
			filename = User2System(virtAddr, MaxFileLength); // Copy chuoi tu vung nho User Space sang System Space voi bo dem name dai MaxFileLength

			// index la so file dang duoc mo
			int index = kernel->fileSystem->index;
			// printf("%d \n", index);

			// Kiem tra xem OS con mo dc file khong
			if (index >= 15)
			{
				DEBUG(dbgSys, "\n Do not enough slot for open file ...");
				kernel->machine->WriteRegister(2, -1);
			}
			else
			{
				if (type == 0 || type == 1) // chi xu li khi type = 0 hoac 1
				{

					int OpenFileID = -1;
					for (int i = 2; i++; i < 15)
					{
						if (kernel->fileSystem->openf[i] == NULL)
						{
							OpenFileID = i;
							break;
						}
					}
					// printf("%d \n", OpenFileID);
					if (OpenFileID == -1)
					{
						DEBUG(dbgSys, "\n Can not open file ...");
						kernel->machine->WriteRegister(2, -1);
					}
					else
					{
						kernel->fileSystem->openf[OpenFileID] = kernel->fileSystem->Open(filename, type);

						// deep copy filename vao table descriptor
						kernel->fileSystem->tableDescriptor[OpenFileID] = new char[strlen(filename) + 1];
						for (int i = 0; i < strlen(filename); i++)
						{
							kernel->fileSystem->tableDescriptor[OpenFileID][i] = filename[i];
						}
						kernel->fileSystem->tableDescriptor[OpenFileID][strlen(filename)] = '\0';

						kernel->fileSystem->index += 1;
						DEBUG(dbgSys, "\n Open file Success ...");
						kernel->machine->WriteRegister(2, OpenFileID);
					}
				}
				else // xu li khi type khong hop le
				{
					DEBUG(dbgSys, "\n Error Type ...");
					kernel->machine->WriteRegister(2, -1);
				}
			}
			// printf("%d %s \n", 3, kernel->fileSystem->tableDescriptor[3]);
			// Khong mo duoc file return -1

			delete[] filename;
			IncreasePC();
			return;
			break;
		}

		case SC_Close:
		{
			// Input id cua file(OpenFileID)
			//  Output: 0: thanh cong, -1 that bai
			int fid = kernel->machine->ReadRegister(4); // Lay id cua file tu thanh ghi so 4
			if (fid >= 0 && fid <= 14)					// Chi xu li khi fid nam trong [0, 14]
			{
				if (kernel->fileSystem->openf[fid]) // neu dang mo file
				{
					delete kernel->fileSystem->openf[fid]; // Xoa vung nho luu tru file
					kernel->fileSystem->openf[fid] = NULL; // Gan vung nho NULL
					kernel->fileSystem->index -= 1;
					kernel->machine->WriteRegister(2, 0);

					// xoa trong bang table Descriptor
					if (kernel->fileSystem->tableDescriptor[fid] != NULL)
					{
						delete kernel->fileSystem->tableDescriptor[fid];
					}
					kernel->fileSystem->tableDescriptor[fid] = NULL;

					DEBUG(dbgSys, "\n Close file Success ...");
					IncreasePC();
					return;
					break;
				}
				else
				{
					DEBUG(dbgSys, "\n Can not close file ...");
					kernel->machine->WriteRegister(2, -1);
				}
			}
			else
			{
				DEBUG(dbgSys, "\n Can not close file ...");
				kernel->machine->WriteRegister(2, -1);
			}
			IncreasePC();
			return;
			break;
		}

		case SC_Remove:
		{
			DEBUG(dbgSys, "\n SC_Remove calling ...");
			int virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so name tu thanh ghi so 4
			char *filename;
			filename = User2System(virtAddr, MaxFileLength); // Copy chuoi tu vung nho User Space sang System Space voi bo dem name dai MaxFileLength

			for (int i = 3; i < 20; i++)
			{

				if (kernel->fileSystem->tableDescriptor[i] != NULL && strcmp(filename, kernel->fileSystem->tableDescriptor[i]) == 0)
				{
					DEBUG(dbgSys, "\n cannot remove file (file is openning) ...");
					kernel->machine->WriteRegister(2, -1);
					IncreasePC();
					return;
				}
			}

			if (kernel->fileSystem->Remove(filename))
			{
				DEBUG(dbgSys, "\n Remove file success...");
				kernel->machine->WriteRegister(2, 0);
				;
			}
			else
			{
				DEBUG(dbgSys, "\n Can not found file ...");
				kernel->machine->WriteRegister(2, -1);
			}
			IncreasePC();
			return;
			break;
		}

		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;

	case NoException:
		kernel->interrupt->setStatus(SystemMode);
		DEBUG(dbgSys, "Switched to system mode\n");
		break;

	case PageFaultException:
		DEBUG(dbgSys, "Shutdown, No valid translation found.");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case ReadOnlyException:
		DEBUG(dbgSys, "Shutdown, Write attempted to page marked 'read-only'.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case BusErrorException:
		DEBUG(dbgSys, "Shutdown, Translation resulted in an invalid physical address.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case AddressErrorException:
		DEBUG(dbgSys, "Shutdown, Unaligned reference or one that was beyond the end of the address space.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case OverflowException:
		DEBUG(dbgSys, "Shutdown, Integer overflow in add or sub.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case IllegalInstrException:
		DEBUG(dbgSys, "Shutdown, Unimplemented or reserved instr.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case NumExceptionTypes:
		DEBUG(dbgSys, "Shutdown, Other exceptions.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	default:
		DEBUG(dbgSys, "Unexpected user mode exception.\n");
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}

	ASSERTNOTREACHED();
}

void handleHelp(const char *buffer)
{
	int length = 0;
	while (buffer[length] != '\0')
		length++; // Dem do dai that cua chuoi
	for (int i = 0; i < length; i++)
	{
		kernel->synchConsoleOut->PutChar(buffer[i]);
	}
}
