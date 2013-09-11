//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
//
//  owFile.c - Contains the main file functions for the 1-Wire Public Domain
//             Kit
//
//  version 1.00
//

#include <ctype.h>
#include <string.h>
#include "owfile.h"
#include "rawmem.h"
#include "mbeprom.h"

// External Function Prototypes
extern void owClearError(void);

// Globals
uchar    LastFile;        // 1
FileInfo Han[5];          // 60
CurrentDirectory CD;      // 69
uchar    Internal;        // 1
// Page_WR in file layer
PAGE_TYPE LastPage;        // 1
uchar    FPage;           // 1
ushort   buflen;          // 2
char     mxpgs;           // 1
char     BadRec;          // 1
uchar    current[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


//--------------------------------------------------------------------------
// Create a sub-directory.  Note that a sub-directory is created empty
// and only an empty sub-directory can be removed.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// DirName    the file information that is to be created.
//
// Returns:  1(SUCCESS) : Operation successful
//           0(FAILURE) : error
//
SMALLINT owCreateDir(int portnum, uchar *SNum, FileEntry *DirName)
{
   uchar nwdir[] = { 0xAA, 0, 'R', 'O', 'O', 'T', 0, 0 };
   short i;
   PAGE_TYPE pg;
   SMALLINT bytes_written = 0;
   DirectoryPath reset;

   // check for a valid session device

   if(!ValidRom(portnum,SNum))
   	return FALSE;

   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // check to see if filename passed is valid
   if (!Valid_FileName(DirName))
   {
      OWERROR(OWERROR_XBAD_FILENAME);
      return FALSE;
   }

   // get the starting page
   if(CD.ne == 0)
   	pg = 0;
   else
   	pg = CD.Entry[CD.ne-1].page;

   // check to see if there is any room for another sub-directory
   if(CD.ne == 10)
   {
   	OWERROR(OWERROR_XUNABLE_TO_CREATE_DIR);
   	return FALSE;
   }

   // fill the file handle 4
   for (i = 0; i < 4; i++)
      Han[4].Name[i] = DirName->Name[i];
   Han[4].Ext = 0x7F;
   Han[4].Read = FALSE;

   // create the new sub-directory page
   // search through directory to verify file not found
   if(FindDirectoryInfo(portnum,SNum,&pg,&Han[4]))
   {
   	OWERROR(OWERROR_REPEAT_FILE);
   	return FALSE;
   }

   // if starting page is not 0 then is not default root so change
   if (pg != 0)
   {
      for (i = 0; i < 4; i++)
         nwdir[i+2] = CD.Entry[CD.ne-1].name[i];
      nwdir[6] = CD.Entry[CD.ne-1].page;
   }

   // Set the internal flag
   Internal = TRUE;

   // use the writefile command to accomplish this task
   if(!owWriteFile(portnum,SNum,4,nwdir,FILE_NAME_LEN))
      return FALSE;

   // turn off the shorternal flag
   Internal = FALSE;

   // check success of write
   if(bytes_written < 0)
      return FALSE;

   // reset current directory
   reset.Ref = '0';
   reset.NumEntries = 0;
   if(!owChangeDirectory(portnum,SNum,&reset))
      return FALSE;

   return TRUE;
}

//--------------------------------------------------------------------------
// Remove a sub-directory.  Note that a sub-directory is created empty
// and only an empty sub-directory can be removed.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// DirName    the file information that is to be removed.
//
// Returns:  1(SUCCESS) : Operation successful
//           0(FAILURE) : error
//
SMALLINT owRemoveDir(int portnum, uchar *SNum, FileEntry *DirName)
{
   uchar buf[64],BM[32],pgbuf[32],p,jobBM[32];
   short e,i,j;
   PAGE_TYPE pg = CD.Entry[CD.ne-2].page;
   PAGE_TYPE rpg,tpg;
   int   len = 0;
   uchar pgjobstat;
   SMALLINT bank = 1;
   DirectoryPath reset;

   // check for a valid session device
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // check to see if filename passed is valid
   if (!Valid_FileName(DirName))
   {
      OWERROR(OWERROR_XBAD_FILENAME);
      return FALSE;
   }

   // fill the file handle 4
   for (i = 0; i < 4; i++)
      Han[4].Name[i] = DirName->Name[i];
   Han[4].Ext = 0x7F;
   Han[4].Read = FALSE;

   // search through directory to verify file not found
   if(!FindDirectoryInfo(portnum,SNum,&pg,&Han[4]))
   {
   	OWERROR(OWERROR_FILE_NOT_FOUND);
   	return FALSE;
   }

	if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Han[4].Spage,&len))
		return FALSE;

   // check for empty
   if ((len != 8) || (buf[len-1] != 0))
   {
   	OWERROR(OWERROR_DIRECTORY_NOT_EMPTY);
   	return FALSE;
   }

   // Set the internal flag
   Internal = TRUE;

   // use the deletefile command to accomplish this task
   // check for special case of extension 100
   if(Han[4].Ext == 100 && (!owIsWriteOnce(bank,portnum,SNum)))
   {
   	OWERROR(OWERROR_WRONG_TYPE);
   	return FALSE;
   }

   // check to see if this entry is read only
   if (Han[4].Attrib == 0x01 && Han[4].Ext != 0x7F)
   {
   	OWERROR(OWERROR_READ_ONLY);
   	return FALSE;
   }

   // check existing open files with same the name
   // automatically close the file
   for (i = 0; i < 4; i++)
   {
       if(Han[i].Name[0] != 0)
       {
            for (j = 0; j < 4; j++)
                if ((Han[i].Name[j] & 0x7F) != Han[4].Name[j])
                   break;

            if (j == 4 && ((Han[i].Ext & 0x7F) == Han[4].Ext))
            {
               Han[i].Name[0] = 0;  // release the handle
               break;
            }
       }
   }

   // get the bitmap so that it can be adjusted
   if(!ReadBitMap(portnum,SNum,&BM[0]))
   	return FALSE;

   // check for addfile
   if (Han[4].Ext == 100)
   {
      tpg = Han[4].Spage;
      for (i = 0; i < 256; i++)
      {
         // read a page in the file
         if(!ExtendedRead_Page(portnum,SNum,&pgbuf[0],tpg))
         	return FALSE;

         // check if only in buffer
         // or non written buffered eprom page
         if(isJob(portnum,SNum))
         {
            // see if page set to write but has not started yet
            pgjobstat = isJobWritten(portnum,SNum,tpg);

            if ((pgjobstat == PJOB_ADD) &&
                (pgjobstat != PJOB_START))
            {
               // clear the job
               if(!setJobWritten(portnum,SNum,tpg,PJOB_NONE))
                  return FALSE;

               // clear this page in the bitmap if clear in original
               if(!getOrigBM(portnum,SNum,&jobBM[0]))
                  return FALSE;
               if(!(jobBM[tpg/8] & (0x01 << (tpg%8))))
                  if(!BitMapChange(portnum,SNum,(uchar)tpg,0,&BM[0]))
                     return FALSE;
            }
         }

         // get continuation pointer
         tpg = pgbuf[29];

         if (tpg == 0xFF)
            break;
      }
   }
   // regular file
   else
   {
      // read the file to eliminate the bits in the bitmap
      if(!ExtRead(portnum,SNum,&buf[0],Han[4].Spage,-1,TRUE,&BM[0],&len))
      	return FALSE;
   }

   // read the directory page to see what the situation is
	if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Han[4].Dpage,&len))
		return FALSE;

   // check for inappropriate directory length
   if ((len-1) % 7)
   {
   	OWERROR(OWERROR_FILE_READ_ERR);
   	return FALSE;
   }

   // get number of entries
   e = len / 7;

   // check for condition that this is last enty in a directory packet
   if (e == 1)
   {
      // remember the current pages pointer
      p = buf[len-1];

      // read the previous page to change its continuation pointer
		if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Han[4].PDpage,&len))
			return FALSE;

      // set the prev dir page cont ptr to current page cont ptr
      buf[len-1] = p;

      // clear this page in the bitmap
      if(!BitMapChange(portnum,SNum,Han[4].Dpage,0,&BM[0]))
      	return FALSE;

      // set the page number to write
      rpg = Han[4].PDpage;
   }
   // must keep this page and just remove the entry
   else
   {
      // remember the current pages pointer
      p = buf[len-1];

      // shift directory page over to remove the entry
      for (i = Han[4].Offset; i < (len - 7); i++)
         buf[i] = buf[i+7];

      // reduce the lenght of the directory page by 1 entry
      len -= 7;

      // set the continuation page
      buf[len-1] = p;

      // set the page number to write
      rpg = Han[4].Dpage;

   }

   // write the directory page needed
	if(!Write_Page(portnum,SNum,&buf[0],rpg,len))
		return FALSE;
   else
   {
      if(!BitMapChange(portnum,SNum,Han[4].Spage,0,&BM[0]))
         return FALSE;
   }

   // update the bitmap
   if(!WriteBitMap(portnum,SNum,&BM[0]))
      return FALSE;

   // turn off the internal flag
   Internal = FALSE;

   // reset current directory
   reset.Ref = '0';
   reset.NumEntries = 0;
   if(!owChangeDirectory(portnum,SNum,&reset))
      return FALSE;

   return TRUE;
}

//--------------------------------------------------------------------------
//  Write a file by reference to a hnd.  Buffer and
//  length are provided.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// hnd        the handle of the file
// buf        the information to write to the file
// len        the len of the information to be written
//
//  Returns:   TRUE  File Write successful
//             FALSE File Write a failure
//
SMALLINT owWriteFile(int portnum, uchar *SNum, short hnd,
                     uchar *buf, int len)
{
   uchar BM[32],dbuf[32],tbuf[32];
   short i,j;
   int dlen,templen;
   PAGE_TYPE next,avail,tpg,dpg,pg;
   PAGE_TYPE fpg = 0,prefpg = 0;
   uchar     pgbuf[32];
   uchar     newpg[32];
   int       flag,preflag = FALSE;
   FileEntry Dmmy;
   uchar *ptr;
   SMALLINT bank,mavail;
   FileInfo Info;
   int new_entry = TRUE;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

	// check to see if handle valid number
   if ((hnd > 3 || hnd < 0) && (Internal == FALSE))
   {
   	OWERROR(OWERROR_HANDLE_NOT_EXIST);
   	return FALSE;
   }

   // check to see if hnd provided is valid file reference
   if (!Han[hnd].Name[0])
   {
   	OWERROR(OWERROR_HANDLE_NOT_USED);
   	return FALSE;
   }

   bank = getBank(portnum,SNum,Han[hnd].Spage,REGMEM);

   // check to see if hnd is a write reference
   if(owIsReadOnly(bank,portnum,SNum))
   {
   	OWERROR(OWERROR_READ_ONLY);
   	return FALSE;
   }

   // check if this is an addfile
   if (Han[hnd].Ext == 100)
   {
      templen = len;
      if(!owWriteAddFile(portnum,SNum,hnd,APPEND,0,buf,&templen))
         return FALSE;
   }

   // get the bitmap to find empties and to adjust
   if(!ReadBitMap(portnum,SNum,&BM[0]))
   	return FALSE;

   // check to see if file to write is too long (not take into account directory)
   if((Han[hnd].Ext == 101) || (Han[hnd].Ext == 102))
   {
      // check if longer then page or some other file already got the page
      if ((len > 28) ||
          (BM[Han[hnd].Spage / 8] & (0x01 << (Han[hnd].Spage % 8))))
      {
         if(len != 0)
         {
      	   OWERROR(OWERROR_OUT_OF_SPACE);
      	   return FALSE;
         }
      }
   }

   mavail = GetMaxWrite(portnum,SNum,&BM[0]);
   if (mavail == 0 || len > mavail)
   {
   	OWERROR(OWERROR_OUT_OF_SPACE);
   	return FALSE;
   }

   // try to find an empty spot in the directory
   if(!ReadNumEntry(portnum,SNum,-1,&Dmmy,&dpg))
   	return FALSE;

   // read the directory page indicated to see if it has space or is the last
   tpg = (uchar)dpg;

   if(!Read_Page(portnum,SNum,&dbuf[0],REGMEM,&tpg,&dlen))
   	return FALSE;

   for(i=0;i<4;i++)
      Info.Name[i] = Han[hnd].Name[i];
   Info.Ext = Han[hnd].Ext;
   // special case for first directory page
   i = 7;
   // check to see if first dir page is a valid dir
   if ((dbuf[0] != 0xAA) && (dbuf[1] != 0) && (dlen > 29))
   {
     	OWERROR(OWERROR_FILE_READ_ERR);
     	return FALSE;
   }
   else if((dbuf[0] != 0xAA) || (dbuf[1] != 0))
   {
      i = 0;
   }

   // loop through each directory entry to find FInfo
   // loop to search through directory pages
   do
   {
      // read a page in the directory
      if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&fpg,&flag))
      	return FALSE;

      // if have a good read then search inside page for EntNum
      if (flag > 0)
      {
         // special case for first directory page
         i = 0;
         if (fpg == 0)
         {
            i = 7;
            // check to see if first dir page is a valid dir
            if (pgbuf[0] != 0xAA || pgbuf[1] != 0)
            {
            	OWERROR(OWERROR_FILE_READ_ERR);
            	return FALSE;
            }
         }

         // loop through each directory entry to find FInfo
         for (;i < flag; i += 7)
         {
            for (j = 0; j < 4; j++)   // compare file name
               if ((toupper(pgbuf[i+j]) != Info.Name[j]) &&
                   !((pgbuf[i+j] == 0x20) && (Info.Name[j] == 0xCC)))
                  break;


            if ((j != 4) || ((pgbuf[i+4] & 0x7F) != Info.Ext))
               continue;  // loop if not match

            Han[hnd].Spage  = pgbuf[i+5];
            Han[hnd].Ext    = pgbuf[i+4] & 0x7F;
            Han[hnd].NumPgs = (len/28) + ((len % 28) ? 1 : 0);
            pgbuf[i+6] = Han[hnd].NumPgs;
            Han[hnd].Attrib = (pgbuf[i+4] & 0x80) ? 1 : 0;
            if(Han[hnd].Ext == 0x7F)
               Han[hnd].Attrib <<= 1;
            Han[hnd].Dpage  = (uchar) dpg;
            Han[hnd].Offset = (uchar) i;

            new_entry = FALSE;
            for(i=0;i<flag;i++)
               newpg[i] = pgbuf[i];
            preflag   = flag;
            prefpg    = fpg;
            break;
         }

         // poshort to next page
         fpg = pgbuf[flag-1];
      }
      else
         return FALSE;  // return error
   }
   while(fpg);

   // in light of the directory page read check space again
   if (dlen == 29)
      mavail-= 28;
   if (mavail <= 0 || len > mavail)
   {
   	OWERROR(OWERROR_OUT_OF_SPACE);
   	return FALSE;
   }

   // Record where the file begins
   if((Han[hnd].Ext != 101) && (Han[hnd].Ext != 102) && (new_entry))
   {
      FindEmpties(portnum,SNum,&BM[0],&avail,&next);
      Han[hnd].Spage = avail;
   }

   // write the file in the available space
   if((Han[hnd].Ext == 101) || (Han[hnd].Ext == 102))  // Monitary file exception
   {
      // copy the data in to a temp buffer and add a continuation pointer
      for (i = 0; i < len; i++)
         tbuf[i] = buf[i];
      tbuf[i] = 0;

      // write the page
      if(!Write_Page(portnum,SNum,&tbuf[0],Han[hnd].Spage,(len+1)))
      	return FALSE;

      // set the bitmap page taken
      if(!BitMapChange(portnum,SNum,Han[hnd].Spage,1,&BM[0]))
      	return FALSE;
   }
   else
   {
   	if(!ExtWrite(portnum,SNum,Han[hnd].Spage,&buf[0],len,&BM[0]))
   		return FALSE;
   }

   if(new_entry)
   {
      // construct the new directory entry
      Han[hnd].NumPgs = (Internal) ? 0 : ((len / 28) + ((len % 28) ? 1 : 0));
      // don't allow external zero number of pages unless a directory
      if ((!Han[hnd].NumPgs) && (Han[hnd].Ext != 0x7F))
         Han[hnd].NumPgs = 1;
      Han[hnd].Attrib = 0;

      // get an empty page
      FindEmpties(portnum,SNum,&BM[0],&avail,&next);

      // check to see if need to write a new directory page
      if (dlen == 29)
      {
         // write the new page
		   if(!Write_Page(portnum,SNum,(uchar *) &Han[hnd],avail,8))
			   return FALSE;

         // set the bitmap to account for this new directory page
         if(!BitMapChange(portnum,SNum,avail,1,&BM[0]))
      	   return FALSE;

         // change the exhisting directory page continuation pointer
         dbuf[dlen-1] = avail;
      }
      // add the new directory entry to exhisting directory page
      else
      {
         // remember the continuation pointer
         pg = dbuf[dlen-1];

         // get a pointer shorto the handle
         ptr = (uchar *) &Han[hnd];

         // loop to add the new entry
         for (i = 0; i < 7; i++)
            dbuf[dlen-1+i] = ptr[i];

         // adjust the length of the directory page
         dlen += 7;

         // correct the continuation pointer
         dbuf[dlen-1] = pg;
      }
   }

   // special case of dir page == 0 and local bitmap
   if ((dpg == 0) && (dbuf[2] & 0x80))
   {
      // update the local bitmap
      for (i = 0; i < 2; i++)
         dbuf[3+i] = BM[i];
      dbuf[5] = 0;
      dbuf[6] = 0;
   }
   // non page 0 or non local bitmap
   else
   {
      // write the bitmap
      if(!WriteBitMap(portnum,SNum,&BM[0]))
      	return FALSE;
   }

   // now rewrite the directory page
   if(new_entry)
   {
	   if(!Write_Page(portnum,SNum,&dbuf[0],dpg,dlen))
		   return FALSE;
   }
   else
   {
      if(!Write_Page(portnum,SNum,&newpg[0],prefpg,preflag))
         return FALSE;
   }

   // free the file handle
   if(len != 0)
      Han[hnd].Name[0] = 0;

   return TRUE;

}


//--------------------------------------------------------------------------
//  Deletes an indicated file.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// flname     the name and information of the file to delete
//
//  return:  TRUE  : file deleted
//           FALSE : error
//
SMALLINT owDeleteFile(int portnum, uchar *SNum, FileEntry *flname)
{
   short e,i,j;
   uchar BM[32],buf[32],p;
   uchar pgbuf[34];
   int   len,err;
   PAGE_TYPE pg = 0,tpg;
   FileInfo Info;
   SMALLINT  bank = 1;
   DirectoryPath reset;
   uchar pgjobstat;
   uchar jobBM[32];

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if filename passed is valid
   if ((Valid_FileName(flname) == FALSE) || (((flname->Ext & 0x7F) == 0x7F) &&
       (Internal == FALSE)))
   {
   	OWERROR(OWERROR_XBAD_FILENAME);
   	return FALSE;
   }

   // check for special case of extension 100
   if(flname->Ext == 100 && (!owIsWriteOnce(bank,portnum,SNum)))
   {
   	OWERROR(OWERROR_WRONG_TYPE);
   	return FALSE;
   }

   // depending on the directory set the first page to look for entry
   pg = (CD.ne == 0) ? 0 : CD.Entry[CD.ne-1].page;

   // copy filename over to info packet
   for (i = 0; i < 4; i++)
       Info.Name[i] = flname->Name[i];
   Info.Ext = flname->Ext;

   // check directory list to find the location of the file name
   if(!FindDirectoryInfo(portnum,SNum,&pg,&Info))
   {
      OWERROR(OWERROR_FILE_NOT_FOUND);
   	return FALSE;
   }

   // check to see if this entry is read only
   if (Info.Attrib == 0x01 && Info.Ext != 0x7F)
   {
   	OWERROR(OWERROR_READ_ONLY);
   	return FALSE;
   }

   // check existing open files with same the name (3.10)
   // automatically close the file
   for (i = 0; i < 4; i++)
   {
       if(Han[i].Name[0] != 0)
       {
            for (j = 0; j < 4; j++)
                if ((Han[i].Name[j] & 0x7F) != flname->Name[j])
                   break;

            if (j == 4 && ((Han[i].Ext & 0x7F) == flname->Ext))
            {
               Han[i].Name[0] = 0;  // release the handle
               break;
            }
       }
   }

   // get the bitmap so that it can be adjusted
   if(!ReadBitMap(portnum,SNum,BM))
   	return FALSE;

   // check for addfile
   if (Info.Ext == 100)
   {
      tpg = Info.Spage;
      for (i = 0; i < 256; i++)
      {
         // read a page in the file
         if(!ExtendedRead_Page(portnum,SNum,&pgbuf[0],tpg))
         	return FALSE;

         // check if only in buffer
         // or non written buffered eprom page
         if(isJob(portnum,SNum))
         {
            // see if page set to write but has not started yet
            pgjobstat = isJobWritten(portnum,SNum,tpg);

            if ((pgjobstat == PJOB_ADD) &&
                (pgjobstat != PJOB_START))
            {
               // clear the job
               if(!setJobWritten(portnum,SNum,tpg,PJOB_NONE))
                  return FALSE;

               // clear this page in the bitmap if clear in original
               if(!getOrigBM(portnum,SNum,&jobBM[0]))
                  return FALSE;
               if(!(jobBM[tpg/8] & (0x01 << (tpg%8))))
                  if(!BitMapChange(portnum,SNum,(uchar)tpg,0,&BM[0]))
                     return FALSE;
            }
         }

         // get continuation pointer
         tpg = pgbuf[29];

         if (tpg == 0xFF)
            break;
      }
   }
   // regular file
   else
   {
      owClearError();
      // read the file to eliminate the bits in the bitmap
      if(!ExtRead(portnum,SNum,&buf[0],Info.Spage,-1,TRUE,BM,&len))
      {
         if(!BitMapChange(portnum,SNum,Info.Spage,0,BM))
      	   return FALSE;

         err = owGetErrorNum();
         if(err != OWERROR_INVALID_PACKET_LENGTH)
         {
            OWERROR(err);
      	   return FALSE;
         }
      }
   }

   // read the directory page to see what the situation is
	if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Info.Dpage,&len))
		return FALSE;

   // check for inappropriate directory length
   if ((len-1) % 7)
   {
   	OWERROR(OWERROR_FILE_READ_ERR);
   	return FALSE;
   }

   // get number of entries
   e = len / 7;

   // check for condition that this is last enty in a directory packet
   if (e == 1)
   {
      // remember the current pages pointer
      p = buf[len-1];

      // read the previous page to change its continuation pointer
		if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Info.PDpage,&len))
			return FALSE;

      // set the prev dir page cont ptr to current page cont ptr
      buf[len-1] = p;

      // clear this page in the bitmap
      if(!BitMapChange(portnum,SNum,Info.Dpage,0,BM))
      	return FALSE;

      // set the page number to write
      pg = Info.PDpage;
   }
   // must keep this page and just remove the entry
   else
   {
      // remember the current pages pointer
      p = buf[len-1];

      // shift directory page over to remove the entry
      for (i = Info.Offset; i < (len - 7); i++)
         buf[i] = buf[i+7];

      // reduce the lenght of the directory page by 1 entry
      len -= 7;

      // set the continuation page
      buf[len-1] = p;

      // set the page number to write
      pg = Info.Dpage;
   }

   // write the directory page needed
	if(!Write_Page(portnum,SNum,&buf[0],pg,len))
		return FALSE;
   else
   {
      if(!BitMapChange(portnum,SNum,Info.Spage,0,&BM[0]))
         return FALSE;
   }

   // update the bitmap
   if(!WriteBitMap(portnum,SNum,BM))
      return FALSE;

   // reset current directory
   reset.Ref = '0';
   reset.NumEntries = 0;
   if(!owChangeDirectory(portnum,SNum,&reset))
      return FALSE;

   return TRUE;
}


//--------------------------------------------------------------------------
//  Changes the attributes of a file.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// attr       the attribute to change
// flname     the file name on which the attribute is to be changed
//
//  return:  TRUE  : file attribute changed
//           FALSE : error
//
SMALLINT owAttribute(int portnum, uchar *SNum, short attr,
                     FileEntry *flname)
{
   uchar buf[32];
   short i;
   PAGE_TYPE pg=0;
   FileInfo Info;
   int len;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if filename passed is valid
   if (!Valid_FileName(flname))
   {
   	OWERROR(OWERROR_XBAD_FILENAME);
   	return FALSE;
   }

   // check to see if this attribute is supported
   if ((((flname->Ext & 0x7F) == 0x7F) && ((attr != 0x02) && (attr != 0))) ||
       (((flname->Ext & 0x7F) != 0x7F) && ((attr != 0x01) && (attr != 0)))    )
   {
   	OWERROR(OWERROR_FUNC_NOT_SUP);
   	return FALSE;
   }

   // depending on the directory set the first page to look for entry
   pg = (CD.ne == 0) ? 0 : CD.Entry[CD.ne-1].page;

   // copy filename over to info packet
   for (i = 0; i < 4; i++)
       Info.Name[i] = flname->Name[i];
   Info.Ext = flname->Ext;

   // check directory list to find the location of the name
   if(!FindDirectoryInfo(portnum,SNum,&pg,&Info))
   	return FALSE;

   // check to see if the attribute already matches the entry
   if (Info.Attrib == attr)
      return TRUE;

   // read the directory page that the entry to change is in
	if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Info.Dpage,&len))
		return FALSE;

   // exchange the new attribute for the old
   if (attr)
      buf[4+Info.Offset] |= 0x80;                               // set attr bit
   else
      buf[4+Info.Offset] = (buf[4+Info.Offset] | 0x80) ^ 0x80;  // clear attr bit

   // write back the directory page with the new name
	if(!Write_Page(portnum,SNum,&buf[0],Info.Dpage,len))
		return FALSE;

	return TRUE;
}


//--------------------------------------------------------------------------
//  Change the name of a file that has been opened with the file handle
//  'hnd'.  The new name in 'flname'.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// hnd        the handle of the file to rename
// flname     the renamed file name
//
//  return:   TRUE  : filename changed
//            FALSE : error
//
SMALLINT owReNameFile(int portnum, uchar *SNum, short hnd,
                      FileEntry *flname)
{
   uchar buf[32];
   short i,j;
   PAGE_TYPE pg = 0;
   FileInfo Info;
   int len;
   SMALLINT bank;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if handle valid number
   if (hnd > 3 || hnd < 0)
   {
   	OWERROR(OWERROR_HANDLE_NOT_EXIST);
   	return FALSE;
   }

   // check to see if filename passed is valid
   if (!Valid_FileName(flname) || (flname->Ext & 0x80) || (flname->Ext > 100) ||
       ((flname->Ext == 100) && (Han[hnd].Ext != 100)) ||
       ((flname->Ext != 100) && (Han[hnd].Ext == 100)) )
   {
   	OWERROR(OWERROR_XBAD_FILENAME);
   	return FALSE;
   }

   // check to see if hnd provided is valid file reference
   if (!Han[hnd].Name[0])
   {
   	OWERROR(OWERROR_HANDLE_NOT_USED);
   	return FALSE;
   }

   bank = getBank(portnum,SNum,Han[hnd].Spage,REGMEM);

   // check to see if hnd is a write reference
   if(owIsReadOnly(bank,portnum,SNum))
   {
   	OWERROR(OWERROR_READ_ONLY);
   	return FALSE;
   }

   // depending on the directory set the first page to look for entry
   pg = (CD.ne == 0) ? 0 : CD.Entry[CD.ne-1].page;

   // copy filename over to NewFile
   for (i = 0; i < 4; i++)
       Info.Name[i] = flname->Name[i];
   Info.Ext = flname->Ext;

   // check directory list to see if new name is already in list
   if(FindDirectoryInfo(portnum,SNum,&pg,&Info))
   {
   	// file name already exists
   	Han[hnd].Name[0] = 0;   // release the handle
   	return FALSE;
   }

   // read the directory page that the entry to change is in
   if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Han[hnd].Dpage,&len))
   	return FALSE;

   // exchange the new name for the old
   for (j = 0; j < 4; j++)
      buf[j+Han[hnd].Offset] = flname->Name[j];
   buf[Han[hnd].Offset+4] = flname->Ext;
   // check to keep attribute the same as old
   if (Han[hnd].Attrib)
      buf[Han[hnd].Offset+4] |= 0x80;

   // write back the directory page with the new name
	if(!Write_Page(portnum,SNum,&buf[0],Han[hnd].Dpage,len))
		return FALSE;

   Han[hnd].Name[0] = 0;  // release handle on success

	return TRUE;
}


//--------------------------------------------------------------------------
//  Read a file by reference to a handle.  Buffer and
//  max length are provided.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// hnd        the handle of the file
// buf        the information to that was read
// maxlen     the max length of the buffer that stores the
//            read data
// fl_len     the len of the data read
//
//  return: TRUE  : file read
//          FLASE : error
//
SMALLINT owReadFile(int portnum, uchar *SNum, short hnd,
                    uchar *buf, int maxlen, int *fl_len)
{
   short len,i;
   uchar BM[32],pg;
   uchar pgbuf[64];
   SMALLINT bank;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if handle valid number
   if (hnd > 3 || hnd < 0)
   {
   	OWERROR(OWERROR_HANDLE_NOT_EXIST);
   	return FALSE;
   }

   // check to see if hnd provided is valid file reference
   if (!Han[hnd].Name[0])
   {
   	OWERROR(OWERROR_HANDLE_NOT_USED);
   	return FALSE;
   }

   bank = getBank(portnum,SNum,Han[hnd].Spage,REGMEM);

   // check to see if hnd is a write reference
   if(owIsReadOnly(bank,portnum,SNum))
   {
   	OWERROR(OWERROR_READ_ONLY);
   	return FALSE;
   }

   // check for open 'add' file exception
   if (((Han[hnd].Ext == 100) && (Han[hnd].NumPgs == 0)) ||
       (Han[hnd].Ext == 101) || (Han[hnd].Ext == 102))
   {
      pg = Han[hnd].Spage;
      len = 0;
      for (;;)
      {
         for(i=0;i<32;i++)
            pgbuf[i] = 0xFF;

         // read a page in the file
         if(!ExtendedRead_Page(portnum,SNum,&pgbuf[0],pg))
         	return FALSE;

         // Monitary File
         if((Han[hnd].Ext == 101) || (Han[hnd].Ext == 102))
         {
            if ((pgbuf[0] > 29) || ((pgbuf[0] + 7) > maxlen))
            {
            	OWERROR(OWERROR_BUFFER_TOO_SMALL);
            	return FALSE;
            }

            // copy the counter/tamper to the end of the expected data
            for (i = 0; i < owGetExtraInfoLength(bank,SNum); i++)
               buf[i+pgbuf[0]-1] =  pgbuf[i+32];

            break;
         }
         // AddFile
         else
         {
            if ((len + 28) > maxlen)
            {
            	OWERROR(OWERROR_BUFFER_TOO_SMALL);
            	return FALSE;
            }

            for (i = 1; i <= 28; i++)
               buf[len++] = pgbuf[i];

            pg = pgbuf[29];

            if (pg == 0xFF)
            {
               for(i=29;i>0;i--)
                  if(pgbuf[i] != 0xFF)
                  {
                     *fl_len = len - (29-i-1);
                     break;
                  }

               if(i == 0)
                  *fl_len = len - 28;

               return TRUE;
            }
         }
      }
   }

   // if not a non-terminated addfile
   if (!((Han[hnd].Ext == 100) && (Han[hnd].NumPgs == 0)))
   {
		if(!ExtRead(portnum,SNum,&buf[0],Han[hnd].Spage,maxlen,FALSE,BM,fl_len))
			return FALSE;
   }

   return TRUE;
}


//--------------------------------------------------------------------------
//  Opens a file by first verifying that the file exists on the device and
//  to assign a handle.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// flname     the name of the file and info. to open
// hnd        the assigned handle
//
//  return: TRUE  : file found
//          FALSE : error
//
SMALLINT owOpenFile(int portnum, uchar *SNum, FileEntry *flname, short *hnd)
{
   short i,hn = 0;
   PAGE_TYPE pg = 0;
   SMALLINT bank = 1;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if filename passed is valid
   if (!Valid_FileName(flname) || ((flname->Ext & 0x7F) > 102))
   {
   	OWERROR(OWERROR_XBAD_FILENAME);
   	return FALSE;
   }

   // check for special case of extension 100  (2.10)
   if (((flname->Ext == 100) &&
        (!owIsWriteOnce(bank,portnum,SNum))) ||
       ((flname->Ext == 101) && ((SNum[0] == 0x1A)  || (SNum[0] == 0x1C) ||
       									(SNum[0] == 0x1D))) )
   {
   	OWERROR(OWERROR_WRONG_TYPE);
   	return FALSE;
   }

   // find a handle
	for (i = 0; i < 4; i++)
      if(Han[i].Name[0] == 0)
      {
         hn = i;
         break;
      }
      else if((Han[i].Name[0] != 0) && (i == 3))
      {
      	OWERROR(OWERROR_HANDLE_NOT_AVAIL);
      	return FALSE;
      }

   // depending on the directory set the first page to look for entry
   pg = (CD.ne == 0) ? 0 : CD.Entry[CD.ne-1].page;

   // copy the filename to the open handle
   for (i = 0; i < 4; i++)
      Han[hn].Name[i] = flname->Name[i];
   Han[hn].Ext = flname->Ext & 0x7F;

   // search through directory to find file to open
   if(!FindDirectoryInfo(portnum,SNum,&pg,&Han[hn]))
   {
   	Han[hn].Name[0] = 0;  // release the handle
      OWERROR(OWERROR_FILE_NOT_FOUND);
   	return FALSE;
   }

   // set the read flag
   Han[hn].Read = 1;

   // setting extra information
   flname->Spage  = Han[hn].Spage;
   flname->NumPgs = Han[hn].NumPgs;

   // return the handle number
   *hnd = hn;

   return TRUE;
}


//--------------------------------------------------------------------------
//  Formats a touch memory.  Supports local and remote bitmap.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
//
//  return:  TURE  : touch memory formatted
//           FALSE : error
//
SMALLINT owFormat(int portnum, uchar *SNum)
{
   uchar nwdir[] = { 0xAA, 0, 0x80, 0x01, 0, 0, 0, 0, 0};
   uchar bm[34],temp_buff[34];
   short cnt;
   int i,j,len;
   int maxP,findagain;
   SMALLINT bank = 1;
   PAGE_TYPE bitmap,eprom,next,last=0;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // zero the bitmap
   for (i = 0; i < 34; i++)
      bm[i] = 0;

   // invalidate current directory of device on this com port
   CD.ne = 0;

   maxP = maxPages(portnum,SNum);

   // check for file bitmap
   if ((maxP > 32) ||
       (owIsWriteOnce(bank,portnum,SNum)))
   {
      // tweek the standard directory for remote bitmap
      nwdir[2] = 0x00;  // bitmap control byte
      nwdir[3] = 0x00;  // reset bitmap
      if((SNum[0] == 0x0A) || (SNum[0] == 0x0C) || (SNum[0] == 0x21))
      {
         bitmap = 1;
      	nwdir[5] = bitmap;     // page address of bitmap
      }
      else if((SNum[0] == 0x0B) || (SNum[0] == 0x0F) || (SNum[0] == 0x13))
      {
         bitmap = 8;
      	nwdir[5] = bitmap;     // page address of bitmap
      }
      else
      {
         bitmap = 0;
      	nwdir[5] = bitmap;
      }

      // get eprom exception
      if ((!owIsWriteOnce(bank,portnum,SNum)) || (SNum[0]== 0x21) ||
          (SNum[0] == 0x14))
         nwdir[6] = (maxP > 223) ? 2 : 1; // pages in bitmap
      else
      {
         cnt =  ((maxP + 1) / 8) +
                (((maxP + 1) % 8) ? 1 : 0);
         nwdir[6] =  (cnt / 8) + ((cnt % 8) ? 1 : 0);
      }

      // zero the bitmap
      for (i = 0; i < 34; i++)
         bm[i] = 0;

      // set the pages to be used in the bitmap
      if((!owIsWriteOnce(bank,portnum,SNum)) || (SNum[0]== 0x21) ||
          (SNum[0] == 0x14))
         // nvram so bitmap is in regular space
         bm[0] = (nwdir[6] == 1) ? 0x03 : 0x07;
      else
         // must be an eprom so bitmap does not take up regular memory space
         bm[0] = 0x01;

      // write a null in page 0 before anything else
		if(!Write_Page(portnum,SNum,&nwdir[0],0,0))
			return FALSE;

      // depending on type, bitmap may be in different places
      if((!owIsWriteOnce(bank,portnum,SNum)) || (SNum[0] == 0x14) ||
         (SNum[0] == 0x18) || (SNum[0] == 0x23))
      {
         // calculate length and continuation pointers
         if (bm[0] == 0x07)
         {
            // set the continuation pointer
            bm[28] = bitmap + 1;
            // set length
            i = 29;
         }
         else
            // set length
            i = (maxP + 1)/8 + 1;

         // write the first bitmap page
			if(!Write_Page(portnum,SNum,bm,bitmap,i))
				return FALSE;

         // optionally write the second bitmap page
         if (bm[0] == 0x07)
         {
            i = (maxP + 1)/8 - 27;

            if(!Write_Page(portnum,SNum,&bm[29],(PAGE_TYPE) (bitmap+1),i))
            	return FALSE;
         }
      }
      // eprom so set page 0 in bitmap //  not set bitmap on format
      else
      {
         // set the page 0 just in case it has not been set
         if(!ReadBitMap(portnum,SNum,bm))
         	return FALSE;

         // try to clear all possible buffer pages
         for (i = 0; i < maxP; i++)
            if(!BitMapChange(portnum,SNum,(uchar)i,0,bm))
            	return FALSE;
      }
   }

   if(owIsWriteOnce(bank,portnum,SNum))
   {
      bank  = 0;
      eprom = 0;

      if(Read_Page(portnum,SNum,&temp_buff[0],REGMEM,&eprom,&len))
      {
         if((eprom != 0) && (last == 0))
            last = eprom;

         FindEmpties(portnum,SNum,bm,&eprom,&next);
         findagain = FALSE;
         do
         {
            if(last == eprom)
            {
               findagain = TRUE;
               if(!BitMapChange(portnum,SNum,eprom,1,bm))
                  return FALSE;
               if(!WriteBitMap(portnum,SNum,bm))
                  return FALSE;
            }
            else
            {
               if(owReadPage(bank,portnum,SNum,eprom,FALSE,&temp_buff[0]))
               {
                  for(j=0;j<owGetPageLength(bank,SNum);j++)
                     if(temp_buff[j] != 0xFF)
                     {
                        findagain = TRUE;
                        break;
                     }

                  if(j == owGetPageLength(bank,SNum))
                  {
                     findagain = FALSE;
                     if(owCanRedirectPage(bank,SNum))
                     {
                        if(!redirectPage(bank,portnum,SNum,last,eprom))
                           return FALSE;
                     }

                     if(!Write_Page(portnum,SNum,&nwdir[0],(PAGE_TYPE)eprom,8))
                        return FALSE;

                     if(!BitMapChange(portnum,SNum,eprom,1,bm))
         	            return FALSE;
                     if(!WriteBitMap(portnum,SNum,bm))
         	            return FALSE;
                  }
                  else
                  {
                     if(!BitMapChange(portnum,SNum,eprom,1,bm))
                        return FALSE;
                     if(!WriteBitMap(portnum,SNum,bm))
                        return FALSE;
                  }
               }

            }

            if(findagain)
            {
               FindEmpties(portnum,SNum,bm,&eprom,&next);

               if(eprom == last)
               {
                  return FALSE;
               }
               else
               {
                  if(owCanRedirectPage(bank,SNum))
                  {
                     if(!redirectPage(bank,portnum,SNum,last,eprom))
                        return FALSE;
                  }
               }
            }
         }
         while(findagain);
      }
      else
      {
         if(!Write_Page(portnum,SNum,&nwdir[0],eprom,8))
            return FALSE;

         if(!BitMapChange(portnum,SNum,eprom,1,bm))
         	return FALSE;
         if(!WriteBitMap(portnum,SNum,bm))
         	return FALSE;
      }
   }
   else
   {
	   if(!Write_Page(portnum,SNum,&nwdir[0],0,8))
		   return FALSE;

      if(!BitMapChange(portnum,SNum,0,1,bm))
       	return FALSE;

      if(!WriteBitMap(portnum,SNum,bm))
       	return FALSE;
   }

   for(i=0;i<4;i++)
      if(!owCloseFile(portnum,SNum,(short)i))
         OWERROR(OWERROR_FILE_NOT_FOUND);

   owClearError();

   return TRUE;
}


//--------------------------------------------------------------------------
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// FE         the infomation of the first file/dir found
//
//  return:   FALSE error or no files found
//            204   if first found
//
SMALLINT owFirstFile(int portnum, uchar *SNum, FileEntry *FE)
{
	PAGE_TYPE pg;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   LastFile = 0;   // reset the file counter

   // check to see if in a sub-directory, if yes then return '.'
   if (CD.ne > 0)
   {
      FE->Name[0] = '.';
      FE->Name[1] = ' ';
      FE->Name[2] = ' ';
      FE->Name[3] = ' ';
      FE->Ext = 0x7F;
      FE->Spage = CD.Entry[CD.ne-1].page;
      FE->NumPgs = 0;
      FE->Attrib = CD.Entry[CD.ne-1].attrib;
      // attempt to read the bitmap
      if(!ReadBitMap(portnum,SNum,&FE->BM[0]))
			return FALSE;
   }
   // not sub-directory
   else
   {
      // read the part of the directory to find the first file
      if(!ReadNumEntry(portnum,SNum,1,FE,&pg))
      	return FALSE;
   }

   LastFile += 1;   // update the file counter

   return FIRST_FOUND;      //  don't know how many now so return max
}

//--------------------------------------------------------------------------
// Find next file and put it shorto the string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// FE         the infomation of the first file/dir found
//
//  return:  TRUE  : file found
//           FALSE : no file found, error
//
SMALLINT owNextFile(int portnum, uchar *SNum, FileEntry *FE)
{
   PAGE_TYPE pg,page;

   // check for a first reference
   if (!LastFile)
      return owFirstFile(portnum,SNum,FE);

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if the directory is valid
   pg = (CD.ne == 0) ? 0 : CD.Entry[CD.ne-1].page;;

   // check to see if in a sub-directory, if yes and LastFile = 1 then return '..'
   if ((CD.ne > 0) && (LastFile == 1))
   {
      FE->Name[0] = '.';
      FE->Name[1] = '.';
      FE->Name[2] = ' ';
      FE->Name[3] = ' ';
      FE->Ext = 0x7F;
      // check for root exception for previous directory
      if (CD.ne == 1)
      {
         FE->Spage = 0;
         FE->Attrib = 0;
      }
      // must be at least 2 levels deep so previous is not root
      else
      {
         FE->Spage  = CD.Entry[CD.ne-2].page;
         FE->Attrib = CD.Entry[CD.ne-2].attrib;
      }
      FE->NumPgs = 0;
      // attempt to read the bitmap
      if(!ReadBitMap(portnum,SNum,&FE->BM[0]))
      	return FALSE;
   }
   // not (sub-directory and not back reference)
   else
   {
      // read the part of the directory to find the next file
      // figure the entry number if in sub-directory
      pg = (CD.ne > 0) ? LastFile - 1 : LastFile + 1;

      owClearError();

      if(!ReadNumEntry(portnum,SNum,pg,FE,&page))
      {
      	if(owGetErrorNum() == OWERROR_FILE_NOT_FOUND)
         {
      		LastFile = 0;    // reset the file counter
            return FALSE;
         }
      	else if(!owHasErrors())
         {
            LastFile = 0;
      		return FALSE;
         }
         else
            return FALSE;
      }
   }

   LastFile += 1;   // update the file counter

   return TRUE;
}


//--------------------------------------------------------------------------
// Get the current directory path
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
//
// returns    the current directory path
//
DirectoryPath owGetCurrentDir(int portnum, uchar *SNum)
{
   DirectoryPath retpath;
   int i,j;

   retpath.NumEntries = CD.ne;        // set the number of directory levels
   retpath.Ref = '\\';                // symbol for root

   // loop to copy each entry
   for (i = 0; i < CD.ne; i++)
      for (j = 0; j < 4; j++)
         retpath.Entries[i][j] = CD.Entry[i].name[j];

   return retpath;
}


//--------------------------------------------------------------------------
//  owChangeDirectory
//
//  Set or read the current directory.  The 'Operation' flag tells what the
//  operation will be.  If 'Operation' is 1 then the working directory is read.
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// CDBuf      the directory to change to
//
//  return:  TRUE : directory set or read successfully
//           FALSE: error
//
//
SMALLINT owChangeDirectory(int portnum, uchar *SNum, DirectoryPath *CDBuf)
{
   short i,j;
   uchar ROM[8];
   CurrentDirectory TCD;
   uchar pg=0;
   FileInfo fbuf;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if new directory is a reference from current directory
   if (CDBuf->Ref == '.')
      TCD = CD; // copy current directory
   // else starting from root with an empty directory
   else
      TCD.ne = 0;

   // loop through new directory and verify each level
   for (i = 0; i < CDBuf->NumEntries; i++)
   {
      // check for a back reference
      if (CDBuf->Entries[i][0] == '.' && CDBuf->Entries[i][1] == '.')
      {
         // reference TCD back a level if possible
         if (TCD.ne > 0)
            TCD.ne--;
         else
         {
          	OWERROR(OWERROR_INVALID_DIRECTORY);
           	return FALSE;
         }
      }
      // check for non-current reference
      else if (!(CDBuf->Entries[i][0] == '.')) //)
      {
         // add the forward reference to TCD
         for (j = 0; j < 4; j++)
         {
            TCD.Entry[TCD.ne].name[j] = toupper(CDBuf->Entries[i][j]);
            fbuf.Name[j] = TCD.Entry[TCD.ne].name[j]; // also copy to temp buffer
         }
         TCD.Entry[TCD.ne].page = 0;   // zero the page reference
         fbuf.Ext = 0x7F;  // set the extention to directory

         // read the current level to verify new level is valid
         pg = (TCD.ne > 0) ? TCD.Entry[TCD.ne-1].page : 0;

         // search through the next layer for the entry
         if(!FindDirectoryInfo(portnum,SNum,&pg,&fbuf))
         {
            OWERROR(OWERROR_INVALID_DIRECTORY);
          	return FALSE;
         }

         // remember the page that this level starts on
         TCD.Entry[TCD.ne].page = fbuf.Spage;
         // also remember the attribute of the directory
         TCD.Entry[TCD.ne].attrib = fbuf.Attrib;

         // since this level is a success go to next
         TCD.ne++;
      }
   }

   // success
   CD = TCD;

   // get the current ROM
   ROM[0] = 0;
   owSerialNum(portnum,&ROM[0],TRUE);

   // set the rom
   for (i = 0; i < 8; i++)
      CD.DRom[i] = ROM[i];

   return TRUE;
}


//--------------------------------------------------------------------------
//  Create a file by first verifying that the file does not exist on the
//  device and to assign a handle to it for writing.
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// maxwrite   the max that can be written
// hnd        the handle of the file created
// flname     the name and information for the file
//
//  return: TRUE  : file created
//          FALSE : error
//
SMALLINT owCreateFile(int portnum, uchar *SNum, int *maxwrite, short *hnd,
                      FileEntry *flname)
{
   short hn,flag,j;
   uchar i;
   PAGE_TYPE pg = 0,avail,next;
   uchar BM[32],pgbuf[52];
	SMALLINT bank = 1;
   int maxP;

   // check device type
   if(!ValidRom(portnum,SNum))
   	return FALSE;

   // check to see if filename passed is valid
   if (!Valid_FileName(flname) || (flname->Ext > 102))
   {
      OWERROR(OWERROR_XBAD_FILENAME);
      return FALSE;
   }

   // check for special case of extension 100
   if ((flname->Ext == 100) && (!owIsWriteOnce(bank,portnum,SNum)))
   {
   	OWERROR(OWERROR_WRONG_TYPE);
   	return FALSE;
   }

   // check for special case of extension 101
   if ((flname->Ext == 101) && ((SNum[0] != 0x1A) || (SNum[0] != 0x1C) ||
   									  (SNum[0] != 0x1D)))
   {
   	OWERROR(OWERROR_WRONG_TYPE);
   	return FALSE;
   }

   // check existing open files to with same name
   for (i = 0; i < 4; i++)
   {
       if (Han[i].Name[0] != 0)
       {
            for (j = 0; j < 4; j++)
                if ((Han[i].Name[j] & 0x7F) != flname->Name[j])
                   break;

            if (j == 4 && ((Han[i].Ext & 0x7F) == flname->Ext))
            {
            	OWERROR(OWERROR_REPEAT_FILE);  // file already exists
            	return FALSE;
            }
       }
   }

   hn = -1;
   // find a handle
   for (i = 0; i < 4; i++)
      if (Han[i].Name[0] == 0)
      {
         hn = i;
         break;
      }

   if(hn == -1)
   {
   	OWERROR(OWERROR_HANDLE_NOT_AVAIL);
   	return FALSE;
   }

   // depending on the directory set the first page to look for entry
   pg = (CD.ne == 0) ? 0 : CD.Entry[CD.ne-1].page;

   // copy the filename to the open handle
   for (i = 0; i < 4; i++)
      Han[hn].Name[i] = flname->Name[i];
   Han[hn].Ext = flname->Ext;

   // clear other parts of structure (3.10)
   Han[hn].Spage = 0;
   Han[hn].NumPgs = 0;
   Han[hn].Attrib = 0;

   // search through directory to find file to open
   owClearError();
   if(!FindDirectoryInfo(portnum,SNum,&pg,&Han[hn]))
   {
   	if(owGetErrorNum() == OWERROR_FILE_NOT_FOUND)
   	{
   		Han[hn].Name[0] = 0;
   		return FALSE;
   	}
   }
   else
   {
   	Han[hn].Name[0] = 0;
   	OWERROR(OWERROR_REPEAT_FILE);
   	return FALSE;
   }

   // clear the read flag
   Han[hn].Read = 0;

   // read the bitmap to calculate the maxwrite
   if(!ReadBitMap(portnum,SNum,BM))
   {
   	Han[hn].Name[0] = 0;
   	return FALSE;
   }

	maxP = maxPages(portnum,SNum);
   // if monitary file
   if((flname->Ext == 101) || (flname->Ext == 102))
   {
      // fill all pages except monitary pages
      if(SNum[0] == 0x18)
      {
         flag = 7;
      }
      else
      {
         flag = (maxP+1) / 4;
         if (flag < 3)
            flag = 3;
      }
      for (i = 0; i <= (maxP - flag); i++)
      {
      	if(!BitMapChange(portnum,SNum,i,1,BM))
         {
            Han[hn].Name[0] = 0;
      		return FALSE;
         }
      }

      // also set any pages used in open (101) files
      for (i = 0; i < 4; i++)
         if ((Han[i].Name[0] != 0) &&
            ((Han[i].Ext == 101) || (Han[i].Ext == 102)) && (hn != i))
         {
         	if(!BitMapChange(portnum,SNum,Han[i].Spage,1,BM))
            {
               Han[hn].Name[0] = 0;
         		return FALSE;
            }
         }

      // check if any space
      flag = GetMaxWrite(portnum,SNum,BM);
      if (flag < 28)
      {
         Han[hn].Name[0] = 0;            // release the handle
         OWERROR(OWERROR_OUT_OF_SPACE);  // return error code
         return FALSE;
      }

      // get a page
      FindEmpties(portnum,SNum,BM,&avail,&next);
      flname->Spage = avail;     // for user to use
      Han[hn].Spage = avail; // for driver
      // read the page to get the counter for user
      if(!ExtendedRead_Page(portnum,SNum,&pgbuf[0],avail))
      {
      	Han[hn].Name[0] = 0;
      	return FALSE;
      }

      // copy the counter/tamper into the bitmap for user
      if(SNum[0] == 0x18)
      {
         for (i = 0; i < 8; i++)
            flname->BM[i] = pgbuf[i+32];
      }
      else if((SNum[0] == 0x33) || (SNum[0] == 0xB3))
      {
         for (i = 0; i < 8; i++)
            flname->BM[i] = 0x00;
      }

      *maxwrite = 28;
   }
   else
   {
      // count empty pages in bitmap
      flag = GetMaxWrite(portnum,SNum,BM);
      if (flag > 28)
         flag -=28;
      *maxwrite = flag;
   }

   pgbuf[0] = 0xAA;
   pgbuf[1] = 0;
   if(!owWriteFile(portnum,SNum,hn,&pgbuf[0],0))
   {
      Han[hn].Name[0] = 0;
      return FALSE;
   }

   // return the handle number
   *hnd = hn;

   return TRUE;
}


//--------------------------------------------------------------------------
//  Close a file by freeing up its handle.
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// hnd        the handle of the file to close
//
//  return:   TRUE  : file closed
//            FALSE : error
//
SMALLINT owCloseFile(int portnum, uchar *SNum, short hnd)
{
   // check that the handle is in the correct range
   if ((hnd < 4) && (hnd >= 0) && (Han[hnd].Name[0] != 0))
   	Han[hnd].Name[0] = 0;   //zero first byte of handle area
   else
   {
   	OWERROR(OWERROR_HANDLE_NOT_EXIST);
   	return FALSE;  // error for handle does not exist
   }

   return TRUE;
}


//--------------------------------------------------------------------------
// Search throuch the directory of the device for the entry 'flname' and
// return the page number to the caller. Also copy shorto standard 'FileInfo'
// format at DirEntry.
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// pg         the next or current page
// FInfo      the file information that was gotten
//
// returns:
//          TRUE  : directory page number was found
//          FALSE : error
//
SMALLINT FindDirectoryInfo(int portnum, uchar *SNum, PAGE_TYPE *pg,
                           FileInfo *FInfo)
{
   short     i,j;
   uchar     pgbuf[30];
   PAGE_TYPE tpg,ppage,spg;
   int       flag = 0;

   // start previouse page as current page
   ppage = *pg;
   spg   = *pg;

   // loop to search through directory pages
   do
   {
      // read a page in the directory
      tpg = *pg;

      if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&tpg,&flag))
      	return FALSE;

      // if have a good read then search inside page for EntNum
      if (flag > 0)
      {
         // special case for first directory page
         i = 0;
         if (*pg == spg)
         {
            i = 7;
            // check to see if first dir page is a valid dir
            if (pgbuf[0] != 0xAA || pgbuf[1] != 0)
            {
            	OWERROR(OWERROR_FILE_READ_ERR);
            	return FALSE;
            }
         }

         // loop through each directory entry to find FInfo
         for (;i < flag; i += 7)
         {
            for (j = 0; j < 4; j++)   // compare file name
               if ((toupper(pgbuf[i+j]) != FInfo->Name[j]) &&
                   !((pgbuf[i+j] == 0x20) && (FInfo->Name[j] == 0xCC)))
                  break;


            if ((j != 4) || ((pgbuf[i+4] & 0x7F) != FInfo->Ext))
               continue;  // loop if not match

            // must be a match at this poshort
            // get the extension
            FInfo->Ext = pgbuf[i+4] & 0x7F;
            // get the start page
            FInfo->Spage = pgbuf[i+5];
            // number of pages
            FInfo->NumPgs = pgbuf[i+6];
            // get the readonly/hidden flag
            FInfo->Attrib = (pgbuf[i+4] & 0x80) ? 1 : 0;
            // shift attribute if his hidded flag
            if (FInfo->Ext == 0x7F)
               FInfo->Attrib <<= 1;
            // remember the page entry found
            FInfo->Dpage = *pg;
            // remember the offset of entry in directory
            FInfo->Offset = (uchar)i;
            // previouse directory page
            FInfo->PDpage = ppage;
            // return the current page number
            return TRUE;
         }

         // poshort to next page
         ppage = *pg;
         *pg = pgbuf[flag-1];
      }
      else
         return FALSE;  // return error
   }
   while(*pg);

   // return failure to find directory entry
   return FALSE;
}


//--------------------------------------------------------------------------
// Read the directory to find the EntNum entry and load its value shorto
// the FileEntry. If the entry number 'EntNum' is -1 then instead of looking
// for an entry number, this function is looking for empty directory place.
// If an empty place is not found then the last page of the directory is
// returned.
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// EntNum     the entry number
// FEntry     the file entry number information
// pg         the page for the data
//
//  Returns:
//          TRUE  : entry found
//          FALSE : error
//
SMALLINT ReadNumEntry(int portnum, uchar *SNum, short EntNum,
                       FileEntry *FEntry, PAGE_TYPE *pg)
{
   uchar pgbuf[30];
   int   firstpg;
   short EntCnt=0,i,j,tw;
   int   len = 0;

   // set the entry to null
   FEntry->Name[0] = 0;

   // check to see if CD is still valid and get the starting page of working dir
   *pg = (CD.ne == 0) ? 0 : CD.Entry[CD.ne-1].page;
   firstpg = TRUE;

   // loop to search through directory pages
   do
   {
      // read a page in the directory
      if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,pg,&len))
      	return FALSE;

      // special case for first directory page
      tw = 0;
      if (firstpg)
      {
         firstpg = FALSE;
         tw = 1;
         // check to see if first dir page is a valid dir
         if (pgbuf[0] != 0xAA || pgbuf[1] != 0 || len < 7)
         {
         	OWERROR(OWERROR_FILE_READ_ERR);
         	return FALSE;
         }
      }

      // check to see if searching for an empty spot
      if (EntNum == -1)
      {
         // if the length is less then 4*7+1 or last page
         if ((len < 29) || (pgbuf[len-1] == 0))
            return TRUE;    // return the page number spot found on
      }
      // check to see if EntNum is in this page
      else if ((EntCnt + (len / 7) - tw) >= EntNum)
      {
         // then get it
         j = (EntNum - EntCnt + tw - 1) * 7;
         // copy the name,extension,startpage,numpages,attributes
         for (i = 0; i < 4; i++)
            FEntry->Name[i] = pgbuf[j+i];
         FEntry->Ext = pgbuf[j+4] & 0x7F;
         FEntry->Spage = pgbuf[j+5];
         FEntry->NumPgs = pgbuf[j+6];
         FEntry->Attrib = (pgbuf[j+4] & 0x80) ? 1 : 0;
         if (FEntry->Ext == 0x7F)
            FEntry->Attrib <<= 1;
         break;
      }

      // get the pointer to the next page
      *pg = pgbuf[len-1];

      // for the end of the list of dir and files
      if(*pg == 0)
      {
         OWERROR(OWERROR_FILE_NOT_FOUND);
         return FALSE;
      }

      // increment the EntCnt
      EntCnt += ((len / 7) - tw);
   }
   while (*pg && (len > 0));

   // attempt to read the bitmap
   if(!ReadBitMap(portnum,SNum,&FEntry->BM[0]))
   	return FALSE;

   // check status
   if (!FEntry->Name[0])
   {
      if (!*pg)
      {
      	OWERROR(OWERROR_FILE_NOT_FOUND);
      	return FALSE;
      }
      else
      {
      	OWERROR(OWERROR_FILE_READ_ERR);
      	return FALSE;
      }
   }

   return TRUE;
}


//--------------------------------------------------------------------------
// Write the bitmap for the current device from the Bmap buffer.
// The bitmap could be local or remote.  The first page of the device
// is read to determine the Bitmap type and location.
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// Bmap       the bitmap to be written
//
// Return:  TRUE  : bitmap written successfully
//          FALSE : error
//
//
SMALLINT WriteBitMap(int portnum, uchar *SNum, uchar *Bmap)
{
   short i,j,b,flag;
   int len,maxP,mBcnt;
   uchar pgbuf[32];
   PAGE_TYPE pg = 0,tpg;
	SMALLINT  bank = 1;

   // depending on type, bitmap may be in different places
   if(!owIsWriteOnce(bank,portnum,SNum))
   {
      // read the first page to get local bitmap or address of bitmap file
		if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&pg,&len))
			return FALSE;

      // make sure is a directory
      if (pgbuf[0] != 0xAA || pgbuf[1] != 0x00)
      {
      	OWERROR(OWERROR_FILE_READ_ERR);
      	return FALSE;
      }

		maxP = maxPages(portnum,SNum);
      // get the number of bytes in bitmap
      b = maxP / 8;
      b += ((maxP % 8) ? 1 : 0);

      // check for local bitmap
      if (pgbuf[2] & 0x80)
      {
         // check for impossible number of pages local
         if (maxP > 32)
         {
         	OWERROR(OWERROR_FILE_READ_ERR);
         	return FALSE;
         }

         // first zero out the existing bitmap
         for (i = 3; i <= 6; i++)
            pgbuf[i] = 0;

         // copy the new bitmap over
         for (i = 0; i < b; i++)
            pgbuf[i+3] = Bmap[i];
      }
      // remote bitmap
      else // (2.00Bug) else was missing in 2.00
      {
         tpg = pgbuf[5];  // remote bitmap location

         maxP = maxPages(portnum,SNum);

         // check for 2 page bitmap
         if(maxP > 224)
         {
            // 2 pages so read the first page to get continuation
				if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&tpg,&len))
					return FALSE;

            // sanity check
            if (len != 29)
            {
            	OWERROR(OWERROR_FILE_READ_ERR);
            	return FALSE;
            }

            // get the continuation page
            pg = pgbuf[len-1];

            // check and copy over
            flag = 0;
            for (i = 0; i < (len-1); i++)
            {
               if (pgbuf[i] != Bmap[i])
               {
                  flag = 1;
                  pgbuf[i] = Bmap[i];
               }
            }

            // write back the first bitmap page if it has changed
            if (flag)
            {
               // write the first page of the bitmap
					if(!Write_Page(portnum,SNum,&pgbuf[0],tpg,len))
						return FALSE;
            }

            // create the second page
            for (i = 0; i < (b-28); i++)
               pgbuf[i] = Bmap[i+28];
            pgbuf[i] = 0;  // end continuation page pointer

            // set the new length
            len = b-27;
         }
         else  // (2.00Bug) this else and code for 1 page bitmap files was missing
         {
            pg = pgbuf[5]; // remote bitmap location

            // copy the new bitmap over
            for (i = 0; i < b; i++)
               pgbuf[i] = Bmap[i];

            len = b + 1; // get the correct length

            pgbuf[b] = 0; // end continuation page pointer
         }
      }

      // loop to write a page of the bitmap
		if(!Write_Page(portnum,SNum,&pgbuf[0],pg,len))
			return FALSE;
   }
   // Else this is a an program job
   else
   {
      // is there a job that has been opened then read from buffer and not the part
      if(isJob(portnum,SNum))
      {
        	maxP = maxPages(portnum,SNum);;

         // bitmap is in the status memory
         // get number of status bytes needed
         mBcnt = (maxP / 8) +
                 (((maxP % 8) > 0) ? 1 : 0);

         // loop to copy from the programjob buffer
         for(i=0;i<mBcnt;i++)
         {
            // set the program job
            if(!setProgramJob(portnum,SNum,Bmap[i],i))
               return FALSE;

            // check to see if this nullifies any previous page writes
            for(j=0;j<8;j++)
            {
               // if the bitmap is empty then don't write anything there
               if(!((Bmap[i] >> j) & 0x01))
                  if(!setJobWritten(portnum,SNum,(PAGE_TYPE)(i*8+j),PJOB_NONE))
                     return FALSE;
            }
         }

         return TRUE;
      }
      else
      {
      	// error condition, should never happen here
      	OWERROR(OWERROR_NO_PROGRAM_JOB);
      	return FALSE;
      }
   }

   return TRUE;
}


//--------------------------------------------------------------------------
// Read the bitmap from the current device and place it in the Bmap buffer.
// The bitmap could be local or remote.  The first page of the device
// is read to determine the Bitmap type and location.
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// Bmap       the returned bit map
//
// Return:  TRUE  : bitmap read
//          FALSE : error
//
//
SMALLINT ReadBitMap(int portnum, uchar *SNum, uchar *Bmap)
{
   short i,j,l,flag,mBcnt;
   uchar pgbuf[32],tpg=0,pg;
   SMALLINT bank = 1;
   int      page = 0;
   int      len  = 0;
   int      maxP = 0;

   // depending on type, bitmap may be in different places
   if (!owIsWriteOnce(bank,page,SNum))
   {
      // read the first page to get local bitmap or address of bitmap file
      if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&tpg,&len))
      	return FALSE;

      // make sure is a directory
      if (pgbuf[0] != 0xAA || pgbuf[1] != 0x00)
      {
      	OWERROR(OWERROR_FILE_READ_ERR);
      	return FALSE;
		}

      // check for local bitmap
      if (pgbuf[2] & 0x80)
      {
         // its local, but does this make sense for this size of device
         maxP = maxPages(portnum,SNum);

         if(maxP > 32)
         {
         	OWERROR(OWERROR_FILE_READ_ERR);
         	return FALSE;
         }

       	// clear out bitmap
       	for(i = 0; i <= len-1; i++)
            Bmap[i] = 0;
         // copy over the local bitmap
         for(i = 0; i <= maxP/8; i++)
            Bmap[i] = pgbuf[i+3];

         // return true
         return TRUE;
      }
      // must be remote
      else
      {
         pg = pgbuf[5];  // remote bitmap location

         // read the first page of the bitmap
         if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&pg,&len))
         	return FALSE;

			maxP = maxPages(portnum,SNum);

         // check size and continuation pointers to make sure they are valid
         if (((maxP > 224) && ((len != 29) ||
             (pgbuf[len-1] == 0))) ||
             ((maxP <= 224) &&
              (((maxP+1)/8 != (len - 1)) ||
               (pgbuf[len-1] != 0))  ))
         {
         	OWERROR(OWERROR_FILE_READ_ERR);
         	return FALSE;
         }

         // copy bitmap over to the buffer
         for (i = 0; i < (len-1); i++)
            Bmap[i] = pgbuf[i];

         // check to see if need a second bitmap page
         if (pgbuf[len-1])
         {
            // get the page number from the continuation pointer
            pg = pgbuf[len-1];

            // read the second page of the bitmap
            if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&pg,&len))
            	return FALSE;

            // check if size of rest of bitmap makes sense
            if ((27 + len) != ((maxP+1)/8))
            {
            	OWERROR(OWERROR_FILE_READ_ERR);
            	return FALSE;
            }

            // copy rest of bitmap to buffer
            for (i = 0; i < (len-1); i++)
               Bmap[28+i] = pgbuf[i];

            return TRUE;
         }

         return TRUE;
      }
   }
   // DS1982/DS2405A type program
   else if(owIsWriteOnce(bank,page,SNum))
   {
   	if((SNum[0] == 0x0B) || (SNum[0] == 0x0F))
   	{
   		maxP = maxPages(portnum,SNum);

   		mBcnt = ((maxP+1) / 8) + ((((maxP+1) % 8) > 0) ? 1 : 0);
   		l     = (mBcnt / 8) + (((mBcnt % 8) > 0) ? 1 : 0);

         // check for the an Program Job
         flag = FALSE;
         if(isJob(portnum,SNum))
         {
            // loop to copy from the programjob buffer
            if(!getProgramJob(portnum,SNum,&Bmap[0],mBcnt))
               return FALSE;
            flag = TRUE;
         }

      	// read from the part
      	if (!flag)
      	{
         	// loop through the number of bitmap pages
         	for (i = 0; i < l; i++)
         	{
            	// read stat mem page
               tpg = i + 8;

					if(!Read_Page(portnum,SNum,&pgbuf[0],STATUSMEM,&tpg,&len))
						return FALSE;

            	// check results
            	if (len == 8)
            	{
               	// copy bitmap read shorto buffer
               	for (j = 0; j < 8; j++)
                  	if (j <= mBcnt)
                     	Bmap[j + i * 8] = ~pgbuf[j];
                  	else
                     	break;   // quit the loop because done
            	}
            	else
               	return FALSE;
         	}
      	}

      	return TRUE;
      }
      else
      {
         // is there a job that has been opened then read from buffer and not the part
         if(isJob(portnum,SNum))
         {
            // to copy from the programjob buffer
            if(!getProgramJob(portnum,SNum,&Bmap[0],1))
               return FALSE;

				return TRUE;
         }

         tpg = 0;
      	// bitmap is in the status memory page 0, byte
      	if(!Read_Page(portnum,SNum,&pgbuf[0],STATUSMEM,&tpg,&len))
      		return FALSE;

      	// check result of status read
      	if (len == 8)
      	{
         	Bmap[0] = (~(pgbuf[0] >> 4)) & 0x0F;
         	return TRUE;
      	}
      }
   }
   else
		OWERROR(OWERROR_WRONG_TYPE);

	return FALSE;
}


//--------------------------------------------------------------------------
//  Calculate and return the numbers of bytes available to write in the
//  current device.  Note that the value is adjusted for the contingency
//  that the addition of a new file in the directory may make the directory
//  a page longer.
//
// portnum  the port number of the port being used for the
//          1-Wire Network.
// SNum     the serial number for the part that the read is
//          to be done on.
// BM       the bitmap for the part
//
// return the number of bytes available to the File I/O
//
SMALLINT GetMaxWrite(int portnum, uchar *SNum, uchar *BM)
{
   uchar t=0;
   short bcnt=0,cnt=0,i;
   int maxP = 0;
   SMALLINT ret = 0;

 	maxP = maxPages(portnum,SNum);

   // loop through each page bit
   for (i = 0; i <= maxP; i++)
   {
      // check if it is time to get the next bm byte
      if ((i % 8) == 0)
         t = BM[bcnt++];

      // check for empty pages
      if (!(t & 0x01))
         cnt++;

      // look at next page
      t >>= 1;
   }

   if(((cnt+1)*28) > (maxP*28))
      ret = cnt*28;
   else
      ret = (cnt+1)*28;

   // don't take shorto acount the effect of a new directory page
   return ret;
}




//--------------------------------------------------------------------------
// Check to see if the serial number being used is the current one.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
//
// returns    'true' if it is the current serial number
//            otherwise 'false'
//
SMALLINT ValidRom(int portnum,uchar *SNum)
{
	int   i;
	uchar check[8];

   if(current[0] == 0x00)
   {
      for(i=0;i<8;i++)
         current[i] = SNum[i];

      owSerialNum(portnum,&SNum[0],FALSE);
   }
   else
   {
	   owSerialNum(portnum,&check[0],TRUE);

	   for(i=0;i<8;i++)
		   if(check[i] != SNum[i])
			   return FALSE;
   }

	return TRUE;
}


//--------------------------------------------------------------------------
// Do bit manipulation to the provided bitmap.  Change the 'BitNum' bit to
// the 'State'.  This function needs Handle and D to check the max page
// number and see if clearing a bit is possible (Eprom).
//
// Find first file and copy it into string DirEnt.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// BitNum     the bit number to be changed
// State      the state of the bit to be changed to
// BM         the pointer to the bit map that is changed
//
// This function returns:
//             TRUE   : operation is ok for this device
//             FALSE  : could not do operation because incompatible with
//                      device type
//
SMALLINT BitMapChange(int portnum, uchar *SNum, uchar BitNum,
                      uchar State, uchar *BM)
{
   PAGE_TYPE  maxP;
   SMALLINT   bank = 1;
   int        page = 0;

   maxP = maxPages(portnum,SNum);

   // check for a not possible request
   if(BitNum > maxP)
   {
   	OWERROR(OWERROR_TOO_LARGE_BITNUM);
      return FALSE;
   }

   // check for the clearing of an eprom device (not possible)
   if(owIsWriteOnce(bank,page,SNum))
   {
      if (State == 0)
      {
         // check if only in buffer
         // or non written buffered eprom page
         if(isJob(portnum,SNum))
         {
            // see if page set to write but has not started yet
            if ((isJobWritten(portnum,SNum,BitNum) == PJOB_WRITE) &&
                ((isJobWritten(portnum,SNum,BitNum) & PJOB_START) != PJOB_START) )
            {
               if(!setJobWritten(portnum,SNum,BitNum,PJOB_NONE))
                  return FALSE;
               BM[BitNum / 8] = (BM[BitNum / 8] | (0x01 << (BitNum % 8))) ^
                                        (0x01 << (BitNum % 8));
            }
         }
         else
            return TRUE;
      }
      else
         BM[BitNum / 8] |=  (0x01 << (BitNum % 8));
   }
   // NVRAM
   else
   {
      // depending on the state do the opertion
      if (State)
         BM[BitNum / 8] |=  (0x01 << (BitNum % 8));
      else
         BM[BitNum / 8] = (BM[BitNum / 8] | (0x01 << (BitNum % 8))) ^
                                  (0x01 << (BitNum % 8));
   }

   return TRUE;
}

//--------------------------------------------------------------------------
// Function to decide if the filename passed to it is valid.
//
// flname  the filename
//
// return TRUE if the filename is legal
//
SMALLINT Valid_FileName(FileEntry *flname)
{
    uchar i;

    // loop to check if filename is valid and convert to upper case
    for (i = 0; i < 4; i++)
    if (!isalnum(flname->Name[i]) && !strrchr(" !#$%&'-@^_`{}~",flname->Name[i]))
       return FALSE;
    else
       flname->Name[i] = toupper(flname->Name[i]); // convert to upper case

     return TRUE;
}

//--------------------------------------------------------------------------
// Function to search through the provided bitmap 'BM' and return the
// first 'avail'able page and the 'next' available page.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// BM         the bit map
// avail      the available page for File I/O
// next       the next page that is available for File I/O
//
void FindEmpties(int portnum, uchar *SNum, uchar *BM, PAGE_TYPE *avail,
                 PAGE_TYPE *next)
{
   uchar First=1,t=0,i,bcnt=0;
   int tmp_pgs = maxPages(portnum,SNum);

   // set available and next available to 0
   *avail = 0;
   *next  = 0;

   // loop through all of the pages in the bitmap
   for (i = 0; i <= tmp_pgs; i++)
   {
       // check if it is time to get the next bm byte
      if ((i % 8) == 0)
         t = BM[bcnt++];

      // check for empty pages
      if (!(t & 0x01))
      {
         if (First)
         {
             *avail = i;
             First = 0;
         }
         else
         {
             *next = i;
             return;
         }
      }

      // look at next page
      t >>= 1;
   }
}


//--------------------------------------------------------------------------
// This function returns the maximum number of pages that can be used for
// the file I/O operations.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
//
// This function returns:  The number of pages for File I/O
//
//
int maxPages(int portnum, uchar *SNum)
{
   SMALLINT  bank = 1;
   SMALLINT  i;
   int       tmp_pgs = 0;

   if(owIsWriteOnce(bank,portnum,SNum))
   {
      for(i=0;i<owGetNumberBanks(SNum[0]);i++)
      {
         if(owIsNonVolatile(i,SNum) && owIsGeneralPurposeMemory(i,SNum))
         {
            tmp_pgs = tmp_pgs + owGetNumberPages(i,SNum);
         }
      }
   }
   else
   {
      for(i=0;i<owGetNumberBanks(SNum[0]);i++)
      {
         if(owIsNonVolatile(i,SNum) && owIsGeneralPurposeMemory(i,SNum) &&
            owIsReadWrite(i,portnum,SNum))
         {
            tmp_pgs = tmp_pgs + owGetNumberPages(i,SNum);
         }
      }
   }

   return (tmp_pgs-1);
}

//--------------------------------------------------------------------------
// Change the current rom
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
//
void ChangeRom(int portnum, uchar *SNum)
{
	int   i;

   for(i=0;i<8;i++)
      current[i] = SNum[i];

   owSerialNum(portnum,&SNum[0],FALSE);
}

//--------------------------------------------------------------------------
// gets the last page of the data
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
//
PAGE_TYPE getLastPage(int portnum, uchar *SNum)
{
   return LastPage;
}

//  hnd could be just create with TMCreateFile or it could be an existing
//  file that was opened with TMOpenAddFile
//  length are provided.
//
//  portnum   the port number of the port being used for the
//            1-Wire Network.
//  SNum       the serial number for the part that the read is
//            to be done on.
//  hnd       the handle for the file
//  operation the operation to be performed on the file
//  offset    the offset for the buffer of data
//  buf       the data to be added.
//  len       the length of the data to be added
//
//  Returns:   TRUE  File Write successful return number of bytes written
//             FALSE File Write a failure, error
//
SMALLINT owWriteAddFile(int portnum, uchar *SNum, short hnd, short operation,
                        short offset, uchar *buf, int *len)
{
   uchar BM[32],pgbuf[34];
   int i,cnt=0,mavail,dlen,offcnt;
   short spot,op;
   PAGE_TYPE next,avail,tpg,pg,dpg;
   FileEntry Dmmy;
   uchar *ptr;

   // check device type
   if(!ValidRom(portnum,SNum))
      return FALSE;

   // check to see if handle valid number
   if (hnd > 3 || hnd < 0)
   {
      OWERROR(OWERROR_HANDLE_NOT_EXIST);
      return FALSE;
   }

   // check to see that hnd is valid
   if (!Han[hnd].Name[0])
   {
   	OWERROR(OWERROR_HANDLE_NOT_USED);
   	return FALSE;
   }

   // check to see if extension is 100
   if(Han[hnd].Ext != 100)
   {
      OWERROR(OWERROR_FUNC_NOT_SUP);
      return FALSE;
   }

   // check to see if the file is aleady terminated
   if(Han[hnd].NumPgs != 0)
   {
      OWERROR(OWERROR_ADDFILE_TERMINATED);
      return FALSE;
   }

   // get the bitmap to find empties if needed
   if(!ReadBitMap(portnum,SNum,&BM[0]))
      return FALSE;

   // get current empty space on device
   mavail = GetMaxWrite(portnum,SNum,&BM[0]);

   // check to see if this is a new file or an existing file
   // file already exist
   if(Han[hnd].Read == 1)
   {
      // start at starting page of the opened file
      pg = Han[hnd].Spage;
      offcnt = -1;

      // loop to search through file pages
      for(;;)
      {
         // read a page in the file
         if(!owReadPageExtraCRC(0,portnum,SNum,pg,&pgbuf[0],&pgbuf[32]))
            return FALSE;

         // check for an already terminated file
         if (pgbuf[0] != 0xFF)
         {
            OWERROR(OWERROR_ADDFILE_TERMINATED);
            return FALSE;
         }

         // append
         if (operation == APPEND)
         {
            // check to see this is the last page
            if (pgbuf[29] == 0xFF)
            {
               for (spot = 28; spot >= 0; spot--)
                  if (pgbuf[spot] != 0xFF)
                     break;
               if (spot <= 0)
                  spot = 1;
               else
                  spot++;

               // 'spot' is the spot to append to on page 'pg'
               break;
            }
         }
         // offset
         else
         {
            // loop to look for the spot
            for (spot = 1; spot <= 28; spot++)
            {
               if (++offcnt == offset)
                  break;
            }

            // check to see if got to the offset point or out of pages
            if (offcnt == offset || pgbuf[29] == 0xFF)
               break;
         }

         // then try the next page in the chain
         pg = pgbuf[29];
      }
   }
   // file is new so have to add to directory
   else
   {
      // try to find an empty spot in the directory
      if(!ReadNumEntry(portnum,SNum,-1,&Dmmy,&dpg))
         return FALSE;

      // read the directory page indicated to see if it has space or is the last
      // loop while error during read up to 3 count
      cnt = 0;
      tpg = (uchar)dpg;

      if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&tpg,&dlen))
         return FALSE;

      // in light of the directory page read check space again
      if (dlen == 29)
         mavail -= 28;
      if (mavail < 0 || *len > mavail)
      {
         OWERROR(OWERROR_OUT_OF_SPACE);
         return FALSE;
      }

      // Record where the file begins
      FindEmpties(portnum,SNum,&BM[0],&avail,&next);
      Han[hnd].Spage = avail;

      // set the bitmap to claim this page
      if(!BitMapChange(portnum,SNum,avail,1,&BM[0]))
         return FALSE;

      // construct the new directory entry
      Han[hnd].NumPgs = 0;
      Han[hnd].Attrib = 0;

      // check to see if need to write a new directory page
      if (dlen == 29)
      {
         // get an empty page for the new directory page
         FindEmpties(portnum,SNum,&BM[0],&avail,&next);

         // write the new page
         if(!Write_Page(portnum,SNum,(uchar *) &Han[hnd],avail,8))
            return FALSE;

         // set the bitmap to account for this new directory page
         if(!BitMapChange(portnum,SNum,avail,1,&BM[0]))
            return FALSE;

         // change the exhisting directory page continuation pointer
         pgbuf[dlen-1] = avail;
      }
      // add the new directory entry to exhisting directory page
      else
      {
         // remember the continuation pointer
         pg = pgbuf[dlen-1];

         // get a pointer into the handle
         ptr = (uchar *) &Han[hnd];

         // loop to add the new entry
         for (i = 0; i < 7; i++)
            pgbuf[dlen-1+i] = ptr[i];

         // adjust the length of the directory page
         dlen += 7;

         // correct the continuation pointer
         pgbuf[dlen-1] = pg;
      }

      // special case of dir page == 0 and local bitmap
      if ((dpg == 0) && (pgbuf[2] & 0x80))
      {
         // update the local bitmap
         for (i = 0; i < 2; i++)
            pgbuf[3+i] = BM[i];
         pgbuf[5] = 0;
         pgbuf[6] = 0;
      }
      // non page 0 or non local bitmap
      else
      {
         // write the bitmap
         if(!WriteBitMap(portnum,SNum,&BM[0]))
            return FALSE;
      }

      // now rewrite the directory page
      if(!Write_Page(portnum,SNum,&pgbuf[0],dpg,dlen))
         return FALSE;

      pg = Han[hnd].Spage;
      spot = 1;
      offcnt = 0;
   }

   // check to see if need to add some blank pages (except for pointer)
   if (operation == 0 && offcnt != offset)
   {
      // start on 'pg' and add pages until get to offset
      for (;;)
      {
         // loop to look for the spot
         for (; spot <= 28; spot++)
         {
            if (offcnt == offset)
               break;

            offcnt++;
         }

         // check to see if got to the offset point or out of pages
         if (offcnt == offset)
            break;

         // else get another page
         FindEmpties(portnum,SNum,&BM[0],&avail,&next);

         // decrement the number of pages available
         mavail -= 28;
         if (mavail < 0 || avail == 0xFF)
         {
            OWERROR(OWERROR_OUT_OF_SPACE);
            return FALSE;
         }

         // write the pointer byte
         if(!WriteJobByte(portnum,SNum,pg,29,avail,0))
            return FALSE;

         // set bitmap to claim that new page
         if(!BitMapChange(portnum,SNum,avail,1,&BM[0]))
            return FALSE;

         pg = avail;

         spot = 1;
         offcnt++;
      }
   }

   // Now should be able to write data at 'spot' on page 'pg'
   if((*len != 0) && (Han[hnd].Ext == 100))
   {
      cnt = 0;
      for (;;)
      {
         // get the 'zeros' flag
         op = (operation == APPEND) ? 0 : 1;

         // set bitmap to claim the page
         if(!BitMapChange(portnum,SNum,pg,1,&BM[0]))
            return FALSE;

         // loop to write the rest of the page or until done
         for (; spot <= 28; spot++)
         {
            // write a byte to the program job list
            if(!WriteJobByte(portnum,SNum,pg,spot,buf[cnt++],op))
               return FALSE;

            // check to see if done
            if (cnt >= *len)
            {
               // first write the bitmap before ending with success
               if(!WriteBitMap(portnum,SNum,&BM[0]))
                  return FALSE;

               // reset read flag so could read file now
               Han[hnd].Read = 1;

               return TRUE;
            }
         }

         // data more than one page.
         spot = 1;
         FindEmpties(portnum,SNum,&BM[0],&avail,&next);
         if(!WriteJobByte(portnum,SNum,pg,29,avail,0))
            return FALSE;
         pg = avail;
      }
   }
   else
   {
      if(!BitMapChange(portnum,SNum,pg,1,&BM[0]))
         return FALSE;
      else
         if(!setJobWritten(portnum,SNum,pg,PJOB_ADD))
            return FALSE;

      return TRUE;
   }

   // set the redirection byte
   // else get another page
   FindEmpties(portnum,SNum,&BM[0],&avail,&next);

   // decrement the number of pages available
   mavail -= 28;
   if (mavail < 0 || avail == 0xFF)
   {
      OWERROR(OWERROR_OUT_OF_SPACE);
      return FALSE;
   }

   // write the pointer byte
   if(!WriteJobByte(portnum,SNum,pg,29,avail,0))
      return FALSE;

   pg = avail;
   spot = 1;

   return TRUE;
}


//--------------------------------------------------------------------------
//  Terminate an 'add' file by adding the length and CRC16 bytes to each
//  page.
//
//  portnum   the port number of the port being used for the
//            1-Wire Network.
//  SNum      the serial number for the part that the read is
//            to be done on.
//  flname    the file to be written to the part
//
//  return:  TRUE : 'add' file terminated
//           FALSE: error
//
SMALLINT owTerminateAddFile(int portnum, uchar *SNum, FileEntry *flname)
{
   short i,j;
   uchar pgbuf[34],buf[32],pgcnt;
   PAGE_TYPE pg=0;
   int flag;
   FileInfo Info;

   // check device type
   if(!ValidRom(portnum,SNum))
      return FALSE;

   // check to see if filename passed is valid
   if ((Valid_FileName(flname) == 0) || ((flname->Ext & 0x7F) != 100))
   {
      OWERROR(OWERROR_XBAD_FILENAME);
      return FALSE;
   }

   // check existing open files with same the name (3.10)
   // automatically close the file
   for (i = 0; i < 4; i++)
   {
       if (Han[i].Name[0] != 0)
       {
            for (j = 0; j < 4; j++)
                if ((Han[i].Name[j] & 0x7F) != flname->Name[j])
                   break;

            if (j == 4 && ((Han[i].Ext & 0x7F) == flname->Ext))
            {
               Han[i].Name[0] = 0;  // release the handle
               break;
            }
       }
   }

   // depending on the directory set the first page to look for entry
   if(CD.ne == 0)
   	pg = 0;
   else
   	pg = CD.Entry[CD.ne-1].page;

   // copy filename over to info packet
   for (i = 0; i < 4; i++)
       Info.Name[i] = flname->Name[i];
   Info.Ext = flname->Ext;

   // check directory list to find the location of the file name
   if(!FindDirectoryInfo(portnum,SNum,&pg,&Info))
      return FALSE;

   // check to see if the file is aleady terminated
   if (Info.NumPgs != 0)
   {
      OWERROR(OWERROR_ADDFILE_TERMINATED);
      return FALSE;
   }

   pg = Info.Spage;
   pgcnt = 0;
   for (;;)
   {
      // loop while error during read up to 3 count
      if(!owReadPageExtraCRC(0,portnum,SNum,pg,&pgbuf[0],&pgbuf[32]))
         return FALSE;

      // read success, check to see if already terminated
      if (pgbuf[0] != 0xFF || pgbuf[30] != 0xFF ||
          pgbuf[31] != 0xFF || pgbuf[32] != 0xFF)
      {
         OWERROR(OWERROR_ADDFILE_TERMINATED);
         return FALSE;
      }

      // count the page
      pgcnt++;

      // terminte the page in the program job
      if(!TerminatePage(portnum,SNum,pg,&pgbuf[0]))
         return FALSE;

      // check for end of file
      if (pgbuf[29] == 0xFF)
      {
         // write the last page pointer to 0
         if(!WriteJobByte(portnum,SNum,pg,29,0x00,0))
            return FALSE;
         break;
      }

      // get the next page in the list
      pg = pgbuf[29];
   }

   // read the directory page that the entry to change length
   if(!Read_Page(portnum,SNum,&buf[0],REGMEM,&Info.Dpage,&flag))
      return FALSE;

   // set the number of pages in the buffer for the file
   buf[Info.Offset+6] = pgcnt;

   // write back the directory page with the new name
   if(!Write_Page(portnum,SNum,&buf[0],Info.Dpage,flag))
      return FALSE;

   return TRUE;
}