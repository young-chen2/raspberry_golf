#include "shell.h"
#include "shell_commands.h"
#include "uart.h"
#include "malloc.h"
#include "strings.h"
#include "printf.h"
#include "pi.h"
#include "ps2.h"
#include "keyboard.h"
#include "ps2_keys.h"

#define LINE_LEN 80
#define MIN(a,b) (((a) < (b)?(a):(b))
static input_fn_t shell_read;
static formatted_fn_t shell_printf;

// NOTE TO STUDENTS: It will greatly help our grading if you use the following
// format strings in the following contexts. We provide the format strings; you
// will need to determine what the right arguments (if any) should be.
//
// When the user attempts to execute a command that doesn't exist:
// "error: no such command `%s`. Use `help` for list of available commands.\n"
//
// When the user attempts to run `help <cmd>` for a command that doesn't exist:
// "error: no such command `%s`.\n"
//
// When the user attempts to `peek` or `poke` an address that is not 4-byte aligned:
// "error: %s address must be 4-byte aligned\n"
//
// When running `peek` on an address, the resulting printout should be formatted thus:
// "%p:   %08x\n"

int cmd_history(int argc, const char *argv[]);
static const command_t commands[] = {
    {"help",   "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",   "<...> echos the user input to the screen", cmd_echo},
    {"reboot", "reboot the raspberry pi back to bootloader", cmd_reboot},
    {"peek", "[address] print contents of memory at address", cmd_peek},
    {"poke", "stores [value] into memory at [address]", cmd_poke},
    {"history", "history of recent commands prefixed with its command number", cmd_history}
};

static const int NUM_COMMANDS = sizeof(commands) / sizeof(command_t);
static char* hist[1000];
static int num_hist = 0;

int cmd_history(int argc, const char *argv[]){
    if (argc == 1){
        /* If less than 10 histories, print all */
        if (num_hist <= 10){
            for (int i = 0; i < num_hist; i++){
                shell_printf("%d %s\n", i + 1, hist[i]);
            }
            return 0;
        }
        /* If more than 10 histories, print last 10 */ 
        else{
            for (int i = num_hist - 10; i < num_hist; i++){
                shell_printf("%d %s\n", i - num_hist + 11, hist[i]);
            }
            return 0;
        }

    }
    else{
        return 1;
    }

}
int cmd_poke(int argc, const char *argv[]){
    /* If no both address and value, cmd_poke would alert */
    if (argc <= 2){
        shell_printf("error: poke expects 2 arguments [address] [value]\n");
        return 1;
    } 
    else{
        const char *input = argv[1], *rest = NULL;
        const char *input2 = argv[2], *rest2 = NULL;
        int val = strtonum(input, &rest);
        int val2 = strtonum(input2, &rest2);
        if (rest == &input[0]){
            shell_printf("error: poke cannot convert '%s'\n", argv[1]);
            return 1;
        }

        else if (rest2 == &input2[0]){
            shell_printf("error: poke cannot convert '%s'\n", argv[2]);
            return 1;
        }

        else{
            /* Make sure memory is aligned by 4 bytes */
            if ((val & 0b11) != 0){
                shell_printf("error: %s address must be 4-byte aligned\n", argv[1]);
                return 1;
            }
            else{
                *(int *) val = val2;
                shell_printf("%p:   %08x\n", (int*)val, val2);
                return 0;
            }
        }
    }

}


int cmd_peek(int argc, const char *argv[]){
    /* If no address argument, cmd_peek would alert */
    if (argc <= 1){
        shell_printf("error: peek expects %d argument [address]\n", 1);
        return 1;
    }

    else{
        /* Make sure string is valid address */
        const char *input = argv[1], *rest = NULL;
        int val = strtonum(input, &rest);
        if (rest == &input[0]){
            shell_printf("error: peek cannot convert '%s'\n", argv[1]);
            return 1;
        }
        else{
            /* Need to make sure memory is aligned by 4 bytes */
            if ((val & 0b11) != 0){
                shell_printf("error: peek address must be 4-byte aligned\n");
                return 1;
            }
            else{
                shell_printf("%p:   %08x\n", (int*) val, *(int *) val);
                return 0;
            }
        }
    }
}

int cmd_echo(int argc, const char *argv[])
{
    for (int i = 1; i < argc; ++i)
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

int cmd_reboot(int argc, const char *argv[]){
    uart_putchar(EOT);
    pi_reboot();
    return 0;
}

int cmd_help(int argc, const char *argv[])
{
    if (argc == 1){
        for (int i = 0; i < NUM_COMMANDS; i++){
            shell_printf("%s: %s\n", commands[i].name, commands[i].description);
        }
        return 0;
    }

    else{
        for (int i = 0; i < NUM_COMMANDS; i++){
            if (strcmp(commands[i].name, argv[1]) == 0){
                shell_printf("%s: %s\n", commands[i].name, commands[i].description);
                return 0;
            }
        }
        shell_printf("error: no such command `%s`.\n", argv[1]);
        return 1;
    }
}

void shell_init(input_fn_t read_fn, formatted_fn_t print_fn)
{
    shell_read = read_fn;
    shell_printf = print_fn;
}

void shell_bell(void)
{
    uart_putchar('\a');
}

void shell_readline(char buf[], size_t bufsize)
{   
    /* Step 1: Initialize position for writing in buf */
    int cur_hist = num_hist;
    int max_pos = 0;
    int pos = 0;
    while (1){
        /* Read one char from keyboard */
        char cur = shell_read();

        /* If meet return key, ignore latter commands! */
        if (cur == '\n'){
            buf[max_pos] = '\0';
            shell_printf("\n");
            break;
        }

        /* If meet backspace key, delete previous letter */
        else if (cur == '\b'){
            /* Start Managing buf */
            if (pos == max_pos){
                if (pos > 0){
                    pos--;
                    max_pos--;
                    buf[pos] = '\0';
                    shell_printf("\b \b");
                }
            }
            else if (pos < max_pos && pos > 0){
                /* Step 1: Cut a hole at target position */
                pos--;
                max_pos--;
                buf[pos] = '\0';
                /* Step 2: Manages second part of line */    
                int i = pos + 1;
                while (buf[i] != '\0'){
                    buf[i-1] = buf[i];
                    i++;
                }
                buf[i-1] = '\0';
        
                /* End managing buf */
                
                /* Start managing shell_printf */
                shell_printf("\b \b");

                for (int i = pos + 1; i < LINE_LEN; i++){
                    shell_printf(" ");
                }
                for (int i = LINE_LEN; i > pos + 1; i--){
                    shell_printf("\033[D");
                }
                shell_printf(buf + pos);
                /* Restore cursor */
                for (int i = max_pos; i > pos; i--){
                    shell_printf("\033[D");
                }    
            
            }
            else{
                shell_bell();
            }
        }

        /* Add in left and right arrow key to add in cursor */
        else if (cur == PS2_KEY_ARROW_LEFT){
            if (pos > 0){
                pos--;
                shell_printf("\033[D");
            }
            else{
                shell_bell();
            }
        }
        /* Add in favorite vim command: ctrl + a to move cursor front of line */
        else if (cur == 0x1){
            while (pos > 0){
                pos--;
                shell_printf("\033[D");
            }
        }
        
        else if (cur == PS2_KEY_ARROW_RIGHT){
            if (pos >= LINE_LEN || pos >= max_pos){
                shell_bell();    // Cannot move beyond last char or line limit
            }
            else{
                pos++;
                shell_printf("\033[C");
            }
        }

        else if (cur == PS2_KEY_ARROW_UP){
           if (cur_hist > 0){
               cur_hist--;
              
                    
               for (int i = pos; i > 0; i--){
                   shell_printf("\033[D");
               }
               for (int i = 0; i < LINE_LEN; i++){
                   shell_printf(" ");
               }
               for (int i = LINE_LEN; i > 0; i--){
                   shell_printf("\033[D");
               }
               shell_printf("%s", hist[cur_hist]);
               memcpy(buf, hist[cur_hist], strlen(hist[cur_hist]));
               buf[strlen(hist[cur_hist])] = '\0';
               max_pos = strlen(hist[cur_hist]);
               pos = max_pos;
           }
           else{
               shell_bell();
           }
        }

        else if (cur == PS2_KEY_ARROW_DOWN){
            if (cur_hist < num_hist){
           //     pos = strlen(hist[cur_hist]);
                cur_hist++;


               for (int i = pos; i > 0; i--){
                   shell_printf("\033[D");
               }
               for (int i = 0; i < LINE_LEN; i++){
                   shell_printf(" ");
               }
               for (int i = LINE_LEN; i > 0; i--){
                   shell_printf("\033[D");
               }
               shell_printf("%s", hist[cur_hist]);
               memcpy(buf, hist[cur_hist], strlen(hist[cur_hist]));
               buf[strlen(hist[cur_hist])] = '\0';
               max_pos = strlen(hist[cur_hist]);
               pos = max_pos;
            }
            else{
                shell_bell();
            }
        }
        
        
        else{
            /* If another letter is in, and out of space,
             * ring the bell! */
            if (pos >= bufsize - 1){
                shell_bell();
            }
            
            /* Otherwise, write in and push position one
             * forward, print the new char on screen */
            else{
                if (pos == max_pos){
                    buf[pos] = cur;
                    buf[pos + 1] = '\0';
                    shell_printf(buf + pos);
                    max_pos++;
                    pos++;
                }
                else{
                    /* manage the buf */
                    pos++;
                    max_pos++;
                    for (int i = max_pos; i > pos - 1; i--){
                        buf[i] = buf[i-1];
                    }
                    buf[pos - 1] = cur;
                    /* end manage the buf */
                    /* start manage the shell output */
                    for (int i = pos; i < LINE_LEN; i++){
                        shell_printf(" ");
                    }
                    for (int i = LINE_LEN; i > pos; i--){
                        shell_printf("\033[D");
                    }
                    /* print part 2 of the output */
                    shell_printf("%c", cur);
                    shell_printf(buf + pos);
                    /* Restore cursor */
                    for (int i = max_pos; i > pos; i--){
                        shell_printf("\033[D");
                    }

                    /* end manage the shell output */
                }
            }
        }
    }
}

/**
 * isspace, strndup, and tokenize are code found from
 * lab 4. since header for these functions are already
 * written, i'll just make some in-line annotations.
 *
 */
static bool isspace(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n';
}

static char *strndup(const char *src, size_t n){
    char* str = malloc(n);
    memcpy(str, src, n);
    str[n] = '\0';
    return str;
}


static int tokenize(const char *line, char *array[], int max){

    int ntokens = 0;
    const char *cur = line;

    while (ntokens < max){
        while (isspace(*cur)) cur++;
        if (*cur == '\0') break;
        const char *start = cur;
        while (*cur != '\0' && !isspace(*cur)) cur++;
        /* Make a new array whenever a new token is discovered */
        array[ntokens++] = strndup(start, cur - start);
    }
    return ntokens;
}


int shell_evaluate(const char *line)
{   
    char* array[LINE_LEN];
    /* Tokenize the command line */
    int tot_parts = tokenize(line, array, LINE_LEN);
    // shell_printf("%d", tot_parts);

    if (tot_parts == 0){
        shell_printf("error: no such command `%s`. Use `help` for list of available commands.\n", "");
        return 1;
    }
    for (int i = 0; i < NUM_COMMANDS; i++){
        if (strcmp(commands[i].name, array[0]) == 0){
            return commands[i].fn(tot_parts, (const char**) array);
        }
    }

    shell_printf("error: no such command `%s`. Use `help` for list of available commands.\n", array[0]);
    return 1;
}

void shell_run(void)
{
    shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1)
    {
        char line[LINE_LEN];

        shell_printf("[%d] Pi> ", num_hist + 1);
        shell_readline(line, sizeof(line));
        char* line_ptr = malloc(LINE_LEN);
        if (strcmp(line, "") != 0){
            memcpy(line_ptr, line, strlen(line));
            hist[num_hist] = line_ptr;
            num_hist++;
        }
        shell_evaluate(line);
        
    } 
}
