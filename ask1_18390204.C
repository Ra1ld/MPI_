#include <stdio.h>
#include <stdlib.h>  // malloc functions
#include <mpi.h>    // mpi functions

//================================================================
//Function PROTOTYPES
void menu();

int main(int argc, char *argv[]) {

           
    int curr_rank;                   // rank(of each process)
    int P;                          // size of the communicator MPI_COMM_WORLD
	int N;                         // (USER CHOICE) array size
	int *T;                       //  dynamic array(available to every process)

    // Check Initialization
    if( MPI_Init(&argc, &argv) != MPI_SUCCESS ){
        printf("Initialization failed");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    while(1){

        // Storing info about rank and its communicator(every process)
        MPI_Comm_rank(MPI_COMM_WORLD, &curr_rank);  
        MPI_Comm_size(MPI_COMM_WORLD, &P);

        //Process 0 Initializes Array T(1o erwthma)
        if(curr_rank==0){

            menu();                 // User menu to choose operation
            system("clear");             

            printf("\nProcess speaking(%d)\n", curr_rank);
            printf("please enter array size(N): ");
            fflush(stdout);         // scanf workaround => printing  

            scanf("%d",&N);
            T = (int*)malloc(N*sizeof(int));
            if(T==NULL){
                printf("Initialization failed. Unable to allocate memory. Exiting... \n");
                MPI_Abort(MPI_COMM_WORLD,1);    //Abort MPI (shutting down processes)
                exit(1);                       
            }
            

            printf("\nArray Elements\n");
            for(int i=0; i<N; i++){
                printf("%d: ",i+1);
                fflush(stdout);             
                scanf("%d",&T[i]);
            }        
        }

        // Broadcasting N(from rank 0) to all proccesses
        if(curr_rank == 0){
            for(int i=1; i<P; i++)// i = receiver proccess
                MPI_Send(&N,1,MPI_INT,i,0,MPI_COMM_WORLD);
        }   
        else
            MPI_Recv(&N,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        

        // Allocating array space to other processes(N is available now)
        if(curr_rank != 0){
            T = (int *)malloc(N*sizeof(int));
            if(T==NULL){
                printf("Initialization failed. Unable to allocate memory. Exiting... \n");
                MPI_Abort(MPI_COMM_WORLD,1);    //Abort MPI (shutting down processes)
                exit(1);            
            }  
        }

        // Brodcasting contents of T(from rank 0)
        if(curr_rank == 0){
            for(int i=1; i<P; i++)
                MPI_Send(T,N,MPI_INT,i,1,MPI_COMM_WORLD);
        }    
        else
            MPI_Recv(T,N,MPI_INT,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        

    

        
        // "Fair" Spliting(2o ertwthma) 
        int result;                    // Visible by all processes(used as "buffer")
        int pos;                      
        if (curr_rank == 0) {   
            int is_sorted=1;         // flag for sorting
            int j = 0;             // cycle of processes
            int i;                // array index
            


            for (i = 0; i < N - 1; i++) { // Using the array elemnts

                // if process(j) > total processes => start from 0 prcess
                if (j >= P) { 
                    j = 0; 
                }

                // process 0 checks
                if(j==0){
                    result = T[i] <= T[i+1];
                    /*
                        printf("\nProcess Speaking(%d)\n",curr_rank);
                        printf("T[%d]=%d <= T[%d]=%d\n",i,T[i],i+1,T[i+1]);
                        printf("%d\n",result);
                    */
                }
                else{
                    
                    /*
                        printf("\nProcess Speaking(%d)\n",curr_rank);
                        printf("Sending indexing(%d) to process(%d)\n",i,j);
                    */
                    MPI_Send(&i, 1, MPI_INT,        j,0, MPI_COMM_WORLD); 

                    MPI_Recv(&result,1,MPI_INT,     j,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                    /*
                        printf("\nProcess Speaking(%d)\n",curr_rank);
                        printf("Recevied\n");
                    */

                }

                //Check if comparison "failed"
                if(!result && is_sorted!=0){
                        is_sorted=0;
                        pos=i;
                }


                j++; // Next Rank
            }

            // Process 0 Check(3o erwthma)
            if(is_sorted){
                printf("\nYes\n");
                printf("Array is sorted\n");
            }   
            else{
                printf("\nNO\n");
                printf("First unsorted element: T[%d]=%d\n", pos, T[pos] );
            }

        } 
        else {
            for (int i = curr_rank; i < N - 1; i+=P) { //   i+p so that the arrays are "time" synced
                
                // Receiving inexing value from process 0
                MPI_Recv(&pos, 1, MPI_INT,      0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                result = T[pos] <= T[pos + 1];

                /*
                    if (result) {
                        printf("\nProcess Speaking(%d)\n",curr_rank);
                        printf("T[%d] <= T[%d] (OK)\n\n", pos, pos + 1);
                    } else {
                        printf("\nProcess Speaking(%d)\n",curr_rank);
                        printf("T[%d] > T[%d] (NOT OK)\n\n",pos,i + 1);
                }
                */

                // Sending result to process 0
                MPI_Send(&result,1,MPI_INT,     0,2,MPI_COMM_WORLD); 
            }
        }
        
        
        // Free allocated memory(from every memmory)
        free(T);

        
    }  

    if( MPI_Finalize() != MPI_SUCCESS ){
        printf("Error: MPI_Finalize() failed!\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }


    return 0;
}


void menu(){

    int option;
    do{
        printf("\n=========================\n");
        printf("MENU\t\t\t=\n");
        printf("\t\t\t=\n");
        printf("1. Start/Continue\t=\n");
        printf("2. Exit\t\t\t=\n");
        printf("3. Info\t\t\t=\n");
        printf("\t\t\t=\n");
        printf("=========================\n\n");
        printf("Choice: ");
        fflush(stdout);
        scanf("%d",&option);
        while(option!=1 && option!=2 && option!=3){
            printf("\nInvalid choice. Please try again.\n");
            printf("Choice: ");
            fflush(stdout);
            scanf("%d",&option);
        }
        if(option==2){
            MPI_Abort(MPI_COMM_WORLD,1);
            exit(0);
        }
        else if(option==3){
            system("clear");
            printf("\n************ INFO ************************************************************************************\n\n");
            printf("Author: George Tassios\n");
            printf("Github: r_a1ld\n\n");
            printf("\n¯\\_(ツ)_/¯\n\n");
            printf("Description: A simple program demonstrating parrelel use of blocking operations(send and receive)\n\n");
            printf("******************************************************************************************************\n\n");
        }
    }
    while(option!=1);
}

