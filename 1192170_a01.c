#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h> //TODO: Is this needed?

void writeOutput(char* command, char* output)
{
	printf("The output of: %s : is\n", command);
	printf(">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);
}

int main(int argc, char **argv) {

    // Guarding for CLI args. Causes early return and program termination.
    if (argc < 2) {
        printf("ERROR: You must provide an input file as a CLI argument. Program Terminating.\n");
        return 1;
    } else if (argc > 2) {
        printf("ERROR: You must provide ONLY an input file as a CLI argument. No other flags or arguments are allowed. Program Terminating.\n");
        return 1;
    }

    // Set up file for reading
    char *inputFilePath = argv[1];
    char *cmds; //where commands go. Needs to be dynamically allocated once # of bytes/chars in file is known.
    long inputFileSize;
    int returnSuccess;

    FILE *inputFile = fopen(inputFilePath, "r");
    // Guarding for opening the input file. Causes early return and program termination.
    if (inputFile == NULL) {
        printf("ERROR: Could not open file %s. Program Terminating.\n", inputFilePath);
        return 1;
    }

    returnSuccess = fseek(inputFile, 0, SEEK_END);
    // Guarding for moving file pointer to end of file so its size can be found. Causes early return and program termination.
    if (returnSuccess !=0 ) {
        printf("ERROR: Could not seek to end of %s. Program Terminating.\n", inputFilePath);
        return 1;
    }

    inputFileSize = ftell(inputFile);
    // Guarding for moving file pointer back to the start of file so can now be read into mem. Causes early return and program termination.
    returnSuccess = fseek(inputFile, 0, SEEK_SET);
    if (returnSuccess !=0 ) {
        printf("ERROR: Could not seek back to start of %s. Program Terminating.\n", inputFilePath);
        return 1;
    }

    //printf("File is %ld bytes\n", inputFileSize); //TODO: remove this
    cmds = malloc((inputFileSize + 1) * sizeof(char));

    size_t elmtRead = fread(cmds, sizeof(char), inputFileSize, inputFile);
    elmtRead++; //TODO: Remove this silly line
    elmtRead--; //TODO: Remove this silly line

    fclose(inputFile);
    //printf("Read %lu chars\n", elmtRead); //TODO: remove this
    //TODO: Make a comparison from inputFileSize to elmtRead. Ensure data types make sense. Terminate if can't read whole file.

    //TODO: remove this
    //printf("\n====FILE START====\n%s\n====EOF====\n", cmds);

    //TODO: break up mega string into lines for each command. Then a sperate string of each arggument
    //TODO: Each line ends with a CR LF. Use the LF to split into new lines.

    // Set up the pipe for sending the output of the child process back to the parent
    int pipeEnd[2];
    returnSuccess = pipe(pipeEnd);
    if (returnSuccess != 0 ) {
        printf("ERROR: Could not open a new pipe. Program Terminating.\n");
        return 1;
    }

    //fork the process and save the PID so it can be compared
    int pid = fork();

    //The PID is 0 for the child process
    if (pid == 0) {
        // Take the standard out of this new child and point into into the write side of the pipe
        // This should also stop the output from showing in the terminal
        dup2(pipeEnd[1], STDOUT_FILENO);

        // Close both ends of the pipe on the child side
        close(pipeEnd[0]);
        close(pipeEnd[1]);

        // Execule the command. This also effectivle terminated the child right at this point since the
        // process memory is swaped for that of the command. No need for a return.
        execlp("ps", "ps", NULL);
    }

    //Use the file descriptor to open the STDOUT of the child as a file in this parent
    FILE *childOutFile = fdopen(pipeEnd[0], "r");
    long childOutFileSize = 128; //TODO: stop hardcoding this and try reading the file until EOF is reached. Or try to find a way to seek to the end of the file.
    char childOut[childOutFileSize + 1];
    // Guarding for opening the output of the child file. Causes early return and program termination.
    if (childOutFile == NULL) {
        printf("ERROR: Could not read from pipe. Program Terminating.\n");
        return 1;
    }
    
    //read from the file
    fread(childOut, sizeof(char), childOutFileSize, childOutFile);
    
    printf("====FILE START====\n%s\n====EOF====\n", childOut); //TODO: remove this


    fclose(childOutFile);

    // Close both ends of the pipe on the parent side
    close(pipeEnd[0]);
    close(pipeEnd[1]);
    
    //TODO: need waitpid?
    //waitpid();

    return 0;
}
