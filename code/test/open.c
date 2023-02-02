#include"syscall.h"

int main(){
    Open("text.txt", 0);
    Close(3);
    Open("text.txt", 1);
    Remove("text2.txt");
    Halt();
}