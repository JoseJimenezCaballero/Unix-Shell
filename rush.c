#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>



                                            //**********SUMMARY**********//

/*Shell works by running in a loop unitl it takes the exit built in command. It parses the string entered and removes all tabs
and whitespaces and new lines. After this it will check for parrallelism or redirection commmands and deal with them by setting
flags(boolean values) for each as well as storing the output file var on another array. Every command is stored in a 2 dimensional array
with the arguments next to it and ending with a null value. This is done for the execv function. Once all the parsing and storing is done,
the program will then check if the command is a built-in command or an external command. If it is an external command it works by creating 
a child through fork() and executing the command. The parent waits for all commands to be executed and restarts the loop.
*/



// Function to compute the length of an array of strings
int array_length(char *strings[]) {
    int length = 0;

    while (strings[length] != 0) {
        length++;
    }
    return length;
}


int main(int argc, char *argv[]) {
    char *buffer;
    size_t bufsize = 255;
    size_t chars;
    int index = 0;
    
    buffer = (char *)malloc(bufsize * sizeof(char));
    char error_message[30] = "An error has occurred\n";
    int commands_arr_index = 0; //index to keep track of array of commands/arguments
    char* path1[255] = {"/bin"}; //initial path

    if(argc != 1){ //check if called with arguments
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        fflush(stdout);
        exit(0);
    }
    

    while(1){
        //print rush and clear the output and getline
        printf("rush> ");
        fflush(stdout);
        chars = getline(&buffer,&bufsize,stdin);
        char *dest = (char *)malloc(bufsize * sizeof(char));
        strcpy(dest,buffer); //copy over buffer to dest
        dest[chars-1] = '\0';//remove the new line char from the string
        int out_bool = 0; //boolean value to know if the output is needed to be sent to a file

        int out_var_index = 0;//index var for this array
        int parra_bool = 0;//bool value to know if its a parrallel command
        char* commands_arr[255][255]={0}; //this array will contain arrays of arguments
        int output_array_bool[255]={0};//this array will contain the index of each arguments output. So its a 1 if it will out to a file 0 to stdout
        char* out_var_array[255][255]={0};//array will hold the output file name of any process that will output to a file

        if(dest[0] == '\0'){ //if the string is empty
            fflush(stdout);
            continue;
        }

        char *found; //used to get individual parts of the parsed string. if its null the loop breaks
        while( (found = strsep(&dest," \t")) != NULL ){
            if(strcmp("", found) == 0){ //if the curr string is a whitespace
                continue;
            }
            else if(strcmp("&", found) == 0){
                parra_bool = 1;
                out_bool = 0;
                continue;
            }
            else if(strcmp(">", found) == 0){
                out_bool = 1; //make redirection true
                output_array_bool[commands_arr_index] = 1;
                continue;
            }
            else{
                if(parra_bool){
                    commands_arr[commands_arr_index][index] = NULL;//null for execv
                    commands_arr_index++; //go to next array index
                    index = 0; //reset index
                    out_var_index = 0;//reset index for out var
                    commands_arr[commands_arr_index][index] = found; //add the command to the new array 
                    index++; //increment index
                    parra_bool = 0;//turn off flag


                }
                else if(out_bool){//if the redirection variable is true, then whatever arguments come after the > goes to the out_var
                    output_array_bool[commands_arr_index] = 1; //will add a flag at the index of that set of arguments that it will out to file
                    out_var_array[commands_arr_index][out_var_index] = found;//add the name of file to array
                    out_var_index++;
                }
                else{
                    commands_arr[commands_arr_index][index] = found; //cmmds_arr = {{"ls","-r"},
                                                                    //             {"ls","-t","/tmp"}}
                    index++;
                }
            }

        }

        if(commands_arr[0][0] == NULL && out_var_array[0][0] == NULL){ //if command line is empty
            fflush(stdout);
            continue;
        }
        else if(commands_arr[0][0] == NULL){ //if command line is empty BUT NOT OUT OUTPUT
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            fflush(stdout);
            continue;
        }


        char* command = commands_arr[0][0]; //contains the command 

        index = 0;//reset index vars
        out_var_index = 0;
        int command_len = commands_arr_index + 1; //copy over len of array of commands
        commands_arr_index = 0;
        int command_len_copy = command_len; //copy of len used for wait(). copied bc i will modify it 

       //****BUILT-IN COMMANDS***
        //exit command
        
        if(strcmp("exit", command) == 0 ){
            if(array_length(commands_arr[0])!=1){//if it has arguments it throws an error
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                fflush(stdout);
                continue;
            }
            free(buffer);
            free(dest);
            exit(0);
        }
        //cd command
        else if(strcmp("cd", command) == 0 ) { //if the size of the arguments is 2 then [comm, arg]
            if (chdir(commands_arr[0][1]) == 0 && (array_length(commands_arr[0])) == 2){
                chdir(commands_arr[0][1]);
                continue;
            }
            else{
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                fflush(stdout);
                continue;
            }
        }
        //path command
        else if(strcmp("path", command) == 0){
            if(array_length(commands_arr[0]) == 1){ //if there is only one argment in args array that means it was path by itself
                    path1[0] = ""; //initial path
            }
            else{ 
                for(int i = 0; i < 256; i++){//clear array from prev values
                    path1[i] = 0;
                }
                for(int i = 1; i < array_length(commands_arr[0]); i++){
                    path1[i-1] = commands_arr[0][i];
                }
            }
            continue;

        }


        //if the path is empty then dont allow execution of external commands
        if(strcmp(path1[0], "") != 0){
            int test; //var used to test if path is accessible
            int len = array_length(path1);

            char path_copy[20]; //var used to copy the path to so we can modify it
            pid_t p, cpid; 

            for(int j = 0; j < command_len; j++){
                command = commands_arr[j][0];//we make command equal to whatever command is in the given array for this iteration

                for(int i =0; i < len; i++)//we iterate through the array of paths and see which one allows to execute the given process
                {
                    //error for redirection
                    if(output_array_bool[j]){//check if redirect bool val is true
                        if(array_length(out_var_array[j]) != 1){ //check if theres only one output file after ">"
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            fflush(stdout);
                            continue;
                        }
                        else if(array_length(out_var_array[j]) == 0){
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            fflush(stdout);
                            continue;
                        }
                        else if(strcmp(">", out_var_array[j][0]) == 0){//check that the argument is not another '>'
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            fflush(stdout);
                            continue;
                        }
                    }

                    strcpy(path_copy, path1[i]);
                    strcat(path_copy, "/");
                    strcat(path_copy, command);

                    test = access(path_copy,X_OK); //we check if the given command at this index is accessible
                        
                    if(test == 0){ //if accessible, we make a fork call and use execv to execute the command with the given args, and the parent waits
                        pid_t p = fork(); //we fork to make the execv call

                        if(p == 0){ //if the current process is a child
                            if(output_array_bool[j]){
                                FILE *file;
                                file = fopen(out_var_array[j][0], "w");
                                dup2(fileno(file), STDOUT_FILENO);
                                fclose(file);
                                execv(path_copy,commands_arr[j]);
                                exit(0); //just in case execv returns bc of error
                            }

                            else{
                                execv(path_copy,commands_arr[j]);
                                exit(0);
                            }
                        }

                    }
                    else if(i == len -1){ //if we tried all paths and still no access print error message and continue
                        write(STDERR_FILENO, error_message, strlen(error_message)); 
                        fflush(stdout);
                        continue;
                    }
                    else{
                        continue;
                    }
                }
            }

            while(command_len_copy > 0){ //loop to wait for every fork to return 
                cpid = wait(NULL);
                command_len_copy--;
            }
        }
        else{
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        fflush(stdout);
        continue;
    }
    }

}