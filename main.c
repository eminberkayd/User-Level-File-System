// github.com/eminberkayd

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* diskimage;
FILE* diskimagPtr;
char buffer[512];

//https://www.geeksforgeeks.org/bit-manipulation-swap-endianness-of-a-number/

int swapEndian(int num)
{
    int leftmost_byte = (num & 0x000000FF);
    int left_middle_byte = (num & 0x0000FF00);
    int right_middle_byte = (num & 0x00FF0000);
    int rightmost_byte = (num & 0xFF000000);

    return ((leftmost_byte<<24)|(left_middle_byte<<8)|(right_middle_byte>>8)|(rightmost_byte>>24));
}

struct FAT{
           int value[4096];
}myFAT;

struct FileList {
             char FileName[128][248];
             int FirstBlock[128];
             int FileSize[128]; // in Bytes
}myFileList;


struct DATA{
             char data[4096][512];
}mydata;


void Format(){
// make the FAT and File List empty.
// Empty FAT, first entry is 0xFFFFFFFF and others are 0x0
myFAT.value[0] = 0xFFFFFFFF;

for(int i=1;i<4096;i++){
    myFAT.value[i] = 0x00000000;
}

for(int j=0;j<128;j++){ // Empty File List
    strcpy(myFileList.FileName[j],"NULL");
    myFileList.FirstBlock[j] = 0;
    myFileList.FileSize[j] = 0;
}
// Write new FAT, FileList and DATA to disk image

    fseek(diskimagPtr, 0, SEEK_SET); // take the cursor to the beginning
    fwrite(&myFAT, sizeof(char), sizeof(myFAT), diskimagPtr);
    fwrite(&myFileList, sizeof(char), sizeof(myFileList), diskimagPtr);
    fwrite(&mydata, sizeof(char), sizeof(mydata), diskimagPtr);
    fclose(diskimagPtr);
}

void Write(char *srcPath, char *destFileName){

FILE* srcPtr = fopen(srcPath,"r+");

int empty_blk_num=0; // empty block number in FAT

for(int i=0;i<128;i++){
    if(!strcmp(destFileName,myFileList.FileName[i])){
       printf("ERROR! The %s already exists in the path",destFileName);
       return;
       }
}
// count the empty blocks
int first_empty_blk;
for(int i=0;i<4096;i++){
    if(myFAT.value[i]==0x0){
        empty_blk_num++;
        first_empty_blk = i;
        break;
    }
}
if(empty_blk_num==0){
    printf("There is no available space in the disk!\n");
    return;
}

int total_size_in_bytes=0;
int prev_blk = first_empty_blk;
for(int i=(prev_blk+1);i<4096;i++){
    if(myFAT.value[i]==0x0){
        int sz = fread(buffer,sizeof(char),sizeof(buffer),srcPtr); // buffer is 512
        total_size_in_bytes+=sz;
        if(sz<512){
            myFAT.value[prev_blk] = 0xFFFFFFFF;
            memcpy(mydata.data[i],buffer,sz); //copy the buffer into data
            break;
        }
        else{

            memcpy(mydata.data[i],buffer,sizeof(buffer));
            myFAT.value[prev_blk] = i; //i th index is the next block of prev_blk'th entry
            prev_blk = i;
        }
    }
}

    for(int i=0;i<128;i++){
    if(!strcmp(myFileList.FileName[i],"NULL")){
        strcpy(myFileList.FileName[i],destFileName);
        myFileList.FirstBlock[i] = first_empty_blk;
        myFileList.FileSize[i] = total_size_in_bytes;
        break;
    }
}

    fseek(diskimagPtr, 0, SEEK_SET); // take the cursor to the beginning
    fwrite(&myFAT, sizeof(char), sizeof(myFAT), diskimagPtr);
    fwrite(&myFileList, sizeof(char), sizeof(myFileList), diskimagPtr);
    fwrite(&mydata, sizeof(char), sizeof(mydata), diskimagPtr);
    fclose(diskimagPtr);

}

void Read(char *srcFileName, char *destPath){

FILE* destPtr = fopen(destPath,"w+");

int first_blk; //First Block of Source File
int file_sz; // File Size of Source File
int blk_num; // Block Number Filled by Source File
int found = 0;

for(int i=0;i<128;i++){
    if(!strcmp(myFileList.FileName[i],srcFileName)){
        first_blk = myFileList.FirstBlock[i];
        file_sz = myFileList.FileSize[i];
        found = 1;
        break; //In order to find first file match
    }
}
if(found==0){
   printf("ERROR! File is not found.\n");
   return;
}
blk_num = ((file_sz/512)+1);
int remaining_size = file_sz;
int cr_blk = first_blk; // current block on which operation applies
for(int j=0;j<blk_num;j++){
    if(j+1==blk_num){ //last block to be printed
        memcpy(buffer,mydata.data[cr_blk],remaining_size);
        fwrite(buffer,sizeof(char),remaining_size,destPtr);
    }
    else {
    memcpy(buffer,mydata.data[cr_blk],sizeof(buffer));
    fwrite(buffer,sizeof(char),sizeof(buffer),destPtr);
    remaining_size-=sizeof(buffer);
    cr_blk = swapEndian(myFAT.value[cr_blk]);
    }
}
fclose(destPtr);
fclose(diskimagPtr);

}
void Delete(char *filename){
int firstBlk; //keep the first block
int fileSZ; //keep the file size
int found = 0;
for(int i=0;i<128;i++){
    if(!strcmp(myFileList.FileName[i],filename)){
        found = 1;
        strcpy(myFileList.FileName[i],"NULL");
        firstBlk = myFileList.FirstBlock[i];
        fileSZ = myFileList.FileSize[i];
        myFileList.FirstBlock[i] = 0;
        myFileList.FileSize[i] = 0;
        break;
    }}
    if(found==0){
        printf("File not found!\n");
        return;
    }

int total_blk = ((fileSZ/512)+1); // total block
int next_blk;
int cr_blk = firstBlk; //current block on which operation applies

for(int j=0;j<total_blk;j++){
    if(myFAT.value[cr_blk]==0xFFFFFFFF){
        myFAT.value[cr_blk]=0x0;//clear FAT
        memset(mydata.data[cr_blk],0,sizeof(mydata.data[cr_blk])); // clear DATA - make 0 all
    }
    else{
    next_blk = swapEndian(myFAT.value[cr_blk]);
    myFAT.value[cr_blk]=0x0; // clear FAT
    memset(mydata.data[cr_blk],0,sizeof(mydata.data[cr_blk])); // clear DATA - make 0 all
    cr_blk = next_blk;}
}
// Write new FAT, FileList and DATA to disk image

    fseek(diskimagPtr, 0, SEEK_SET); // take the cursor to the beginning
    fwrite(&myFAT, sizeof(char), sizeof(myFAT), diskimagPtr);
    fwrite(&myFileList, sizeof(char), sizeof(myFileList), diskimagPtr);
    fwrite(&mydata, sizeof(char), sizeof(mydata), diskimagPtr);
    fclose(diskimagPtr);
}
void List(){
 printf("File Name\tFile Size\n");
for(int i=0; i<128; i++){
    if(strcmp(myFileList.FileName[i],"NULL")){
        printf("%s\t\t%d\n",myFileList.FileName[i],myFileList.FileSize[i]);
    }
}
}

void PrintFileList(){
FILE* fptr = fopen("filelist.txt","w+");

fprintf(fptr,"Item\tFile name\tFirst block\tFile size(Bytes)\n");

for(int i=0;i<128;i++){
        fprintf(fptr,"%d\t%s\t\t%d\t\t%d\n",i,myFileList.FileName[i],myFileList.FirstBlock[i],myFileList.FileSize[i]);
    }

fclose(fptr); // close to file to release its sources
}

void PrintFAT(){
FILE* fptr = fopen("fat.txt","w+");
fprintf(fptr,"Entry\tValue\t\tEntry\tValue\t\tEntry\tValue\t\t\n");
for(int i=0;i<4096;i++){
    if(myFAT.value[i]<0x1000){ // symbols next or empty
    if(i%3==0){
        fprintf(fptr,"%d\t%x\t\t",i,myFAT.value[i]);
    }
    if(i%3==1){
        fprintf(fptr,"%d\t%x\t\t",i,myFAT.value[i]);
    }
    if(i%3==2){
        fprintf(fptr,"%d\t%x\n",i,myFAT.value[i]);
    }

    }
    else{
        if(i%3==0){
        fprintf(fptr,"%d\t%x",i,myFAT.value[i]);
    }
    if(i%3==1){
        fprintf(fptr,"%d\t%x",i,myFAT.value[i]);
    }
    if(i%3==2){
        fprintf(fptr,"%d\t%x\n",i,myFAT.value[i]);
    }

    }
}
fclose(fptr);

}

void Defragment(){

    struct FAT newFAT;
    struct FileList newFileList;
    struct DATA newDATA;

    newFAT.value[0] = 0xFFFFFFFF;
    int found = 0;
    int first_blk; // First block of a file
    int file_sz; // File size of a File in bytes
    int num_blk; // # of blocks the file fills
    int fat_idx=1;
    int file_list_idx=0;
    int next_blk; // next block of a file
    int cur_blk; //current block on which operation applies
    for(int i=0;i<128;i++){
        if(strcmp(myFileList.FileName[i],"NULL")){
            found = 1;
            first_blk = myFileList.FirstBlock[i];
            cur_blk = first_blk;
            file_sz = myFileList.FileSize[i];
            num_blk = ((file_sz/512)+1);
            if(num_blk==1){
                strcpy(newFileList.FileName[file_list_idx],myFileList.FileName[i]);
                newFileList.FirstBlock[file_list_idx] = fat_idx;
                newFAT.value[fat_idx] = 0xFFFFFFFF;
                strcpy(newDATA.data[fat_idx],mydata.data[first_blk]); // change the data also
                fat_idx++;
                continue;
            }
            newFileList.FirstBlock[file_list_idx] = fat_idx;
            newFileList.FileSize[file_list_idx] = file_sz;
            strcpy(newFileList.FileName[file_list_idx],myFileList.FileName[i]);
            for(int k=0;k<num_blk;k++){
                if(myFAT.value[cur_blk]!=0xFFFFFFFF){
                    next_blk = swapEndian(myFAT.value[cur_blk]);
                    newFAT.value[fat_idx] = myFAT.value[cur_blk];
                    strcpy(newDATA.data[fat_idx],mydata.data[cur_blk]);
                    cur_blk = next_blk;
                    fat_idx++;
                }
                else{
                    newFAT.value[fat_idx] = 0xFFFFFFFF;
                    strcpy(newDATA.data[fat_idx],mydata.data[cur_blk]);
                    fat_idx++;
                }

            }
            file_list_idx++;

        }
    }
    if(found==0){
        printf("There is no file in disk, thus no need to Defragment!\n");
        return;
    }

    fseek(diskimagPtr, 0, SEEK_SET); // take the cursor to the beginning
    fwrite(&newFAT, sizeof(char), sizeof(newFAT), diskimagPtr);
    fwrite(&newFileList, sizeof(char), sizeof(newFileList), diskimagPtr);
    fwrite(&newDATA, sizeof(char), sizeof(newDATA), diskimagPtr);
    fclose(diskimagPtr);


}
void Rename(char* newFileName,char* oldFileName){
    int found = 0;
    for(int i=0;i<128;i++){
        if(!strcmp(myFileList.FileName[i],oldFileName)){
            found = 1;
            strcpy(myFileList.FileName[i],newFileName);
            //printf("%s",myFileList.FileName[i]);
            break;

        }
    }
    if(found==0){
        printf("File not found!\n");
    }
    fseek(diskimagPtr,sizeof(myFAT),SEEK_SET);
    fwrite(&myFileList, sizeof(char), sizeof(myFileList), diskimagPtr);
    fclose(diskimagPtr);
}

void Duplicate(char* filename, char* duplicate_name){
    int found = 0;
    for(int i=0;i<128;i++){
        if(!strcmp(filename,myFileList.FileName[i])){
            found = 1;
            //Read(filename,duplicate_name);
            //Write(duplicate_name,duplicate_name);
            //Since read do not work properly, so do Duplicate
        }
    }
    if(found==0){
        printf("File not found!\n");
    }

}

void hide(){
printf("Not implemented\n");
}

void unhide(){
    printf("Not implemented\n");
}




int main(int argc, char *argv[])
{

    diskimage = argv[1];
    diskimagPtr = fopen(diskimage,"r+");

    //READ FAT AND FILELIST
    fread(&myFAT, sizeof(myFAT)/4096, 4096, diskimagPtr);
    fread(&myFileList, sizeof(myFileList)/128, 128, diskimagPtr);
    fread(&mydata, sizeof(mydata)/4096, 4096, diskimagPtr);



    if(!strcmp(argv[2], "-format"))
		Format();

	else if(!strcmp(argv[2], "-write"))
		Write(argv[3], argv[4]);

	else if(!strcmp(argv[2], "-read"))
		Read(argv[3], argv[4]);

    else if(!strcmp(argv[2],"-delete"))
		Delete(argv[3]);

	else if(!strcmp(argv[2], "-list"))
		List();

    else if(!strcmp(argv[2], "-printfilelist"))
		PrintFileList();

	else if(!strcmp(argv[2],"-printfat"))
		PrintFAT(argv[3]);

	else if(!strcmp(argv[2],"-defragment"))
		Defragment();
    else if(!strcmp(argv[2],"-rename"))
		Rename(argv[3],argv[4]);

    else if(!strcmp(argv[2],"-duplicate"))
		Duplicate(argv[3],argv[4]);

    else if(!strcmp(argv[2],"-hide"))
		hide();

    else if(!strcmp(argv[2],"-unhide"))
		unhide();

    else
        printf("command not found.\n");
        exit(-1);

	return 0;
}
