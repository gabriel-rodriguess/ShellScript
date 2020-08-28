
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shell_lib.h"

#define TRUE 1
#define FALSE 0
#define NUMERO_DE_BYTES 4000
#define TAMANHO_DA_ENTRADA 250

void ImprimeString(int argc, char ** argv){
	int i;
	for(i=0; i<argc; i++){
		printf("[%d]: '%s'\n", i, argv[i]);
	}
}

int * DefineSaidaDoPipe(int argc, char ** argv){
  int posicaoDaSaida = EncontraOperador(">", argc, argv);
  if(posicaoDaSaida == -1)
    return NULL;
  
  int * saidaDoPipe = malloc(2 * sizeof(int));
  pipe(saidaDoPipe);

  return saidaDoPipe;
}

int * ExecutaSequenciaDeComandos(char ** comandos, 
                                  int quantidadeDeComandos, 
                                  int * inicioDoComando, 
                                  int * entradaDoPipe, 
                                  int * saidaDoPipe)
{

  int i, inicio;
  char **comandoAtual;

  i = inicioDoComando[quantidadeDeComandos - 1];
  
  int **descritorDeArquivo = malloc(quantidadeDeComandos + sizeof(int *));
  for(i = 0; i < quantidadeDeComandos; i++){
    descritorDeArquivo[i] = malloc(2 * sizeof(int));
    pipe(descritorDeArquivo[i]);
  }

  pid_t *filho = malloc(quantidadeDeComandos * sizeof(pid_t));

  for(i = 0; i < quantidadeDeComandos; i++){
    inicio = inicioDoComando[i];
    comandoAtual = &comandos[inicio];

    filho[i] = fork();

    if(filho[i] == 0){
      if(i == 0){
        if(entradaDoPipe != NULL){
          dup2(entradaDoPipe[0], STDIN_FILENO);
        }

        close(descritorDeArquivo[i][0]);

        if(quantidadeDeComandos > 1)
          dup2(descritorDeArquivo[i][1], STDOUT_FILENO);
        else if(saidaDoPipe != NULL){
          dup2(saidaDoPipe[1], STDOUT_FILENO);
        }
      }

      else if(i == quantidadeDeComandos - 1){
        if(saidaDoPipe != NULL){
          dup2(saidaDoPipe[1], STDOUT_FILENO);
        }

        close(descritorDeArquivo[i-1][1]);
        dup2(descritorDeArquivo[i-1][0], STDIN_FILENO);

      }
      else{
        close(descritorDeArquivo[i-1][1]);
        dup2(descritorDeArquivo[i-1][0], STDIN_FILENO);

        close(descritorDeArquivo[i][0]);
        dup2(descritorDeArquivo[i][1], STDOUT_FILENO);
      }
      
      execvp(comandoAtual[0], comandoAtual);

			fprintf(stderr,"Comando %d (%s) desconhecido\n", i, comandoAtual[0]);
      
      return NULL;
    }
    close(descritorDeArquivo[i][1]);
    waitpid(filho[i], NULL, 0);
  }                              
}

int CopiaArquivo(int entradaDescritorDeArquivo, int saidaDescritorDeArquivo){

  char buffer[NUMERO_DE_BYTES];
  int numeroDeBytesLidos, numeroDeBytesEscritos, n;
  do{
    memset(buffer, 0, NUMERO_DE_BYTES);
    numeroDeBytesLidos = read(entradaDescritorDeArquivo, buffer, NUMERO_DE_BYTES);
    if(numeroDeBytesLidos < 0){
      close(entradaDescritorDeArquivo);
      close(saidaDescritorDeArquivo);
      perror("Leitura");
      return -1;
    }
    if(numeroDeBytesLidos > 0){
      numeroDeBytesEscritos = write(saidaDescritorDeArquivo, buffer, numeroDeBytesLidos);
      if(numeroDeBytesEscritos < 0){
        perror("Escrita");
        close(entradaDescritorDeArquivo);
        close(saidaDescritorDeArquivo);
        return -1;
      }
      while(numeroDeBytesEscritos < numeroDeBytesLidos){ 
        n = write(saidaDescritorDeArquivo, buffer + numeroDeBytesEscritos, 
        numeroDeBytesLidos - numeroDeBytesEscritos); 

        if(n < 0){
          perror("Escrita");
          close(entradaDescritorDeArquivo);
          close(saidaDescritorDeArquivo);
          return -1;
        }
        numeroDeBytesEscritos += n;
      }
    }
  } while(numeroDeBytesLidos > 0);

  return 0;
}

int * DefineEntradaDoPipe(int argc, char ** argv){

  int posicaoDeEntrada = EncontraOperador("<", argc, argv);

  if(posicaoDeEntrada == -1){
    return NULL;
  }
  
  int * entradaDoPipe = malloc(2 * sizeof(int));
  if(pipe(entradaDoPipe) < 0){
    perror("Erro PIPE!");
    return NULL;
  } 

  int entradaDoDescritorDeArquivo = open(argv[posicaoDeEntrada + 1], 
      O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  
  if (entradaDoDescritorDeArquivo < 0){
    perror("Abra o arquivo de entrada");
    return NULL;
  }

  int quantidadeDeComandos = ContaPipe(posicaoDeEntrada + 1, argc, argv);
  
  if(quantidadeDeComandos >= 1){
    int * comandosDeEntrada = PegaSequenciaDePipes(posicaoDeEntrada + 3, argc, argv);

    int pipeAux[2];
    pipe(pipeAux);
    CopiaArquivo(entradaDoDescritorDeArquivo, pipeAux[1]);
    close(pipeAux[1]);

    ExecutaSequenciaDeComandos(RemoveOperadores(argc, argv), quantidadeDeComandos, 
    comandosDeEntrada, pipeAux, entradaDoPipe);
    
    close(entradaDoPipe[1]);
    return entradaDoPipe;
  }
  CopiaArquivo(entradaDoDescritorDeArquivo, entradaDoPipe[1]);
  close(entradaDoPipe[1]);

  return entradaDoPipe;  
}

int EncontraOperador(char * operador, int argc, char ** argv){
  for(int i = 0; i < argc; i++){
    if(strcmp(operador, argv[i]) == 0)
      return i;
  }

  return -1;
}

char ** RemoveOperadores(int argc, char ** argv){
  char ** comandos = malloc((argc + 1) * sizeof(char *));  
  int tamanho;

  for(int i = 0; i < argc; i++){
    tamanho = strlen(argv[i]);
    comandos[i] = malloc(tamanho);
    if(!EhOperador(argv[i]))
      memcpy(comandos[i], argv[i], tamanho);
    else
      comandos[i] = NULL;
  }
  comandos[argc] = NULL;

  return comandos;
}

int ContaPipe(int inicio, int argc, char ** argv){
  int contador = 0;
  for(int i = inicio; i< argc; i++){
    if(EhOperador(argv[i])){
      if(strcmp(argv[i], "|") == 0){
        contador++;

      }
      else
      break;
    }
  }
  return contador;
}

int EhOperador(char * palavra){
  if(palavra[0] == '<' || palavra[0] == '>' || palavra[0] == '|'){
    return TRUE;
  }else
  return FALSE;
}

int * PegaSequenciaDePipes(int inicio, int argc, char ** argv){

  int i, quantideDePipes = 0;
  int * inicioDoComando = malloc(1 * sizeof(int));
  inicioDoComando[0] = inicio;

  for(i = inicio; i < argc; i++){

    if(argv[i] == NULL)
      break;
    
    if(EhOperador(argv[i]) && strcmp(argv[i], "|") != 0)
      break;

    if(strcmp(argv[i], "|") == 0){
      quantideDePipes++;
      inicioDoComando = realloc(inicioDoComando, (quantideDePipes + 1) * sizeof(int));
      inicioDoComando[quantideDePipes] = i + 1;
    }
  }
  return inicioDoComando;
}

char ** LerComandos(){
  char * comando = malloc(TAMANHO_DA_ENTRADA * sizeof(char));

  printf("digite o comando: ");  
  scanf("%[^\n]%*c", comando);

  char * p, * inicioDoToken, * aux = malloc(sizeof(char));
  int c, tamanhoDoToken = 0, quantidadeDeLinhas = 0, i;

  int verificaEstado = 0;

  for(p = comando; *p != '\0'; p++){
    if(verificaEstado == 0){
        if(*p == ' ') 
            continue;
        if(*p == '\'' || *p == '\"')
          verificaEstado = 1;
        else
          verificaEstado = 2;
        quantidadeDeLinhas++;
        continue;
    }
    if(verificaEstado == 1){
        if(*p == '\'' || *p == '\"')
          verificaEstado = 0;
        continue;
    }
    if(verificaEstado == 2){
        if (*p == ' ')
          verificaEstado = 0;
        continue;
    }
  }

  quantidadeDeLinhas++;
  char ** matrizDeArgumentos = malloc((quantidadeDeLinhas) * sizeof(char *));
  matrizDeArgumentos[-1] = NULL;
  int numeroDoComando = 0;
  verificaEstado = 0;

  for(p = comando; *p != '\0'; p++){
    c = (unsigned char) *p;
    if(verificaEstado == 0){
        tamanhoDoToken = 0;
        if(c == ' ')
          continue;
        
        if(c == '\'' || c == '\"'){
          verificaEstado = 1;
          inicioDoToken = p + 1;
        }
        else{
          verificaEstado = 2;
          inicioDoToken = p;
          tamanhoDoToken++;
        }
        continue;
    }
    if(verificaEstado == 1){
        if(c == '\'' || c == '\"'){
          matrizDeArgumentos[numeroDoComando] = malloc(tamanhoDoToken * sizeof(char));
          memcpy(matrizDeArgumentos[numeroDoComando], inicioDoToken, tamanhoDoToken);
          verificaEstado = 0;
          numeroDoComando++;
          continue;
        }
        tamanhoDoToken++;
        continue;
    }
    if(verificaEstado == 2){
        if(*p == ' '){
          matrizDeArgumentos[numeroDoComando] = malloc(tamanhoDoToken * sizeof(char));
          memcpy(matrizDeArgumentos[numeroDoComando], inicioDoToken, tamanhoDoToken);
          verificaEstado = 0;
          numeroDoComando++;
          continue;
        }
        tamanhoDoToken++;
        continue;        
    }
  }
    
    if(verificaEstado == 2){
      matrizDeArgumentos[numeroDoComando] = malloc(tamanhoDoToken * sizeof(char));
      memcpy(matrizDeArgumentos[numeroDoComando], inicioDoToken, tamanhoDoToken);
    }
  return matrizDeArgumentos;
}

int PegaQuantidadeDeComandos(char ** comandos){
  int i = 0;
  while (comandos[i] != NULL)
    i++;
  return i;
}