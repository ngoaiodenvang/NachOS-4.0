#include "syscall.h"

int main() {
    int num = RandomNum();
    PrintInt(num);
    PrintChar('\n');
    Halt();
}