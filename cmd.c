// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include "cmd.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define READ		0
#define WRITE		1

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	return chdir(dir->string) == 0;
}

static char* shell_pwd()
{
	char *buf;
	buf = malloc(1024);
	getcwd(buf, 1024);
	return buf;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	return SHELL_EXIT;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	if ((strcmp(s->verb->string, "exit") == 0) || (strcmp(s->verb->string, "quit") == 0)) {
		return shell_exit();
	}

	if (strcmp(s->verb->string, "cd") == 0) {
		if (s->params != 0x0) {
			if (s->out != 0x0) {
				int fd;
				char filename[500];
				filename[0] = '\0';
				strcat(filename, s->out->string);
				if (s->out->next_part != 0x0) {
					word_t *word = s->out->next_part;
					while (word != 0x0) {
						if (word->expand) {
							if (getenv(word->string) != NULL) {
								strcat(filename, getenv(word->string));
							}
						} else {
							strcat(filename, word->string);
						}
						word = word->next_part;
					}
				}
				if (access(filename, F_OK) != 0) {
					fd = open(filename, O_WRONLY | O_CREAT, 0777);
				} else {
					if (s->io_flags != 0) {
						fd = open(filename, O_WRONLY | O_APPEND);
					} else {
						fd = open(filename, O_WRONLY | O_TRUNC);
					}
				}

				int newOut = dup(STDOUT_FILENO);

				dup2(fd, STDOUT_FILENO);
				close(fd);

				close(STDOUT_FILENO);

				dup2(newOut, STDOUT_FILENO);
				close(newOut);
			}
			bool err = shell_cd(s->params);
			if (err)
				return 0;

			return 1;
		}
		return 0;
	}

	if (strcmp(s->verb->string, "pwd") == 0) {
		char* curr_dir = shell_pwd();
		
		if (s->out != 0x0) {
			int fd;
			char filename[500];
			filename[0] = '\0';
			strcat(filename, s->out->string);
			if (s->out->next_part != 0x0) {
				word_t *word = s->out->next_part;
				while (word != 0x0) {
					if (word->expand) {
						if (getenv(word->string) != NULL) {
							strcat(filename, getenv(word->string));
						}
					} else {
						strcat(filename, word->string);
					}
					word = word->next_part;
				}
			}
			if (access(filename, F_OK) != 0) {
				fd = open(filename, O_WRONLY | O_CREAT, 0777);
			} else {
				if (s->io_flags != 0) {
					fd = open(filename, O_WRONLY | O_APPEND);
				} else {
					fd = open(filename, O_WRONLY | O_TRUNC);
				}
			}

			int newOut = dup(STDOUT_FILENO);

			dup2(fd, STDOUT_FILENO);
			close(fd);

			write(STDOUT_FILENO, curr_dir, strlen(curr_dir));
			write(STDOUT_FILENO, "\n", 1);
			free(curr_dir);

			close(STDOUT_FILENO);

			dup2(newOut, STDOUT_FILENO);
			close(newOut);
		} else {
			printf("%s\n", curr_dir);
        	free(curr_dir);
		}

		return 0;
	}

	if(s->verb->next_part != 0x0 && strcmp(s->verb->next_part->string, "=") == 0) {
		char value[500];
		//char *value = malloc(200);
		value[0] = '\0';

		word_t *word = s->verb->next_part->next_part;
		while (word != 0x0) {
			if (word->expand) {
				if (getenv(word->string) != NULL) {
					strcat(value, getenv(word->string));
				}
			} else {
				strcat(value, word->string);
			}
			word = word->next_part;	
		}

		return setenv(s->verb->string, value, 1);
	}

	int status;
	pid_t pid = fork();

	if (pid == 0) {
		int size;

		if ((s->in != 0x0) && (access(s->in->string, F_OK) != 0)) {
			char filename[500];
			filename[0] = '\0';
			strcat(filename, s->in->string);
			if (s->in->next_part != 0x0) {
				word_t *word = s->in->next_part;
				while (word != 0x0) {
					if (word->expand) {
						if (getenv(word->string) != NULL) {
							strcat(filename, getenv(word->string));
						}
					} else {
						strcat(filename, word->string);
					}
					word = word->next_part;
				}
			}
			int fd = open(filename, O_RDONLY | O_CREAT, 0777);
			dup2(fd, STDIN_FILENO);
			close(fd);
		} else if ((s->in != 0x0) && (access(s->in->string, F_OK) == 0)){
			char filename[500];
			filename[0] = '\0';
			strcat(filename, s->in->string);
			if (s->in->next_part != 0x0) {
				word_t *word = s->in->next_part;
				while (word != 0x0) {
					if (word->expand) {
						if (getenv(word->string) != NULL) {
							strcat(filename, getenv(word->string));
						}
					} else {
						strcat(filename, word->string);
					}
					word = word->next_part;
				}
			}
			int fd = open(filename, O_RDONLY);
			dup2(fd, STDIN_FILENO);
			close(fd);
		}

		if ((s->out != 0x0) && (s->out == s->err)) {
			char filename[500];
			filename[0] = '\0';
			strcat(filename, s->out->string);
			if (s->out->next_part != 0x0) {
				word_t *word = s->out->next_part;
				while (word != 0x0) {
					if (word->expand) {
						if (getenv(word->string) != NULL) {
							strcat(filename, getenv(word->string));
						}
					} else {
						strcat(filename, word->string);
					}
					word = word->next_part;
				}
			}
			if (access(filename, F_OK) != 0) {
				int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
			} else {
				int fd = open(filename, O_WRONLY | O_TRUNC);
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
			}
		} else {
			if ((s->out != 0x0) && (access(s->out->string, F_OK) != 0)) {
				int fd = open(s->out->string, O_WRONLY | O_CREAT, 0777);
				dup2(fd, STDOUT_FILENO);
				close(fd);
			} else if ((s->out != 0x0) && (access(s->out->string, F_OK) == 0)) {
				int fd;
				if (s->io_flags == 0) {
					fd = open(s->out->string, O_WRONLY | O_TRUNC);
				} else {
					fd = open(s->out->string, O_WRONLY | O_APPEND);
				}

				dup2(fd, STDOUT_FILENO);
				close(fd);
			}
			
			if ((s->err != 0x0) && (access(s->err->string, F_OK) != 0)) {
				int fd = open(s->err->string, O_WRONLY | O_CREAT, 0777);
				dup2(fd, STDERR_FILENO);
				close(fd);
			} else if ((s->err != 0x0) && (access(s->err->string, F_OK) == 0)) {
				int fd;
				if (s->io_flags == 0) {
					fd = open(s->err->string, O_WRONLY | O_TRUNC);
				} else {
					fd = open(s->err->string, O_WRONLY | O_APPEND);
				}

				dup2(fd, STDERR_FILENO);
				close(fd);
			}
		}

		execvp(s->verb->string, get_argv(s, &size));
		printf("Execution failed for '");
		printf("%s'\n", s->verb->string);
		exit(1);
	} else {
		waitpid(pid, &status, 0);
	}

	return WEXITSTATUS(status);
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	pid_t child_pid1;
	pid_t child_pid2;
	int status1, status2;

	child_pid1 = fork();

	if (child_pid1 == 0) {
		parse_command(cmd1, level, father);
	} else {
		pid_t child_pid2 = fork();
		if (child_pid2 == 0) {
			parse_command(cmd2, level, father);
		} else {
			waitpid(child_pid1, &status1, 0);
			waitpid(child_pid2, &status2, 0);
		}
	}

	return WEXITSTATUS(status1) && WEXITSTATUS(status2);
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	int pipefd[2];
	int size;
	int ret;
	int status;
    pid_t child_pid1, child_pid2;

    pipe(pipefd);

	child_pid1 = fork();

    if (child_pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
		ret = parse_command(cmd1, level, father);
        exit(ret);
    } else {
		child_pid2 = fork();

        if (child_pid2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
			ret = parse_command(cmd2, level, father);
            exit(ret);
        } else {
            close(pipefd[0]);
            close(pipefd[1]);

			waitpid(child_pid1, NULL, 0);
            waitpid(child_pid2, &status, 0);
        }
    }

	return WEXITSTATUS(status);
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	if (c->op == OP_NONE) {
		return parse_simple(c->scmd, level, father);
	}

	int all_ret = 0, ret1, ret2;
	int return_nzero, return_zero;
	switch (c->op) {
	case OP_SEQUENTIAL:
		ret1 = parse_command(c->cmd1, level, father);
		ret2 = parse_command(c->cmd2, level, father);
		break;

	case OP_PARALLEL:
		all_ret = run_in_parallel(c->cmd1, c->cmd2, level, father);
		break;

	case OP_CONDITIONAL_NZERO:
		return_nzero = parse_command(c->cmd1, level, father);
		if (return_nzero != 0) {
			ret1 = parse_command(c->cmd2, level, father);
			if (ret1 != 0) {
				all_ret = 0;
			}
		}
		break;

	case OP_CONDITIONAL_ZERO:
		return_zero = parse_command(c->cmd1, level, father);
		if (return_zero == 0) {
			ret1 = parse_command(c->cmd2, level, father);
			if (ret1 != 0) {
				all_ret = 1;
			}
		}
		break;

	case OP_PIPE:
		all_ret = run_on_pipe(c->cmd1, c->cmd2, level, father);
		break;

	default:
		return SHELL_EXIT;
	}

	return all_ret;
}
