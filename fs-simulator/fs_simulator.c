#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void ls(int curr_dir){
	char str[32] = {0};
	snprintf(str,32,"%d",curr_dir);
	FILE *f = fopen(str,"r");
	char name[32] = {0};
	int inode_num;

	while((fread(&inode_num,sizeof(int),1,f))!=0){
		fread(name,sizeof(char),32,f);
		printf("%d %s\n",inode_num,name);
}	
}

int cd(int curr_dir,char dir_name[], char inode_list[]){
	char str[32] = {0};
	snprintf(str,32,"%d",curr_dir);
	FILE *f = fopen(str,"r");
	char name[32] = {0};
	int inode_num;

	while((fread(&inode_num,sizeof(int),1,f))!=0){
		fread(name,sizeof(name),1,f);
		if(strcmp(name,dir_name) == 0 && inode_list[inode_num]=='d'){
			curr_dir = inode_num;
			return inode_num;
	}		
}

	printf("Directory %s not found\n",dir_name);
	return curr_dir;
}

void mkdir_shrey(int prev_dir,char new_dir_name[],int *max_inode,char inode_list[]){
	char str[32] = {0};
	snprintf(str,32,"%d",prev_dir);
	FILE *f = fopen(str,"r");
	int inode_num;
	char name[32] = {0};
	while((fread(&inode_num,sizeof(int),1,f))!=0){
		fread(name,sizeof(char),32,f);
		if(strcmp(name,new_dir_name)==0){
			printf("Directory %s already present, please choose another name\n",new_dir_name);
			return;
		}

	}
	(*max_inode)++;
	//creating and writing into the new dir_file
	char curr_dir_name_dot[32] =".";
	char prev_dir_name_dot[32] ="..";
	char str_2[32] = {0};
	snprintf(str_2,32,"%d",(*max_inode));
	f = fopen(str_2,"w");
	fwrite(max_inode,sizeof(int),1,f);
	fwrite(curr_dir_name_dot,sizeof(char),32,f);
	fwrite(&prev_dir,sizeof(int),1,f);
	fwrite(prev_dir_name_dot,sizeof(char),32,f);
	fclose(f);

	//making changes into indodes_list file
	f = fopen("inodes_list","a");
	fwrite(max_inode,sizeof(int),1,f);
	char d='d';
	fwrite(&d,sizeof(char),1,f);
	fclose(f);

	//making changes to inode_list
	inode_list[*max_inode] = d;
	
	//making changes to prev_dir
	char str_3[32] = {0};
	snprintf(str_3,32,"%d",prev_dir);
	f = fopen(str_3,"a");
	fwrite(max_inode,sizeof(int),1,f);
	fwrite(new_dir_name,sizeof(char),32,f);
	fclose(f);
	return;	
}


void touch(int prev_dir,char new_file_name[],int *max_inode,char inode_list[]){
	char str[32] = {0};
	snprintf(str,32,"%d",prev_dir);
	FILE *f = fopen(str,"r");
	int inode_num;
	char name[32] = {0};
	
	while((fread(&inode_num,sizeof(int),1,f)) != 0){
		fread(name,sizeof(char),32,f);
		if(strcmp(name,new_file_name) == 0){
			printf("File name '%s' already exists.\n",new_file_name);
			return;
		}
	}
	(*max_inode)++;

	//creating and writing into new file
	char str_2[32] = {0};
	snprintf(str_2,32,"%d",*max_inode);
	f = fopen(str_2,"w");
	fwrite(max_inode,sizeof(int),1,f);
	fwrite(new_file_name,sizeof(char),32,f);
	fclose(f);

	//editing inodes_list file
	f = fopen("inodes_list","a");
	fwrite(max_inode,sizeof(int),1,f);
	char d = 'f';
	fwrite(&d,sizeof(char),1,f);
	fclose(f);

	//making changes to inodes_list
	inode_list[*max_inode] = d;

	//making changes to parent_dir
	char str_3[32] = {0};
	snprintf(str_3,32,"%d",prev_dir);
	f = fopen(str_3,"a");
	fwrite(max_inode,sizeof(int),1,f);
	fwrite(new_file_name,sizeof(char),32,f);
	fclose(f);
		
}

int main(int argc, char *argv[]){
	FILE *file;
	int i;
	int check;
	char inode_list[1024] = {0};
	int max_inode = 0;
	
	if(argc<2){
		printf("Please provide a directory name");
		return 0;

	}

	chdir(argv[1]);
	file = fopen("inodes_list","r");
	check = fread(&i,sizeof(int),1,file);
	while(check!=0)
	{
		fread(&(inode_list[i]),sizeof(char),1,file);
		max_inode=i;
		check = fread(&i,sizeof(int),1,file);
		
	}
	int curr_dir = 0;

	while(1){
		char command_name[32];
		printf("> ");
		fgets(command_name,32,stdin);	
		int count = 1;
		int i,j;
		char first[32]={0};
		char second[32]={0};
		int len = strlen(command_name);

		for(i=0;i<len;i++){
			if(command_name[i]==' '){
				first[i]='\0';
				break;
}
			if(command_name[i] != '\n'){
			first[i] = command_name[i];}

}
		i++;
		for(j=0;i<len;i++,j++){
			if(command_name[i] == ' '){
				count = 2;
}

			else if(command_name[i] == '\0' || command_name[i] == '\n'){
				break;
			
}
	
			second[j] = command_name[i];
}
			second[j] = '\0';


		if(count>2){
			printf("Command invalid\n");
			continue;
		}	


		if(strcmp(first,"ls")==0){
			ls(curr_dir);}

		else if(strcmp(first,"exit")==0){
			break;}

		else if(strcmp(first,"cd")==0){
			curr_dir = cd(curr_dir,second,inode_list);
		}

		else if(strcmp(first,"mkdir")==0){

			if(max_inode>=1023){
				printf("Maximum number of inodes reached!\n");
				continue;
			}
			mkdir_shrey(curr_dir,second,&max_inode,inode_list);
		}

			
		else if(strcmp(first,"touch") == 0){
			if(max_inode>=1023){
				printf("Maximum number of inodes reached!\n");
				continue;
			}
			touch(curr_dir,second,&max_inode,inode_list);
		}		

		else{
			printf("Command does not exit\n");
}

		

}
	return 1;
}
