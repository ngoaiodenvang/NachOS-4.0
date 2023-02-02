#include "syscall.h"

int main(){
    
    
    int i = 33;
    char ch;
    while (i < 127 ){
        PrintInt(i);
        PrintChar('\t');
        PrintChar((char)i);
        PrintChar('\n');
        i = i + 1;
        
    }

    
    Halt();
}