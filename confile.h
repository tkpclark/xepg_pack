#include <stdlib.h>
#include<stdio.h>
#include <string.h>

#define CONFIG_FILE "app.config"
char* read_config();
char *getParameterValue(char *line);
char *getParameterName(char *line);
