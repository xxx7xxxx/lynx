#include <stdio.h>
#include <string.h>

#include "configure.h"

#define CONF_FILE_PATH  "./lynx.conf"


#define LINE_MAX        1024


struct conf_atom conf[] = {
        {"model", MODEL_REACTOR}
        /*could add configures*/
};


void start_conf()
{
        char    line[LINE_MAX];
        FILE    *fconf;
        char    key[LINE_MAX];
        char    value[LINE_MAX];

        fconf = fopen(CONF_FILE_PATH, "r");
        if (fconf == NULL)
                fprintf (stderr, "lynx.conf does not exist!\n");

        while (fgets(line, LINE_MAX, fconf) != NULL) {
                if (sscanf(line, "%s %s", key, value) <= 0)
                        continue;
                printf("%s %s\n", key, value);
                if (strcasecmp(conf[MODEL_SUB].key, "model") == 0) {
                        if (strcasecmp(value, "threads") == 0) {
                                conf[MODEL_SUB].value = MODEL_THREADS;
                        }
                }

                /*could add configures*/

        }

        fclose(fconf);
}
