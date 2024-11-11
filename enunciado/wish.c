#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 1024
#define MAX_ARGS 100

// Variable global para el path
//char *path[] = {"/bin", "/usr/bin", NULL}; // Se puede modificar según sea necesario

// Variable global para el path, inicializada con algunos valores predeterminados
char **path = NULL;
int path_count = 0; // Número de elementos en el path

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

// Función para actualizar la variable global path
void update_path(char **args) {
    // Liberar el path existente
    for (int i = 0; i < path_count; i++) {
        free(path[i]);
    }
    free(path);

    // Contar el número de argumentos (rutas) proporcionados
    path_count = 0;
    while (args[path_count + 1] != NULL) {
        path_count++;
    }

    // Asignar nuevo espacio para path y copiar las rutas
    path = malloc((path_count + 1) * sizeof(char*));
    for (int i = 0; i < path_count; i++) {
        path[i] = strdup(args[i + 1]);
    }
    path[path_count] = NULL; // Terminador para el arreglo
}

// Función para ejecutar un comando
void execute(char **args) {
    if (args[0] == NULL) return; // No command entered

    // Comando "exit" para salir del shell
    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            fprintf(stderr, "An error has occurred\n");
        } else {
            exit(0); // Termina el shell solo si no hay argumentos adicionales
        }
        return;
    }

    // Comando "cd" para cambiar de directorio
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            fprintf(stderr, "An error has occurred\n");
        } else if (chdir(args[1]) != 0) {
            fprintf(stderr, "An error has occurred\n");
        }
        return; // Retornar sin forkear el proceso
    }

    // Comando "path" para actualizar las rutas
    if (strcmp(args[0], "path") == 0) {
        update_path(args);
        return; // Retornar sin forkear el proceso
    }

    // Verificar si hay redirección de salida
    int redirect_index = -1;
    int redirect_count = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            redirect_index = i;
            redirect_count++;
        }
    }

    // Validar errores de redirección
    if (redirect_count > 1) {
        // Error: múltiples símbolos ">"
        fprintf(stderr, "An error has occurred\n");
        return;
    } else if (redirect_index != -1 && args[redirect_index + 1] == NULL) {
        // Error: no hay archivo después de ">"
        fprintf(stderr, "An error has occurred\n");
        return;
    } else if (redirect_index != -1 && args[redirect_index + 2] != NULL) {
        // Error: más de un argumento después de ">"
        fprintf(stderr, "An error has occurred\n");
        return;
    }

    // Si se encuentra redirección
    char *output_file = NULL;
    if (redirect_index != -1) {
        output_file = args[redirect_index + 1];
        args[redirect_index] = NULL; // Cortar los argumentos para el comando
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo: redirigir la salida si es necesario
        if (output_file != NULL) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                fprintf(stderr, "An error has occurred\n");
                exit(EXIT_FAILURE);
            }
            // Redirigir stdout al archivo
            if (dup2(fd, STDOUT_FILENO) < 0) {
                fprintf(stderr, "An error has occurred\n");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }

        // Intentar ejecutar el comando en cada directorio del path
        char full_path[MAX_LINE];
        for (int i = 0; i < path_count; i++) {
            snprintf(full_path, sizeof(full_path), "%s/%s", path[i], args[0]);
            execv(full_path, args);
        }

        // Si llega aquí, execv falló en todos los intentos
        fprintf(stderr, "An error has occurred\n");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error en fork()
        fprintf(stderr, "An error has occurred\n");
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
        fprintf(stderr, "An error has occurred\n");
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

    path = malloc(2 * sizeof(char*));
    path[0] = strdup("/usr/bin");
    path[1] = NULL;
    path_count = 1;

    if (argc == 1) {
        // Sin argumentos, ejecutar en modo interactivo
        interactive_mode();
    } else if (argc == 2) {
        // Un argumento, ejecutar en modo batch
        batch_mode(argv[1]);
    } else {
        // Demasiados argumentos, imprimir error
        fprintf(stderr, "An error has occurred\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < path_count; i++) {
        free(path[i]);
    }
    free(path);

    return 0;
}
