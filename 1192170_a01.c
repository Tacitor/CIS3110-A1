#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h> //TODO: Is this needed?

#define VALID_CHAR_HI 0x7e
#define VALID_CHAR_LO 0x20

void writeOutput(char* command, char* output)
{
	printf("The output of: %s : is\n", command);
	printf(">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);
}

int loadFile(char *inputFilePath, FILE *inputFile, char **cmdsPTR, long *size) {
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

    *size = ftell(inputFile);
    // Guarding for moving file pointer back to the start of file so can now be read into mem. Causes early return and program termination.
    returnSuccess = fseek(inputFile, 0, SEEK_SET);
    
    if (returnSuccess !=0 ) {
        printf("ERROR: Could not seek back to start of %s. Program Terminating.\n", inputFilePath);
        return 1;
    }

    printf("File is %ld bytes\n", *size); //TODO: remove this
    *cmdsPTR = malloc((*size + 1) * sizeof(char));

    size_t elmtRead = fread(*cmdsPTR, sizeof(char), *size, inputFile);

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
    long inputFileSize;

    int returnSuccess = loadFile(argv[1], inputFile, &cmds, &inputFileSize);

    if (returnSuccess) {
        return returnSuccess;
    }

    //TODO: remove this
    printf("====FILE START====\n%s\n====EOF====\n", cmds);

    //TODO: break up mega string into lines for each command. Then a sperate string of each arggument
    //TODO: Each line ends with a CR LF. Use the LF to split into new lines.

    //TODO: Loop through this string and check each line ends with a LF. Ensure there is an EOF too. Or use the inputFileSize to find the end if no EOF.
    //TODO: Don't need EOF since the file is in a char array now. The string is null terminated.

    //TODO: Move all this string splitting and arg formatting stuff into own function

    //Always start with one line since the first line doesn't have a LF before it.
    //Only exception is a blank file but there won't be any commands to run there anyway.
    int numLines = 1;
    for (int i = 0; i <= inputFileSize; i++) {
        //Count the LF chars. No need to count CR chars since there will always be a LF, not always a CR.
        if (cmds[i] == 10) {
            numLines++;
        }
    }

    int validCharPerLine[numLines];
    int spacesPerLine[numLines];
    char* intermediateSplitCMDS[numLines];
    memset(validCharPerLine, 0, sizeof(validCharPerLine));
    memset(spacesPerLine, 0, sizeof(spacesPerLine));

    //reset numLines to 0 now to use as an index
    numLines = 0;
    for (int i = 0; i <= inputFileSize; i++) {
        
        // Filter out control chars and spaces. When feeding into execlp() execvp() each string needs to have chars between
        // 0x21 (!) to 0x7e (~) (including both ends of range).
        // Do include spaces (0x20) still since they will need to be used later to split the line into args
        if (cmds[i] >= VALID_CHAR_LO && cmds[i] <= VALID_CHAR_HI) {
            validCharPerLine[numLines]++;

            //keep a seperate tally of the spaces
            if (cmds[i] == ' ') {
                spacesPerLine[numLines]++;
            } 
        } else if (cmds[i] == 10) {
            numLines++;
        }
    }

    // For each line keep track of how many char are in each arg delimited by spaces
    int *charsPerArg[numLines];

    for (int i = 0; i <= numLines; i++) {
        // Add one more for fencepost problem reasons
        charsPerArg[i] = calloc((spacesPerLine[i] + 1), sizeof(int));
    }

    int validCharLastSpace;
    int argNum;
    int charNum = 0;
    int lineLen = 0;
    //Read each line into its own string
    for (int i = 0; i <= numLines; i++) {

        //make array of validCharPerLine[i] + 1. This has validCharPerLine[i] chars and 1 null on the end
        lineLen = validCharPerLine[i] + 1;
        intermediateSplitCMDS[i] = malloc(sizeof(char) * (lineLen)); //get string of length validCharPerLine[i] + 1

        //Reset for every new line
        argNum = validCharLastSpace = 0;

        for (int j = 0; j < validCharPerLine[i]; j++) {
            intermediateSplitCMDS[i][j] = cmds[charNum]; //TODO: This assumes the first char is valid. Add a check for that I guess.

            if (cmds[charNum] == ' ') {
                charsPerArg[i][argNum++] = j - validCharLastSpace;

                validCharLastSpace = j + 1; // Add 1 more to j to skip the space itself
            }

            //advance past the non valid chars
            do { //TODO: This ONLY WORKS if the check here for a valid char is the same as above. Therfore turn this check into a function.
                charNum++;
            }
            while (!(cmds[charNum] >= VALID_CHAR_LO && cmds[charNum] <= VALID_CHAR_HI));
        }

        //account for arg after last space (or only one if there is no space)
        charsPerArg[i][argNum] = validCharPerLine[i] - validCharLastSpace;

        //terminate new string
        intermediateSplitCMDS[i][lineLen-1] = '\0';
    }

    for (int i = 0; i <= numLines; i++) {
        for (int j = 0; j <= spacesPerLine[i]; j++) {
            printf("[%d][%d] = %d\n", i, j, charsPerArg[i][j]);
        }
    }







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
        char* args[] = {"cat", "makefile", NULL};
        execvp(args[0], args);
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

    free(cmds);

    for (int i = 0; i <= numLines; i++) {
        free(intermediateSplitCMDS[i]);
        free(charsPerArg[i]);
    }

    return 0;
}
