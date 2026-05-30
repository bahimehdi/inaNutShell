#include "custom.h"

#include "utils.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ROOT_SUBDIR "/inaNutShell"

int is_custom(const char *cmd)
{
    if (cmd == NULL)
        return 0;
    return strcmp(cmd, "bahi") == 0 || strcmp(cmd, "mellaki") == 0;
}

/* Build $HOME/inaNutShell/<author>, creating both folders on first use.
   Returns a newly malloc'd path the caller must free, or NULL on error. */
static char *author_dir(const char *author)
{
    const char *home;
    char *root;
    char *path;
    size_t home_len;
    size_t root_len;

    home = getenv("HOME");
    if (home == NULL || *home == '\0')
        home = "/tmp";
    home_len = strlen(home);
    root_len = home_len + strlen(ROOT_SUBDIR) + 1;
    root = malloc(root_len);
    if (root == NULL) {
        perror("malloc");
        return NULL;
    }
    snprintf(root, root_len, "%s%s", home, ROOT_SUBDIR);
    if (mkdir(root, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
        free(root);
        return NULL;
    }
    path = malloc(strlen(root) + 1 + strlen(author) + 1);
    if (path == NULL) {
        perror("malloc");
        free(root);
        return NULL;
    }
    sprintf(path, "%s/%s", root, author);
    free(root);
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
        free(path);
        return NULL;
    }
    return path;
}

static char *join_path(const char *base, const char *leaf)
{
    size_t len;
    char *out;

    len = strlen(base) + 1 + strlen(leaf) + 1;
    out = malloc(len);
    if (out == NULL) {
        perror("malloc");
        return NULL;
    }
    snprintf(out, len, "%s/%s", base, leaf);
    return out;
}

static int custom_cd(const char *author)
{
    char *dir;
    int rc;

    dir = author_dir(author);
    if (dir == NULL)
        return 1;
    rc = 0;
    if (chdir(dir) != 0) {
        fprintf(stderr, "%s cd: %s: %s\n", author, dir, strerror(errno));
        rc = 1;
    }
    free(dir);
    return rc;
}

static int custom_pwd(const char *author)
{
    char *dir;

    dir = author_dir(author);
    if (dir == NULL)
        return 1;
    printf("%s\n", dir);
    free(dir);
    return 0;
}

static int custom_ls(const char *author, Command *cmd)
{
    char *dir;
    char *target;
    DIR *dp;
    struct dirent *entry;

    dir = author_dir(author);
    if (dir == NULL)
        return 1;
    if (cmd->argc >= 3) {
        target = join_path(dir, cmd->argv[2]);
        free(dir);
        if (target == NULL)
            return 1;
    } else {
        target = dir;
    }
    dp = opendir(target);
    if (dp == NULL) {
        fprintf(stderr, "%s ls: %s: %s\n", author, target, strerror(errno));
        free(target);
        return 1;
    }
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        printf("%s\n", entry->d_name);
    }
    closedir(dp);
    free(target);
    return 0;
}

static int custom_mkdir(const char *author, Command *cmd)
{
    char *dir;
    char *target;

    if (cmd->argc < 3) {
        fprintf(stderr, "%s mkdir: missing folder name\n", author);
        return 1;
    }
    dir = author_dir(author);
    if (dir == NULL)
        return 1;
    target = join_path(dir, cmd->argv[2]);
    free(dir);
    if (target == NULL)
        return 1;
    if (mkdir(target, 0755) != 0) {
        fprintf(stderr, "%s mkdir: %s: %s\n", author, target, strerror(errno));
        free(target);
        return 1;
    }
    printf("created: %s\n", target);
    free(target);
    return 0;
}

static int custom_whoami(const char *author)
{
    if (strcmp(author, "bahi") == 0)
        printf("Mehdi Bahi\n");
    else
        printf("Mustapha Mellaki\n");
    printf("GI engineering student, ENSA Kenitra (S8)\n");
    printf("Co-author of inaNutShell\n");
    return 0;
}

static int custom_help(const char *author)
{
    printf("%s <sub-command> [args]\n", author);
    printf("  cd              go to %s's folder\n", author);
    printf("  pwd             print the absolute path of %s's folder\n", author);
    printf("  ls [subdir]     list files in %s's folder\n", author);
    printf("  mkdir <name>    create a sub-folder in %s's folder\n", author);
    printf("  whoami          print info about %s\n", author);
    printf("  help            show this message\n");
    return 0;
}

int exec_custom(Command *cmd)
{
    const char *author;
    const char *sub;

    if (cmd == NULL || cmd->argc == 0)
        return 0;
    author = cmd->argv[0];
    if (cmd->argc < 2)
        return custom_help(author);
    sub = cmd->argv[1];
    if (strcmp(sub, "cd") == 0)
        return custom_cd(author);
    if (strcmp(sub, "pwd") == 0)
        return custom_pwd(author);
    if (strcmp(sub, "ls") == 0)
        return custom_ls(author, cmd);
    if (strcmp(sub, "mkdir") == 0)
        return custom_mkdir(author, cmd);
    if (strcmp(sub, "whoami") == 0)
        return custom_whoami(author);
    if (strcmp(sub, "help") == 0)
        return custom_help(author);
    fprintf(stderr, "%s: unknown sub-command '%s' (try '%s help')\n",
        author, sub, author);
    return 1;
}
