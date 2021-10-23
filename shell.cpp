/**
 *Shell
 *Operating Systems
 *v20.08.28
 */

/**
	Hint: Control-click on a functionname to go to the definition
	Hint: Ctrl-space to auto complete functions and variables
	*/

// function/class definitions
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <vector>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

// although it is good habit, you don't have to type 'std' before many objects by including this line
using namespace std;

//we globally declare our fstream
fstream my_file;

// Command is a data structure existing of a vector of strings, named parts, holding command parts
// "tail -c 5", would give parts[0] = "tail", parts[1] = "-c", parts[2] = 5
struct Command
{
	vector<string> parts = {};
};

// Expression is a data structure holding a vector of commands and extra information
struct Expression
{
	vector<Command> commands;
	string inputFromFile;	//if this is not empty, input should come from file (string = filename)
	string outputToFile;	//if this is not empty, output should go trough file (stirng = filename)
	bool background = false;	//if this is true, we should run in the background
};

// Parses a string to form a vector of arguments. The seperator is a space char (' ').
vector<string> splitString(const string &str, char delimiter = ' ')
{
	vector<string> retval;
	for (size_t pos = 0; pos < str.length();)
	{
		// look for the next space
		size_t found = str.find(delimiter, pos);
		// if no space was found, this is the last word
		if (found == string::npos)
		{
			retval.push_back(str.substr(pos));
			break;
		}
		// filter out consequetive spaces
		if (found != pos)
			retval.push_back(str.substr(pos, found - pos));
		pos = found + 1;
	}
	return retval;
}

// wrapper around the C execvp so it can be called with
// C++ strings (easier to work with)
// always start with the command itself
// always terminate with a NULL pointer
// DO NOT CHANGE THIS FUNCTION UNDER ANY CIRCUMSTANCE
int execvp(const vector<string> &args)
{
	// build argument list
	const char **c_args = new
	const char *[args.size() + 1];
	for (size_t i = 0; i < args.size(); ++i)
	{
		c_args[i] = args[i].c_str();
	}
	c_args[args.size()] = nullptr;
	// replace current process with new process as specified
	int retval = ::execvp(c_args[0], const_cast< char **> (c_args));
	// if we got this far, there must be an error
	// in case of failure, clean up memory (this won't overwrite errno normally, but let's be sure)
	int err = errno;
	delete[] c_args;
	errno = err;
	return retval;
}

// Executes a command with arguments. In case of failure, returns error code.
// This code also checks for so called special commands, cd and exit, and uses them
int executeCommand(const Command &cmd)
{
	auto &parts = cmd.parts;
	if (parts.size() == 0)
		return EINVAL;

	if (cmd.parts[0] == "cd")
	{
		// *Case in which there is no extra argument applied, so only cd
		if (parts.size() == 1)
		{
			cerr << "Error 5, expected argument with cd." << endl;
		}

		// *Run actual code and check for chdir errors
		else if (chdir(cmd.parts[1].c_str()))
		{
			cerr << "Error 6, invalid input for cd provided." << endl;
		}
	}

	//If we find an exit we return 1, we then later use exit() to terminate.
	if (cmd.parts[0] == "exit")
	{
		return 1;
	}

	//This part executes normal commands by calling the wrapped c function execvp.
	int retval = execvp(parts);

	return retval;
}

//given function that displays the prompt
void displayPrompt()
{
	char buffer[512];
	char *dir = getcwd(buffer, sizeof(buffer));
	if (dir)
	{
		cout << "\e[32m" << dir << "\e[39m";	// the strings starting with '\e' are escape codes, that the terminal application interpets in this case as "set color to green"/"set color to default" }
	}
	cout << "$ ";
	flush(cout);
}

//this is the function that checks the command line
//it also contains the c
string requestCommandLine(bool showPrompt)
{
	if (showPrompt)
	{
		displayPrompt();
	}
	string retval;

	while (getline(cin, retval))
	{
		return retval;
	}

	//If there is no valid input anymore, we believe ctrl d is pressed and exit 
	if (!cin.good())
	{
		cerr << "You pressed ctrl D, aborting...\n";
		exit(0);
	}

	return "";
}

// note: For such a simple shell, there is little need for a full blown parser (as in an LL or LR capable parser).
// Here, the user input can be parsed using the following approach.
// First, divide the input into the distinct commands (as they can be chained, separated by `|`).
// Next, these commands are parsed separately. The first command is checked for the `<` operator, and the last command for the `>` operator.
// Note that we fill expressions here.
Expression parseCommandLine(string commandLine)
{
	Expression expression;
	vector<string> commands = splitString(commandLine, '|');
	for (size_t i = 0; i < commands.size(); ++i)
	{
		string &line = commands[i];
		vector<string> args = splitString(line, ' ');

		// Check for background tasks.
		if (i == commands.size() - 1 && args.size() > 1 && args[args.size() - 1] == "&")
		{
			expression.background = true;
			args.resize(args.size() - 1);
		}

		//check for file output
		if (i == commands.size() - 1 && args.size() > 2 && args[args.size() - 2] == ">")
		{
			expression.outputToFile = args[args.size() - 1];
			args.resize(args.size() - 2);
		}

		//check for file input
		if (i == 0 && args.size() > 2 && args[args.size() - 2] == "<")
		{
			expression.inputFromFile = args[args.size() - 1];
			args.resize(args.size() - 2);
		}

		//push to vector
		expression.commands.push_back({ args });
	}
	return expression;
}

// framework for executing "date | tail -c 5" using raw commands
// two processes are created, and connected to each other
int step1(bool showPrompt)
{
	// create communication channel shared between the two processes
	// ...
	int pipefd[2];

	pipe(pipefd);

	pid_t child1 = fork();
	if (child1 == 0)
	{
		// redirect standard output (STDOUT_FILENO) to the input of the shared communication channel
		// free non used resources (why?)
		// Command cmd = {{string("date")}};
		// executeCommand(cmd);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		Command cmd = {
		{
			string("date")
			}
		};
		executeCommand(cmd);
		//abort();	// if the executable is not found, we should abort. (why?)

	}
	pid_t child2 = fork();
	if (child2 == 0)
	{
		// redirect the output of the shared communication channel to the standard input (STDIN_FILENO).
		// free non used resources (why?)
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		Command cmd = {
		{
			string("tail"), string("-c"), string("5")
			}
		};
		executeCommand(cmd);
		//Command cmd = {{string("tail"), string("-c"), string("5")}};

		//abort();	// if the executable is not found, we should abort. (why?)
	}
	// display nice warning that the executable could not be found

	close(pipefd[0]);
	close(pipefd[1]);
	// free non used resources (why?)
	// wait on child processes to finish (why both?)
	waitpid(child1, nullptr, 0);
	waitpid(child2, nullptr, 0);
	return 0;
}

int makeProcess(int in, int out, Command cmd)
{
	cerr<<"EXECUTE"<<endl;
	pid_t child = fork();
	int exec = 0;
	if (child == 0)
	{
		if (in != 0)
		{
			dup2(in, 0);
			close(in);
		}
		if (out != 1)
		{
			dup2(out, 1);
			close(out);
		}
		exec = executeCommand(cmd);
	}
	waitpid(child, nullptr, 0);
	return exec;
}

//Execute is the main function we use to execute expressions.
//If needed we use forks and piper for | seperated user inputs
int execute(Expression expression)
{

	//If we should run in the background, create a fork 
	if (expression.background)
	{
		pid_t backChild = fork();
		if (backChild == 0)
		{
			expression.background = false;
			//recursion
			execute(expression);
			close(0);

			//the child exits
			exit(0);
		}
	}
	//Normal case, aka not running anything in the background.
	else
	{
		int pipefd[2];
		int input, output;
		input = dup(STDIN_FILENO);
		output = dup(STDOUT_FILENO);
		if (!expression.inputFromFile.empty())
		{
			input = open(expression.inputFromFile.c_str(), O_RDONLY);		}
		//case in which there is more then one command (where we need forks because a | is found)
		if (expression.commands.size() > 1)
		{
			pid_t child = fork();
			if (child == 0)
			{
				int in = 0;

				//for the amount of expressions present, run the following loop
				for (int i = 0; i < (int) expression.commands.size(); i++)
				{
					if(!expression.inputFromFile.empty() && i == 0){
						dup2(input, STDIN_FILENO);
						close(input);
					}
				//	if (expression.commands.size() - 1 == i)
				//	{
					//	if (!expression.outputToFile.empty())
					//	{
					//		output = open(expression.outputToFile.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
						//	dup2(output, STDOUT_FILENO);
					//		close(output);
						//}else{
					//		in = pipefd[0];
						//}
					//}else{
						pipe(pipefd);
						makeProcess(in, pipefd[1], expression.commands[i]);

						close(pipefd[1]);

						in = pipefd[0];
					//}
				}

				if (in != 0)
				{
					dup2(in, 0);
				}
				if (!expression.outputToFile.empty())
				{
					output = open(expression.outputToFile.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
					dup2(output, STDOUT_FILENO);
					close(output);
				}
				//we call the actual command handler
				executeCommand(expression.commands[expression.commands.size() - 1]);
			}
			//synchronization
			waitpid(child, nullptr, 0);
		}
		else
		{
			//pid_t child = fork();
			//if (child == 0){ dup2(input, STDIN_FILENO);
			//close(input);
			
			pid_t child = fork();
			
			//if file to write to is found, call subsequent handler
			
			//if file to write to is not present (work on terminal)
			

			if (child == 0)
			{
				dup2(input, STDIN_FILENO);
				close(input);
				if (!expression.outputToFile.empty())
				{

					//executeCommand(expression.commands[0]);
					output = open(expression.outputToFile.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
					dup2(output, STDOUT_FILENO);
					close(output);

					executeCommand(expression.commands[0]);
				}
			 	//actually using the command
				int exit = executeCommand(expression.commands[0]);
				if (exit == 1)
				{
				 		//exit returns 1 if exit is given or if ctrl d is pressed
					//both return into a kill statement, effectively ending
					kill(child, SIGTERM);
					//exit(1);
					//cerr<<"EITHER EXIT OR BAD CIN: "<<exit<<endl;
				}
			}
			waitpid(child, nullptr, 0);
			dup2(0, STDIN_FILENO);
			//pid_t child2 = fork();
			//}
			//waitpid(child, nullptr, 0);
		}
	}

	return 0;
}

//this function is the main handling functions, it calls the input function then executes
//the expression with the execute function
int normal(bool showPrompt)
{
	//handle user input
	string commandLine = requestCommandLine(showPrompt);
	Expression expression = parseCommandLine(commandLine);

	//execute user expressions
	execute(expression);

	return 0;
}

//shell only calls normal, it is called from main
int shell(bool showPrompt)
{
	return normal(showPrompt);
}