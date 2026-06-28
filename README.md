# trvm-v1
Over-simplified Turing complete virtual machine written in C.
## Compile the VM and run
```
gcc vm.c -o vm.out # Probably won't compile on windows.
```
Assemble the hello world program
```
python asmtrvm.py helloWorld.trs helloWorld.trvm
```
Run the hello world program
```
./vm.out helloWorld.trvm
```
## Details
**17 general purpose registers**: r1, r3, r4 etc. Yes you can create more by changing the program easily.

**Extra registers**: instruction pointer, stack pointer and a flags register.

**28 instructions**
debug: shows a stack dump upto 10 indices and register state

halt: quits the vm

jmp: Jumps to a line in program starting from 0, use it as `jmp.pad.<value here>` because that is how the assembler works

movbr: Move bytes into a register

movrr: Copy register value into another register

push, pop: Stack.

shr, shl, and, or, xor, add, sub, mul, div, inc, dec, not: Arithmetic and bitwise operators.

load, store: Load and store...on the stack. `load.<register to load into>.<index register, starting from 0>`

cmpbr, cmprr: Compare bytes to register and compare register to register. Sets the flags register.

jl, jg: Jump if less and Jump if greater. Same as the jump instruction above. checks the flags register.

hostcall: Call linux syscalls. 

int: Call built in functions for printing a register, char input, string input etc.

## Program showcase
### hello world

**NOTE: The assembler does not support comments! Just for demonstration! Get the clean files from the repo.**
```
movbr.r2.6581362 // dlr
movbr.r1.8031924123371070824 // ow olleh
movbr.r3.1 // index 1 in memory
store.r1.r3 // store ow olleh on index 1
dec.r3 // decrement to 0
store.r2.r3 // store dlr on index 0
movbr.r1.5 // print string 
movbr.r2.1 // index 1 (index in memory)
movbr.r3.11 // 11 bytes
int // call the print string (uses write() syscall!)
movbr.r1.0 // setup print character
movbr.r2.10 // newline
int // print the character
```
Output
```
➜  trvm-v1 git:(main) ./asmtrvm.py helloWorld.trs helloWorld.trvm
➜  trvm-v1 git:(main) ./vm.out helloWorld.trvm
STARTING TRVM
hello world
HALTING TRVM
➜  trvm-v1 git:(main)
```
Yes you need to type in base 10 no base 16 support.

Strings are little endian while everything else is in big endian (It's a feature not a bug! XD)

### printing alphabets using loop
```
movbr.r2.65 // 65 = A (ASCII) // line 0
movbr.r1.0 // line 1
int // print char
inc.r2 // increment r2
cmpbr.r2.91 // 90 = Z; compare r2 register with 91
jl.pad.1 // jump if less to line 1
movbr.r2.10 // 10 = newline (ASCII)
movbr.r1.0
int // print newline character
```
Output
```
➜  trvm-v1 git:(main) ✗ ./asmtrvm.py abcd.trs abcd.trvm
➜  trvm-v1 git:(main) ✗ ./vm.out abcd.trvm
STARTING TRVM
ABCDEFGHIJKLMNOPQRSTUVWXYZ
HALTING TRVM
➜  trvm-v1 git:(main) ✗
```