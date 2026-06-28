# trvm-v1
Over-simplified Turing complete virtual machine written in C.
## Compile the VM and run
```
gcc vm.c -o vm
```
Probably won't compile on windows.
Assemble the hello world program
```
python asmtrvm.py helloWorld.trs helloWorld.trvm
```
Run the hello world program
```
./vm helloWorld.trvm
```
