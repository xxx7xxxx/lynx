#ifndef _CONFIGURE_H
#define _CONFIGURE_H

#define MODEL_SUB       0
#define MODEL_REACTOR   1
#define MODEL_THREADS   2

struct conf_atom {
        char    *key;
        int     value;

};

extern struct conf_atom conf[];

void start_conf();

#endif
