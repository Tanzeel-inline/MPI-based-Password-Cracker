#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>

#define LINESIZE 900
#define NAMESIZE 50
#define SALTSIZE 200
#define HASHSIZE 513
#define IDSIZE 3
int getHashandSalt(char *hash, char *salt)
{
    char *filename = (char*)"/etc/shadow";
    FILE *f = fopen(filename,"r");
    if ( f == NULL )
    {
        printf("COULDN'T OPEN SHADOW FILE,FIX THE FILE PATH OR PERMISSIONS!\n");
        return -1;
    }
    char username[NAMESIZE];
    char line[LINESIZE];
    //char salt[SALTSIZE];
    //char hash[HASHSIZE];
    char id[IDSIZE];
	
    //Stores the garbage data
    char ignore[SALTSIZE];
    char *delimiter_dollar = (char*)"$";
    char *delimiter_colon = (char*)":";
    int user_found = 0;
    //Input the username of the user of whom we want to brute force the password
    printf("Enter the username : ");
    scanf("%s",username);
    
    //Iterate the shadow file line by line and find the user encrypter, salt and hash
    while ( fgets(line,LINESIZE,f) )
    {
        if ( strncmp(line,username,strlen(username)) == 0 )
        {
            //printf("%s\n",line);
            user_found = 1;
            break;
        }
    }
    //If user doesn't exist, stop the code
    if ( user_found == 0 )
    {
        printf("User doesn't exist\n");
        return -1;
    }
    //Now break the line to get the salt and hash
    int i = 0;
    char *ptr = strtok(line,delimiter_dollar);
    while ( ptr != NULL ) {
        if ( i == 1 ) {
            strcpy(salt,"$");
            strcat(salt,ptr);
        }
        else if ( i == 2 ) {
            strcat(salt,"$");
            strcat(salt,ptr);
            strcat(salt,"$");
        }
        else if ( i == 3 ) {
            strcpy(hash,strtok(ptr,delimiter_colon));
            break;
        }
        ptr = strtok(NULL,delimiter_dollar);
        i++;
    }
    fclose(f);
    return 1;
}