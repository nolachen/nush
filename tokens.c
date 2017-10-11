// Nola Chen
// CS 3650 - HW04

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "svec.h"

/*
Helper function to empty a given string
 */
void
empty_string(char* str)
{
    // Reset each character in the string to 0
    for (int i = 0; i < strlen(str); ++i) {
        str[i] = 0;
    }

    // Set the null terminator to the first character
    str[0] = '\0';
}

/*
Helper method for tokenize, to abstract out identical code:
- IF current_token is a nonempty string,
- Adds the given current_token string to the given tokens svec
- And then resets the string 
 */
void
add_nonempty_string_to_svec(char* current_token, svec* tokens)
{
    if (strlen(current_token) > 0) {
        add_to_end(tokens, current_token);
        empty_string(current_token);
    }
}

/*
Tokenize the given string input command. Returns a svec*, a list of string tokens.
Recognizes the following types of tokens:
- Command names
- Command arguments
- Shell operators (<, >, |, &, &&, ||, ;)
- Non-command operator arguments
 */
svec*
tokenize(char* command)
{
    // Initialize an empty svec to store the tokens
    svec* tokens = svec_constructor();

    // Initialize the current_token to the empty string
    // Set size to 101 * sizeof(char) to account for the null terminator
    char* current_token = malloc(101 * sizeof(char));
    current_token[0] = '\0';

    // Iterate through each character of the input command
    for (int i = 0; i < strlen(command); ++i) {
        // A space character indicates a new token
        if (isspace(command[i])) {
            // Add current_token to tokens svec and reset current_token
            add_nonempty_string_to_svec(current_token, tokens);
        }

        // || and && have two characters in their operators
        else if ((i < strlen(command) - 1) &&
            ((command[i] == '|' && command[i + 1] == '|') || (command[i] == '&' && command[i + 1] == '&'))) {

            // Add current_token to tokens svec and reset current_token
            add_nonempty_string_to_svec(current_token, tokens);

            // Add the two-character operator string to the tokens svec
            strncat(current_token, &command[i], 2);
            add_nonempty_string_to_svec(current_token, tokens);

            // Skip the next character because we handled it already
            i += 1;
        }

        // The one character operators are: <, >, |, &, ;
        else if (command[i] == '<' || command[i] == '>' || command[i] == '|' || command[i] == '&' || command[i] == ';') {

            // Add the current_token to the tokens svec and reset current_token
            add_nonempty_string_to_svec(current_token, tokens);

            // Add the one-character operator string to the tokens svec
            strncat(current_token, &command[i], 1);
            add_nonempty_string_to_svec(current_token, tokens);
        }

        // If it's a normal character, just add it to the current_token
        else {
            strncat(current_token, &command[i], 1);
        }

    }

    // Free the memory allocated for the current_token string
    free(current_token);

    return tokens;

}

int
main(int _argc, char* _argv[])
{
    char command[100]; // declare a character array to store the input command

    for (;;) {
        // Print input prompt
        printf("tokens$ ");
        char* return_value = fgets(command, sizeof(command), stdin);
        
        // If the return value of fgets is 0, then we've reached end of file
        if (return_value == 0) {
            break;
        }

        // Call method tokenize to return a svec of tokens
        svec* tokens = tokenize(command);

        // Print the tokens in the list
        print_svec(tokens);

        // Free the memory allocated for the tokens svec
        free_svec(tokens);
    }

    return 0;
}
