#include <stdio.h>
#include <stdlib.h>

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
    int fileStatus;

    FILE *inputFile = fopen(inputFilePath, "r");
    // Guarding for opening the input file. Causes early return and program termination.
    if (inputFile == NULL) {
        printf("ERROR: Could not open file %s\n", inputFilePath);
        return 1;
    }

    fileStatus = fseek(inputFile, 0, SEEK_END);
    // Guarding for moving file pointer to end of file so its size can be found. Causes early return and program termination.
    if (fileStatus !=0 ) {
        printf("ERROR: Could not seek to end of %s\n", inputFilePath);
        return 1;
    }

    inputFileSize = ftell(inputFile);
    // Guarding for moving file pointer back to the start of file so can now be read into mem. Causes early return and program termination.
    fileStatus = fseek(inputFile, 0, SEEK_SET);
    if (fileStatus !=0 ) {
        printf("ERROR: Could not seek back to start of %s\n", inputFilePath);
        return 1;
    }

    printf("File is %ld bytes\n", inputFileSize); //TODO: remove this
    cmds = malloc((inputFileSize + 1) * sizeof(char));

    size_t elmtRead = fread(cmds, sizeof(char), inputFileSize, inputFile);
    fclose(inputFile);
    printf("Read %lu chars\n", elmtRead); //TODO: remove this
    //TODO: Make a comparison from inputFileSize to elmtRead. Ensure data types make sense. Terminate if can't read whole file.

    //TODO: remove this
    printf("\n====FILE START====\n%s\n====EOF====\n", cmds);

    return 0;
}
