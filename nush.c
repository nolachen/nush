#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // for open
#include "tokenize.h"
#include "svec.h"

/*
Return an int status: 0 if the program exited successfully, another integer if the program failed
Parameters:
- svec* cmd: pointer to the svec of tokens to execute
- int input_redirect_file: file descriptor of a file to redirect input (0 for stdin)
- int output_redirect_file: file descriptor of a file to redirect output (1 for stdout)
- int background: 0 if the cmd should run in foreground, 1 if cmd should run in background
*/
int
execute(svec* cmd, int input_redirect_file, int output_redirect_file, int background)
{
    // if the command is cd
    if (strcmp(svec_get(cmd, 0), "cd") == 0) {
      int rv;
      if (cmd->size == 1) {
        rv = chdir(getenv("HOME"));
      } else {
        rv = chdir(svec_get(cmd, 1));
      }
      return rv;
    }

    // if the command is exit
    if (strcmp(svec_get(cmd, 0), "exit") == 0) {
      exit(0);
    }

    int cpid;

    if ((cpid = fork())) {
        // parent process
        int status;

        // Don't wait if we're running the cmd in the background
        if (!background) {
          // Wait for the child to finish running
          waitpid(cpid, &status, 0);
        }

        return WEXITSTATUS(status); //status will be 0 if successful
    }
    else {
      // child process

      // Do redirects if necessary
      if (input_redirect_file != STDIN_FILENO) {
        // Redirect stdin to the redirect_file
        dup2(input_redirect_file, STDIN_FILENO);
      }
      if (output_redirect_file != STDOUT_FILENO) {
        // Redirect stdout to the redirect_file
        dup2(output_redirect_file, STDOUT_FILENO);
      }

      // Make sure the cmd array is null terminated
      svec_push_back(cmd, NULL);

      // execvp only returns -1 if it failed. If so, print out the error and exit the child process.
      if (execvp(svec_get(cmd, 0), cmd->data) == -1) {
        perror("Error");
        exit(-1);
      }

      return -1; // return -1 if execution failed
    }
}

/*
Helper method that parses the tokens svec and checks for operators/symbols
*/
void
parse_tokens(svec* cmd) {
  // Iterate through the tokens,
  // Until you hit an operator
  // Then run the commands between the last operator and current operator
  // (current starting token index is kept track of in start_token_idx)

  int start_token_idx = 0; // store the index of the token where we want to start executing commands

  for (int i = 0; i < cmd->size; ++i) {
    char* current_token = svec_get(cmd, i);

    // Semicolon ;
    // If we hit a semicolon, execute the accumulated commands beginning from start_token_idx
    if (strcmp(current_token, ";") == 0) {
      // First execute the tokens from start_token_idx to i
      svec* to_execute = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute, STDIN_FILENO, STDOUT_FILENO, 0);
      free_svec(to_execute);

      // Set the start_token_idx to the current index + 1 (the token after the ;)
      start_token_idx = i + 1;

      // Only try to parse more tokens if there are any
      if (start_token_idx < cmd->size) {
        svec* tokens_after_semicolons = get_sub_svec(cmd, i + 1, cmd->size);
        parse_tokens(tokens_after_semicolons);
        free_svec(tokens_after_semicolons);
      }

      // Return, because parse_tokens is already recursively called for the rest of the tokens
      return;
    }

    // PIPE | - this case is more complicated
    // Split tokens on the pipe: LEFT and RIGHT tokens
    // Fork to execute commands on LEFT recursively
    // Fork to execute commands on RIGHT recursively
    // Wait for children
    else if (strcmp(current_token, "|") == 0) {
      int cpid1;
      int cpid2;

      svec* left_tokens = get_sub_svec(cmd, start_token_idx, i);
      svec* right_tokens = get_sub_svec(cmd, i + 1, cmd->size);

      int pipefd[2];

      // pipefd = pipe file descriptors
      // pipefd[0] is read end of pipe
      // pipefd[1] is the write end of the pipe
      // data written to write end of pipe (pipefd[1]) is buffered until it is read by the read end
      // left : write end
      // right : read end
      pipe(pipefd);

      // left side - write - pipefd[1]
      if (!(cpid1 = fork())) {
        // child 1
        close(pipefd[0]); // close read end

        dup2(pipefd[1], STDOUT_FILENO); // creates a copy of the write end into the stdout file descriptor

        // Make sure the token array is null terminated
        svec_push_back(left_tokens, NULL);

        execvp(svec_get(left_tokens, 0), left_tokens->data);
        // Should never get here
      }

      // right side - read - pipefd[0]
      if (!(cpid2 = fork())) {
        // child 2
        close(pipefd[1]); // close write end

        dup2(pipefd[0], STDIN_FILENO); // creates a copy of the read end into the stdin file descriptor

        parse_tokens(right_tokens);

        // Exit out of child process
        _exit(0);
      }

      int status1; // status of left side, child 1
      int status2; // status of right side, child 2

      // close the read and write pipes
      close(pipefd[0]);
      close(pipefd[1]);

      // wait for the left and right sides to finish
      waitpid(cpid1, &status1, 0);
      waitpid(cpid2, &status2, 0);

      free_svec(left_tokens);
      free_svec(right_tokens);

      return;
    }

    // Output redirect >
    else if (strcmp(current_token, ">") == 0) {
      // The next token after the > operator will be the file to redirect output to
      char* redirect_file_path = svec_get(cmd, i + 1); // get the file path
      // create file if it doesnt exist, and make it read and write
      int fd = open(redirect_file_path, O_RDWR | O_CREAT | O_TRUNC, 0666);

      // Execute the tokens, passing in the file descriptor of the redirect file
      svec* to_execute = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute, STDIN_FILENO, fd, 0);
      free_svec(to_execute);

      // close the file
      close(fd);

      // Add 2 to the current index to account for the ">" and the filename
      start_token_idx = i + 2;

      // Only parse the next tokens if they exist
      if (start_token_idx < cmd->size) {
        svec* tokens_after_redirect = get_sub_svec(cmd, start_token_idx, cmd->size);
        parse_tokens(tokens_after_redirect);
        free_svec(tokens_after_redirect);
      }

      return;
    }

    // Input redirect <
    else if (strcmp(current_token, "<") == 0) {
      // The next token after the < operator will be the file to redirect input to
      char* redirect_file_path = svec_get(cmd, i + 1); // get the file path
      int fd = open(redirect_file_path, O_RDONLY); // only need to read to use this file as input

      // Execute the tokens, passing in the file descriptor of the input redirect file
      svec* to_execute = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute, fd, STDOUT_FILENO, 0);
      free_svec(to_execute);

      //close file
      close(fd);

      // Add 2 to the current index to account for the "<" and the filename
      start_token_idx = i + 2;

      // Parse next tokens if they exist
      if (start_token_idx < cmd->size) {
        svec* tokens_after_redirect = get_sub_svec(cmd, start_token_idx, cmd->size);
        parse_tokens(tokens_after_redirect);
        free_svec(tokens_after_redirect);
      }
      return;
    }

    // Background operator &
    else if (strcmp(current_token, "&") == 0) {
      // Execute the accumulated tokens passing in a background flag
      svec* to_execute_background = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute_background, STDIN_FILENO, STDOUT_FILENO, 1);
      free_svec(to_execute_background);

      // Parse tokens after the &
      start_token_idx = i + 1;
      if (start_token_idx < cmd->size) {
        svec* tokens_after_background = get_sub_svec(cmd, i + 1, cmd-> size);
        parse_tokens(tokens_after_background);
        free_svec(tokens_after_background);
      }

      return;
    }

    // And operator &&
    else if (strcmp(current_token, "&&") == 0) {
      svec* tokens_before_and = get_sub_svec(cmd, start_token_idx, i);
      // Execute the accumulated tokens to see if it was successful
      if (execute(tokens_before_and, STDIN_FILENO, STDOUT_FILENO, 0) == 0) {
        // execution was successful, continue to parse tokens after &&
        svec* tokens_after_and = get_sub_svec(cmd, i + 1, cmd->size);
        parse_tokens(tokens_after_and);
        free_svec(tokens_after_and);
      }

      // Execution was unsuccessful, can stop. Unless there is a semicolon
      int semicolon_idx = svec_find(cmd, ";");
      start_token_idx = semicolon_idx + 1;
      if (semicolon_idx > -1 && start_token_idx < cmd->size) {
        svec* to_run = get_sub_svec(cmd, start_token_idx, cmd->size);
        parse_tokens(to_run);
        free_svec(to_run);
      }

      return;
    }

    // Or operator ||
    else if (strcmp(current_token, "||") == 0) {
      svec* tokens_before_or = get_sub_svec(cmd, start_token_idx, i);
      // Execute the accumulated tokens to see if it was successful
      if (execute(tokens_before_or, STDIN_FILENO, STDOUT_FILENO, 0) != 0) {
        // execution was unsuccessful, continue to parse tokens after ||
        svec* tokens_after_or = get_sub_svec(cmd, i + 1, cmd->size);
        parse_tokens(tokens_after_or);
        free_svec(tokens_after_or);
      }

      // execution was successful, can stop now. Unless there's a semicolon
      int semicolon_idx = svec_find(cmd, ";");
      start_token_idx = semicolon_idx + 1;
      if (semicolon_idx > -1 && start_token_idx < cmd->size) {
        svec* to_run = get_sub_svec(cmd, start_token_idx, cmd->size);
        parse_tokens(to_run);
        free_svec(to_run);
      }

      return;
    }

    // The current token is not an operator/special symbol
    // So just continue
    else {
      continue;
    }
  }

  // Execute the last tokens (after all operators have been found)
  svec* last_tokens_to_execute = get_sub_svec(cmd, start_token_idx, cmd->size);
  execute(last_tokens_to_execute, STDIN_FILENO, STDOUT_FILENO, 0);
  free_svec(last_tokens_to_execute);
}

int
main(int argc, char* argv[])
{
    char cmd[256];
    svec* tokens;

    // Command will be passed in through user input after running ./nush with no command line args
    if (argc == 1) {
        for (;;) {
          // Print input prompt
          printf("nush$ ");
          fflush(stdout);

          char* return_value = fgets(cmd, 256, stdin);

          // If the return value of fgets is 0, then we've reached end of file
          if (return_value == 0) {
            return 0;
          }

          // Call method tokenize to return a svec of tokens
          tokens = tokenize(cmd);
          // Only execute if the user has entered a command
          if (tokens->size > 0) {
            parse_tokens(tokens);
          }
          free_svec(tokens);
        }
    }
    // Script file is passed in as the first command line arg
    else {
      FILE* script = fopen(argv[1], "r");
      if (script != NULL) {
        while (fgets(cmd, 256, script) != NULL) {
          tokens = tokenize(cmd);
          parse_tokens(tokens);
          free_svec(tokens);
        }
      } else {
        // script file is null
        return 0;
      }
      fclose(script);
    }

    return 0;
}
