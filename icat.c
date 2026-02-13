/* 
 * Author: Sarthak Sameer Bal
 * Project Name: icat (an improved alternative to cat)
 *
 * icat is an implementation of the UNIX command 'cat' made by Torbjorn Granlund and Richard M. Stallman.
 * icat can parse through structured data like JSON, CSV, XML. At least thats the plan right now.
 *
 * If you're reading this, icat is not complete due to time constraint, but as far as normal implementation of the cat command, it is usable and does the job.
*/

#define VERSION "v0.1"

#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include<getopt.h>
#include<errno.h>

typedef uint64_t u64;

// switches that can be toggled
typedef struct {
    bool show_all;
    bool number_nonblank;
    bool show_ends;
    bool number;
    bool squeeze_blank;
    bool show_tabs;
    bool show_nonprinting;
} options;

// basic states for persistent tracking across files
typedef struct {
    u64 line_number;
    bool line_beginning;
    bool prev_line_blank;
} state;

void print_help() {
    printf(
	    "Usage cat [OPTIONS]... [FILE]...\n"
	    "Concatenate FILE(s) to standard output.\n"
	    "With no FILE, or when FILE is -, read standard input.\n\n"
	    "  -A          equivalent to -vET\n"
	    "  -b          number nonempty output lines, overrides -n\n"
	    "  -e          equivalent to -vE\n"
	    "  -E          display $ or ^M$ at end of each line\n"
	    "  -n          number all output lines\n"
	    "  -s          suppress repeated empty output lines\n"
	    "  -t          equivalent to -vT\n"
	    "  -T          display TAB characters as ^I\n"
	    "  -u          (ignored)\n"
	    "  -v          use ^ and M- notation, except for LFD and TAB\n"
	    "  -h          display this help and exit\n"
	    "  -v          output version informationn\n"
	  );
}

// to make error checking easier with int return
int chr_checked(int ch){
    if(putchar(ch) == EOF){
	return -1;
    }
    return 0;
}

int str_checked(char *s) {
    while(*s != '\0') {
	if(chr_checked((unsigned char)*s++) != 0){
	    return -1;
	}
    }
    return 0;
}

int print_line_number(state *state){
    if(printf("%6lu\t", state->line_number++) < 0){
	return -1;
    }
    return 0;
}

int visible_chr(unsigned char ch, options *opt){
    if(!opt->show_nonprinting){
	return chr_checked(ch);
    }

    if (ch == '\n' || ch == '\t'){
	return chr_checked(ch);
    }

    if (ch < 32) {
	if(chr_checked('^') != 0 || chr_checked(ch + 64) != 0){
	    return -1;
	}
	return 0;
    }

    if (ch == 127) {
	return str_checked("^?");
    }

    if (ch >= 128) {
	unsigned char inner = (unsigned char)(ch - 128);
	if(str_checked("M-") != 0) {
	    return -1;
	}

	if(inner < 32) {
	    if(chr_checked('^') != 0 || chr_checked(inner + 64) != 0){
		return -1;
	    }
	    return 0;
	}

	if (inner == 127) {
	    return str_checked("^?");
	}
	return chr_checked(inner);
    }
    return chr_checked(ch);
}

int file_stream(FILE *file, char *name, options *opt, state *state) {
    (void)name; 

    int ch;
    while ((ch = getc(file)) != EOF) {
        unsigned char uch = (unsigned char)ch;

        if (state->line_beginning) {
            bool current_line_is_blank = (uch == '\n');

            if (opt->squeeze_blank && current_line_is_blank && state->prev_line_blank) {
                continue;
            }

            if (opt->number_nonblank) {
                if (!current_line_is_blank) {
                    if (print_line_number(state) != 0) {
                        return -1;
                    }
                }
            } else if (opt->number) {
                if (print_line_number(state) != 0) {
                    return -1;
                }
            }

            state->line_beginning = false;
        }

        if (uch == '\t' && opt->show_tabs) {
            if (str_checked("^I") != 0) {
                return -1;
            }
            continue;
        }

        if (uch == '\n') {
            if (opt->show_ends && chr_checked('$') != 0) {
                return -1;
            }
            if (chr_checked('\n') != 0) {
                return -1;
            }

            state->line_beginning = true;
            state->prev_line_blank = true;
            continue;
        }

        state->prev_line_blank = false;

        if (visible_chr(uch, opt) != 0) {
            return -1;
        }
    }

    if (ferror(file)) {
        return -2;
    }

    return 0;
}

void error_check(const char *subject) {
    fprintf(stderr, "cat: %s: %s\\n", subject, strerror(errno));
}


int main(int argc, char **argv){
    options opts = {0};
    state state = {
        .line_number = 1,
        .line_beginning = true,
        .prev_line_blank = false,
    };

    int opt;
    while ((opt = getopt(argc, argv, "AbeEnstTuvh")) != -1) {
        switch (opt) {
            case 'A':
                opts.show_nonprinting = true;
                opts.show_ends = true;
                opts.show_tabs = true;
                break;
            case 'b':
                opts.number_nonblank = true;
                break;
            case 'e':
                opts.show_nonprinting = true;
                opts.show_ends = true;
                break;
            case 'E':
                opts.show_ends = true;
                break;
            case 'n':
                opts.number = true;
                break;
            case 's':
                opts.squeeze_blank = true;
                break;
            case 't':
                opts.show_nonprinting = true;
                opts.show_tabs = true;
                break;
            case 'T':
                opts.show_tabs = true;
                break;
            case 'u':
                break;
            case 'v':
                opts.show_nonprinting = true;
                break;
            case 'h':
                print_help();
                return 0;
            default:
                print_help();
                return 1;
        }
    }

    if (opts.number_nonblank) {
        opts.number = false;
    }

    int exit_code = 0;

    if (optind == argc) {
        int rc = file_stream(stdin, "-", &opts, &state);
        if (rc == -1) {
            error_check("stdout");
            return 1;
        }
        if (rc == -2) {
            error_check("stdin");
            return 1;
        }
    } else {
        for (int i = optind; i < argc; i++) {
            char *name = argv[i];

            if (strcmp(name, "-") == 0) {
                int rc = file_stream(stdin, "-", &opts, &state);
                if (rc == -1) {
                    error_check("stdout");
                    return 1;
                }
                if (rc == -2) {
                    error_check("stdin");
                    exit_code = 1;
                }
                continue;
            }

            FILE *in = fopen(name, "rb");
            if (in == NULL) {
                error_check(name);
                exit_code = 1;
                continue;
            }

            int rc = file_stream(in, name, &opts, &state);

            if (rc == -1) {
                error_check("stdout");
                fclose(in);
                return 1;
            }

            if (rc == -2) {
                error_check(name);
                exit_code = 1;
            }

            if (fclose(in) != 0) {
                error_check(name);
                exit_code = 1;
            }
        }
    }

    if (fflush(stdout) != 0) {
        error_check("stdout");
        return 1;
    }

    return exit_code;

}
