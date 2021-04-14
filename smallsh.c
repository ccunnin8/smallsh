#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_LENGTH

struct command {
    char *command;
    struct argument *arguments;
    int inputFile;
    int outputFile;
    bool background;
};

struct argument {
    char *argument;
    struct argument *nextArgument;
};

struct command parseInput(char *input) {
    struct command cmd= { NULL, NULL, 0, 0, false };
    struct argument oldArg = { NULL, NULL };
    struct argument *firstArg = &oldArg; 

    // store arguments to parse 
    char arguments[MAX_LENGTH] = { 0 };
    int argumentCount = 1;

    // get first 
    char *saveptr;
    char *token = strtok_r(input, " ", &saveptr);
    cmd.command = token;

    while (token = strtok_r(NULL, " ", &saveptr)) {
        if (argumentCount == 1) {
            oldArg.argument = token;
        } else {
            struct argument newArg = { token, NULL };
            oldArg.nextArgument = &newArg;
            oldArg = newArg;
            argumentCount++;
        }
    }
    cmd.arguments = firstArg; 

    return cmd;
}

int main(void) {
    while (1) {
        char userInput[MAX_LENGTH] = { 0 };
        printf(": ");
        scanf("%s", userInput);
        printf("%s", userInput);
        // struct command input = parseInput(userInput);
        // printf("Command: %s\n", input.command);
        // fflush(stdout);
    }
    return 0;
}