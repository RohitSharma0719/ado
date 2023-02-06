#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dberror.h"
#include "storage_mgr.h"


void initStorageManager ( ){
   //Metadata is loaded dynamically into the memory when opening the file at runtime. No initialization of variables required.
}

// Create a new page file fileName. The initial file size should be one page.
// This method should fill this single page with ’\0’ bytes.
RC createPageFile(char *fileName)
{
	//Open the file in W+ mode. File would be created if it does not exist.
	FILE *fp=fopen(fileName, "w+");
	if(fp==NULL)
	{
		return RC_FILE_NOT_FOUND;
	}

	//Create a array of size equal to PAGE_SIZE
	char block[PAGE_SIZE];
	// Initialize the array contents to '\0'
	
	for(int i=0;i<PAGE_SIZE;i++)
	{
		block[i]='\0';
	}
	
	//Write the variable to the file.
	int j=fwrite(block, 1, PAGE_SIZE, fp);
	int close_status=fclose(fp);        
	//Check the number of bytes written.
	if(j<PAGE_SIZE)
	{
		return RC_WRITE_FAILED;
	}
	//Check if file is closed successfully
	if(close_status!=0)
	{
		return RC_FAILED_CLOSE;
	}
	return RC_OK;
}

// openPageFile
// – Opens an existing page file. Should return RC FILE NOT FOUND if the file does not exist.
// – The second parameter is an existing file handle.
// – If opening the file is successful,
//  then the fields of this file handle should be initialized with the information
// about the opened file. 
// For instance, you would have to read the total number of pages that are stored
// in the file from disk.

RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
	FILE *fp=fopen(fileName, "r+");
	if(fp==NULL)
	{
		return RC_FILE_NOT_FOUND;
	}

	//To calculate the number of pages, seek to the end of the file and calucate the size using ftell and divide by PAGE_SIZE
	fseek(fp, 0, SEEK_END);
	fHandle->totalNumPages = (ftell(fp)+1)/PAGE_SIZE;

	fHandle->curPagePos = 0;
	fHandle->fileName = fileName;
	
	//Reset the file pointed to the begining of the file.
	fseek(fp, 0, SEEK_SET);
	fHandle->mgmtInfo = fp;
	return RC_OK;
}

// closePageFile , destroyPageFile
// Close an open page file or destroy (delete) a page file.
RC closePageFile(SM_FileHandle *fHandle)
{
	//Check if the pointer info to a file is available to close it.
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_FOUND;
	int close_status=fclose(fHandle->mgmtInfo);
	//Check if we are able to close the file successfully.
	if(close_status!=0)
	{
		return RC_FAILED_CLOSE;
	}
	return RC_OK;
}

RC destroyPageFile(char *fileName)
{
	int is_file_deleted=remove(fileName);
	//Check if we are able to delete the file successfully.
	if(is_file_deleted!=0)
		return RC_FAILED_DELETE;
	return RC_OK;
}

//
// • readBlock
// – The method reads the block at position pageNum from a file and stores its content in the memory pointed
// to by the memPage page handle.
// – If the file has less than pageNum pages, the method should return RC READ NON EXISTING PAGE.
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
	{
		//Check if the page number is withing the range.
		if((pageNum>(fHandle->totalNumPages))||(pageNum<0))
		{
			return RC_READ_NON_EXISTING_PAGE;
		}
		//Seek to the PAGE from where we want to read.
		fseek(fHandle->mgmtInfo, PAGE_SIZE*pageNum, SEEK_SET);
		
		//Read 1 Page of data and check the number of bytes read and compare it to the PAGE_SIZE
		if(fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo)<PAGE_SIZE)
		{
			return RC_FAILED_READ;
		}
		//Update the PageNum                
		fHandle->curPagePos=pageNum;
		return RC_OK;
	}

// • getBlockPos
// – Return the current page position in a file
RC getBlockPos(SM_FileHandle *fHandle)
{
	//Check if there exisits any PAGE to be read
	if(fHandle->curPagePos <0)
		return RC_READ_NON_EXISTING_PAGE;
	return(fHandle->curPagePos);
}

// • readFirstBlock , readLastBlock
// – Read the first respective last page in a file
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	//Check if there exisits any PAGE to be read
	if((fHandle->totalNumPages)< 1 )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	return(readBlock(0, fHandle, memPage));
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	//Check if there exisits any PAGE to be read
	if((fHandle->totalNumPages)< 1 )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}   
	return(readBlock((fHandle->totalNumPages-1), fHandle, memPage));
}

// readPreviousBlock , readCurrentBlock , readNextBlock
// – Read the current, previous, or next page relative to the curPagePos of the file.
// – The curPagePos should be moved to the page that was read.
// – If the user tries to read a block before the first page or after the last page of the file, the method should
// return RC READ NON EXISTING PAGE.

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	//Check if there exisits any PAGE to be read
	if((fHandle->curPagePos-1)>(fHandle->totalNumPages) || (fHandle->curPagePos-1)< 0 )
	{
		return RC_READ_NON_EXISTING_PAGE;    
	}
	return(readBlock((fHandle->curPagePos-1), fHandle, memPage));
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	//Check if there exisits any PAGE to be read
	if((fHandle->curPagePos)>(fHandle->totalNumPages) || (fHandle->curPagePos)< 0 )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	return(readBlock(fHandle->curPagePos, fHandle, memPage));
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	//Check if there exisits any PAGE to be read
	if(((fHandle->curPagePos)+1)>(fHandle->totalNumPages) || (fHandle->curPagePos+1)< 1 )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	return(readBlock((fHandle->curPagePos+1), fHandle, memPage));
}

// • writeBlock , writeCurrentBlock
// – Write a page to disk using either the current position or an absolute position.
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	//Check the validity of the pageNum
	if(pageNum<0){
		return RC_READ_NON_EXISTING_PAGE;
	}

	//Ensure capacity before writting.
	RC ensure_capacity_check=ensureCapacity(pageNum,fHandle);
	if(ensure_capacity_check!=RC_OK){
		return ensure_capacity_check;
	}
	if(fseek(fHandle->mgmtInfo, PAGE_SIZE*pageNum, SEEK_SET)==0)
	{
		int bytes_written=fwrite(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
		//Compare the amount of bytes written to disk.
		if(bytes_written<PAGE_SIZE)
		{
			return RC_WRITE_FAILED;
		}
		//Update the current page position attribute.
		fHandle->curPagePos = pageNum;

		//Update the total number of pages if a new page was written.
		fseek(fHandle->mgmtInfo, 0, SEEK_END);
		fHandle->totalNumPages=ftell(fHandle->mgmtInfo)/PAGE_SIZE;
		return RC_OK;
	}
	else
	{
		return RC_WRITE_FAILED;
	}
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	return(writeBlock (fHandle->curPagePos, fHandle, memPage));
}

// • appendEmptyBlock
// – Increase the number of pages in the file by one. The new last page should be filled with zero bytes.
RC appendEmptyBlock(SM_FileHandle *fHandle)
{
	//Initialize a variable of block size PAGE_SIZE.
	char block[PAGE_SIZE];
	for(int i=0; i<PAGE_SIZE; i++)
	{
		block[i]='\0';
	}
	return(writeBlock(fHandle->totalNumPages, fHandle, block));
}

// • ensureCapacity
// – If the file has less than numberOfPages pages then increase the size to numberOfPages.
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
	//Compare to check if the capacity is already present. If not, add pages iteratively.
	int totalNumPages=fHandle->totalNumPages;
	int pagesToBeAdded=numberOfPages-totalNumPages;
	while (pagesToBeAdded>0)
	{
		//Try to append the block, return error message if we do not succeed.
		RC append_status=appendEmptyBlock(fHandle);
		if(append_status!=RC_OK)
		{
			return append_status;
		}
		//Reduce the page counter pending to be added.
		pagesToBeAdded--;
	}
	return RC_OK;
}

