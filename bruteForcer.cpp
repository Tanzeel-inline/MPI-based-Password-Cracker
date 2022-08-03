#include "hashParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <string>

#define ALPHABETS 26
#define LENGTH 5

//TAGS FOR COMMUNICATION
#define CODE_TAG_FROM_MASTER_TO_SLAVE 1
#define CODE_TAG_FROM_SLAVE_TO_MASTER 6
#define SALT_TAG 2
#define HASH_TAG 3
#define COMBINATION_START_TAG 4
#define COMBINATION_END_TAG 5
//MASTER RANK
#define MASTER_RANK 0
//TO CHECK IF PASSWORD WAS FOUND AFTER NUMBER OF COMBINATIONS TRIED
#define CHECK_AFTER 100
long correct_generator[LENGTH];
/*This function inputs the number ( combinations number ) aganist which we have to generate possible password,
this functions checks in which length range does the current number fall, combination of length 3 or 4, then converts it to that length combination by subtracting the possible combination of lower lengths*/
int getValue(int num, int *total)
{
	for ( int i = 0 ; i < LENGTH ; i++ )
	{
		if ( num >= correct_generator[i] )
		{
			num -= correct_generator[i];
		}
		else
		{
			*total = i + 1;
			return num;
		}
	}
    *total = LENGTH - 1;
    return num;
}
/*This function generates the possible password aganist the current combination, encrypts the current generated password, gets the hash and compares it aganist the hash of the user, if they match, then returns 1, else returns 0*/
int generateAndcompare(int array_start[], char *hash , long &start,
 int &working_index, char *delimiter_dollar, char generate_hash[], char password[], char *salt, int myrank)
{
    //Generate the combination from current value
    int m = 0;
    for ( int j = 0 ; j < LENGTH ; j++ )
    {
        if ( array_start[j] >= 0 )
        {
            password[m] = array_start[j] + 97;
            m++;
        }
    }
    password[m] = '\0';
    printf("%s by process %d\n",password,myrank);
    //Performing encryption and comparison
    //Generating the hash from the key
    char *data = crypt(password,salt);

    //Now break the generated key to get the hash
    int n = 0;
    char *ptr = strtok(data,delimiter_dollar);
    while ( ptr != NULL ) {
        if ( n == 2 ) {
            strncpy(generate_hash,ptr,strlen(ptr));
            break;
        }
        ptr = strtok(NULL,delimiter_dollar);
        n++;
    }
    //Matching hash
    if ( strncmp(generate_hash,hash,strlen(hash)) == 0 )
    {
		//Re
        m = 0;
        for ( int j = 0 ; j < LENGTH ; j++ )
        {
            if ( array_start[j] >= 0 )
            {
                password[m] = array_start[j] + 97;
                m++;
            }
        }
        password[m] = '\0';
        printf("Password is : %s\n",password);
        return 1;
    }
    //Upgrade the combination
    array_start[working_index]++;
    if ( array_start[working_index] > ALPHABETS - 1 )
    {
        int index = working_index;
        while ( index > 0 && array_start[index] > ALPHABETS - 1 )
        {
            array_start[index] -= (ALPHABETS);
            array_start[index-1]++;
            index--;
        }
    }
    return 0;
}
int main(int argc, char *argv[])
{
	for ( int i = 0 ; i < LENGTH ; i++ )
	{
		correct_generator[i] = pow(ALPHABETS, i + 1);
	}
    //Variables for saving salt and hash
    char generate_hash[HASHSIZE];
    char *delimiter_dollar = (char*)"$";    
    char *salt = (char*)malloc(sizeof(char) *SALTSIZE);
    char *hash = (char*)malloc(sizeof(char) *HASHSIZE);
    //Variables for combination generation
    int array_start[LENGTH];
	int array_end[LENGTH];
    char password[LENGTH];
    int temp_start_length = 0;
    int temp_end_length = 0;
    long temp_start = 0;
    long temp_end = 0;
    //Variables for MPI
    int myrank, nprocs;
    //Variables for start, end
    long start, end;
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);

    //Master process
    if ( myrank == 0 )
    {
        //Variable declaration area
        int chunk_num = 1;
        int code_from_slave = 1;
        int code_to_slave = 0;
        int chunk_completed = 0;
        MPI_Status status;
        long start , end;
        MPI_Request request;
        bool parent_found = false;
        //Getting hash and salt fof the specified user
        int received = getHashandSalt(hash,salt);
        printf("%s\n",salt);
        printf("%s\n",hash);
        unsigned long total_combinations = 0 ;
        for ( int i = 1 ; i <= LENGTH ; i++ )
        {
            total_combinations += pow(ALPHABETS, i);
        }
        long total_chunks = nprocs - 1;
        long thread_chunk = total_combinations / total_chunks;
        long master_chunk = total_combinations - (thread_chunk * total_chunks);
        
        printf("Total combinations are : %ld\n",total_combinations);
        printf("Each thread chunk is : %ld\n",thread_chunk);
        printf("Master thread chunk is : %ld\n",master_chunk);

        //Send the code to the slave process
        for ( int i = 1 ; i < nprocs ; i++ )
        {
            MPI_Send(&received,1,MPI_INT,i,CODE_TAG_FROM_MASTER_TO_SLAVE,MPI_COMM_WORLD);
            //IF the user is valid then we have salt and hash to send
            if ( received != -1 )
            {
                MPI_Send(salt,SALTSIZE,MPI_CHAR,i,SALT_TAG,MPI_COMM_WORLD);
                MPI_Send(hash,HASHSIZE,MPI_CHAR,i,HASH_TAG,MPI_COMM_WORLD);
            }
        }
        if ( received == -1 )
        {
            MPI_Finalize();
        }
        printf("Hash sent successfully\n");
        //Now master process will loop until either code 0 is received or all chunk has completed
        while ( chunk_completed < total_chunks )
        {
            MPI_Recv(&code_from_slave,1,MPI_INT,MPI_ANY_SOURCE,CODE_TAG_FROM_SLAVE_TO_MASTER,MPI_COMM_WORLD,&status);
            //Password Found
            if ( code_from_slave == 0 )
            {
                break;
            }
            //Process have completes its chunk, so we increment chunks completed
            else if ( code_from_slave == 2 )
            {
                chunk_completed ++;
            }
            //Request for chunk
            else if ( code_from_slave == 1 )
            {
                
                //Write the code = 1 , (sending the chunk)
                code_to_slave = 1;
                MPI_Send(&code_to_slave,1,MPI_INT,status.MPI_SOURCE,CODE_TAG_FROM_MASTER_TO_SLAVE,MPI_COMM_WORLD);
                //Calculate the start and end of current chunk
                start = thread_chunk * (chunk_num - 1);
                end = thread_chunk * chunk_num;
                //Write start and end
                MPI_Send(&start,1,MPI_LONG,status.MPI_SOURCE,COMBINATION_START_TAG,MPI_COMM_WORLD);
                MPI_Send(&end,1,MPI_LONG,status.MPI_SOURCE,COMBINATION_END_TAG,MPI_COMM_WORLD);
                //Increment the chunk
                chunk_num++;
            }
            if ( chunk_num > total_chunks )
            {
                start = thread_chunk * (chunk_num - 1);
                end = total_combinations;
                temp_start_length = 0;
                temp_end_length = 0;
                temp_start = getValue(start,&temp_start_length);
                temp_end = getValue(end,&temp_end_length);
                for ( int j = 0 ; j < LENGTH - temp_start_length ; j++ )
                {
                    array_start[j] = -1;
                }
                //Start array conversion
                int index = LENGTH - 1;
                while ( temp_start > ALPHABETS )
                {
                    array_start[index] = temp_start % ALPHABETS;
                    temp_start = temp_start / ALPHABETS;
                    index--;
                }
                array_start[index] = temp_start;
            
                int working_index = LENGTH - 1;
                for ( long h = start ; h < end ; h++)
                {
                    if ( generateAndcompare(array_start,hash,start,
                        working_index,delimiter_dollar,generate_hash,password, salt, myrank) == 1 )
                    {
                        parent_found = true;
                        break;
                    }
                }
                if ( parent_found )
                {
                    break;
                }
            }
        }
        code_to_slave = 0;
        for ( int i = 1; i < nprocs ; i++ )
        {
            MPI_Isend(&code_to_slave,1,MPI_INT,i,CODE_TAG_FROM_MASTER_TO_SLAVE,MPI_COMM_WORLD,&request);
        }

    }
    else
    {
        //Variable declaration area
        bool notfound = false;
        int code_to_master = -1;
        int code_from_master = -1;
        MPI_Request request;
        MPI_Status status;
        long start, end;
        int flag = 0;
        //----------------------------------------------------Getting the hash-----------------------------------
        //Receive the code (whether the user entered correct name that exist in shadow file or not)
        MPI_Recv(&code_from_master,1,MPI_INT,MASTER_RANK,CODE_TAG_FROM_MASTER_TO_SLAVE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        //IF the code is -1 that means user has enter invalid user name
        if ( code_from_master == -1 )
        {
            notfound = true;
        }
        //Else we need to receive the SALT and HASH from master process
        else
        {
            MPI_Recv(salt,SALTSIZE,MPI_CHAR,MASTER_RANK,SALT_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(hash,HASHSIZE,MPI_CHAR,MASTER_RANK,HASH_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }

        //--------------------------------Getting combinations to perform and performing comparison with hash---------------
        
        /*Code = 1, requesting for the chunk*/
        code_to_master = 1;
        MPI_Isend(&code_to_master,1,MPI_INT,MASTER_RANK,CODE_TAG_FROM_SLAVE_TO_MASTER,MPI_COMM_WORLD,&request);
        MPI_Recv(&code_from_master,1,MPI_INT,MASTER_RANK,CODE_TAG_FROM_MASTER_TO_SLAVE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        if ( code_from_master != 1 )
        {
            MPI_Finalize();
        }
        //Receving the starting and ending point for combinations of this process
        MPI_Recv(&start,1,MPI_LONG,MASTER_RANK,COMBINATION_START_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&end,1,MPI_LONG,MASTER_RANK,COMBINATION_END_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        //Starting MPI_Irecv call so that after evry 1000 iteration we can see if number was received or not
        MPI_Irecv(&code_from_master,1,MPI_INT,MASTER_RANK,CODE_TAG_FROM_MASTER_TO_SLAVE,MPI_COMM_WORLD,&request);
        //Work on chunk here, if password found any time, send the code 0 to parent and then send password to parent
        //Converting the number into useful form so that we can generate combinations without any problem
        //Acquiring the true number to make correct combination
        temp_start_length = 0;
        temp_end_length = 0;
        temp_start = getValue(start,&temp_start_length);
        temp_end = getValue(end,&temp_end_length);
        for ( int j = 0 ; j < LENGTH - temp_start_length ; j++ )
        {
            array_start[j] = -1;
        }
        //Start array conversion
        int index = LENGTH - 1;
        while ( temp_start > ALPHABETS )
        {
            array_start[index] = temp_start % ALPHABETS;
            temp_start = temp_start / ALPHABETS;
            index--;
        }
        array_start[index] = temp_start;
    
        int working_index = LENGTH - 1;
        for ( long h = start ; h < end ; h++)
        {
            //printf("Process %d\n",myrank);
            if ( generateAndcompare(array_start,hash,start,
                working_index,delimiter_dollar,generate_hash,password, salt,myrank) == 1 )
            {
                code_to_master = 0;
                MPI_Send(&code_to_master,1,MPI_INT,MASTER_RANK,CODE_TAG_FROM_SLAVE_TO_MASTER,MPI_COMM_WORLD);
                notfound = true;
                break;
            }
            //CHeck if master thread sent some code
            MPI_Test(&request,&flag,&status);
            {
                if ( flag != 0 )
                {
                    notfound = true;
                    break;
                }
            }
        }
        if ( !notfound )
        {
            //Send code 2 to parent to tell I have search one chunk, password was not found
            code_to_master = 2;
            MPI_Isend(&code_to_master,1,MPI_INT,MASTER_RANK,CODE_TAG_FROM_SLAVE_TO_MASTER,MPI_COMM_WORLD,&request);
        }
    }
    MPI_Finalize();
}