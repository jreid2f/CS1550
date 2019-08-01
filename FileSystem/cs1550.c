/*
	FUSE: Filesystem in Userspace
	Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

	This program can be distributed under the terms of the GNU GPL.
	See the file COPYING.
*/

#define	FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

//size of a disk block
#define	BLOCK_SIZE 512

//we'll use 8.3 filenames
#define	MAX_FILENAME 8
#define	MAX_EXTENSION 3

//How many files can there be in one directory?
#define MAX_FILES_IN_DIR (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + (MAX_EXTENSION + 1) + sizeof(size_t) + sizeof(long))

//The attribute packed means to not align these things
struct cs1550_directory_entry
{
	char dname[MAX_FILENAME + 1];
	int nFiles;	//How many files are in this directory.
				//Needs to be less than MAX_FILES_IN_DIR

	struct cs1550_file_directory
	{
		char fname[MAX_FILENAME + 1];	//filename (plus space for nul)
		char fext[MAX_EXTENSION + 1];	//extension (plus space for nul)
		size_t fsize;					//file size
		long nStartBlock;				//where the first block is on disk
	} __attribute__((packed)) files[MAX_FILES_IN_DIR];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_FILES_IN_DIR * sizeof(struct cs1550_file_directory) - sizeof(int)];
} ;

typedef struct cs1550_root_directory cs1550_root_directory;

#define MAX_DIRS_IN_ROOT (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + sizeof(long))

struct cs1550_root_directory
{
	int nDirectories;	//How many subdirectories are in the root
						//Needs to be less than MAX_DIRS_IN_ROOT
	struct cs1550_directory
	{
		char dname[MAX_FILENAME + 1];	//directory name (plus space for nul)
		long nStartBlock;				//where the directory block is on disk
	} __attribute__((packed)) directories[MAX_DIRS_IN_ROOT];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_DIRS_IN_ROOT * sizeof(struct cs1550_directory) - sizeof(int)];
} ;


typedef struct cs1550_directory_entry cs1550_directory_entry;
typedef struct cs1550_file_directory cs1550_file_directory;

//How much data can one block hold?
#define	MAX_DATA_IN_BLOCK (BLOCK_SIZE)

struct cs1550_disk_block
{
	//All of the space in the block can be used for actual data
	//storage.
	char data[MAX_DATA_IN_BLOCK];
};

typedef struct cs1550_disk_block cs1550_disk_block;

static int getRoot(cs1550_root_directory *direct){
	FILE *file = fopen(".disk", "rb");
	int root;

	root = fread(direct, sizeof(cs1550_root_directory), 1 ,file);
	fclose(file);
	return root;
}

static int findDirectory(char *directory){
	cs1550_root_directory root;
	int value;
	int index = -1;
	int direct = 0;
	int i;

	value = getRoot(&root);

	if(value == -1){
		return value;
	}

	for(i = 0; i < root.nDirectories && !direct; i++){
		if(strcmp(root.directories[i].dname, directory) == 0){
			index = root.directories[i].nStartBlock;
			direct = 1;
		}
	}
	
	return (int)index;
}

static int getDirectory(cs1550_directory_entry *direct, int index){
	FILE *file = fopen(".disk", "rb");
	int value = -1;
	int block;

	if(file == NULL){
	   return value;
	}

	block = fseek(file, index, SEEK_SET);
	
	if(block == -1){
	   return value;
	}

	value = fread(direct, sizeof(cs1550_directory_entry), 1, file);
	fclose(file);
	return value;
}

static int check_file(cs1550_directory_entry* direct, char* attrib, char* ext){
	int i;
	for(i = 0; i < direct->nFiles; i++){
	   cs1550_file_directory current = direct->files[i];
	
	   if(strcmp(attrib, current.fname) == 0 && strcmp(ext, current.fext) == 0){
		return i;
	   }
	}

	return -1;
}

/*
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not. 
 *
 * man -s 2 stat will show the fields of a stat structure
 */
static int cs1550_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	char directory[MAX_FILENAME+1];
	char attributes[MAX_FILENAME+1];
	char extend[MAX_EXTENSION+1];
	int readFile = 0;
	int no_direct;
	int found_direct;

	//directory[0] = 0;
	//attributes[0] = 0;
	//extend[0] = 0;

	memset(stbuf, 0, sizeof(struct stat));
	struct cs1550_directory_entry current;

	//is path the root dir?
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		directory[0] = '\0';
		attributes[0] = '\0';
		extend[0] = '\0';

		sscanf(path, "/%[^/]/%[^.].%s", directory, attributes, extend);

		// Check if name is subdirectory
		if(strcmp(directory, "\0") != 0 && strcmp(attributes, "\0") == 0){
			no_direct = findDirectory(directory);

			if(no_direct != -1){
				found_direct = getDirectory(&current, no_direct);

				if(found_direct == -1){
					return -ENOENT;
				}
				else{
					stbuf->st_mode = S_IFDIR | 0755;
					stbuf->st_nlink = 2;
					res = 0;
				}
			}

		        else{
			      // Else return that path doesn't exist
			      return -ENOENT;	
			}
		}
		
		// Check if name is a regular file
		else{
			no_direct = findDirectory(directory);
			found_direct = getDirectory(&current, no_direct);
			res = -ENOENT;
			int i;

			for(i = 0; i < current.nFiles && !readFile; i++){
				if(strcmp(current.files[i].fname, attributes) == 0){
					if(strcmp(current.files[i].fext, extend) == 0){
						stbuf->st_mode = S_IFREG | 0666;
						stbuf->st_nlink = 1;
						stbuf->st_size = current.files[i].fsize;
						res = 0;
						readFile = 1;
					}
				}
			}			
		}
	}
	return res; //returns 0 on success
}

/* 
 * Called whenever the contents of a directory are desired. Could be from an 'ls'
 * or could even be when a user hits TAB to do autocompletion
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)

{
	//Since we're building with -Wall (all warnings reported) we need
	//to "use" every parameter, so let's just cast them to void to
	//satisfy the compiler
	(void) offset;
	(void) fi;
	
	FILE* direct = fopen(".directories", "a");
	fclose(direct);

	FILE* file = fopen(".directories", "r");
	struct cs1550_directory_entry current;
	int test = 0;

	//This line assumes we have no subdirectories, need to change
	if (strcmp(path, "/") == 0){
	    filler(buf, ".", NULL, 0);
	    filler(buf, "..", NULL, 0);

	    while(fread(&current, sizeof(current), 1, file) > 0 && !feof(file)){
		filler(buf, current.dname, NULL, 0);
	    }
	}
	
	/*
 	* 
 	*/
	else{
	    while(fread(&current, sizeof(current), 1, file) > 0 && !feof(file)){
		if(strcmp(current.dname, path + 1) == 0){
		    int i;
		    for(i = 0; i < current.nFiles; i++){
			char fileName[20];
			fileName[0] = 0;

			strcat(fileName, current.files[i].fname);
			
			if(strlen(current.files[i].fext) > 0){
			    strcat(fileName, ".");
			}

			strcat(fileName, current.files[i].fext);
			filler(buf, fileName, NULL, 0);
		    }
		    test = 1;
		}
	    }

	    if(!test){
		fclose(file);
		return -ENOENT;
	    }
	}
	return 0;
}

/* 
 * Creates a directory. We can ignore mode since we're not dealing with
 * permissions, as long as getattr returns appropriate ones for us.
 */
static int cs1550_mkdir(const char *path, mode_t mode)
{
	(void) path;
	(void) mode;
	char directory[MAX_FILENAME + 1];
	char attributes[MAX_FILENAME + 1];
	char extend[MAX_EXTENSION + 1];

	directory[0] = '\0';
	attributes[0] = '\0';
	extend[0] = '\0';	

	sscanf(path, "/%[^/]/%[^.].%s", directory, attributes, extend);

	//checks if the name is beyond 8 chars
	if(strlen(directory) > MAX_FILENAME){
		return -ENAMETOOLONG;
	}
	
	//checks if the directory is not under the root dir only
	if(strlen(attributes) > 0){
		return -EPERM;
	}

	struct cs1550_directory_entry current;

	FILE *f = fopen(".directories", "a");
	fclose(f);

	FILE *d = fopen(".directories", "r");

	fread(&current, sizeof(current), 1, d);

	while(strcmp(current.dname, path + 1) != 0 && fread(&current, sizeof(current), 1, d) > 0);

	//checks if the directory already exist
	if(strcmp(current.dname, path + 1) == 0){
		return -EEXIST;
	}
	fclose(d);

	struct cs1550_directory_entry nxt;
	FILE *m = fopen(".directories", "a");

	strcpy(nxt.dname, path + 1);
	nxt.nFiles = 0;

	fwrite(&nxt, sizeof(nxt), 1, m);
	fclose(m);

	return 0;
}

/* 
 * Removes a directory.
 * DOES NOT NEED MODIFIED
 */
static int cs1550_rmdir(const char *path)
{
	(void) path;
    return 0;
}

/* 
 * Does the actual creation of a file. Mode and dev can be ignored.
 *
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
	(void) mode;
	(void) dev;
	char directory[MAX_FILENAME + 1];
	char attributes[MAX_FILENAME + 1];
	char extend[MAX_EXTENSION + 1];

	directory[0] = '\0';
	attributes[0] = '\0';
	extend[0] = '\0';
	sscanf(path, "/%[^/]/%[^.].%s", directory, attributes, extend);
	
	//checks if the name is beyond 8.3 chars
	if(strlen(attributes) > MAX_FILENAME || strlen(extend) > 3){
	    return -ENAMETOOLONG;
	}

	//checks if the file is trying to be created in root directory
	if(strlen(attributes) == 0){
	    return -EPERM;
	}

	cs1550_directory_entry current;
	int indexD = findDirectory(directory);
	if(indexD == -1){
	   return -ENOENT;
	}

	int indexF = check_file(&current, attributes, extend);
	//checks if the file already exists
	if(indexF == -1){
	   return -EEXIST;
	}

	int createFile = current.nFiles;
	
	if(createFile > MAX_FILES_IN_DIR){
	   return -EPERM;
	}

	strcpy(current.files[createFile].fname, attributes);
	strcpy(current.files[createFile].fext, extend);

	current.files[createFile].fsize = 0;

	int space = -1;
	current.files[createFile].nStartBlock = space;

	current.nFiles++;

	FILE* file = fopen(".directories", "r+b");
	fseek(file, indexD, SEEK_SET);
	fwrite(&current, sizeof(current), 1, file);
	fclose(file);

	return 0;
}

/*
 * Deletes a file
 * DOES NOT NEED MODIFIED
 */
static int cs1550_unlink(const char *path)
{
    (void) path;

    return 0;
}

/* 
 * Read size bytes from file into buf starting from offset
 *
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi)
{
	(void) buf;
	(void) offset;
	(void) fi;
	(void) path;
	int file_size;
	char directory[MAX_FILENAME + 1];
	char attributes[MAX_FILENAME + 1];
	char extend[MAX_EXTENSION + 1];

	directory[0] = '\0';
	attributes[0] = '\0';
	extend[0] = '\0';

	FILE *f = fopen(".disk", "a");
	fclose(f);

	sscanf(path, "/%[^/]/%[^.].%s", directory, attributes, extend);
	
	cs1550_directory_entry current;
	int direct = findDirectory(directory);
	getDirectory(&current, direct);
	int fileIndex = check_file(&current, attributes, extend);
	cs1550_file_directory currentF = current.files[fileIndex];
	file_size = current.files[fileIndex].fsize;

	//check to make sure the path exists
	if(strcmp(attributes, "\0") == 0){
	   return -EISDIR;
	}

	//check that size is > 0
        if(file_size < 0){
           return -ENOENT;
        }

	//check that offset is <= to the file size
	if(offset > file_size){
	   return -EFBIG;
	}

	//read in data
	int index = currentF.nStartBlock * 512 + offset;
	int read = currentF.fsize - offset;

	if(read < size){
	   size = read;
	}
	FILE *file = fopen(".disk", "rb");
	fseek(file, index, SEEK_SET);
	int bytes = fread(buf, 1, size, file);
	fclose(f);

	return bytes;
}

/* 
 * Write size bytes from buf into file starting from offset
 *
 */
static int cs1550_write(const char *path, const char *buf, size_t size, 
			  off_t offset, struct fuse_file_info *fi)
{
	(void) buf;
	(void) offset;
	(void) fi;
	(void) path;

	char directory[MAX_FILENAME + 1];
	char attributes[MAX_FILENAME + 1];
	char extend[MAX_EXTENSION + 1];

	directory[0] = '\0';
	attributes[0] = '\0';
	extend[0] = '\0';

	FILE *file = fopen(".disk", "a");
	fclose(file);

	sscanf(path, "/%[^/]/%[^.].%s", directory, attributes, extend);
	//check to make sure path exists
	cs1550_directory_entry currentD;
	int index = findDirectory(directory);
	
	if(index == -1){
	    return -ENOENT;
	}

	int cFile = check_file(&currentD, attributes, extend);
	
	if(cFile == -1){
	    return -ENOENT;
	}
	
	cs1550_file_directory currentF = currentD.files[cFile];
	//check that size is > 0
	if(size <= 0){
	   return 0;
	}
	//check that offset is <= to the file size
	if(offset > currentF.fsize){
	   return -EFBIG;
	}
	//write data
	int file_bytes = (offset + size) - currentF.fsize;
	int used = 0;
	int file_blocks = 0;

	while(used * BLOCK_SIZE < currentF.fsize){
		used++;
	}

	int any_bytes = (BLOCK_SIZE * used) - currentF.fsize;

	int i = file_bytes;
	while(i > any_bytes){
	   file_blocks++;
	   i -= BLOCK_SIZE;
	}
	//set size (should be same as input) and return, or error
	if(file_blocks > 0){
	   int cur_blocks = currentF.fsize / BLOCK_SIZE;
	   if(currentF.fsize % BLOCK_SIZE != 0){
		cur_blocks++;
	   }

	   /*if(currentF.nStartBlock != -1){
		return;
	   }*/
	}

	int writ = currentD.files[cFile].nStartBlock * BLOCK_SIZE + offset;
	FILE* f = fopen(".disk", "r+b");
	fseek(f, writ, SEEK_SET);
	fwrite(buf, size, 1, f);
	fclose(f);

	if(file_blocks > 0){
	   currentD.files[cFile].fsize += file_blocks;
	}

	FILE* m = fopen(".directories", "r+b");
	fseek(m, index, SEEK_SET);
	fwrite(&currentD, sizeof(currentD), 1, m);
	fclose(m);

	return size;
}

/******************************************************************************
 *
 *  DO NOT MODIFY ANYTHING BELOW THIS LINE
 *
 *****************************************************************************/

/*
 * truncate is called when a new file is created (with a 0 size) or when an
 * existing file is made shorter. We're not handling deleting files or 
 * truncating existing ones, so all we need to do here is to initialize
 * the appropriate directory entry.
 *
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;

    return 0;
}


/* 
 * Called when we open a file
 *
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;
    /*
        //if we can't find the desired file, return an error
        return -ENOENT;
    */

    //It's not really necessary for this project to anything in open

    /* We're not going to worry about permissions for this project, but 
	   if we were and we don't have them to the file we should return an error

        return -EACCES;
    */

    return 0; //success!
}

/*
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file 
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush (const char *path , struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;

	return 0; //success!
}


//register our new functions as the implementations of the syscalls
static struct fuse_operations hello_oper = {
    .getattr	= cs1550_getattr,
    .readdir	= cs1550_readdir,
    .mkdir	= cs1550_mkdir,
	.rmdir = cs1550_rmdir,
    .read	= cs1550_read,
    .write	= cs1550_write,
	.mknod	= cs1550_mknod,
	.unlink = cs1550_unlink,
	.truncate = cs1550_truncate,
	.flush = cs1550_flush,
	.open	= cs1550_open,
};

//Don't change this.
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
