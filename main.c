#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);



    //Init 2 parallel arrays
    const int maxData=100;
    const int maxLineSize=100;
    const int passGrade = 60;
    int ids[maxData];
    int grades[maxData];
    int Dataset_Size=0;
    int passers=0;


    //var to store recieve status
    MPI_Status* status = (MPI_Status*) malloc(sizeof(MPI_Status));

    //if i'm master:
    if(world_rank == 0)
    {
        //open file and init string line
        FILE* f= freopen("data.txt","r",stdin);
        char line[maxLineSize];

        //Read while there exits new lines
        while(Dataset_Size < maxData && fgets(line, sizeof(line), f) != NULL)
        {
            //generate 2 tokens
            const char* tok1 = strtok(line, " ");
            const char* tok2 = strtok(NULL, " ");

            //Parse tokens
            int i = atoi(tok1);
            int g = atoi(tok2);
            //printf("Id= %d, grade= %d\n",i,g);

            //Fill the arrays with content
            ids[Dataset_Size]= i;
            grades[Dataset_Size] = g;

            //recalculate dataset size
            Dataset_Size++;
        }
    }
    //let all process know Dataset_Size
    MPI_Bcast(&Dataset_Size,1, MPI_INT, 0, MPI_COMM_WORLD);
    if(world_rank == 0)
    {

        //send the data segment for each core
        for(int i=1; i<world_size; i++)
        {
            //Shift by steps according to rank
            int newStart = (i-1)*(Dataset_Size/(world_size-1));

            //Divide equally across all processes
            int segmentSize = (Dataset_Size/(world_size-1));
            if(i == world_size-1)
            {
                segmentSize = Dataset_Size-newStart;
            }

            //Send 2 values
            MPI_Send(ids   +newStart,segmentSize,MPI_INT,i,0,MPI_COMM_WORLD);
            MPI_Send(grades+newStart,segmentSize,MPI_INT,i,0,MPI_COMM_WORLD);
        }
    }
    else
    {
        //Shift by steps according to rank
        int my_start = ((world_rank-1)*(Dataset_Size/(world_size-1)));
        int segmentSize = (Dataset_Size/(world_size-1));

        if(world_rank == world_size-1)
        {
            segmentSize = Dataset_Size-my_start;
        }

        //Get data from master
        MPI_Recv(ids   +my_start,segmentSize,MPI_INT,0,0,MPI_COMM_WORLD,status);
        MPI_Recv(grades+my_start,segmentSize,MPI_INT,0,0,MPI_COMM_WORLD,status);


        //Start processing
        for(int i=my_start; i<my_start+segmentSize; i++)
        {
            if(grades[i]>=passGrade)
            {
                printf("%d passed the exam\n", ids[i]);
                passers++;
            }
            else
            {
                printf("%d Failed, Please repeat The Exam\n",ids[i]);
            }
        }
    }

    int totalPassers;
    MPI_Reduce(&passers, &totalPassers, 1, MPI_INT, MPI_SUM, 0,MPI_COMM_WORLD);
    if(world_rank == 0)
    {
        printf("Total Number of students passed the exam is %d out of %d", totalPassers, Dataset_Size);
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}
