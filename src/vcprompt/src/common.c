#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"

result_t* init_result(options_t* options)
{
    result_t *rv = (result_t*) calloc(1, sizeof(result_t));
    ASSERT_NOOM(rv);
    if (options->default_branch)
        result_set_branch(rv, options->default_branch);
    if (options->default_revision)
        result_set_revision(rv, options->default_revision, -1);
    return rv;
}

void free_result(result_t* result)
{
    if (!result)
        return;
    free(result->revision);
    free(result->branch);
    free(result);
}

static options_t* _options = NULL;

void set_options(options_t* options)
{
    _options = options;
}

options_t* new_options(const char *format)
{
    options_t *rv = calloc(sizeof(options_t), 1);
    ASSERT_NOOM(rv);
    rv->format = strdup(format);
    ASSERT_NOOM(rv->format);
    return rv;
}

void free_options(options_t* options)
{
    if (!options)
        return;
    free(options->format);
    free(options->default_branch);
    free(options->default_revision);
    free(options);
}

void result_set_revision(result_t* result, const char *revision, int len)
{
    if (result->revision)
        free(result->revision);
    if (len < 0)
        result->revision = strdup(revision);
    else {
        result->revision = malloc(len);
        ASSERT_NOOM(result->revision);
        strncpy(result->revision, revision, len);
    }
}

void result_set_branch(result_t* result, const char *branch)
{
    if (result->branch)
        free(result->branch);
    result->branch = strdup(branch);
    ASSERT_NOOM(result->branch);
}

vccontext_t* init_context(const char *name,
                          options_t* options,
                          int (*probe)(vccontext_t*),
                          result_t* (*get_info)(vccontext_t*))
{
    vccontext_t* context = (vccontext_t*) calloc(1, sizeof(vccontext_t));
    context->options = options;
    context->name = name;
    context->probe = probe;
    context->get_info = get_info;
    return context;
}

void free_context(vccontext_t* context)
{
    free(context);
}

void debug(char* fmt, ...)
{
    va_list args;

    if (!_options->debug)
        return;

    va_start(args, fmt);
    fputs("vcprompt: debug: ", stdout);
    vfprintf(stdout, fmt, args);
    fputc('\n', stdout);
    va_end(args);
}

int isdir(char* name)
{
    struct stat statbuf;
    if (stat(name, &statbuf) < 0) {
        debug("failed to stat() '%s': %s", name, strerror(errno));
        return 0;
    }
    if (!S_ISDIR(statbuf.st_mode)) {
        debug("'%s' not a directory", name);
        return 0;
    }
    return 1;
}

int read_first_line(char* filename, char* buf, int size)
{
    FILE* file;

    file = fopen(filename, "r");
    if (file == NULL) {
        debug("error opening '%s': %s", filename, strerror(errno));
        return 0;
    }

    int ok = (fgets(buf, size, file) != NULL);
    fclose(file);
    if (!ok) {
        debug("error or EOF reading from '%s'", filename);
        return 0;
    }

    /* chop trailing newline */
    chop_newline(buf);

    return 1;
}

int read_last_line(char* filename, char* buf, int size)
{
    FILE* file;

    file = fopen(filename, "r");
    if (file == NULL) {
        debug("error opening '%s': %s", filename, strerror(errno));
        return 0;
    }

    buf[0] = '\0';
    while (fgets(buf, size, file));
    fclose(file);

    if (!buf[0]) {
        debug("empty line read from '%s'", filename);
        return 0;
    }

    /* chop trailing newline */
    chop_newline(buf);

    return 1;
}

int read_file(const char* filename, char* buf, int size)
{
    FILE* file;
    int readsize;

    file = fopen(filename, "r");
    if (file == NULL) {
        debug("error opening '%s': %s", filename, strerror(errno));
        return 0;
    }

    readsize = fread(buf, sizeof(char), size, file);

    fclose(file);

    return readsize;
}

void chop_newline(char* buf)
{
    int len = strlen(buf);
    if (buf[len-1] == '\n')
        buf[len-1] = '\0';
}

void dump_hex(const char* data, char* buf, int datasize)
{
    const char HEXSTR[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    int i;

    for (i = 0; i < datasize; ++i) {
        buf[i * 2] = HEXSTR[(unsigned char) data[i] >> 4];
        buf[i * 2 + 1] = HEXSTR[(unsigned char) data[i] & 0x0f];
    }

    buf[i * 2] = '\0';
}
