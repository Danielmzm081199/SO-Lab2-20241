#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 100

// Función para leer una línea de entrada
char* read_line() {
    char *line = malloc(MAX_LINE);
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        free(line);
        return NULL;
    }
    return line;
}

// Función para dividir la línea en tokens
char** split_line(char *line) {
    int bufsize = MAX_ARGS;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token = strtok(line, " \t\r\n\a");
    int position = 0;

    while (token != NULL) {
        tokens[position++] = token;
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

// Función para ejecutar un comando
void execute(char **args) {
    if (args[0] == NULL) return; // No command entered

    if (strcmp(args[0], "exit") == 0) {
        exit(0); // Termina el shell si el comando es "exit"
    }

    pid_t pid = fork(); // Crear un proceso hijo
    if (pid == 0) {
        // Proceso hijo: ejecuta el comando
        if (execvp(args[0], args) == -1) {
            perror("wish");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error en fork()
        perror("wish");
    } else {
        // Proceso padre: espera a que el hijo termine
        wait(NULL);
    }
}

// Modo interactivo
void interactive_mode() {
    char *line;
    char **args;

    while (1) {
        printf("wish> ");
        line = read_line();
        if (line == NULL) break; // Salir si no hay más entrada
        args = split_line(line);
        execute(args);
        free(line);
        free(args);
    }
}

// Modo batch
void batch_mode(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("wish");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE];
    char **args;

    while (fgets(line, sizeof(line), file) != NULL) {
        args = split_line(line);
        execute(args);
        free(args);
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Sin argumentos, ejecutar en modo interactivo
        interactive_mode();
    } else if (argc == 2) {
        // Un argumento, ejecutar en modo batch
        batch_mode(argv[1]);
    } else {
        // Demasiados argumentos, imprimir error
        fprintf(stderr, "wish: error: too many arguments\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
