# Lab 5 OS

This lab consists of 2 programs.

- **sem_main.c**: Concurrency implementation using semaphores.
- **mon_main.c**: Concurrency implementation using a monitor.

You can compile both using the following command:

```bash
# Compile and run sem_main.c
clang -Wall -o sem_exe sem_main.c && ./sem_exe
# Compile and run mon_main.c
clang -Wall -o mon_exe mon_main.c && ./mon_exe
```

If you want to save the output of a program to a file for further inspection,
simply redirect the stderr! For example for `sem_exe`:

```bash
# Redirect the output to a file on Unix systems
./sem_exe %> sem_out
# For windows checkout this stackoverflow post:
# https://stackoverflow.com/questions/1420965/how-can-i-redirect-windows-cmd-standard-output-and-standard-error-to-a-single-fi
# But this should work:
./sem_exe.exe > sem_out.txt 2>&1
```
