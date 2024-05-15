#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned long mix(unsigned char *str) {
    unsigned long mix = 0;
    unsigned long dis = 31; 
    while (*str) {
        mix = (mix * dis) + (*str++);
    }
    return mix%100;
}

typedef struct custom_node {
    struct custom_node* after;
    struct custom_node* cus;
    char* str;
    char* entry;
} CustomNode;

typedef struct {
    CustomNode* front;
    CustomNode* back;
    CustomNode* store[100];
} LinkedList;

LinkedList init_node_list();

typedef struct proc {
    struct proc* r;
    struct proc* l;
    char* out;
} Proc;

typedef struct {
    int size;
    int inc;
    Proc* front;
    Proc* back;
} History;

History init_history();

LinkedList init_node_list() {
    LinkedList node_list;
    for (int i = 0; i < 100; ++i) {
        node_list.store[i] = NULL;
    }
    node_list.front = NULL;
    node_list.back = NULL;
    return node_list;
}

History init_history() {
    History history;
    history.inc = 0;
    history.size = 5;
    history.front = NULL;
    history.back = NULL;
    return history;
}

void append_to_output(char* output, size_t* output_length, const char* str) {
    size_t str_length = strlen(str);
    *output_length += str_length;
    if (*output_length >= 1300) {
        fprintf(stderr, "String length exceeded\n");
        exit(EXIT_FAILURE);
    }
    strcat(output, str);
}

void add_to_history(History *hist, char* out) {
    if (out == NULL || hist->size == 0) {
        return;
    }
    if (hist->front != NULL && strcmp(hist->front->out, out) == 0) {
        return;
    }
    Proc* new_cmd = malloc(sizeof(Proc));
    if (new_cmd == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    new_cmd->out = strdup(out);
    if (new_cmd->out == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    new_cmd->r = NULL;
    new_cmd->l = NULL;
    if (hist->front == NULL) {
        hist->front = new_cmd;
        hist->back = new_cmd;
        hist->inc = 1;
    } else {
        new_cmd->r = hist->front;
        hist->front->l = new_cmd;
        hist->front = new_cmd;
        if (hist->inc >= hist->size) {
            Proc* temp = hist->back;
            hist->back = hist->back->l;
            hist->back->r = NULL;
            free(temp->out);
            free(temp);
        } else {
            hist->inc++;
        }
    }
}

void strip(char *in) {
    char *check = in;
    bool in_whitespace = false;
    while (*in) {
        if (!isspace((unsigned char)*in)) {
            *check++ = *in;
            in_whitespace = false;
        } else {
            if (!in_whitespace) {
                *check++ = ' ';
                in_whitespace = true;
            }
        }
        in++;
    }
    *check = '\0';
}


void execute_command(char *args[]) {
    pid_t id = fork();
    if (id == 0) {
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (id < 0) {
        exit(EXIT_FAILURE);
    } else {
        int state;
        waitpid(id, &state, WUNTRACED);
    }
}



char* tokenize_input(char* input, LinkedList* node_list, int* pc) {
    const char comma[] = ",";
    char* output = (char*)malloc(1300);
    if (output == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    size_t output_length = 0;
    char* ch;
    
    while ((ch = strsep(&input, " ")) != NULL) {
        if (strcmp(ch, "|") == 0) {
            (*pc)++;
        } else if (ch[0] == '$') {
            char* env_var;
            ch++;
            if ((env_var = getenv(ch)) != NULL) {
                append_to_output(output, &output_length, env_var);
                append_to_output(output, &output_length, comma);
                continue;
            } 
            int num = mix((unsigned char*)ch);
            CustomNode* current_node = node_list->store[num];
            while (current_node != NULL) {
                if (strcmp(current_node->str, ch) == 0) {
                    append_to_output(output, &output_length, current_node->entry);
                    append_to_output(output, &output_length, comma);
                    num = -1;
                    break;
                }
                current_node = current_node->after;
            }
            continue;
        }
        append_to_output(output, &output_length, ch);
        append_to_output(output, &output_length, comma);
    }
    output[output_length] = '\0';
    strip(output);
    return output;
}

void create_pipes(int (*pipe_fds)[2], int num_pipes) {
    for(int i = 0; i < num_pipes; i++) {
        if(pipe(pipe_fds[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
}

void resize_str_array(char*** a, size_t* size, int inc) {
    size_t new_size = *size + inc;
    char** new_array = (char**)realloc(*a, new_size * sizeof(char *));
    if (new_array == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    *a = new_array;
    *size = new_size;
}

void handle_child(int i, int num_pipes, int (*pipe_fds)[2], char** args) {
    if(i < num_pipes) {
        if(dup2(pipe_fds[i][1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
    if(i > 0) {
        if(dup2(pipe_fds[i - 1][0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
    
    for(int j = 0; j < num_pipes; j++) {
        close(pipe_fds[j][0]);
        close(pipe_fds[j][1]);
    }
    
    if(execvp(args[0], args) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}


void handle_pipes(char* command_line, int num_pipes) {
    int (*pipe_fds)[2] = malloc((num_pipes) * sizeof(int[2]));
    pid_t pid;
    create_pipes(pipe_fds, num_pipes);
    for(int i = 0; i < num_pipes + 1; i++) {
        char** args = NULL;
        char* token;
        size_t arg_size = 0;
        int arg_count = 0;
        
        while((token = strsep(&command_line, ",")) != NULL && strcmp(token, "|") != 0) {
            if(strlen(token) == 0) {
                continue;
            }
            resize_str_array(&args, &arg_size, 1);
            args[arg_count] = token;
            arg_count++;
        }
        resize_str_array(&args, &arg_size, 1);
        args[arg_count] = NULL;
        
        pid = fork();
        if(pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if(pid == 0) {
            handle_child(i, num_pipes, pipe_fds, args);
        } 
    }
    for(int i = 0; i < num_pipes; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    
    while(wait(NULL) > 0);
    
    free(pipe_fds);
}

void handle_exit(char** args, int arg_count) {
    if(arg_count > 1) {
        exit(-1);
    }
    exit(0);
}

void handle_cd(char** args, int arg_count) {
    if(arg_count == 1 || arg_count > 2 || chdir(args[1]) == -1) {
        exit(-1);
    }
}

void handle_export(char** args, int arg_count) {
    char* str = strsep(&args[1], "=");
    char* value = strsep(&args[1], " ");
    if(setenv(str, value, 1) != 0) {
        exit(-1);
    }
}

void handle_local(char** args, int arg_count, LinkedList* node_list) {
    CustomNode* new_node = malloc(sizeof(CustomNode));
    new_node->str = strdup(strsep(&args[1], "="));
    new_node->entry = strdup(strsep(&args[1], " "));
    int hash_val = mix((unsigned char*)new_node->str);
    if(node_list->front == NULL) {
        node_list->store[hash_val] = new_node;
        node_list->front = new_node;
        node_list->back = new_node;
    } else if(node_list->store[hash_val] == NULL) {
        node_list->store[hash_val] = new_node;
        node_list->back->after = new_node;
        node_list->back = new_node;
    } else {
        CustomNode* temp = node_list->store[hash_val];
        while(temp) {
            if(strcmp(temp->str, new_node->str) == 0) {
                temp->entry = new_node->entry;
                break;
            }
            if(temp->cus == NULL) {
                temp->cus = new_node;
                node_list->back->after = new_node;
                node_list->back = new_node;
            }
        }
    }
}

void handle_vars(LinkedList* node_list) {
    CustomNode* temp = node_list->front;
    while(temp) {
        if(strlen(temp->entry) != 0) {
            printf("%s=%s\n", temp->str, temp->entry);
        }
        temp = temp->after;
    }
}

void handle_history(char** args, int arg_count, History* history, LinkedList* node_list, char* clean_line) {
    if(arg_count == 1 || args[1] == NULL) {
        Proc* temp = history->front;
        int n = 1;
        while(temp) {
            printf("%i) %s\n", n++, temp->out);
            temp = temp->r;
        }
        return;
    }   
    int cmd_n = atoi(args[1]);
    if(strcmp(args[1], "set") == 0) {
        int n = atoi(args[2]);
        if(n == 0) {
            history->front = NULL;
            history->back = NULL;
            history->inc = 0;
        } else if(n < history->inc) {
            int loop = 0;
            Proc* temp = history->front;
            while(++loop < n) {
                temp = temp->r;
            }
            temp->r = NULL;
            history->back = temp;
            history->inc = n;
        }
        history->size = n;
    } else if(cmd_n != 0) {
        if(cmd_n <= history->inc) {
            Proc* temp = history->front;
            while(--cmd_n != 0) {
                temp = temp->r;
            }
            int pipe_count = 0;
            char* line = tokenize_input(strdup(temp->out), node_list, &pipe_count);
            if(pipe_count == 0) {
                char* token;
                char** args = NULL;
                size_t arg_size = 0;
                int arg_count = 0;
                while((token = strsep(&line, ",")) != NULL) {
                    if(strlen(token) == 0) {
                        continue;
                    }
                    resize_str_array(&args, &arg_size, 1);
                    args[arg_count] = token;
                    arg_count++;
                }
                resize_str_array(&args, &arg_size, 1);
                args[arg_count] = NULL;
                execute_command(args);
            } else {
                handle_pipes(line, pipe_count);
            }
        }
    }
}



void handle_build(char* line, LinkedList* node_list, History* history, char* clean_line) {
    char* token;
    char** args = NULL;
    size_t arg_size = 0;
    int arg_count = 0;
    while((token = strsep(&line, ",")) != NULL) {
        if(strlen(token) == 0) {
            continue;
        }
        resize_str_array(&args, &arg_size, 1);
        args[arg_count] = token;
        arg_count++;
    }
    resize_str_array(&args, &arg_size, 1);
    args[arg_count] = NULL;

    if(strcmp(args[0], "exit") == 0) {
        handle_exit(args, arg_count);
    } else if(strcmp(args[0], "cd") == 0) {
        handle_cd(args, arg_count);
    } else if(strcmp(args[0], "export") == 0) {
        handle_export(args, arg_count);
    } else if(strcmp(args[0], "local") == 0) {
        handle_local(args, arg_count, node_list);
    } else if(strcmp(args[0], "vars") == 0) {
        handle_vars(node_list);
    } else if(strcmp(args[0], "history") == 0) {
        handle_history(args, arg_count, history, node_list, clean_line);
    } else {
        add_to_history(history, clean_line);
        execute_command(args);
    }
}

int main(int argc, char* argv[]) {
    History past = init_history();
    LinkedList node_list = init_node_list();
    
    if (argc != 2) {        
        while (1) {
            printf("wsh> ");
            char *user_input = NULL;
            size_t user_len = 0;
            ssize_t user_read = getline(&user_input, &user_len, stdin);

            if (user_read > 0 && user_input[user_read - 1] == '\n') {
                user_input[user_read - 1] = '\0';
            } else if (user_read == -1) {
                exit(EXIT_SUCCESS);
            }            
            int user_pipe_count = 0;
            char* clean_user_input = strdup(user_input);
            char* user_tokens = tokenize_input(user_input, &node_list, &user_pipe_count);
            if (user_pipe_count == 0) {
                handle_build(user_tokens, &node_list, &past, clean_user_input);
            } else {
                add_to_history(&past, clean_user_input);
                handle_pipes(user_tokens, user_pipe_count);
            }
            free(user_input); // Free allocated memory for user_input
            free(clean_user_input); // Free allocated memory for clean_user_input
        }
    } else {
        FILE* batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            exit(EXIT_FAILURE);
        }

        char* batch_line = NULL;
        size_t batch_len = 0;
        ssize_t batch_read;
        
        while ((batch_read = getline(&batch_line, &batch_len, batch_file)) != -1) {
            if (batch_read > 0 && batch_line[batch_read - 1] == '\n') {
                batch_line[batch_read - 1] = '\0';
            }
            char* clean_batch_line = strdup(batch_line);
            int batch_pipe_count = 0;
            char* batch_tokens = tokenize_input(batch_line, &node_list, &batch_pipe_count);
            if (batch_pipe_count == 0) {
                handle_build(batch_tokens, &node_list, &past, clean_batch_line);
            } else {
                add_to_history(&past, clean_batch_line);
                handle_pipes(batch_tokens, batch_pipe_count);
            }
            free(clean_batch_line); 
        }
        fclose(batch_file);
        if (batch_line) {
            free(batch_line);
        }
    }

    return 0;
}
