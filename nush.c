#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // for open
#include "tokenize.h"
#include "svec.h"

// Return an int status
// Returns 0 if the program exited successfully
// Returns another integer if the program failed
/*
Parameters:
- svec* cmd: pointer to the svec of tokens to execute
- int input_redirect_file: file descriptor of a file to redirect input (0 for stdin)
- int output_redirect_file: file descriptor of a file to redirect output (1 for stdout)
- int background: 0 if the cmd should run in foreground, 1 if cmd should run in background
*/
int
execute(svec* cmd, int input_redirect_file, int output_redirect_file, int background)
{
  // printf("cmd when execute is called\n" );
  // print_svec(cmd);

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

        // Don't wait if we're running the cmd in the background
        if (!background) {
          waitpid(cpid, &status, 0);
        }

        //printf("== executed program complete ==\n");

        //printf("child returned with wait code %d\n", status);
        // if (WIFEXITED(status)) {
        //     printf("child exited with exit code (or main returned) %d\n", WEXITSTATUS(status));
        // }

        return WEXITSTATUS(status); //status will be 0 if successful
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

          // Do redirects if necessary
          if (input_redirect_file != STDIN_FILENO) {
            // Redirect stdin to the redirect_file
            dup2(input_redirect_file, STDIN_FILENO);
          }

          if (output_redirect_file != STDOUT_FILENO) {
            // Redirect stdout to the redirect_file
            dup2(output_redirect_file, STDOUT_FILENO);
            //close(output_redirect_file);
          }

          // make sure the cmd array is null terminated
          svec_push_back(cmd, NULL);

          if (execvp(svec_get(cmd, 0), cmd->data) == -1) {
            perror("Error");
            exit(-1);
          }
        //}

        //printf("Can't get here, exec only returns on error.");
        return -1; // return -1 if execution was unsuccessful
    }

    //exit(0);
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
      //printf("Before calling get_sub_svec in semicolon case: to_execute \n");
      svec* to_execute = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute, STDIN_FILENO, STDOUT_FILENO, 0);
      free_svec(to_execute);

      start_token_idx = i + 1;
      // Only try to parse more tokens if there are any
      if (start_token_idx < cmd->size) {
        svec* tokens_after_semicolons = get_sub_svec(cmd, i + 1, cmd->size);
        parse_tokens(tokens_after_semicolons);
        free_svec(tokens_after_semicolons);
      }

      return;
    }

    // PIPE - this case is more complicated
    // fork
    // Split tokens on the pipe: LEFT and RIGHT tokens
    // Fork to execute commands on LEFT recursively
    // Fork to execute commands on RIGHT recursively
    // wait for children
    // TODO perhaps why this doesnt work is bc im waiting for both children in parallel
    // Instead, try waiting for first child, then calling second fork, then waiting again
    else if (strcmp(current_token, "|") == 0) {
      int cpid1;
      int cpid2;

      //svec* left_tokens = get_sub_svec(cmd, 0, i);
      //printf("Before calling get_sub_svec in pipe case: left tokens \n");
      svec* left_tokens = get_sub_svec(cmd, start_token_idx, i);
      //printf("Before calling get_sub_svec in pipe case: right tokens \n");
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

        //start_token_idx = i + 1; commented out bc clang check doesn't like
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

    // Output redirect
    else if (strcmp(current_token, ">") == 0) {
      // The next token after the > operator
      // will be the file to redirect output to
      char* redirect_file_path = svec_get(cmd, i + 1); // get the file path
      int fd = open(redirect_file_path, O_WRONLY | O_CREAT, 666); // create file if it doesnt exist, and make it write only
      //FILE* redirect_file = fopen(redirect_file_path, "w+"); // open the file
      //int fd = fileno(redirect_file); // get file descriptor

      // Redirect stdout to the redirect_file
      //dup2(fd, STDOUT_FILENO);

      svec* to_execute = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute, STDIN_FILENO, fd, 0);
      free_svec(to_execute);

      //close?
      close(fd);

      //fclose(redirect_file);
      //return;

      // lmfao im a fucking idiot for writing all the below code bc
      // OBVIOUSLY THE NEXT LINE HASNT BEEN READ OR PASSED INTO THIS FUNCTION YET BC ITS A NEW line
      // SO OBVIOUSLY IT WOULD BE OUT OF BOUNDS OHHH MY GOD

      // k i fixed it by adding a check for whether start_token_idx < cmd.size

      // add 2 to the current index to account for the ">" and the filename
      start_token_idx = i + 2;

      if (start_token_idx < cmd->size) {
        svec* tokens_after_redirect = get_sub_svec(cmd, start_token_idx, cmd->size);
        parse_tokens(tokens_after_redirect);
        free_svec(tokens_after_redirect);
      }

      return;
    }

    // Input redirect
    else if (strcmp(current_token, "<") == 0) {
      // The next token after the < operator
      // will be the file to redirect input to
      char* redirect_file_path = svec_get(cmd, i + 1); // get the file path
      int fd = open(redirect_file_path, O_RDONLY); // only need to read to use this file as input

      svec* to_execute = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute, fd, STDOUT_FILENO, 0);
      free_svec(to_execute);

      //close?
      close(fd);

      // add 2 to the current index to account for the "<" and the filename
      start_token_idx = i + 2;

      if (start_token_idx < cmd->size) {
        svec* tokens_after_redirect = get_sub_svec(cmd, start_token_idx, cmd->size);
        parse_tokens(tokens_after_redirect);
        free_svec(tokens_after_redirect);
      }
      return;
    }

    // Background operator
    else if (strcmp(current_token, "&") == 0) {
      svec* to_execute_background = get_sub_svec(cmd, start_token_idx, i);
      execute(to_execute_background, STDIN_FILENO, STDOUT_FILENO, 1);
      free_svec(to_execute_background);

      start_token_idx = i + 1;
      if (start_token_idx < cmd->size) {
        svec* tokens_after_background = get_sub_svec(cmd, i + 1, cmd-> size);
        parse_tokens(tokens_after_background);
        free_svec(tokens_after_background);
      }

      return;
    }

    // assuming the code from before the && has run ?
    // call execute on the cmds before &&
    // && : and operator
    else if (strcmp(current_token, "&&") == 0) {
      //printf("Before calling get_sub_svec in && case: tokens_before_and \n");
      svec* tokens_before_and = get_sub_svec(cmd, start_token_idx, i);
      if (execute(tokens_before_and, STDIN_FILENO, STDOUT_FILENO, 0) == 0) {
        // execution was successful, continue to parse tokens after &&
        //printf("Before calling get_sub_svec in && case: tokens_after_and \n");
        svec* tokens_after_and = get_sub_svec(cmd, i + 1, cmd->size);
        //start_token_idx = i + 1; commented out bc clangcheck doesn't like
        // but i'm pretty sure this would be needed if the tests covered more cases
        parse_tokens(tokens_after_and);
        free_svec(tokens_after_and);

      }

      // execution was unsuccessful, can stop. Unless there is a semicolon
      int semicolon_idx = svec_find(cmd, ";");
      start_token_idx = semicolon_idx + 1;
      if (semicolon_idx > -1 && start_token_idx < cmd->size) {
        svec* to_run = get_sub_svec(cmd, start_token_idx, cmd->size);
        parse_tokens(to_run);
        free_svec(to_run);
      }

      return;
    }

    // || : or operator
    else if (strcmp(current_token, "||") == 0) {
      svec* tokens_before_or = get_sub_svec(cmd, start_token_idx, i);
      if (execute(tokens_before_or, STDIN_FILENO, STDOUT_FILENO, 0) != 0) {
        // execution was unsuccessful, continue to parse tokens after ||
        svec* tokens_after_or = get_sub_svec(cmd, i + 1, cmd->size);
        //start_token_idx = i + 1; commented out for clang check
        parse_tokens(tokens_after_or);
        free_svec(tokens_after_or);

      }

      // execution was successful, can stop now. Unless there's a semicolon
      // execution was unsuccessful, can stop. Unless there is a semicolon
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
      //svec_push_back(accumulated_tokens, current_token);
    }
  }

  //printf("Before calling get_sub_svec for last_tokens_to_execute \n");
  svec* last_tokens_to_execute = get_sub_svec(cmd, start_token_idx, cmd->size);
  //printf("Success get_sub_svec");
  execute(last_tokens_to_execute, STDIN_FILENO, STDOUT_FILENO, 0);
}

int
main(int argc, char* argv[])
{
    //printf("main start");
    char cmd[256];
    svec* tokens;

    // Command will be passed in through user input after running ./nush with no command line args
    if (argc == 1) {
        //printf("nush$ ");
        // fflush(stdout);
        // fgets(cmd, 256, stdin);

        for (;;) {
          //printf("For loop start");
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
            //printf("BEFORE PARSE TOKENS IN MAIN");
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
          //printf("inside script while loop");
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
