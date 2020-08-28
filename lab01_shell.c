#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shell.h"

int main(int argc, char  **argv)
{
  while(1){
    char ** comandos = LerComandos();
    int quantidadeDeComandos = PegaQuantidadeDeComandos(comandos);

    if(quantidadeDeComandos > 0){
      int * inicioDoComando = PegaSequenciaDePipes(0, quantidadeDeComandos, comandos); 
      int quantidadeDeOperacoes = ContaPipe(0, quantidadeDeComandos, comandos); 
      char ** entradaDoUsuario = RemoveOperadores(quantidadeDeComandos, comandos); 

      int quantidadeDeComandosExecutados = quantidadeDeOperacoes + 1; 

      int posicaoDeSaida = EncontraOperador(">", quantidadeDeComandos, comandos); 

      int * entradaDoPipe = DefineEntradaDoPipe(quantidadeDeComandos, comandos); 
      int * saidaDoPipe = DefineSaidaDoPipe(quantidadeDeComandos, comandos);


      ExecutaSequenciaDeComandos(entradaDoUsuario, quantidadeDeComandosExecutados, 
      inicioDoComando, entradaDoPipe, saidaDoPipe);

      int descritorDeArquivoSaida;
      if(saidaDoPipe != NULL){
        close(saidaDoPipe[1]);

        descritorDeArquivoSaida = open(comandos[posicaoDeSaida + 1], O_WRONLY | O_CREAT | 
        O_TRUNC | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if(descritorDeArquivoSaida < 0){
          perror("Abre arquivo de saida");
          return -1;
        }
        CopiaArquivo(saidaDoPipe[0], descritorDeArquivoSaida);
      }      
    }
    fflush(stdin);
    fflush(stdout);
  }
  return 0;
}