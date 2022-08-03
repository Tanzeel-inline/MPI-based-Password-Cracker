# MPI based Password Cracker

 - The following project asks the user to input the username and it then reads the /etc/shadow file to get the hash of the password. 
 - The application then uses brute-force **(testing all possible combinations)** to identify the password. 
 - As you can imagine, trying all possible combinations on a single machine is not feasible, and having developed and programmed the cluster in our course, we will use a cluster-based system where multiple nodes will assist the password cracking process.
 - The project assumes that password has only alphabetic characters between (a - z) and maximum length of the password is 8.
 ### **This project can be modified to bruteforce crack the MD5, SHA-256, and SHA-512 hashes**

# Compilation and Execution
## Install the MPI library on Debian using:
	sudo apt-get install mpich
## Ensure that library is installed using:
	which mpirun
	which mpiexec
## Compile the program using:
	mpic++ bruteForcer.cpp -o <exe_name> -lcrypt
## Run the program using:
	mpiexec -n <num_of_threads> <file_name> ./brute
**Note: If you are using clusters then save the configuration inside the <file_name>  to specify how many threads to run on each machines, otherwise don't input any filename.**