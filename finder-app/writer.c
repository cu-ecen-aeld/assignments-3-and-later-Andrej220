#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    openlog(NULL,0,LOG_USER);
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid Number of arguments: %d", argc);
        return 1;
    }
 
    char *filename = argv[1];
    char *string = argv[2];
    
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Unable to open file %s", filename);
        return 1;
    }
    
    fprintf(file, "%s\n", string);
    fclose(file);
    syslog(LOG_DEBUG, "Writing \"%s\" to %s", string, filename);
    closelog();
    return 0;
}
