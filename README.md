# Line Detection With MPI

This simple MPI program written in C was a homework given in CMPE300 - Analysis of Algorithms in Bogazici University Computer Engineering department.

It is heavily commented and one who wants to learn to develop with MPI can get help from this code. 

First you need to install MPI, here is a tutorial : https://www.cmpe.boun.edu.tr/sites/default/files/mpi_install_tutorial.pdf

Program gets a .txt file which includes RGB coded values of a squared size image (like nxn) and after smoothing and thresholding with a supplied value by user, gives an output of n-4 x n-4 binarized image with lines detected (black and white) in the same directory as output.txt.

How to compile : mpicc -g main.c main.o

How to run : mpiexec -n <Number of Processors> main.o <input file> <threshold value[0-255]> <size of squared image (ex. 200)>

How to visualize output (python3 and PIL library should be installed first) : python3 visualize.py

# Example

Initial image

![alt tag](https://raw.githubusercontent.com/mbugc/line-detection-with-mpi/master/images/initial.png)

Line Detection With Threshold 10

![alt tag](https://raw.githubusercontent.com/mbugc/line-detection-with-mpi/master/images/10.jpeg)

Line Detection With Threshold 25

![alt tag](https://raw.githubusercontent.com/mbugc/line-detection-with-mpi/master/images/25.jpeg)

Line Detection With Threshold 40

![alt tag](https://raw.githubusercontent.com/mbugc/line-detection-with-mpi/master/images/40.jpeg)
