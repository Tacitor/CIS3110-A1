#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define VALID_CHAR_HI 0x7e
#define VALID_CHAR_LO 0x20
#define CHILD_OUT_FILE_SIZE 16384 //TODO: Is there a better (dynamic) way to do this? Is there a better buffer size?

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

    *cmdsPTR = calloc((*size + 1), sizeof(char));
    size_t elmtRead = fread(*cmdsPTR, sizeof(char), *size, inputFile);
    fclose(inputFile);

    if (!(*size >= 0 && ((size_t)*size) == elmtRead)) {
        printf("ERROR: Bad read from %s. Size read does not match size of file. Program Terminating.\n", inputFilePath);
        return 1;
    }

    return 0;
}

int isCharValid(char c) {
    return (c >= VALID_CHAR_LO && c <= VALID_CHAR_HI);
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
    char *allCmds = NULL; //where commands go. Needs to be dynamically allocated once # of bytes/chars in file is known.
    long inputFileSize;

    int returnSuccess = loadFile(argv[1], inputFile, &allCmds, &inputFileSize);

    if (returnSuccess) {
        return returnSuccess;
    }

    /**
     * =-=-=-=Begin splitting of args=-=-=-=
     */

    //Always start with one line since the first line doesn't have a LF before it.
    //Only exception is a blank file but there won't be any commands to run there anyway.
    int numLines = 1;
    for (int i = 0; i <= inputFileSize; i++) {
        //Count the LF chars. No need to count CR chars since there will always be a LF, not always a CR.
        if (allCmds[i] == 10) {
            numLines++;
        }
    }

    int validCharPerLine[numLines];
    int spacesPerLine[numLines];
    char* linesCmds[numLines];
    memset(validCharPerLine, 0, sizeof(validCharPerLine));
    memset(spacesPerLine, 0, sizeof(spacesPerLine));

    //reset numLines to 0 now to use as an index
    numLines = 0;
    for (int i = 0; i <= inputFileSize; i++) {
        
        // Filter out control chars and spaces. When feeding into execlp() execvp() each string needs to have chars between
        // 0x21 (!) to 0x7e (~) (including both ends of range).
        // Do include spaces (0x20) still since they will need to be used later to split the line into args
        if (isCharValid(allCmds[i])) {
            validCharPerLine[numLines]++;

            //keep a seperate tally of the spaces
            if (allCmds[i] == ' ') {
                spacesPerLine[numLines]++;
            } 
        } else if (allCmds[i] == 10) {
            numLines++;
        }
    }

    // Add one back since it is done being an index and resumes its previous role of storing the number of lines.
    numLines++;

    // For each line keep track of how many char are in each arg delimited by spaces
    int *charsPerArg[numLines];

    for (int i = 0; i < numLines; i++) {
        // Add one more for fencepost problem reasons
        charsPerArg[i] = calloc((spacesPerLine[i] + 1), sizeof(int));
    }

    int validCharLastSpace;
    int argNum;
    int charNum = 0;
    int lineLen = 0;

    // Advance past the non valid chars if there are any
    while (charNum <= inputFileSize && !isCharValid(allCmds[charNum])) {
        charNum++;
    }

    //Read each line into its own string
    for (int i = 0; i < numLines; i++) {

        //make array of validCharPerLine[i] + 1. This has validCharPerLine[i] chars and 1 null on the end
        lineLen = validCharPerLine[i] + 1;
        linesCmds[i] = malloc(sizeof(char) * (lineLen)); //get string of length validCharPerLine[i] + 1

        //Reset for every new line
        argNum = validCharLastSpace = 0;

        for (int j = 0; j < validCharPerLine[i]; j++) {
            linesCmds[i][j] = allCmds[charNum];

            if (allCmds[charNum] == ' ') {
                charsPerArg[i][argNum++] = j - validCharLastSpace;

                validCharLastSpace = j + 1; // Add 1 more to j to skip the space itself
            }

            // Advance one and keep going past the non valid chars
            do {
                charNum++;
            }
            while (charNum <= inputFileSize && !isCharValid(allCmds[charNum]));
        }

        //account for arg after last space (or only one if there is no space)
        charsPerArg[i][argNum] = validCharPerLine[i] - validCharLastSpace;

        //terminate new string
        linesCmds[i][lineLen-1] = '\0';
    }

    // This single string of all commands is no longer needed and can be freed
    free(allCmds);

    // Finally now each line can be broken up into its args
    // An array of an array of strings. The top level array is the line/command.
    // The middle level is the string that makes up an arg within a given line/command.
    // The lowest level is the array of chars that is the string
    char **argsCmds[numLines];

    for (int i = 0; i < numLines; i++) {
        // Add on two more. The first extra char* is for fencepost reasons to get the last arg after the last space.
        // The second extra char* is to have a NULL char* to make execvp() happy.
        argsCmds[i] = malloc(sizeof(char*) * (spacesPerLine[i] + 2));

        charNum = 0;

        for (int j = 0; j < (spacesPerLine[i] + 1); j++) {
            argsCmds[i][j] = malloc(sizeof(char) * (charsPerArg[i][j] + 1)); //Add 1 to NULL terminate the string

            for (int k = 0; k < charsPerArg[i][j]; k++) {
                argsCmds[i][j][k] = linesCmds[i][charNum++];
            }

            //skip the space
            charNum++;

            //add in terminator to new string
            argsCmds[i][j][charsPerArg[i][j]] = '\0';
        }

        argsCmds[i][spacesPerLine[i] + 1] = NULL;
    }

    // This memory can now be released. Only linesCmds and argsCmds are needed from here on
    for (int i = 0; i < numLines; i++) {
        free(charsPerArg[i]);
    }

    /**       
     * =-=-=-= End splitting of args =-=-=-=
     */

    for (int i = 0; i < numLines; i++) {
        // Set up the pipe for sending the output of the child process back to the parent
        int pipeEnd[2];
        returnSuccess = pipe(pipeEnd);
        if (returnSuccess != 0 ) {
            printf("ERROR: Could not open a new pipe. Program Terminating.\n");
            return 1;
        }

        //fork the process and save the PID so it can be compared
        pid_t pid = fork();

        //The PID is 0 for the child process
        if (pid == 0) {
            // Close read end of the pipe for the child side
            close(pipeEnd[0]);

            // Take the standard out of this new child and point into into the write side of the pipe
            // This should also stop the output from showing in the terminal
            dup2(pipeEnd[1], STDOUT_FILENO);

            // Execule the command. This also effectivle terminated the child right at this point since the
            // process memory is swaped for that of the command. No need for a return.
            //Need the format of:
            //char* args[] = {"cat", "makefile", NULL};
            execvp(argsCmds[i][0], argsCmds[i]);

            // This should never be called but it doesn't hurt to have it
            close(pipeEnd[1]); // Close write end of the pipe for the child side
            exit(EXIT_SUCCESS);
        }

        // Close write end of the pipe for the parent side
        close(pipeEnd[1]);

        //Use the file descriptor to open the STDOUT of the child as a file in this parent
        char childOut[CHILD_OUT_FILE_SIZE + 1] = {0};

        //read from the file
        read(pipeEnd[0], childOut, CHILD_OUT_FILE_SIZE);

        // Now with the output of the child from the pipe stored into a string it can be send to the output function
        writeOutput(linesCmds[i], childOut);

        // Close read end of the pipe for the parent side
        close(pipeEnd[0]);

        int status;
        pid_t out = waitpid(pid, &status, 0);

        if (out != pid) {
            printf("ERROR: Wait condition failed for child PID %d for command %s\n", pid, linesCmds[i]);
        } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            printf("ERROR: PID %d for command %s exited, status=%d\n", pid, linesCmds[i], WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("ERROR: PID %d for command %s KILLED by signal %d\n", pid, linesCmds[i], WTERMSIG(status));
        }
    }

    // The final print is now done and this remaining memory can be freed.
    for (int i = 0; i < numLines; i++) {
        free(linesCmds[i]);

        for (int j = 0; j < (spacesPerLine[i] + 1); j++) {
            free(argsCmds[i][j]);
        }
        free(argsCmds[i]);
    }

    return 0;
}
