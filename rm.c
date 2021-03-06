
/*
 * rm.c
 * 
 * Jonathan Metzger
 * Spring 2018
 *
 * Project for CS4513 Distributed Computing Systems
 *
 */

#include "common.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>

/* copy to dumpster */
void fromdifferent_partition(char* file, char* dumpster_path, struct stat file_stat);

/* initialize FILE */
FILE* src;
FILE* tar;

/* initialize DIR */
DIR* dir;

/* initialize structs */
struct stat dumpster_stat;
struct stat file_stat;
struct stat source_dir;
struct dirent* d;
struct stat current_stat;

size_t bytes;


/* initialize integers */
int dup_number = 0;
int mkdir_call = 0;
int rename_call = 0;
int unlink_call = 0;
int count_file = 0;
int rmdir_call = 0;
int stat_call = 0;
int utime_call = 0;
int chmod_call = 0;
int remove_call = 0;
int i = 0;
int f_flag = 0;
int r_flag = 0;
int tot_flag = 0; 
int duplicate = 0; 

/* initialize characters */
char* dumpster_name = DUMPSTER;
char* ext = EXTENSION; /* extension char */ 
char* dumpster_path = NULL;
char* new_path = NULL;
char* new_dumpster_path = NULL;
char* file = NULL;
char* temp_path  = NULL;

/* main */
int main(int argc, char** argv)
{   

    printf("\n");

    // end_line();

    // program_title();

    /* parse command line arguments */
    flag_check(argc, argv);

    ERROR_no_file(argc);

    /* check env for setup dumpster */
    set_dumpster();

    count_file = argc - optind;

    char* files[count_file];

    /* create for multiple files */
    for(i = 0; i < count_file; i++)
    {
        files[i] = argv[i + optind];
    }

    /* move file(s) */
    for(i = 0; i < count_file; i ++)
    {
        file = files[i];
        
        /* get file */
        set_file(file);
        if(f_flag)
        {
            /* see if -f flag */
            check_f_flag(file);
        }
        else {
            /* check dumpster */
            check_dumpster(file);
        }
    }

    // end_line();
    return 0;
}

/* force remove */
void remove_force(char* directory)
{
    /* sety directory */
    dir = opendir(directory);
    ERROR_opendir_call();
    
    d = readdir(dir);

    /* look for files to be removed */
    while(d)
    {
        if((strcmp(d->d_name, "..") == 0) || (strcmp(d->d_name, ".") == 0))
        {
            d = readdir(dir);
            continue;
        }

        file = concat(directory, "/");
        file = concat(file, d->d_name);

        stat_call = stat(file, &file_stat);
        ERROR_stat_call();
        
        /* remove file */
        if(S_ISREG(file_stat.st_mode))
        {
            remove_call = remove(file);
            //printf("> [ Deleting { %s } file permanently ... ]\n\n", d->d_name);
            ERROR_remove_call();
        }

        /* recursively remove directories with files inside */
        else if(S_ISDIR(file_stat.st_mode))
        {
            remove_force(file);
            rmdir_call = rmdir(file);
            char* dir_name = basename(file);
            //printf("> [ Deleting { %s } directory permanently ... ]\n\n", dir_name);
            ERROR_rmdir_call();
        }
        /* free up the file */
        free(file);
        d = readdir(dir);
    }
    closedir(dir);
}

/* remove directory */
void move_directory(char* current_path, char* current_dumpster_path, int same)
{

    /* get dumpster path */
    get_dumpsterPath(current_path, current_dumpster_path, &new_dumpster_path);

    stat_call = stat(current_path, &source_dir);
    ERROR_stat_call();

    mkdir_call = mkdir(new_dumpster_path, source_dir.st_mode);
    ERROR_duplicate_dir();
    ERROR_mkdir_call();

    /* get directory */
    dir = opendir(current_path);
    ERROR_opendir_call();

    d = readdir(dir);
    while(d)
    {
        if((strcmp(d->d_name, "..") == 0) || (strcmp(d->d_name, ".") == 0))
        {
            d = readdir(dir);
            continue;
        }
        /* get file path */
        char* file = concat(current_path, "/");
        file = concat(file, d->d_name);

        stat_call = stat(file, &current_stat);
        ERROR_stat_call();

        /* check for files */
        if(S_ISREG(current_stat.st_mode))
        {
            if(same)
            {
                
                get_dumpsterPath(file, new_dumpster_path, &new_path);

                rename_call = rename(file, new_path);
                ERROR_rename_call();

                chmod_call = chmod(new_path, current_stat.st_mode);
                ERROR_chmod_call();
            }
            /* different partition */
            else
            {
                fromdifferent_partition(file, new_dumpster_path, current_stat);

                unlink_call = unlink(file);
                ERROR_unlink_call();
            }
        } 

        /* check for directories */
        else if(S_ISDIR(current_stat.st_mode))
        {
            move_directory(file, new_dumpster_path, same);

            rmdir_call = rmdir(file);
            ERROR_rmdir_call();
        }
        free(file);
        d = readdir(dir); 
    }
    closedir(dir);

    chmod_call = chmod(new_dumpster_path, source_dir.st_mode);
    ERROR_chmod_call();

    /* get time */
    const struct utimbuf source_time = {source_dir.st_atime, source_dir.st_mtime};
    
    utime_call = utime(new_dumpster_path, &source_time);
    ERROR_utime_call();
    
    free(new_dumpster_path);
}

/* copy to dumpster */
void fromdifferent_partition(char* file, char* dumpster_path, struct stat file_stat)
{
    /* get correct path to dumpster */
    get_dumpsterPath(file, dumpster_path, &new_path);
    ERROR_duplicate_dir();
    char buf[1024];
    ERROR_open_call(file, &new_path);
    /* checks if can read or write */
    while(bytes = fread(buf, 1, 1024, src))
    {
        fwrite(buf, 1, bytes, tar);
    }
    if(ferror(src) || ferror(tar))
    {
        printf("Error reading and writing file");
        exit(-1);
    }
    fclose(src);
    fclose(tar);
    chmod_call = chmod(new_path, file_stat.st_mode);
    ERROR_chmod_call();

    /* gets time */
    const struct utimbuf source_time = {file_stat.st_atime, file_stat.st_mtime};
    utime_call = utime(new_path, &source_time);
    ERROR_utime_call();

    free(new_path);
    return;
}

/* get dumpster path */
void get_dumpsterPath(char* file, char* dumpster_path, char** new_path)
{
    *new_path = concat(dumpster_path, "/");
    *new_path = concat(*new_path, basename(strdup(file)));
    ext = get_extension(*new_path);
    /* ignores non-duplicates */
    if(strcmp(ext, ".0")) 
    { 
        *new_path = concat(*new_path, ext); 
    }
}

/* get extension */
char* get_extension(char* path)
{
    temp_path = path;
    duplicate = 0;
    dup_number = 1;
    char array[2];
    array[1] = '\0';
    
    /* when file exists */
    while(access(temp_path, F_OK) != -1)
    {
        duplicate = 1;
        array[0] = dup_number + 48;
        temp_path = concat(path, concat(".", array));
        ERROR_limit_dumpster();
    }

    /* ignores if not duplicate */
    if(!duplicate)
    {
        array[0] = 48;
        return concat(".", array);
    }
    array[0] = dup_number - 1 + 48;
    free(temp_path);
    return concat(".", array);
}

/* (OPTIONAL) Title of the Program */
void program_title()
{
    printf("\tWelcome to Dumpster Diving\n");
    printf("\tCreated by Jonathan Metzger\n");
    printf("\t{ rm } tool Utility\n\n");
    end_line();
}

/* Prints help message and quit */
void usage()
{
    end_line();
    fprintf(stderr, "rm - Moves files to the dumpster\n\n");
    fprintf(stderr, "usage: ./rm -f -h -r < file(s) ... > \n");
    fprintf(stderr, "  -f:\tforce a complete remove\n");
    fprintf(stderr, "  -h:\tusage message\n");
    fprintf(stderr, "  -r:\tremove any subdirectories\n\n");
    end_line();
    exit(1);
}

/* checks flag of input */
void flag_check(int argc, char **argv)
{
    int option;
    while ((option = getopt (argc, argv, "fhr")) != -1)
    {
        switch (option)
        {

        /* Set to force remove */
            case 'f':
            f_flag++; 
            tot_flag++;
            break;

        /* -h show help message */
            case 'h':
            tot_flag++;
            usage();
            exit(-1);
            break;

        /* Set to copy directories recursively */
            case 'r':
            r_flag++; 
            tot_flag++;
            break;

            default:
            exit(-1);

        }
    }
}

/* no file present */
void ERROR_no_file(int argc)
{
    if (optind == argc)
    {
        fprintf(stderr, "** ERROR: no file present ... **\n\n");
        ERROR_call();
    }
}

/* sets dumpster path and stat */
void set_dumpster()
{
    /* finds dumpster path */
    dumpster_path = strcat(getenv("PWD"), "/");
    strcat(dumpster_path, dumpster_name);
    dir = opendir(dumpster_path);
    /* if none, sets dumpster */
    if(dir == NULL)
    {
    	fprintf(stderr, "** ERROR: dumpster does not exist ... **\n");
        printf("> [ Creating new dumpster directory ... ]\n\n");
        mkdir(dumpster_path, 0700);
    }
    if(!dumpster_path)
    {
        fprintf(stderr, "** ERROR: dumpster does not exist ... **\n\n");
		ERROR_call();
    }
    stat_call = stat(dumpster_path, &dumpster_stat);
    if(stat_call)
    {
        fprintf(stderr, "** ERROR: stat() call failed. ** \n");
		ERROR_call();
    }
}

/* sets file path and stat */
void set_file(char* file)
{
    if(access(file, F_OK) == -1)
    {
        fprintf(stderr, "** ERROR: file(s) do not exist ... **\n\n");
        end_line();
        usage();
    }

    stat_call = stat(file, &file_stat);
    if(stat_call)
    {
        fprintf(stderr, "** ERROR: stat() call failed. ** \n");
        ERROR_call();
    }
}

/* checks if -r flag is present */
void check_r_flag()
{
    if(!r_flag)
    {
        printf("** ERROR: -r option missing for directory. **\n\n");
        ERROR_call();
    }
}

/* checks if -f flag is present */
void check_f_flag(char* file)
{
    /* with f flag */
    if(f_flag)
    {
        if(S_ISREG(file_stat.st_mode))
        {
            /* remove file */
            rmdir_call = remove(file);
            //printf("> [ Deleting { %s } file permanently ... ]\n\n", file);
            ERROR_remove_call();
        }

        else if(S_ISDIR(file_stat.st_mode))
        {   
            /* checks if -r is present */
            check_r_flag();
            /* remove directory */
            remove_force(file);
            rmdir_call = remove(file);
            //printf("> [ Deleting { %s } directory permanently ... ]\n\n", file);
            ERROR_rmdir_call();
        }
    }
}

/* checks if dumpster in directory */
void check_dumpster(char* file)
{
    /* same partition */
    if(file_stat.st_dev == dumpster_stat.st_dev)
    {
        if(S_ISREG(file_stat.st_mode))
        {
            char* new_path;
            get_dumpsterPath(file, dumpster_path, &new_path);
            rename(file, new_path);
            //printf("> [ Moving { %s } file to dumpster ... ]\n\n", file);
        }
        else if(S_ISDIR(file_stat.st_mode))
        {
            check_r_flag();
            move_directory(file, dumpster_path, 1);
            rmdir_call = rmdir(file);
            ERROR_rmdir_call();
            //printf("> [ Moving { %s } directory to dumpster ... ]\n\n", file);
        }
    }
    /* different partition */
    else
    {
        /* Check for file or directory. */
        if(S_ISREG(file_stat.st_mode))
        {
            fromdifferent_partition(file, dumpster_path, file_stat);
            unlink_call = unlink(file);
            ERROR_unlink_call();
        }
        else if(S_ISDIR(file_stat.st_mode))
        {
            check_r_flag();
            move_directory(file, dumpster_path, 0);
            rmdir_call = rmdir(file);
            ERROR_rmdir_call();
        }
    }
}

/* ##### ERROR CALLS ##### */

/* ERROR call */
void ERROR_call()
{    
    usage();
    exit(-1);
}

/* ERROR call for remove() */
void ERROR_remove_call()
{
    if(rmdir_call)
    {
        fprintf(stderr, "** ERROR: remove() call failed. ** \n");
        ERROR_call();
    }
}

/* ERROR call for rename() */
void ERROR_rename_call()
{
    if(rename_call)
    {
        fprintf(stderr, "** ERROR: rename() call failed ... **\n");
        ERROR_call();
    }
}

/* ERROR call for rmdir() */
void ERROR_rmdir_call()
{

    if(rmdir_call)
    {
        fprintf(stderr, "** ERROR: rmdir() call failed. ** \n");
        ERROR_call();
    }
}

/* ERROR call for unlink() */
void ERROR_unlink_call()
{
    if(unlink_call)
    {
        fprintf(stderr, "** ERROR: unlink() call failed. ** \n");
        ERROR_call();
    }
}

/* ERROR call for stat() */
void ERROR_stat_call()
{
    if(stat_call)
    {
        fprintf(stderr, "** ERROR: stat() call failed. ** \n");
        ERROR_call();
    }
}

/* ERROR call for utime() */
void ERROR_utime_call()
{
    if(utime_call)
    {
        fprintf(stderr, "** ERROR: utime() call failed. ** \n");
        ERROR_call();
    }
}
/* ERROR call for open() */
void ERROR_open_call(char* file, char* new_path)
{


    src = fopen(file, "r");
    if(src == NULL)
    {
        fprintf(stderr, "** ERROR: cannot open { %s } file. ** \n", file);
        ERROR_call();
    }

    tar = fopen(new_path, "w");
    if(tar == NULL)
    {
        fprintf(stderr, "** ERROR: cannot open { %s } file. ** \n", new_path);
        ERROR_call();
    }
}

/* ERROR call for chmod() */
void ERROR_chmod_call()
{
	if(chmod_call)
    {
        fprintf(stderr, "** ERROR: chmod() call failed. ** \n");
        ERROR_call();
    }
}

/* ERROR call for opendir() */
void ERROR_opendir_call()
{
    if(dir == NULL)
    {
        fprintf(stderr, "** ERROR: opendir() call failed. ** \n\n");
        ERROR_call();
    }
}

/* ERROR call for mkdir() */
void ERROR_mkdir_call()
{
    if(mkdir_call)
    {   
        fprintf(stderr, "** ERROR: mkdir() call failed. ** \n\n");
        ERROR_call();
    }
}

/* limits the dumpster size */
void ERROR_limit_dumpster()
{
	if(dup_number == 10)
    {
        fprintf(stderr, "** ERROR: dumpster is full. ** \n\n");
        ERROR_call();
    }
    dup_number ++;
}

/* sets directory to extension */
void ERROR_duplicate_dir()
{
    if(mkdir_call)
    {
        new_dumpster_path = strcat(new_dumpster_path, ext);
        mkdir_call = mkdir(new_dumpster_path, source_dir.st_mode);
    }
}




