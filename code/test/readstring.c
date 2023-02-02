#include "syscall.h"

int main(){
    char* a;

    ReadString(a, 255);
    
    PrintString(a);
    
    Halt();
}