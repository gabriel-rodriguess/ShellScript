#ifndef _SHELL
#define _SHELL
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shell_lib.c"
#endif

char ** LerComandos();
int PegaQuantidadeDeComandos(char ** comandos);
int * PegaSequenciaDePipes(int inicio, int argc, char ** argv);
int EhOperador(char * palavra);
int ContaPipe(int inicio, int argc, char ** argv);
char ** RemoveOperadores(int argc, char ** argv);
int EncontraOperador(char * operador, int argc, char ** argv);
int * DefineEntradaDoPipe(int argc, char ** argv);
int CopiaArquivo(int entradaDescritorDeArquivo, int saidaDescritorDeArquivo);
int * ExecutaSequenciaDeComandos(char ** comandos, int quantidadeDeComandos, int * inicioDoComando, int * entradaDoPipe, int * saidaDoPipe);
int * DefineSaidaDoPipe(int argc, char ** argv);

