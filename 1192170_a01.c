#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h> //TODO: Is this needed?

void writeOutput(char* command, char* output)
{
	printf("The output of: %s : is\n", command);
	printf(">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);
}

int loadFile(char *inputFilePath, FILE *inputFile, char **cmdsPTR) {
    // Set up file for reading
    long inputFileSize;
    int returnSuccess;

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

    printf("File is %ld bytes\n", inputFileSize); //TODO: remove this
    *cmdsPTR = malloc((inputFileSize + 1) * sizeof(char));

    size_t elmtRead = fread(*cmdsPTR, sizeof(char), inputFileSize, inputFile);

    fclose(inputFile);
    printf("Read %lu chars\n", elmtRead); //TODO: remove this
    //TODO: Make a comparison from inputFileSize to elmtRead. Ensure data types make sense. Terminate if can't read whole file.

    return 0;
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

    FILE *inputFile = fopen(argv[1], "r");
    char *cmds; //where commands go. Needs to be dynamically allocated once # of bytes/chars in file is known.

    int returnSuccess = loadFile(argv[1], inputFile, &cmds);

    if (returnSuccess) {
        return returnSuccess;
    }

    //TODO: remove this
    printf("====FILE START====\n%s\n====EOF====\n", cmds);

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
        execlp("cat", "cat", "makefile", NULL);
    }

    //Use the file descriptor to open the STDOUT of the child as a file in this parent
    long childOutFileSize = 16384; //TODO: Is there a better (dynamic) way to do this? Is there a better buffer size?
    char childOut[childOutFileSize + 1];

    //read from the file
    read(pipeEnd[0], childOut, childOutFileSize);    
    
    printf("====FILE START====\n%s====EOF====\n", childOut); //TODO: remove this

    // Close both ends of the pipe on the parent side
    close(pipeEnd[0]);
    close(pipeEnd[1]);
    
    //TODO: need waitpid?
    //waitpid();

    return 0;
}
