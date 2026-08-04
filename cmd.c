#include<stdio.h>
#include<string.h>
#include"main.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include<signal.h>

char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval",
						"set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source",
						"exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help", "bg", "fg", "jobs", NULL};
int last_status = 0;
int current_pid = 0;
char current_cmd[100];          
job_t *head = NULL;
extern char *external_commands[152];


void extract_external_commands(char **external_commands)
{
    int ext_cmd_count = 0;
    int fd = open("externalcmd.txt", O_RDONLY);
    if (fd == -1)
    {
        printf("Failed to open file\n");
        return;
    }

    int i = 0;
    char temp[100];
    char ch;

    while (read(fd,&ch,1) > 0)
    {
		if (ch == '\r')   
        continue;
        if( ch == '\n')
        {
            temp[i] = '\0';
            external_commands[ext_cmd_count] = malloc(strlen(temp)+1);
            strcpy(external_commands[ext_cmd_count],temp);
			
            ext_cmd_count++;
            i = 0;
        }
        else
        {
            temp[i++] = ch;
        }
    }
    
    if(i > 0)
    {
        temp[i] ='\0';
        external_commands[ext_cmd_count] = malloc(strlen(temp) + 1);
        strcpy(external_commands[ext_cmd_count],temp);
        ext_cmd_count++;
    }

    external_commands[ext_cmd_count] = NULL;
    close(fd);
	// for(int i=0;external_commands[i]!=NULL;i++)
	// {
	// 	printf("%s\n",external_commands[i]);
	


}

char command[100];
char *get_command(char *input_string)
{
    int i = 0;int j=0;
	//char *command=(char *)malloc(sizeof(char)*50);
	while(input_string[j] == ' ')
        j++;
    while (input_string[j] != ' ' && input_string[j] != '\0' )
    {
        command[i] = input_string[j++];
        i++;
    }

    command[i] = '\0';
    // printf("%s\n",command);
	// printf("%s\n",input_string);
    return command;
}

int check_command_type(char *command)
{
	//printf("%s\n",command);
	//printf("%ld\n",strlen(command));
	for(int i=0;builtins[i]!=NULL;i++)
	{
		if(strcmp(command,builtins[i])==0)
		{
			return BUILTIN;
		}
	}
	for(int i=0;external_commands[i]!=NULL;i++)
	{
		//printf("cmd = %s (%ld)\n", command, strlen(command));
    	//printf("ext = %s (%ld)\n\n", external_commands[i], strlen(external_commands[i]));
		if(strcmp(command,external_commands[i])==0)
		{
			// printf("ext");
			return EXTERNAL;
		}
	}
	printf("%s",command);
	printf("\nnocmd\n");
	return NO_COMMAND;

}
void execute_internal_commands(char *input_string)
{
	if(strcmp(input_string,"exit")==0)
	{
		exit(0);
	}
	else if(strcmp(input_string,"pwd")==0)
	{
		char buff[50];
        if (getcwd(buff,50) != NULL)
        {
            printf("%s\n",buff);
            last_status = 0;
        }
        else
        {
            last_status = 1;
        }
	}
	else if(strncmp(input_string,"cd",2)==0)
	{
		// //("%s\n",input_string);
		// chdir(input_string+3);
		// char buff[50];
		// getcwd(buff,50);
		// printf("%s\n",buff);
        if (chdir(input_string+3) != 0)
        {
            perror("cd failed");
            last_status = 1;
        }
        else
        {
            char buff[50];
            getcwd(buff,50);
            printf("%s\n",buff);
            last_status = 0;
        }
		
	}

    else if(strcmp(input_string,"jobs")==0)
    {
        print_jobs();
        last_status = 0;
    }
    else if(strcmp(input_string,"fg")==0)
    {
        fg();
        last_status = 0;
    }
    else if(strcmp(input_string,"bg")==0)
    {
        bg();
        last_status = 0;
    }
    else if (strncmp(input_string, "echo", 4) == 0)
    {
        char *ptr = input_string + 4;

        // skip spaces
        while (*ptr == ' ')
            ptr++;

        int newline = 1;

        if (strncmp(ptr, "-n", 2) == 0)
        {
            newline = 0;
            ptr += 2;
            while (*ptr == ' ')
                ptr++;
        }

        //  HANDLE VARIABLES
        if (strcmp(ptr, "$$") == 0)
        {
            printf("%d", getpid());
        }
        else if (strcmp(ptr, "$?") == 0)
        {
            printf("%d", last_status);
        }
        else if (strcmp(ptr, "$SHELL") == 0)
        {
            char *shell = getenv("SHELL");
            if (shell)
                printf("%s", shell);
        }
        else
        {
            printf("%s", ptr);
        }

        if (newline)
            printf("\n");
        last_status = 0;
    }
}
void execute_external_commands(char *input_string)
{
    
    int pipefound = 0;

    if (strchr(input_string, '|') != NULL)
        pipefound = 1;

  
    if (!pipefound)
    {
        char *argv[10];
        int i = 0, j = 0;

        while (input_string[j] != '\0')
        {
            while (input_string[j] == ' ')
                j++;

            if (input_string[j] == '\0')
                break;

            argv[i++] = &input_string[j];

            while (input_string[j] != ' ' && input_string[j] != '\0')
                j++;

            if (input_string[j] != '\0')
            {
                input_string[j] = '\0';
                j++;
            }
        }

        argv[i] = NULL;

        execvp(argv[0], argv);
        perror("exec failed");
        exit(1);
    }
    
    else
    {
        char *commands[10];
        int cmd_count = 0;

        commands[cmd_count++] = strtok(input_string, "|");
        while ((commands[cmd_count++] = strtok(NULL, "|")));
        cmd_count--;

        int fd[2];
        int in = 0;

        for (int i = 0; i < cmd_count; i++)
        {
            if (i < cmd_count - 1)
                pipe(fd);

            int pid = fork();

            if (pid == 0)
            {
                dup2(in, 0);

                if (i < cmd_count - 1)
                    dup2(fd[1], 1);

                if (i < cmd_count - 1)
                {
                    close(fd[0]);
                    close(fd[1]);
                }

                // parse each command
                char *argv[10];
                int j = 0;

                char *token = strtok(commands[i], " ");
                while (token != NULL)
                {
                    argv[j++] = token;
                    token = strtok(NULL, " ");
                }
                argv[j] = NULL;

                execvp(argv[0], argv);
                perror("exec failed");
                exit(1);
            }
            else
            {  
                wait(NULL);

                if (i < cmd_count - 1)
                {
                    close(fd[1]);
                    in = fd[0];
                }
            }
        }
    }
     
}
void signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTSTP)
    {
        if (current_pid == 0)
        {
            write(1, "\nminishell$ ", 13);
        }
        else
        {
            if (signo == SIGTSTP)
            {
                kill(current_pid, SIGTSTP);
                add_job(current_pid, current_cmd);
            }
            write(1, "\n", 1);
        }
    }
}
// void signal_handler(int sig_num)
// {
//     if(sig_num == SIGCHLD)
//     {
//         int child_pid;
//         int child_status;
        
//         // Clean up any terminated children
//         while((child_pid = waitpid(-1, &child_status, WNOHANG)) > 0)
//         {
//             // Remove from job list if it exists
//             delete_jobs(child_pid);
//         }
//     }
//     else if((sig_num == SIGINT || sig_num == SIGTSTP) )
//     {
//         if(pid == 0)
//         {
//         printf("%s\n",prompt);
//         fflush(stdout);
//         }
//     }
// }


void scan_input(char *prompt, char *input_string)
{
	signal(SIGINT,signal_handler);//terminate
	signal(SIGTSTP,signal_handler);//Stop
    signal(SIGCHLD, sigchld_handler);

	while(1)
	{
		printf("%s ",prompt);
		scanf("%[^\n]",input_string);
		getchar();
		
		//printf("%s\n",input_string);
		if(strncmp(input_string,"PS1=",4)==0)
		{
			strcpy(prompt,input_string+4);
		}
		else
		{
			char *c=get_command(input_string);
			
			int type=check_command_type(c);
			
			if(type==BUILTIN)
			{
				execute_internal_commands(input_string);
			
			}
            else if(type==EXTERNAL)
            {
                strcpy(current_cmd, input_string);
				current_pid =fork();
				if(current_pid ==0)
				{
					signal(SIGINT,SIG_DFL);
					signal(SIGTSTP,SIG_DFL);

                	execute_external_commands(input_string); 
				}
				
				else if(current_pid >0)
                {
                    int status;
                    waitpid(current_pid ,&status,WUNTRACED);

                    if (WIFEXITED(status))
                        last_status = WEXITSTATUS(status);   // ⭐ ADD THIS

                    current_pid =0;
                }
				else
				{
					perror("fork failed");
				}
            }
			else{
				printf("Enter a valid command\n");
			}

		}


	}
		
}

