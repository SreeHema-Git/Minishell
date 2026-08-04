#include<stdio.h>
 #include <stdlib.h>
 #include"main.h"
extern int current_pid ;
 void add_job(pid_t pid, char *cmd)
{
    job_t *new = malloc(sizeof(job_t));

    new->pid = pid;
    strcpy(new->cmd, cmd);
    new->next = NULL;

    if (head == NULL)
    {
        head = new;
    }
    else
    {
        job_t *temp = head;
        while (temp->next)
            temp = temp->next;

        temp->next = new;
    }
}
void delete_job(pid_t pid)
{
    job_t *temp = head, *prev = NULL;

    while (temp)
    {
        if (temp->pid == pid)
        {
            if (prev == NULL)
                head = temp->next;
            else
                prev->next = temp->next;

            free(temp);
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}
void print_jobs()
{
    job_t *temp = head;
    int i = 1;

    while (temp)
    {
        printf("[%d] %d %s\n", i++, temp->pid, temp->cmd);
        temp = temp->next;
    }
}
void fg()
{
    if (head == NULL)
    {
        printf("No jobs\n");
        return;
    }

    job_t *temp = head;

    // go to last job
    while (temp->next)
        temp = temp->next;

    current_pid = temp->pid;

    kill(temp->pid, SIGCONT);

    int status;
    waitpid(temp->pid, &status, WUNTRACED);

    delete_job(temp->pid);

    current_pid = 0;
}
void bg()
{
    if (head == NULL)
    {
        printf("No jobs\n");
        return;
    }

    job_t *temp = head;

    while (temp->next)
        temp = temp->next;

    kill(temp->pid, SIGCONT);
}
void sigchld_handler(int sig)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        delete_job(pid);
    }
}