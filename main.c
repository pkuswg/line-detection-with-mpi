/**
*	Author: Buğra Çil
*
*	Notes : Code is generic, you can try this with any square input, but you need to change sizeOfImage global variable in main function's start.
*			You can try with any size of processors that divides sizeOfImage (101 works), except sizeOfImage==size of processors.
*			I didn't have time to do benchmark tests.
**/

#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//	Allocates memory for 2D array and returns created array.
int** create2DArray(int sizeX, int sizeY)
{
    int* data = (int *) malloc(sizeX * sizeY * sizeof(int));
    int** array= (int **) malloc(sizeX * sizeof(int*));
    int i;

    for (i=0; i<sizeX; i++)
    {
        array[i] = &(data[sizeY * i]);
    }

    return array;
}


// Allocates memory for 1D array and returns created array.
int* createArray(int size)
{
    int* array = (int *) malloc(size * sizeof(int));

    return array;
}

//	Gets elements of 3x3 array, finds average of them as an integer (truncated) and returns it.
int meanFilter(int a, int b, int c, int k, int l, int m, int x, int y, int z)
{
	int avg;

	avg = ( a + b + c + k + l + m + x + y + z)/9;

	return avg;
}

//	Gets elements of 3x3 array, applies 4 filters, checks if results bigger than threshold, if so return 255, if not return 0.
int thresholding(int a, int b, int c, int k, int l, int m, int x, int y, int z, int threshold)
{
	int i;
	int *array = createArray(4);
    
    array[0] = -1*a -b -c + 2*k + 2*l + 2*m -x -y -z;
    array[1] = -1*a +2*b -c -k +2*l -m -x +2*y -z;
    array[2] = -1*a -b +2*c -k +2*l -m +2*x -y -z;
    array[3] = 2*a -b -c -k +2*l -m -x -y +2*z;

    for(i=0; i<4; i++){
    	if(array[i]>threshold) return 255;
    }
    return 0;
}

int main(int argc, char** argv) {

	int rank;	//	Rank of the current process. 
    int size;	//	The total number of the processes.
  	int i, j, k;	//	Loop variables

  	FILE *ifp, *ofp, *slavefp;
	char *inputFileName = argv[1];	// 	The input file name
	char *outputFileName = "output.txt";	//	The output file name
	int threshold = atoi(argv[2]);	//	The third argument, threshold value
	int sizeOfImage=atoi(argv[3]);	//	The fourth argument, size of squared image

	ifp = fopen(inputFileName, "r");	//	Read from input file.
	ofp = fopen(outputFileName, "w");	// 	Write to output file.
	slavefp = fopen(outputFileName, "a");	//	Append to output file (for testing).

  	MPI_Init ( &argc, &argv );	// Starts MPI
	MPI_Comm_rank ( MPI_COMM_WORLD, &rank ); 	//	Get current process rank.
	MPI_Comm_size ( MPI_COMM_WORLD, &size );	//	Get number of processes.
	int slaves = size-1;	//	Number of slave processes.

  	//	Each slave will have a 2D array which have rows associated with that slave process.
	int **slaveRows = create2DArray(sizeOfImage/slaves, sizeOfImage);
	//	New 2D array to collect smoothed results
	int **smoothed = create2DArray(sizeOfImage/slaves, sizeOfImage);
	//	Each slave will have an array, which is the first row from below slave.
	int *belowRow = createArray(sizeOfImage);
	//	Each slave will have an array, which is the last row from above slave.
	int *aboveRow = createArray(sizeOfImage);

	/*
	*	This part controls command line arguments, prints error if something is wrong.
	*/
	if (ifp == NULL) {
	  	fprintf(stderr, "Can't open input file %s!\n", inputFileName);
		return 0;	
	}

	if (ofp == NULL) {
	  	fprintf(stderr, "Can't open output file %s!\n", outputFileName);
	  	return 0;
	}

	if (sizeOfImage % slaves != 0){
		fprintf(stderr, "Number of slave processors doesn't divide size of image!");
		return 0;
	}

	if (sizeOfImage / slaves == 1){
		fprintf(stderr, "Number of slave processors can't be exactly same to size of image!");
		return 0;
	}

	//	----------------------- SMOOTHING PART -----------------------
 
	//	MASTER PART
	if(rank==0){
		//	Initial input 2D array, will be freed when chunks have been sent to slaves.
  		int **input = create2DArray(sizeOfImage, sizeOfImage);
  		int cellValue;

		//	Read textfile into 2D array
		for(i=0; i<sizeOfImage; i++){
			for(j=0; j<sizeOfImage; j++){
				if (fscanf(ifp, "%d", &cellValue) == 1) {
					input[i][j] = cellValue;
				}
			}
		}	

		//	Input data will be divided to equal chunks and be sent to slaves, afterwards free the input array.
		for(k=1; k<size; k++){
			//	New 2D array to scatter rows
			int **scatter = create2DArray(sizeOfImage/slaves, sizeOfImage);
			for(i=0; i<sizeOfImage/slaves; i++){
				for(j=0; j<sizeOfImage; j++){
					scatter[i][j] = input[i+((k-1)*(sizeOfImage/slaves))][j];
				}
			}
			//	Send associated rows to associated process
			MPI_Send(&scatter[0][0], sizeOfImage/slaves * sizeOfImage, MPI_INT, k, 0, MPI_COMM_WORLD);
		}
	}

	//	SLAVES PART | Receive associated array
	if(rank!=0){
		//	Receive from root, assigned 2D array.
		MPI_Recv(&(slaveRows[0][0]), sizeOfImage/slaves * sizeOfImage, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	//	Wait until all processes receive their rows
	MPI_Barrier(MPI_COMM_WORLD);

	//	SLAVES PART | Send below and above rows from adjacent ones.
	if(rank!=0){
		//	Except the very first slave, send 1st row to above process, receive last row of above process as aboveRow.
		if(rank>1){
			MPI_Ssend(&slaveRows[0][0], sizeOfImage, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
			MPI_Recv(&aboveRow[0], sizeOfImage, MPI_INT, rank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		//	Except the very last slave, send last row to below process, receive 1st row of below process as belowRow.
		if(rank<slaves){
			MPI_Recv(&belowRow[0], sizeOfImage, MPI_INT, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Ssend(&slaveRows[sizeOfImage/slaves -1][0], sizeOfImage, MPI_INT, rank+1, 1, MPI_COMM_WORLD);
		}
	}

	//	Wait until all processes get their below and above rows.
	MPI_Barrier(MPI_COMM_WORLD);

	//	SLAVES PART | Apply mean filter to each pixel
	if(rank!=0){
		//	If it is the first slave, we need to just check i=1,2.. and belowRow came from below process.
		if(rank==1){
			for(i=1; i<sizeOfImage/slaves; i++){
				for(j=1; j<sizeOfImage-1; j++){
					if(i==(sizeOfImage/slaves - 1)){
						smoothed[i][j] = meanFilter(slaveRows[i-1][j-1], slaveRows[i-1][j], slaveRows[i-1][j+1], 
													slaveRows[i][j-1], slaveRows[i][j], slaveRows[i][j+1], 
													belowRow[j-1], belowRow[j], belowRow[j+1]);
					}else{
						smoothed[i][j] = meanFilter(slaveRows[i-1][j-1], slaveRows[i-1][j], slaveRows[i-1][j+1], 
													slaveRows[i][j-1], slaveRows[i][j], slaveRows[i][j+1], 
													slaveRows[i+1][j-1], slaveRows[i+1][j], slaveRows[i+1][j+1]);
					}
				}
			}
		}
		//	If it is neither first nor last slave, we need to check aboveRow came from above process, i=0,1,2.. and belowRow came from below process.
		if(1<rank && rank<slaves){
			for(i=0; i<sizeOfImage/slaves; i++){
				for(j=1; j<sizeOfImage-1; j++){
					if(i==0){
						smoothed[i][j] = meanFilter(aboveRow[j-1], aboveRow[j], aboveRow[j+1], 
													slaveRows[i][j-1], slaveRows[i][j], slaveRows[i][j+1], 
													slaveRows[i+1][j-1], slaveRows[i+1][j], slaveRows[i+1][j+1]);
					}
					if(i==(sizeOfImage/slaves - 1)){
						smoothed[i][j] = meanFilter(slaveRows[i-1][j-1], slaveRows[i-1][j], slaveRows[i-1][j+1], 
													slaveRows[i][j-1], slaveRows[i][j], slaveRows[i][j+1], 
													belowRow[j-1], belowRow[j], belowRow[j+1]);
					}
					if(0<i && i<(sizeOfImage/slaves -1) && 2<(sizeOfImage/slaves)){
						smoothed[i][j] = meanFilter(slaveRows[i-1][j-1], slaveRows[i-1][j], slaveRows[i-1][j+1], 
													slaveRows[i][j-1], slaveRows[i][j], slaveRows[i][j+1], 
													slaveRows[i+1][j-1], slaveRows[i+1][j], slaveRows[i+1][j+1]);
					}
				}
			}
		}
		//	If it is the last slave, we need to check i=0,1.. and aboveRow came from above process.
		if(rank==slaves){
			for(i=0; i<sizeOfImage/slaves -1; i++){
				for(j=1; j<sizeOfImage-1; j++){
					if(i==0){
						smoothed[i][j] = meanFilter(aboveRow[j-1], aboveRow[j], aboveRow[j+1], 
													slaveRows[i][j-1], slaveRows[i][j], slaveRows[i][j+1], 
													slaveRows[i+1][j-1], slaveRows[i+1][j], slaveRows[i+1][j+1]);
					}else{
						smoothed[i][j] = meanFilter(slaveRows[i-1][j-1], slaveRows[i-1][j], slaveRows[i-1][j+1], 
													slaveRows[i][j-1], slaveRows[i][j], slaveRows[i][j+1], 
													slaveRows[i+1][j-1], slaveRows[i+1][j], slaveRows[i+1][j+1]);
					}
				}
			}
		}
	}

	//	Wait until all process apply their rows mean filter
	MPI_Barrier(MPI_COMM_WORLD);

	//	-------------------- SMOOTHING PART IS OVER --------------------

	//	--------------------- LINE DETECTION PART -----------------------
	//	Now we exchanged slaveRows with smoothed, we will apply filter from smoothed array and store it in slaveRows

	//	SLAVES PART | Send below and above rows from adjacent ones.
	if(rank!=0){
		//	Except the very first slave, send 1st row to above process, receive last row of above process as belowRow.
		if(rank>1){
			MPI_Ssend(&smoothed[0][0], sizeOfImage, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
			MPI_Recv(&aboveRow[0], sizeOfImage, MPI_INT, rank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		//	Except the very last slave, send last row to below process, receive 1st row of below process as belowRow.
		if(rank<slaves){
			MPI_Recv(&belowRow[0], sizeOfImage, MPI_INT, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Ssend(&smoothed[sizeOfImage/slaves -1][0], sizeOfImage, MPI_INT, rank+1, 1, MPI_COMM_WORLD);
		}
	}

	//	Wait until all processes get their below and above rows.
	MPI_Barrier(MPI_COMM_WORLD);

	//	SLAVES PART | Apply thresholding to each pixel
	if(rank!=0){
		//	If it is the first slave, we need to just check i=1,2.. and belowRow came from below process.
		if(rank==1){
			for(i=1; i<sizeOfImage/slaves; i++){
				for(j=1; j<sizeOfImage-1; j++){
					if(i==(sizeOfImage/slaves - 1)){
						slaveRows[i][j] = thresholding(smoothed[i-1][j-1], smoothed[i-1][j], smoothed[i-1][j+1], 
													smoothed[i][j-1], smoothed[i][j], smoothed[i][j+1], 
													belowRow[j-1], belowRow[j], belowRow[j+1], threshold);
					}else{
						slaveRows[i][j] = thresholding(smoothed[i-1][j-1], smoothed[i-1][j], smoothed[i-1][j+1], 
													smoothed[i][j-1], smoothed[i][j], smoothed[i][j+1], 
													smoothed[i+1][j-1], smoothed[i+1][j], smoothed[i+1][j+1], threshold);
					}
				}
			}
		}
		//	If it is neither first nor last slave, we need to check aboveRow came from above process, i=0,1,2.. and belowRow came from below process.
		if(1<rank && rank<slaves){
			for(i=0; i<sizeOfImage/slaves; i++){
				for(j=1; j<sizeOfImage-1; j++){
					if(i==0){
						slaveRows[i][j] = thresholding(aboveRow[j-1], aboveRow[j], aboveRow[j+1], 
													smoothed[i][j-1], smoothed[i][j], smoothed[i][j+1], 
													smoothed[i+1][j-1], smoothed[i+1][j], smoothed[i+1][j+1], threshold);
					}
					if(i==(sizeOfImage/slaves - 1)){
						slaveRows[i][j] = thresholding(smoothed[i-1][j-1], smoothed[i-1][j], smoothed[i-1][j+1], 
													smoothed[i][j-1], smoothed[i][j], smoothed[i][j+1], 
													belowRow[j-1], belowRow[j], belowRow[j+1], threshold);
					}
					if(0<i && i<(sizeOfImage/slaves -1) && 2<(sizeOfImage/slaves)){
						slaveRows[i][j] = thresholding(smoothed[i-1][j-1], smoothed[i-1][j], smoothed[i-1][j+1], 
													smoothed[i][j-1], smoothed[i][j], smoothed[i][j+1], 
													smoothed[i+1][j-1], smoothed[i+1][j], smoothed[i+1][j+1], threshold);
					}
				}
			}
		}
		//	If it is the last slave, we need to check i=0,1.. and aboveRow came from above process.
		if(rank==slaves){
			for(i=0; i<sizeOfImage/slaves -1; i++){
				for(j=1; j<sizeOfImage-1; j++){
					if(i==0){
						slaveRows[i][j] = thresholding(aboveRow[j-1], aboveRow[j], aboveRow[j+1], 
													smoothed[i][j-1], smoothed[i][j], smoothed[i][j+1], 
													smoothed[i+1][j-1], smoothed[i+1][j], smoothed[i+1][j+1], threshold);
					}else{
						slaveRows[i][j] = thresholding(smoothed[i-1][j-1], smoothed[i-1][j], smoothed[i-1][j+1], 
													smoothed[i][j-1], smoothed[i][j], smoothed[i][j+1], 
													smoothed[i+1][j-1], smoothed[i+1][j], smoothed[i+1][j+1], threshold);
					}
				}
			}
		}
	}

	//	Wait until all process apply their rows thresholding filter
	MPI_Barrier(MPI_COMM_WORLD);

	//	------------------ LINE DETECTION PART IS OVER ---------------------

	//	SLAVES PART | Each slave process send its data to master
	if(rank!=0){
		MPI_Ssend(&slaveRows[0][0], sizeOfImage*(sizeOfImage/slaves), MPI_INT, 0, rank, MPI_COMM_WORLD);
	}

	//	MASTER PART | Master receives data from slaves and concetenates it, shrinks finalArray by [-4][-4], prints finalArray to output.txt
	if(rank==0){
		int **finalArray = create2DArray(sizeOfImage,sizeOfImage);
		for(k=1; k<size; k++){
			MPI_Recv(&finalArray[(k-1)*(sizeOfImage/slaves)][0], sizeOfImage*(sizeOfImage/slaves), MPI_INT, k, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		for(i=2; i<sizeOfImage-2; i++){
			for(j=2; j<sizeOfImage-2; j++){
				fprintf(slavefp, "%d ", finalArray[i][j]);
			}
			fprintf(slavefp, "\n");
		}
	}

	//	Finalize MPI Environment
    MPI_Finalize();

    return 0;
}