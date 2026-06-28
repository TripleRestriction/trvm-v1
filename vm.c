#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

// memory in the multiple of 8bytes; 512 * 8 = 4096bytes or 4KiB
#define MEMORY_SIZE 512
#define ip 0           // instruction pointer reg
#define flags 18       // flags is a register
#define validRegMax 17 // will use 17 as the remainder register
#define validRegMin 1
#define stackPointer 19

typedef struct stack {
    int size;
    uint64_t *top;
    uint64_t *bottom;
    uint64_t *arr;
} stack;

stack *ourStack;

uint64_t *init()
{
    uint64_t *ptr = malloc(sizeof(uint64_t) * MEMORY_SIZE);
    ourStack = malloc(sizeof(stack));
    ourStack->size = MEMORY_SIZE / 2;
    ourStack->arr = ptr;
    ourStack->top = ptr + MEMORY_SIZE;
    ourStack->bottom = ptr + MEMORY_SIZE - 1;
    ptr[stackPointer] =
        (uint64_t)(uintptr_t)
            ourStack->top; // some type casting to store ts in the memory
    return ptr;
}

// debug function
void test123(uint64_t *mem, uint64_t index)
{
    for (size_t i = 1; i <= validRegMax; i++) {
        printf("DEBUG:value of reg%lu is: 0x%016" PRIX64 "\n", i, mem[i]);
    }
    printf("DEBUG:value of stack pointer is: 0x%016" PRIX64 "\n",
           mem[stackPointer]);
    printf("DEBUG:STACK DUMP\n");
    for (size_t i = MEMORY_SIZE - 1; i >= MEMORY_SIZE - 10; i--) {
	printf("%lu ", i);
        printf("0x%016" PRIx64 "\n", mem[i]);
    }
}

bool validRegister(size_t reg)
{
    if (reg > validRegMax || reg < validRegMin)
        return false;
    return true;
}

// function for moving bytes into registers
// mov reg value
void movBR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    uint64_t value = 0;
    // some math that i dont understand
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    mem[reg] = value;
}

// push reg
void pushOnStack(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg) &&
        reg != stackPointer) { // allow pushing stack pointer
        printf("Invalid register");
        exit(1);
    }
    mem[stackPointer] = (uint64_t)(uintptr_t)ourStack->top;
    ourStack->top--;
    (*ourStack->top) = mem[reg];
}

// pop reg
void popOffStack(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    uint64_t value = *(ourStack->top);
    ourStack->top++;
    mem[stackPointer] = (uint64_t)(uintptr_t)ourStack->top;
    mem[reg] = value;
}

// compare register and bytes
void cmpBR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    if (value == mem[reg]) {
        mem[flags] |= 1; // set lowest bit to 1
    } else if (mem[reg] < value) {
        mem[flags] |= (1 << 1); // set 2nd lowest bit to 1
    } else if (mem[reg] > value) {
        mem[flags] |= (1 << 2); // set third lowest bit to 1
    }
}

void cmpRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    if (mem[reg1] == mem[reg2]) {
        mem[flags] |= 1; // set lowest bit to 1
    } else if (mem[reg1] < mem[reg2]) {
        mem[flags] |= (1 << 1); // set 2nd lowest bit to 1
    } else if (mem[reg1] > mem[reg2]) {
        mem[flags] |= (1 << 2); // set third lowest bit to 1
    }
}

//[1 byte jump][1byte garbage][8 byte jump amount]
void jmp(char *prg, uint64_t ins, uint64_t *mem)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    mem[ip] = 10 * value;
}

// jump if equal flag is set
int je(char *prg, uint64_t ins, uint64_t *mem)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    if (mem[flags] & 1) {
        mem[flags] = 0;
        mem[ip] = 10 * value;
        return 1;
    } else {
        return 0;
    }
}

int jl(char *prg, uint64_t ins, uint64_t *mem)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    if (mem[flags] & (1 << 1)) {
        mem[flags] = 0; // reset the lowest bit
        mem[ip] = 10 * value;
        return 1;
    } else {
        return 0;
    }
}

// jump if less than flag is set
int jg(char *prg, uint64_t ins, uint64_t *mem)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    if (mem[flags] & (1 << 2)) {
        mem[flags] = 0; // reset the lowest bit
        mem[ip] = 10 * value;
        return 1;
    } else {
        return 0;
    }
}

// shift right
void shrBR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    value &= 63; // bit masking, because shifting by 64 bits is UB; same as
                 // value = value % 64
    mem[reg] = mem[reg] >> value;
}

// shift left
void shlBR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)(unsigned char)prg[ins + 2 + i]) << ((7 - i) * 8);
    }
    value &= 63; // bit masking
    mem[reg] = mem[reg] << value;
}

// function for moving registers into registers
// mov reg1 reg2 // copies value of reg2 to reg1
void movRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg1] = mem[reg2];
}

// load dest scaler
void load(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t dest = prg[ins + 1];
    size_t scaler = prg[ins + 2];
    if (!validRegister(scaler) && !validRegister(dest)) {
        printf("Invalid register");
        exit(1);
    }
    if (scaler > MEMORY_SIZE/2) {
        printf("Invalid scaler");
	exit(1);
    }
    uint64_t value = *((ourStack->bottom) - mem[scaler]);
    mem[dest] = value;
}

// store source scaler
void store(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t source = prg[ins + 1];
    size_t scaler = prg[ins + 2];
    if (!validRegister(scaler) && !validRegister(source)) {
        printf("Invalid register");
        exit(1);
    }
    if (scaler > MEMORY_SIZE/2) {
        printf("Invalid scaler");
	exit(1);
    }
    *((ourStack->bottom) - mem[scaler]) = mem[source];
}

// reg1 = reg1 & reg2
void andRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg1] = mem[reg2] & mem[reg1];
}

// reg1 = reg1 | reg2
void orRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg1] = mem[reg2] | mem[reg1];
}

// reg1 = reg1 ^ reg2
void xorRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg1] = mem[reg2] ^ mem[reg1];
}

// reg1 = reg1 + reg2
void addRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg1] = mem[reg2] + mem[reg1];
}

// reg1 = reg1 - reg2
void subRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg1] = mem[reg1] - mem[reg2];
}

// reg1 = reg2 * reg1
void mulRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg1] = mem[reg2] * mem[reg1];
}

// reg1 = reg1 / reg2
// store remainder in reg17
void divRR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg1 = prg[ins + 1];
    size_t reg2 = prg[ins + 2];
    if (!validRegister(reg1) && !validRegister(reg2)) {
        printf("Invalid register");
        exit(1);
    }
    if (mem[reg2] == 0) {
        mem[reg2] = 1; // its my vm and i will define what divison by 0 is!
    }
    mem[validRegMax] = mem[reg1] % mem[reg2];
    mem[reg1] = mem[reg1] / mem[reg2];
}

// increment register
void incR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg]++;
}

// decrement register
void decR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg]--;
}

// BitwiseNot a register
void notR(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t reg = prg[ins + 1];
    if (!validRegister(reg)) {
        printf("Invalid register");
        exit(1);
    }
    mem[reg] = ~mem[reg];
}

// just call linux syscalls because me is lazy
void hostCall(uint64_t *mem)
{
    size_t syscallNumber = mem[1]; // rax
    size_t arg0 = mem[2];          // rdi
    size_t arg1 = mem[3];          // rsi
    size_t arg2 = mem[4];          // rdx
    size_t arg3 = mem[5];          // r10
    size_t arg4 = mem[6];          // r9
    size_t arg5 = mem[7];          // r8
    mem[1] = syscall(syscallNumber, arg0, arg1, arg2, arg3, arg4, arg5);
    // store return value in r0
}

// adding actual functions cuz dealing with syscalls is pita
void interrupt(char *prg, uint64_t ins, uint64_t *mem)
{
    size_t intNumber = mem[1]; // rax
    size_t arg0 = mem[2];      // rdi
    size_t arg1 = mem[3];      // rsi
    size_t arg2 = mem[4];      // rdx
    size_t arg3 = mem[5];      // r10
    size_t arg4 = mem[6];      // r9
    size_t arg5 = mem[7];      // r8
    if (intNumber == 0x0) {    // print charater command
        printf("%c", (char)arg0);
    } else if (intNumber == 0x1) { // print register command
        if (!validRegister(arg0)) {
            printf("Invalid register to print!\n");
            exit(1);
        }
        printf("%" PRIX64 "\n", mem[arg0]);
    } else if (intNumber == 0x2) { // number input in register
        if (!validRegister(arg0)) {
            printf("Invalid register to input!\n");
            exit(1);
        }
        scanf("%" PRIu64 "", &mem[arg0]);
    } else if (intNumber == 0x3) { // input char in register
        if (!validRegister(arg0)) {
            printf("Invalid register to input!\n");
            exit(1);
        }
        scanf("%c", (char *)&mem[arg0]);
    } else if (intNumber == 0x4) { // input string
        if (arg0 > MEMORY_SIZE / 2) {
            printf("Invalid index");
	    exit(1);
        }
        if (arg1 > MEMORY_SIZE / 2) {
	    printf("Invalid size");
        }
	read(1, (ourStack->bottom - arg0), arg1);
    } else if (intNumber == 0x5) { // write
        if (arg0 > MEMORY_SIZE/2) {
            printf("Invalid index");
	    exit(1);
        }
        if (arg1 > MEMORY_SIZE / 2) {
	    printf("Invalid size");
        }
	write(1, (ourStack->bottom - arg0), arg1);
    }

}

void runner(char *prg, size_t size)
{
    uint64_t *mem = init();
    int i = 1;
    while (true) {
        if (mem[ip] >= size) {
            break;
        }
        switch ((unsigned char)prg[mem[ip]]) {
            case 0x67:
                test123(mem, mem[ip]);
                break;
            case 0x0:
                free(mem);
                return;
            case 0x63:
                jmp(prg, mem[ip], mem);
                continue; // jump back to the start of the while loop
            case 0x41:
                movBR(prg, mem[ip], mem);
                break;
            case 0xff:
                pushOnStack(prg, mem[ip], mem);
                break;
            case 0xfe:
                popOffStack(prg, mem[ip], mem);
                break;
            case 0xaa:
                shrBR(prg, mem[ip], mem);
                break;
            case 0xbb:
                shlBR(prg, mem[ip], mem);
                break;
            case 0x42:
                movRR(prg, mem[ip], mem);
                break;
            case 0x59:
                load(prg, mem[ip], mem);
                break;
            case 0x60:
                store(prg, mem[ip], mem);
                break;
            case 0x5:
                cmpBR(prg, mem[ip], mem);
                break;
            case 0x6:
                if (je(prg, mem[ip], mem)) {
                    continue;
                } else {
                    break;
                }
            case 0x9:
                if (jl(prg, mem[ip], mem)) {
                    continue;
                } else {
                    break;
                }
            case 0xde:
                if (jg(prg, mem[ip], mem)) {
                    continue;
                } else {
                    break;
                }
            case 0x7:
                cmpRR(prg, mem[ip], mem);
                break;
            case 0x01:
                andRR(prg, mem[ip], mem);
                break;
            case 0x02:
                orRR(prg, mem[ip], mem);
                break;
            case 0x03:
                xorRR(prg, mem[ip], mem);
                break;
            case 0xad:
                addRR(prg, mem[ip], mem);
                break;
            case 0xba:
                subRR(prg, mem[ip], mem);
                break;
            case 0xac:
                mulRR(prg, mem[ip], mem);
                break;
            case 0xdd:
                divRR(prg, mem[ip], mem);
                break;
            case 0x43:
                incR(prg, mem[ip], mem);
                break;
            case 0x44:
                decR(prg, mem[ip], mem);
                break;
            case 0x45:
                notR(prg, mem[ip], mem);
                break;
            case 0x69:
                hostCall(mem);
                break;
            case 0x70:
                interrupt(prg, mem[ip], mem);
                break;
        }
        mem[ip] += 10;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: trvm <program file>\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", argv[1]);
        return 2;
    }

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);

    char *prg = malloc(fileSize);
    if (prg == NULL) {
        printf("Memory allocation for file failed.\n");
        fclose(file);
        return 3;
    }

    size_t bytesRead = fread(prg, sizeof(char), fileSize, file);
    if (bytesRead % 10 != 0) {
        printf("invalid instruction length");
        return 4;
    }
    printf("STARTING TRVM\n");
    runner(prg, fileSize); // call the runner
    free(prg);
    printf("HALTING TRVM\n");
    return 0;
}
