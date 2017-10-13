#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tokenize.h"
#include "svec.h"

// Return an int status
// Returns 0 if the program exited successfully
// Returns another integer if the program failed
int
execute(svec* cmd)
{
    char str_or[] = "||";
    char str_and[] = "&&";


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
        //printf("Parent pid: %d\n", getpid());
        //printf("Parent knows child pid: %d\n", cpid);

        // Child may still be running until we wait.

        int status;
        waitpid(cpid, &status, 0);

        //printf("== executed program complete ==\n");

        //printf("child returned with wait code %d\n", status);
        // if (WIFEXITED(status)) {
        //     printf("child exited with exit code (or main returned) %d\n", WEXITSTATUS(status));
        // }

        return status; //status will be 0 if successful
    }
    else {
        // child process
        //printf("Child pid: %d\n", getpid());
        //printf("Child knows parent pid: %d\n", getppid());

        // for (int ii = 0; ii < strlen(cmd); ++ii) {
        //     if (cmd[ii] == ' ') {
        //         cmd[ii] = 0;
        //         break;
        //     }
        // }

        // The argv array for the child.
        // Terminated by a null pointer.
        //char* args[] = {cmd, "one", 0};

        //printf("== executed program's output: ==\n");
        // printf("tokens: ");
        // print_svec(cmd);
        // printf("end tokens");
        // printf("first element of tokens");
        // printf("%s",svec_get(cmd, 0));

        //if (strcmp())

        //else {
          // Execute the command
          // Print the error if there is one
          if (execvp(svec_get(cmd, 0), cmd->data) == -1) {
            perror("Error: ");
          }
        //}

        printf("Can't get here, exec only returns on error.");
        return -1; // return -1 if execution was unsuccessful
    }
}

/*
Helper method that parses the tokens and checks for operators/symbols
*/
void
parse_tokens(svec* cmd) {
  // Attempt to rewrite this whole function L O L:
  // svec* current_command
  // for (int i = 0; i < cmd->size; ++i) {
  //
  // }





  // First iterates through to just check for semicolons

  // Iterate through the tokens,
  // adding the current ones to a new svec
  // Until you hit an operator
  // Then run the commands you have accumulated
  // Save the result (status)
  // somehow continue running the rest, with the saved status in mind ???
  //
  // Need to first check semicolons, bc semicolons basically is like, run new command
  //
  // Create a function for each "operator"
  // - takes in params: start & end idx of the tokens to run in the args
  // - other param: the whole args svec
  // need args bc we need to run commands after the operator depending on the operator
  int start_token_idx = 0; // store the index of the token where we want to start executing commands
  // TODO mayb delete  this accumulated_tokens lol
  //svec* accumulated_tokens; // store a svec of tokens that need to be run as a command once the next operator is hit

  for (int i = 0; i < cmd->size; ++i) {
    char* current_token = svec_get(cmd, i);
    //svec* current_subcommand = make_svec();
    // If we hit a semicolon, execute the accumulated commands

    // jk gonnna try this first: parse for ALL operators and symbols
    if (strcmp(current_token, ";") == 0) {
      // First execute the tokens from start_token_idx to i
      svec* to_execute = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute);
      free_svec(to_execute);

      svec* tokens_after_semicolons = get_sub_svec(cmd, i + 1, cmd->size);
      parse_tokens(tokens_after_semicolons);
      start_token_idx = i + 1;
      free_svec(tokens_after_semicolons);
      return;
    }

    // PIPE - this case is more complicated
    // fork
    // Split tokens on the pipe: LEFT and RIGHT tokens
    // Fork to execute commands on LEFT recursively
    // Fork to execute commands on RIGHT recursively
    // wait for children
    else if (strcmp(current_token, "|") == 0) {
      int cpid1;
      int cpid2;

      //svec* left_tokens = get_sub_svec(cmd, 0, i);
      svec* left_tokens = get_sub_svec(cmd, start_token_idx, i);
      svec* right_tokens = get_sub_svec(cmd, i + 1, cmd->size);

      int pipefd[2];

      // pipefd = pipe file descriptors
      // pipefd[0] is read end of pipe
      // pipefd[1] is the write end of teh pipe
      // data written to write end of pipe (pipefd[1]) is buffered until it is read by the read end
      // left : write end
      // right : read end
      pipe(pipefd);

      // left side - write - pipefd[1]
      if (!(cpid1 = fork())) {
        // child 1
        close(pipefd[0]); // close read end

        dup2(pipefd[1], STDOUT_FILENO);

        execvp(svec_get(left_tokens, 0), left_tokens->data);
        // Should never get here
      }

      // right side - read - pipefd[0]
      if (!(cpid2 = fork())) {
        // child 2
        close(pipefd[1]); // close write end

        dup2(pipefd[0], STDIN_FILENO);

        start_token_idx = i + 1;
        parse_tokens(right_tokens);

        _exit(0);

      }

      int status1;
      int status2;

      close(pipefd[0]);
      close(pipefd[1]);

      waitpid(cpid1, &status1, 0);
      waitpid(cpid2, &status2, 0);

      free_svec(left_tokens);
      free_svec(right_tokens);

      return;

    }

    // assuming the code from before the && has run ?
    // call execute on the cmds before &&
    else if (strcmp(current_token, "&&") == 0) {
      svec* tokens_before_and = get_sub_svec(cmd, start_token_idx, i);
      if (execute(tokens_before_and) == 0) {
        // execution was successful, continue to parse tokens after &&
        svec* tokens_after_and = get_sub_svec(cmd, i + 1, cmd->size);
        start_token_idx = i + 1;
        parse_tokens(tokens_after_and);
        free_svec(tokens_after_and);

      } else {
        // execution was unsuccessful, can stop now
        return;
      }
    }

    // The current token is not an operator/special symbol
    // So just continue
    else {
      continue;
      //svec_push_back(accumulated_tokens, current_token);
    }
  }

  svec* last_tokens_to_execute = get_sub_svec(cmd, start_token_idx, cmd->size);
  execute(last_tokens_to_execute);

}

int
main(int argc, char* argv[])
{
    char cmd[256];
    svec* tokens;

    // Command will be passed in through user input after running ./nush with no command line args
    if (argc == 1) {
        //printf("nush$ ");
        // fflush(stdout);
        // fgets(cmd, 256, stdin);

        for (;;) {
          // Print input prompt
          printf("nush$ ");
          fflush(stdout);

          char* return_value = fgets(cmd, 256, stdin);

          // If the return value of fgets is 0, then we've reached end of file
          if (return_value == 0) {
            //printf("End of file");
            return 0;
            //break;
          }

          // Call method tokenize to return a svec of tokens
          tokens = tokenize(cmd);
          // Only execute if the user has entered a command
          if (tokens->size > 0) {
            parse_tokens(tokens);
          }
          //TODO idk
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
          //TODO idk
          free_svec(tokens);
        }
      } else {
        // script file is null
        return 0;
      }
      fclose(script);

      // svec* tokens = make_svec();
      // for (int i = 1; i < argc; ++i) {
      //   svec_push_back(tokens, argv[i]);
      // }

      //execute(tokens);
    }

    // Free the memory allocated for the tokens svec
    //free_svec(tokens);
    return 0;
}
