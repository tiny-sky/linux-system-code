/*
 *对于ls命令的实现
 *本版本实现了有关输出顺序-t -s -r的实现
 *以及输出参数-l -i 的实现
 */

#include "ls.h"

int main(int argc,char *argv[])
{
    int sign=1;
    while(*(++argv)!=NULL)
    {
        if(**argv=='-')
            strcat(cnd,*argv+1);
        else
        {
            do_ls(*argv);
            sign=0;
        }
    }
    if(sign)
    do_ls(".");

    sort();
    dostat(ls);
    return 0;

}

void do_ls(char *dirname)
{
    DIR * dir_prt;
    struct dirent *direntp;
    if((dir_prt=opendir(dirname))==NULL)
    perror("Sorry");
    else
    {
        while((direntp=readdir(dir_prt))!=NULL)
        {
            if((memchr(cnd,'a',7))==NULL&&*direntp->d_name!='.')
            {
                sprintf(ls[count].pathname,"%s/%s",dirname,direntp->d_name);
                sprintf(ls[count++].filename,"%s",direntp->d_name);
                ls_R(direntp->d_name,dirname);
            }
            else
            {
                sprintf(ls[count].pathname,"%s/%s",dirname,direntp->d_name);
                sprintf(ls[count++].filename,"%s",direntp->d_name);
                ls_R(direntp->d_name,dirname);
            }
        }
        closedir(dir_prt);
    } 
}

void ls_R(char *file,char *dir)
{
    struct stat info;
    sprintf(ls[count].pathname,"%s/%s",dir,file);
    if(lstat(ls[count].pathname,&info)==-1)
        perror("filename");
    else
    {
        ls[count].size=info.st_size;
        ls[count].time=info.st_mtime;

        if((memchr(cnd,'R',7)!=NULL)&&S_ISDIR(info.st_mode))
        {
            if(strcmp(ls[count].filename,".")==0||strcmp(ls[count].filename,"..")==0)
                return ;
            else
            do_ls(ls[count].pathname);
        } 
    }  
}

void sort(void)
{
   int i,j;
   if(memchr(cnd,'s',7)!=NULL)
   {
    for(i=0;i<count-1;i++)
    {
        for(j=i+1;j<count;j++)
        {
            if((ls[i].size-ls[j].size)<0)
            {
                struct ls_file temp;
                temp=ls[i];
                ls[i]=ls[j];
                ls[j]=temp;
            }
        }
    }
    return ;
   }
   if(memchr(cnd,'r',7)!=NULL)
   {
    i=0,j=count-1;
    while(i<j)
    {
        struct ls_file temp;
        temp=ls[i];
        ls[i]=ls[j];
        ls[j]=temp;
        i++,j--;
    }
    return ;
   } 
}

void dostat(struct ls_file *file)
{
    int i=0;
    struct stat info;
    if(memchr(cnd,'i',7)!=NULL||memchr(cnd,'l',7)!=NULL)
    {
        while(i<count)
        {
            if(stat(ls[i].pathname,&info)==-1)
            perror("filename");
            else
            show_file_info(ls[i].filename,&info);
            i++;
        }
    }
    else
    {
        while(i<count)
        {
            printf(" %-10s ",ls[i].filename);
            i++;
        }
    }
}

void show_file_info(char *filename,struct stat *info_p)
{
    char mode[11];
    mode_to_letters(info_p->st_mode,mode);

    if(memchr(cnd,'i',7)!=NULL)
    {
        printf("%-10lu",info_p->st_ino);
    }
    if(memchr(cnd,'l',7)!=NULL)
    {
        printf(" %s ",mode);
        printf(" %4d ",(int)info_p->st_nlink);
        printf(" %-8s ",uid_to_name(info_p->st_uid));
        printf(" %-8s ",gid_to_name(info_p->st_gid));
        printf(" %8ld ",(long)ls[count].size);
        printf(" %.12s ",ctime(&info_p->st_mtime));
    }
        printf("%s\n",filename);
}

void mode_to_letters(int mode,char str[])
{
    strcpy(str,"----------");

    if(S_ISDIR(mode)) str[0]='d';
    if(S_ISCHR(mode)) str[0]='c';
    if(S_ISBLK(mode)) str[0]='b';

    if(mode & S_IRUSR) str[1]='r';
    if(mode & S_IWUSR) str[2]='w';
    if(mode & S_IXUSR) str[3]='x';

    if(mode & S_IRGRP) str[4]='r';
    if(mode & S_IWGRP) str[5]='w';
    if(mode & S_IXGRP) str[6]='x';

    if(mode & S_IROTH) str[7]='r';
    if(mode & S_IWOTH) str[8]='w';
    if(mode & S_IXOTH) str[9]='x';
}

char *uid_to_name(uid_t uid)
{
    struct passwd * pw_ptr;
    static char numstr[20];
    if((pw_ptr=getpwuid(uid))==NULL){
        sprintf(numstr,"%d",uid);
        return numstr;
    }
    else
        return pw_ptr->pw_name;
}

char *gid_to_name(gid_t gid)
{
    struct group *grp_ptr;
    static char numstr[20];

    if((grp_ptr=getgrgid(gid))==NULL){
        sprintf(numstr,"%d",gid);
        return numstr;
    }
    else
    return grp_ptr->gr_name;
}