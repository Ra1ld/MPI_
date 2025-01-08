#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

//================================================================
//Function PROTOTYPES
int menu();

int main(int argc, char** argv) {

    int curr_rank,P;     // P = size of communicator COMM_WORLD  
    int N,*X;           // X = main sequence   N = sequence size

    int *Recev_arr;         // Splitted sequence array(local)
    int local_arr_size;    // Size of local array(splitted sequence)
    int extras;

    int exit_code;

    // Check Initialization
    if( MPI_Init(&argc, &argv) != MPI_SUCCESS ){
        printf("Error: MPI_Init() failed!\n");
        MPI_Abort(MPI_COMM_WORLD,1);
        exit(1);
    }   

    // Storing info about rank and its communicator(every process)
    MPI_Comm_rank(MPI_COMM_WORLD, &curr_rank);  
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    // root process will display menu and get user input
    if( curr_rank == 0 ){
        exit_code = menu();
    }

    // root process broadcasts the users input(from menu)
    MPI_Bcast(&exit_code,1, MPI_INT,     0,MPI_COMM_WORLD);
    
    // barrier to make sure the other processes wait until the user enters his input from the menu
    // so the other process wont try to intiate the loop without a value
    MPI_Barrier(MPI_COMM_WORLD);

    while(!exit_code){

        if(curr_rank == 0){

           

            // User SIZE CHOICE
            system("clear");
            printf("\n******************************************************************************************************************************************************************\n\n");
            printf("\nProcess speaking(%d)\n",curr_rank);
            printf("Enter the size of array N: ");
            fflush(stdout);
            scanf("%d",&N);
            if( N <= 0 ){
                printf("\n\nSize should not be zero or negative.\nExiting... \n\n");
                MPI_Abort(MPI_COMM_WORLD,1);
                exit(1);
            }

            // CREATE main sequence (X)
            X = (int*)malloc(N*sizeof(int));
            if(X == NULL){
                printf("\nProcess speaking(%d)\n",curr_rank);
                printf("Initialization failed. Unable to allocate memory. Exiting... \n");
                MPI_Abort(MPI_COMM_WORLD,1);
                exit(1);
            }

            // ALLOCATE data to array
            printf("\nArray Elements\n");
            for(int i=0; i<N; i++){
                printf("%d: ",i+1);
                fflush(stdout);
                scanf("%d", &X[i]);
            }
        }

        // Synchronizing all the Processes
        //MPI_Barrier(MPI_COMM_WORLD);
    
        // Broadcasting (N) sequence Size
        MPI_Bcast(&N,1,MPI_INT,     0,MPI_COMM_WORLD);

        // Fair Splitting for each process
        local_arr_size = N / P ;
        extras = N % P;
        if(curr_rank < extras)
            local_arr_size++;

        
        
        // Allocating local array for each process    
        Recev_arr = (int*)malloc(local_arr_size*sizeof(int));
        if(Recev_arr == NULL){
            if(local_arr_size != 0){
                printf("\nProcess speaking(%d)\n",curr_rank);
                printf("Initialization failed. Unable to allocate memory. Exiting... \n");
                MPI_Abort(MPI_COMM_WORLD,1);
                exit(1);
            }
        
        }
        
        



        int *displs = NULL;     // for scatterv
        int *sendcounts= NULL;  // for scatterv

        if(curr_rank == 0){
            displs = (int*)malloc(P*sizeof(int));   
            sendcounts= (int*)malloc(P*sizeof(int));
            if (sendcounts== NULL || displs == NULL){
                printf("\nProcess speaking(%d)\n",curr_rank);
                printf("Initialization failed. Unable to allocate memory. Exiting...\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
                exit(1);
            }

            int offset = 0;
            for (int i = 0; i < P; i++){

                sendcounts[i] = N / P;
                if (i < extras) 
                    sendcounts[i]++;

                // setting up array to start at the right indexing of X for each process
                displs[i] = offset;     

                // Updating offset for next process
                offset += sendcounts[i];
            }
            
        }

        int *recvbuf = (local_arr_size > 0) ? Recev_arr : NULL;         // handling pointer in case "fair spliting" isnt possible
        MPI_Scatterv(X, sendcounts, displs, MPI_INT,    recvbuf, local_arr_size, MPI_INT, 0, MPI_COMM_WORLD);
        
        // Debug 
        /*
        if(curr_rank == 0){
            for(int i=0; i<N; i++){
                printf("\nProcess speaking(%d)\n",curr_rank);
                printf("\nX[%d] = %d\n",i, X[i]);
            }
            printf("Recev_arr[%d] = %d\n",0, Recev_arr[0]);
        }
        else{
            for(int i=0; i<local_arr_size; i++){
                printf("\nProcess speaking(%d)\n",curr_rank);
                printf("Recev_arr[%d] = %d\n",i, Recev_arr[i]);
            }
        }
        */

        
        //calculating (local)min and (local)max for each process
        int local_min;
        int local_max;
        if(local_arr_size>0){
            local_min = Recev_arr[0];
            local_max = Recev_arr[0];
            for(int i=1; i<local_arr_size; i++){
                if (Recev_arr[i] < local_min)     local_min = Recev_arr[i];
                if (Recev_arr[i] > local_max)     local_max = Recev_arr[i]; 
            }
        }
        else{
            /*
                values to be used in case of non-initialized array
                mininum = maximum possible intger so it can render iteself "useless"
                maxmium= minimum possible intger so it can render iteself "useless"
            */ 
            local_min = 2147483647;     
            local_max = -2147483648;    
        }

        // calculating local sums ( ITS NEEDED because reduce expects at least 1 value..even if its zero)...if a Recev_arr is null, we cant use it on reduce
        int sendbuf = 0;    
        int sum;            // result of reduce(the summary of all elements)
        for(int i=0; i<local_arr_size; i++){
            sendbuf += Recev_arr[i];
        }
        MPI_Reduce(&sendbuf, &sum ,1,MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    

        // finding the true max and min of all processes(and average)
        double m;
        int min,max;
        MPI_Reduce(&local_min,  &min, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_max,  &max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
        if(curr_rank == 0){
            m = (double)sum / N;
            printf("\nProcess speaking(%d)\n",curr_rank);
            printf("Average(m): %.5f\n", m);
            printf("Minimum: %d\n", min);
            printf("Maximum: %d\n\n", max);
        }

        // Broadcasting the average value (needed for the calculations of each process local sum of "x-m")
        MPI_Bcast(&m,1,MPI_DOUBLE,   0,MPI_COMM_WORLD);

        double var_local=0.0;                     //  calculation of each process elements
        double var;                              //  final variance value
        for(int i=0; i<local_arr_size; i++)     // ALSO checks for cases N < P  => nwhen a process does not have an local array
                var_local += (Recev_arr[i] - m) * (Recev_arr[i] - m);
        
        MPI_Reduce(&var_local, &var, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        if(curr_rank == 0){
            printf("\nProcess speaking(%d)\n",curr_rank);
            printf("var: %.5f\n", var / N);
        }

    
        double *D = NULL;
        if(curr_rank == 0){
            D = (double*)malloc(N*sizeof(double));  // creating new sequence
            if(D == NULL){
                printf("\nProcess speaking(%d)\n",curr_rank);
                printf("Initialization failed. Unable to allocate memory. Exiting... \n");
                MPI_Abort(MPI_COMM_WORLD,1);
                exit(1);
            }

        
        }
        
        // broadcasting min and max so each process can make its own calculations
        // it has to be done, otherwise the process 0 would have to do everything(not fair splitting).
        MPI_Bcast(&min,1,MPI_INT,   0,MPI_COMM_WORLD);
        MPI_Bcast(&max,1,MPI_INT,   0,MPI_COMM_WORLD);

        // creating temp array to host new values to be sent
        // we cant use the old Recev_arr becuase the new values will be double instead of int
        double *temp = (double *)malloc(local_arr_size*sizeof(double));
        if(temp==NULL){
            if(local_arr_size!=0){
                printf("\nProcess speaking(%d)\n",curr_rank);
                printf("Initialization failed. Unable to allocate memory. Exiting... \n");
                MPI_Abort(MPI_COMM_WORLD,1);
                exit(1);
            } 
        }



        // calculating new values
        for(int i=0; i<local_arr_size; i++){
            if(max==min)    temp[i] = 0.0;  // handling the case where all elements are the same => computer should not divide by zero
            else            temp[i] = (double)(Recev_arr[i] - min) / (max - min) * 100.0;
        }

        // gathering the values to create the new sequence D
        MPI_Gatherv(temp, local_arr_size, MPI_DOUBLE, D, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // printing the new sequence D
        if(curr_rank == 0){
            printf("\nProcess speaking(%d)\n",curr_rank);
            if(max==min)
                printf("All elements are the same therefore the sequence (D) cannot be calculated (divison by infinity).\n");
            else{    
                for(int i=0; i<N; i++){
                    printf("D[%d] = %.2f\n", i, D[i]);
                }
            }
        }

        
        // create struct for every process(to calculate maxloc)
        /*
            BY default, every MAXLOC operation returns a value(the maximun one) which can be a double or int
            and its rank (that this value was found)
            that pair of values can either be in an "classic" array form or in a struct(most conmonly used for storing different type values)
            the MAXLOC CANNOT OPERATE IN ANY DIFFERENT WAY
        */
        struct winner_max{
            double winner_val;
            int winner_rank;
        } in, out;

        // Synchronizing all the processes(so the output is well synchronized too)
        MPI_Barrier(MPI_COMM_WORLD);
        
        if(local_arr_size<=0){              // case where the process doesnt have a local array ( X < P )          
            in.winner_val = -2147483648;    // ensuring that the value wont intefere with the max searching     
            in.winner_rank = curr_rank;    
        }
        else{
            // searching SINGLE max value in case an local array has multiple values(maxloc can only search throguh "single" values)
            in.winner_val = temp[0];
            in.winner_rank = curr_rank;
            for(int i=1; i<local_arr_size; i++){
                if(temp[i] > in.winner_val){
                    in.winner_val = temp[i];
                    in.winner_rank = curr_rank;
                }
            }
        }

        // Searching max and storing it to "out" struct
        MPI_Reduce(&in, &out, 1, MPI_DOUBLE_INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);   // DOUBLE_INT = 1 means that one value is double, the other is int
        if( curr_rank==0 ){
            for( int i=0; i<N; i++ ){
                if( out.winner_val == D[i] ){
                    printf("\n\nProcess speaking(%d)",curr_rank);
                    printf("\nMax value: D[%d]=%.2f\n", i, out.winner_val);
                    break;      // position found, no need to search anymore
                }
            }
        }


        int local_sum = 0;
        if(local_arr_size>0){
            for(int i=0; i<local_arr_size; i++){
                local_sum += Recev_arr[i];
            }
        }
        int output;

        MPI_Scan(&local_sum, &output , 1 , MPI_INT , MPI_SUM , MPI_COMM_WORLD );


        if(local_arr_size>0)
            printf("\n\nProcess speaking(%d)..Prefix_sum=%d\n\n",curr_rank,output);
        else
            printf("\n\nProcess speaking(%d)..this process does not contribute\n\n",curr_rank);
        
        
        


        // freeing memory
        if (curr_rank == 0){ 
            free(X);               // Free X array (the initial one)
            free(displs);         // free array used in reduce
            free(sendcounts);    // Free array used in reduce
            free(D);            // Free D array (new sequence)
        }         
        free(Recev_arr);    // Free the local array
        free(temp); 

        MPI_Barrier(MPI_COMM_WORLD); 

        // root process will display menu and get user input
        if( curr_rank == 0 ){
            printf("******************************************************************************************************************************************************************\n");
            exit_code = menu();           
        }
        // root process broadcasts the users input(from menu)
        MPI_Bcast(&exit_code,1, MPI_INT,     0,MPI_COMM_WORLD);

        // barrier to make sure the other processes wait until the user enters his input from the menu 
        // so they dont go to while loop with the previous "past" value
        MPI_Barrier(MPI_COMM_WORLD);       

    }    

    // Check Finalization(and shut down communication)
    if( MPI_Finalize() != MPI_SUCCESS ){
        printf("Error: MPI_Finalize() failed!\n");
        MPI_Abort(MPI_COMM_WORLD,1);
        exit(1);
    }

    return 0;
}

int menu(){

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
            return 1;
        }
        else if(option==3){
            system("clear");
            printf("\n************ INFO ************************************************************************************\n\n");
            printf("Author: George Tassios\n");
            printf("Github: r_a1ld\n\n");
            printf("\n¯\\_(ツ)_/¯\n\n");
            printf("Description: A simple program demonstrating parrelel use of collective operations\n\n");
            printf("******************************************************************************************************\n\n");
        }
    }
    while(option!=1);

    return 0;
}