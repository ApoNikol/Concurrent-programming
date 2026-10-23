## Table of Contents

- [Description](#description)
- [Compilation](#Compilation)
- [Execution](#Execution)
- [Runtime_details](#Runtime_details)
- [Termination_details](#Termination_details)

# Description

Master-slave prime nums checker program

# Compilation

gcc -Wall -g ask2.c -o ask2 -lpthread

# Execution

if you want to manually enter numbers from the stdin:
./ask2 N
N = The number of workers(threads)

else if you want it to read the numbers from a .bin file:
./ask2 N "file_name.bin"

# Runtime_details

Give as input each time a number and press enter

# Termination_details

If you want to terminate the program for :
Windows: Cntrl+Z and then enter
Linux/Mac: Cntrl+D
or to terminate it without waiting: Cntrl+C
