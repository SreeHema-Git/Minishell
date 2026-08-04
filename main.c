#include<stdio.h>
 #include <stdlib.h>
 #include"main.h"

char *external_commands[152];
int main()
{
    system("clear");
    
     char prompt[25]="minishell$";
    char input_string[100];
    extract_external_commands(external_commands);
    scan_input(prompt,input_string);
}