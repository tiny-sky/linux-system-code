/*this library contains many functions of the ls command */
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

/*Defines the data associated with each file*/
#define number 1024   /*the maximum number of files that can be stored*/
#define length 1024   /*Maximum length of the file name and the path of that*/

/*the macro function defines the value of the if variable*/
#define SORT_S(m,n) (ls[m].size-ls[n].size)
#define SORT_T(m,n) (ls[m].time-ls[n],time)

/*Records of related ls data*/
#ifndef ls_define
struct ls_file{
    char filename[length];   /*filename*/
    char pathname[length];   /*the actual pathname*/
    __off_t size;            /*the size of the file*/
    struct timespec time;    /*the mtime of the file*/
}ls[number];
#define ls_define
#endif

/*Keeps track of the current file ordinals*/
#ifdef ls_define
int count;
#endif

/*Record command parameter*/
char cnd [8];

void do_ls(char *);                        /*This function achieve access to the directory under the file*/
void ls_R(char *,char *);                  /*Determines whether to scan directories */
void sort(void);                           /*Sort by parameter*/
void dostat(struct ls_file *);             /*Obtain detailed informatin*/
void show_file_info(char *,struct stat *); /*show the information*/
void mode_to_letters(int,char[]);          /*Convert the mode to string */
char *uid_to_name(uid_t);                  /*Convert the uid to string*/
char *gid_to_name(gid_t);                  /*Convert the gid to string*/


