#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../input.h"
#include "options.h"

// information about input option
struct option
{
    const char* name;
    const char* help;
    int required;
    const char* type;
    int (*read)(const char*, void*);
    int (*write)(char*, size_t, const void*);
    union {
        void* default_null;
        const char* default_string;
        int default_bool;
        int default_int;
        double default_real;
    } default_value;
    size_t offset;
    size_t size;
};

// mark option as required or optional
#define OPTION_REQUIRED(type) 1, #type, read_##type, write_##type, { .default_null = NULL }
#define OPTION_OPTIONAL(type, value) 0, #type, read_##type, write_##type, { .default_##type = value }

// get offset of option field in input
#define OPTION_FIELD(field) offsetof(struct input, field), sizeof(((struct input*)0)->field)

// macro to declare a new type
#define DECLARE_TYPE(type) \
    int read_##type(const char*, void*); \
    int write_##type(char*, size_t, const void*);

// declare known option types
DECLARE_TYPE(string)
DECLARE_TYPE(bool)
DECLARE_TYPE(int)
DECLARE_TYPE(real)

// list of known options
struct option OPTIONS[] = {
    {
        "image",
        "Input image, FITS file in counts/sec",
        OPTION_REQUIRED(string),
        OPTION_FIELD(image)
    },
    {
        "gain",
        "Conversion factor to counts",
        OPTION_REQUIRED(real),
        OPTION_FIELD(gain)
    },
    {
        "offset",
        "Subtracted flat-field offset",
        OPTION_REQUIRED(real),
        OPTION_FIELD(offset)
    },
    {
        "root",
        "Root element for all output paths",
        OPTION_REQUIRED(string),
        OPTION_FIELD(root)
    },
    {
        "mask",
        "Input mask, FITS file",
        OPTION_OPTIONAL(string, NULL),
        OPTION_FIELD(mask)
    },
    {
        "nlive",
        "Number of live points",
        OPTION_OPTIONAL(int, 300),
        OPTION_FIELD(nlive)
    },
    {
        "ins",
        "Use importance nested sampling",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(ins)
    },
    {
        "mmodal",
        "Multi-modal posterior (if ins = false)",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(mmodal)
    },
    {
        "ceff",
        "Constant efficiency mode",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(ceff)
    },
    {
        "tol",
        "Tolerance for evidence",
        OPTION_OPTIONAL(real, 0.1),
        OPTION_FIELD(tol)
    },
    {
        "eff",
        "Sampling efficiency",
        OPTION_OPTIONAL(real, 0.05),
        OPTION_FIELD(eff)
    },
    {
        "maxmodes",
        "Maximum number of expected modes",
        OPTION_OPTIONAL(int, 100),
        OPTION_FIELD(maxmodes)
    },
    {
        "updint",
        "Update interval for output",
        OPTION_OPTIONAL(int, 1000),
        OPTION_FIELD(updint)
    },
    {
        "seed",
        "Random number seed for sampling",
        OPTION_OPTIONAL(int, -1),
        OPTION_FIELD(seed)
    },
    {
        "fb",
        "Show MultiNest feedback",
        OPTION_OPTIONAL(bool, 0),
        OPTION_FIELD(fb)
    },
    {
        "resume",
        "Resume from last checkpoint",
        OPTION_OPTIONAL(bool, 0),
        OPTION_FIELD(resume),
    },
    {
        "outfile",
        "Output MultiNest files",
        OPTION_OPTIONAL(bool, 0),
        OPTION_FIELD(outfile)
    },
    {
        "maxiter",
        "Maximum number of iterations",
        OPTION_OPTIONAL(int, 0),
        OPTION_FIELD(maxiter)
    }
};

// number of known options
#define NOPTIONS (sizeof(OPTIONS)/sizeof(struct option))

// struct containing data about options for input
struct input_options
{
    struct input* input;
    char resolved[NOPTIONS];
    char message[255];
};

struct input_options* get_options(struct input* input)
{
    struct input_options* options = malloc(sizeof(struct input_options));
    options->input = input;
    memset(options->resolved, 0, NOPTIONS);
    options->message[0] = 0;
    return options;
}

size_t check_options(const struct input_options* options)
{
    for(size_t i = 0; i < NOPTIONS; ++i)
        if(!options->resolved[i])
            return i;
    return -1;
}

void free_options(struct input_options* options)
{
    free(options);
}

void default_options(struct input_options* options)
{
    for(int i = 0; i < NOPTIONS; ++i)
    {
        // copy default value to input
        memcpy((char*)options->input + OPTIONS[i].offset, &OPTIONS[i].default_value, OPTIONS[i].size);
        
        // mark options that are not required as resolved
        options->resolved[i] = !OPTIONS[i].required;
    }
}

const char* options_error(const struct input_options* options)
{
    return options->message;
}

int read_option(const char* name, const char* value, struct input_options* options)
{
    // read all chars in name
    return read_option_n(name, strlen(name), value, options);
}

int read_option_n(const char* name, int n, const char* value, struct input_options* options)
{
    int opt;
    
    // find option
    for(opt = 0; opt < NOPTIONS; ++opt)
        if(strncmp(name, OPTIONS[opt].name, n) == 0)
            break;
    
    // error if option was not found
    if(opt == NOPTIONS)
    {
        snprintf(options->message, sizeof(options->message), "invalid option \"%.*s\"", n, name);
        return ERR_OPTION_NAME;
    }
    
    // error if no value was given
    if(!value || !*value)
    {
        snprintf(options->message, sizeof(options->message), "no value given for option \"%.*s\"", n, name);
        return ERR_OPTION_VALUE;
    }
    
    // try to read option and return eventual errors
    if(OPTIONS[opt].read(value, (char*)options->input + OPTIONS[opt].offset))
    {
        snprintf(options->message, sizeof(options->message), "invalid value \"%s\" for option \"%.*s\"", value, n, name);
        return ERR_OPTION_VALUE;
    }
    
    // mark option as resolved
    options->resolved[opt] = 1;
    
    // report success
    return OPTION_OK;
}

size_t noptions()
{
    return NOPTIONS;
}

const char* option_name(size_t n)
{
    return OPTIONS[n].name;
}

const char* option_type(size_t n)
{
    return OPTIONS[n].type;
}

const char* option_help(size_t n)
{
    return OPTIONS[n].help;
}

int option_required(size_t n)
{
    return OPTIONS[n].required;
}

int option_default_value(char* buf, size_t buf_size, size_t n)
{
    return OPTIONS[n].write(buf, buf_size, &OPTIONS[n].default_value);
}

int option_value(char* buf, size_t buf_size, const struct input* input, size_t n)
{
    return OPTIONS[n].write(buf, buf_size, (char*)input + OPTIONS[n].offset);
}

// begin implementation of option types

int read_string(const char* in, void* out)
{
    char** out_str = out;
    *out_str = malloc(strlen(in) + 1);
    strcpy(*out_str, in);
    return 0;
}

int write_string(char* out, size_t n, const void* in)
{
    char* const* in_str = in;
    snprintf(out, n, "%s", *in_str ? *in_str : "(none)");
    return 0;
}

int read_bool(const char* in, void* out)
{
    int* out_bool = out;
    if(
        strcmp(in, "true") == 0 ||
        strcmp(in, "TRUE") == 0 ||
        strcmp(in, "1") == 0
    )
        *out_bool = 1;
    else if(
        strcmp(in, "false") == 0 ||
        strcmp(in, "FALSE") == 0 ||
        strcmp(in, "0") == 0
    )
        *out_bool = 0;
    else
        return 1;
    return 0;
}

int write_bool(char* out, size_t n, const void* in)
{
    const int* in_bool = in;
    snprintf(out, n, "%s", *in_bool ? "true" : "false");
    return 0;
}

int read_int(const char* in, void* out)
{
    int* out_int = out;
    char* end;
    long l = strtol(in, &end, 10);
    if(*end || l < INT_MIN || l > INT_MAX)
        return 1;
    *out_int = l;
    return 0;
}

int write_int(char* out, size_t n, const void* in)
{
    const int* in_int = in;
    snprintf(out, n, "%d", *in_int);
    return 0;
}

int read_real(const char* in, void* out)
{
    double* out_double = out;
    char* end;
    *out_double = strtod(in, &end);
    if(*end)
        return 1;
    return 0;
}

int write_real(char* out, size_t n, const void* in)
{
    const double* in_double = in;
    snprintf(out, n, "%g", *in_double);
    return 0;
}

// end implementation of option types