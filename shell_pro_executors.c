#include "shell.h"

/**
 * execute_builtin_command - this function runs built-in
 * @oki: struct of type okeoma
 *
 * Return: 0 on success
 * error: 1 on error
*/
int execute_builtin_command(okeoma *oki)
{
	char *built_in_commands[] = {
		"cd",
		"exit",
		"setenv",
		"unsetenv",
		"getenv",
		"alias"
	};

	void (*built_in_funcs[])(okeoma *oki) = {
		&cd_command,
		&exit_command,
		&setenv_command,
		&unsetenv_command,
		&get_env_command,
		&alias_
	};
	int num_built_in_com = sizeof(built_in_commands) / sizeof(char *), i;

	for (i = 0; i < num_built_in_com; i++)
	{
		if (strcmp(oki->av[0], built_in_commands[i]) == 0)
		{
			(*built_in_funcs[i])(oki);
			return (0);
		}
	}
	return (1);
}

/**
 * execute_command - create a process to execute executables
 * @oki: struct of type okeoma
 *
 * Return: the status of the last executed program
*/
int execute_command(okeoma *oki)
{
	char **envi = NULL;

	oki->ec = find_executable(oki);
	if (!oki->ec)
	{
		p(STE, "%s: %d: %s: not found\n", oki->N, oki->c, oki->av[0]);
		if (oki->it)
			exit(127);
	}
	if (oki->ec)
	{
		oki->child_pid = fork();
		if (oki->child_pid == -1)
			perror("fork");
		else if (oki->child_pid == 0)
		{
			envi = env_from_list(oki->head);
			execve(oki->ec, oki->av, envi);
			p(STE, "%s: %d: %s: Permission denied\n", oki->N, oki->c, oki->av[0]);
			return (EXIT_FAILURE);
		}
		else
		{
			waitpid(oki->child_pid, &oki->status, 0);
			if (oki->it)
				fr__(1, envi);
			else
				fr__(2, oki->ec, envi);
			if (WIFEXITED(oki->status))
				return (WEXITSTATUS(oki->status));
		}
	}
	return (-1);
}

/**
 * find_executable - find the path to an execuitable file
 * @oki: struct of type okeoma
 *
 * Return: the full path to the executable if it exists within the path
*/
char *find_executable(okeoma *oki)
{
	char *path_env, *path_copy, *token, *exe_path;
	size_t token_len, exec_name_len;

	if (access(oki->av[0], X_OK) == 0)
	{
		exe_path = oki->av[0];
		return (exe_path);
	}
	path_env = get_env(oki->head, "PATH");
	path_copy = strdup(path_env);
	token = string(path_copy, ":");
	while (token)
	{
		token_len = strlen(token);
		exec_name_len = strlen(oki->av[0]);
		exe_path = (char *)malloc((token_len + exec_name_len + 2) * sizeof(char));
		if (exe_path == NULL)
		{
			free(path_copy);
			return (NULL);
		}
		strcpy(exe_path, token);
		strcat(exe_path, "/");
		strcat(exe_path, oki->av[0]);
		if (access(exe_path, X_OK) == 0)
		{
			free(path_copy);
			return (exe_path);
		}
		free(exe_path);
		token = string(NULL, ":");
	}
	free(path_copy);
	return (NULL);
}

/**
 * B_exc - function handles all command processing
 * @oki: struct of type okeoma
 *
 * Return: void
*/
void B_exc(okeoma *oki)
{
	int z;
	bool affirm;

	find_char(oki->cmd, '#');
	affirm = alias_checker(oki->pos, oki->cmd);
	while (affirm == true)
	{
		oki->cmd = command(oki->pos, oki->cmd);
		affirm = alias_checker(oki->pos, oki->cmd);
	}
	oki->y = find_set(oki->cmd);
	oki->tok2 = str_tok(oki->cmd, "&&||");
	while (oki->tok2 != NULL)
	{
		oki->command = prs(oki->cmd, ";\t\n");
		for (oki->i = 0; oki->command[oki->i] != NULL; oki->i++)
		{
			oki->av = prs(oki->command[oki->i], " \t\n\r");
			if (oki->it)
				oki->c++;
			for (z = 0; oki->av[z] != NULL; z++)
				oki->av[z] = replace(oki->head, oki, oki->av[z]);
			oki->status = execute_builtin_command(oki);
			if (oki->status != 0)
				oki->status = execute_command(oki);
		}
		if (oki->y == 1 && oki->status != 0)
			break;
		if (oki->y == 2 && oki->status == 0)
			break;
		oki->y = find_set(NULL);
		oki->tok2 = str_tok(NULL, "&&||");
	}
}

/**
 * file_handle - does selection for what filestream to read from
 * @oki: struct of type okeoma
 * @argc: argument count
 * @argv: argument vector
 *
 * Return: FILE struct
*/
FILE *file_handle(okeoma *oki, int argc, char **argv)
{
	FILE *file_d;

	if (argc > 1 && argv[1] != NULL)
	{
		file_d = fopen(argv[1], "r");
		oki->N = argv[1];
	}
	else
		file_d = stdin;
	if (file_d == NULL)
	{
		p(STE, "%s: %d: cannot open %s: No such file\n",
		argv[0], oki->c, argv[1]);
		exit(2);
	}
	return (file_d);
}
