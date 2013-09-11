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
//  owPgRW.c - Reads and writes pages for the File I/O operations.
//  version 1.00
//

// Include Files
#include "owfile.h"
#include "rawmem.h"

//--------------------------------------------------------------------------
// Read_Page:
//
// Read a default data structure page from the current Touch Memory.
// The record will be placed shorto a (uchar) memory location
// starting at the location poshorted to by (buf).  This function will 
// read a page out of normal memory (flag = REGMEM) or a status memory
// page out of an eprom (flag = STATUSMEM).  If the page being read is
// redirected as in an eprom device then the page value passes by referece
// is changed to match the new page.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// flag       tells weather to read regular memory or status memory
// buff       location for data read
// page       page number to read packet from
// len        the length of the data read
//
// return TRUE if the data was read correctly
//  
SMALLINT Read_Page(int portnum, uchar *SNum, uchar *buff, uchar flag, 
                   PAGE_TYPE *pg, int *len)
{
	SMALLINT  bank;
	PAGE_TYPE page;
   int       jobopen = FALSE;
   int       cnt = 0;
	uchar     extra[3],space;
	uchar     temp_buff[32];
	uchar     rd_buf[2];
   uchar     addpg;
   int       i;	

	
   // Are program jobs possible
   // is there a job that has been opened and is this a regular memory read
   if(isJob(portnum,SNum))
      jobopen = TRUE;
            
   // loop while redirected
   for (cnt = 0; cnt <= 256; cnt++)
   {       
      // if an program job is open then check there for page
      if(jobopen && (flag != STATUSMEM))
      {
         if(getJobData(portnum,SNum,*pg,buff,len))
         {
            return TRUE;
         }
      }

      // nope so look for page in cache
      if(FindPage(portnum,SNum,pg,flag,TRUE,buff,len,&space))  
      {
         if(*len == 0x41)
         {
            *pg = buff[0];
         }
         else if(flag != STATUSMEM)
         {
     		   return TRUE;
         }
      }

      bank = getBank(portnum,SNum,*pg,flag);
		page = getPage(portnum,SNum,*pg,flag);
			
      // nope so get it from the part      
      // if the page is in the status memory then call read status
      if (flag == STATUSMEM)
      {
      	if(owIsWriteOnce(bank,portnum,SNum) && owHasExtraInfo(bank,SNum))
      	{
      		if(!owReadPageExtraCRC(bank,portnum,SNum,page,&temp_buff[0],&extra[0]))
            {
               if(extra[0] == 0xFF)
         		   return FALSE;
            }
      	}
      	else
      	{
            if(owHasPageAutoCRC(bank,SNum))
            {
          	   if(!owReadPageCRC(bank,portnum,SNum,page,&temp_buff[0]))
                  return FALSE;
            }
            else
            {
               if(!owReadPage(bank,portnum,SNum,page,FALSE,&temp_buff[0]))
  		            return FALSE;
            }
      				
         	extra[0] = 0xFF;
         }

         if(extra[0] != 0xFF)
         {
      	   rd_buf[0] = ~extra[0];
      		addpg = AddPage(portnum,SNum,*pg,&rd_buf[0],STATUSMEM);
      		*pg = (PAGE_TYPE) rd_buf[0];      
            continue;
         }
         else
         {
            *len = 8;
            for(i=0;i<*len;i++)
               buff[i] = temp_buff[i];
            addpg = AddPage(portnum,SNum,*pg,&buff[0],STATUSMEM);
//  		   	   AddPage(portnum,SNum,*pg,&buff[0],*len);
//               AddPage(portnum,SNum,*pg,&buff[0],0x41);
            return TRUE;

         }
      }
      // else call on the regular readpack function
      else 
      {          
         *len = 0;
      	if(owIsWriteOnce(bank,portnum,SNum) && owHasExtraInfo(bank,SNum))
      	{
      		if(!owReadPagePacketExtra(bank,portnum,SNum,page,FALSE,
         										&temp_buff[0],len,&extra[0]))
            {
               if(extra[0] == 0xFF)
         		   return FALSE;
            }
      	}
      	else
      	{
            if(owHasPageAutoCRC(bank,SNum))
            {
         	   if(!owReadPageCRC(bank,portnum,SNum,page,&temp_buff[0]))
                  return FALSE;
            }
            else
            {
               if(!owReadPage(bank,portnum,SNum,page,FALSE,&temp_buff[0]))
  		            return FALSE;
            }
      				
      		extra[0] = 0xFF;
         } 
            
         if(extra[0] != 0xFF)
         {
         	rd_buf[0] = ~extra[0];
      		addpg = AddPage(portnum,SNum,*pg,&rd_buf[0],REDIRMEM);
      		*pg = (PAGE_TYPE) rd_buf[0];  
            continue;
         }
         else
         {
            if(*len > 0)
            {
               if(*len > 32)
               {
                  OWERROR(OWERROR_INVALID_PACKET_LENGTH);
                  return FALSE;
               }

               for(i=0;i<(*len);i++)
                  buff[i] = temp_buff[i];
            }
            else
            {
               if(temp_buff[0] > 32)
               {
                  OWERROR(OWERROR_INVALID_PACKET_LENGTH);
                  return FALSE;
               }

     	     	   for(i=1;i<(temp_buff[0]+1);i++)
     	   		   buff[i-1] = temp_buff[i];
               *len = temp_buff[0];
            }

            addpg = AddPage(portnum,SNum,*pg,&buff[0],*len);
            return TRUE;
         }
    	}
   }         
   
   // could not find the page
   return FALSE;
}           

//--------------------------------------------------------------------------
// Write_Page:
//
// Write a page to a Touch Memory at a provided location (pg).
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// buff       location of the write data
// pg         page number to write the packet
// len        the length of the packet
//
SMALLINT Write_Page(int portnum, uchar *SNum, uchar *buff, PAGE_TYPE pg, int len) 
{
	SMALLINT  bank;
	PAGE_TYPE page;
   uchar     addpg;
   
   // check for length too long for a page
   if (len > 29)
   {
   	OWERROR(OWERROR_INVALID_PACKET_LENGTH);
   	return FALSE;
   }

   // Are program jobs possible
   // is there a job that has been opened 
   if(isJob(portnum,SNum))
   {            
      if(setJobData(portnum,SNum,pg,buff,len))
         return TRUE;
   }
   
   bank = getBank(portnum,SNum,pg,REGMEM);
   page = getPage(portnum,SNum,pg,REGMEM);

   if(!owWritePagePacket(bank,portnum,SNum,page,buff,len))
   	return FALSE;
	else
   {
		addpg = AddPage(portnum,SNum,pg,buff,len);
   }
		
	return TRUE;
}

//--------------------------------------------------------------------------
// ExtWrite writes an extended file at a location.  The bitmap bits are
// set each time a pages is written and verified.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// strt_page  the page to start the extended write
// file       location of the file to write
// fllen      the length of the file
// BM         the bitmap
//
// return TRUE if the write was successful.
//
SMALLINT ExtWrite(int portnum, uchar *SNum, uchar strt_page, uchar *file, 
                  int fllen, uchar *BM)
{
	int   pgs = 0;
	int   len = 0;
	PAGE_TYPE avail,next;
	uchar WrStr[32];
   int   i,j;
	                        
   if(fllen == 0)
   {
      if(!BitMapChange(portnum,SNum,strt_page,1,BM))
         return FALSE;

      return TRUE;
   }

   // calculate number of universal pages in file                     
   pgs = fllen / 28;    
   if (fllen % 28 || !fllen) 
      pgs++;

   // find available page and next available page
   FindEmpties(portnum,SNum,BM,&avail,&next);

   if(avail == 0)
   { 
   	OWERROR(OWERROR_OUT_OF_SPACE);
   	return FALSE;
   }  

   next = avail;
   avail = strt_page;
   // loop to write each page of the file
   for(i=0;i<pgs;i++)
   {
      // check to see if next chunk is last chunk and adjust size
      len = fllen - (i*28);
      if (len > 28)  
         len = 28;
      
      // copy next chuck to write buffer
      for (j=0; j<len; j++)
         WrStr[j] = file[(i*28)+j];

      // add the continuation pointer                   
      if(i == (pgs-1)) 
         WrStr[len] = (uchar) 0;  
      else 
         WrStr[len] = (uchar) next;
      
      // write the page
      if(!Write_Page(portnum,SNum,&WrStr[0],avail,(len+1)))
      	return FALSE;
                          
      // set the page just written to in bitmap  BM 
      if(!BitMapChange(portnum,SNum,avail,1,BM))
         return FALSE;

      // find available page and next available page
      FindEmpties(portnum,SNum,BM,&avail,&next);

      if((avail == 0) && ((pgs-1) != i))
      { 
   	   OWERROR(OWERROR_OUT_OF_SPACE);
   	   return FALSE;
      }  
   }

   // must be completed because it has made it this far
   return TRUE;
}

//----------------------------------------------------------------------
//  ExtRead, reads extended file structure files.  It is designed such
//  that if a file is contiguous then it is not reset between the reading
//  of each page packet.  The bitmap BM has a page bit cleared when a
//  page is read correctly.  The varialbe maxlen is the max number of
//  bytes that can be fit into the buffer 'buf'.  If maxlen is set to
//  -1 then the file is read but it is not copied over into buf.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// file       location of the file to write
// start_page the starting page for the file
// maxlen     the maximum length the file buffer can handle
// del        tell weather to delete the file or not.
// BM         the bitmap
// fl_len     the length of the file that was read
//
// return TRUE if the read was successful
//
SMALLINT ExtRead(int portnum, uchar *SNum, uchar *file, int start_page,
						 int maxlen, uchar del, uchar *BM, int *fl_len)

{
	SMALLINT  bank;
	int       done = FALSE;
	int       buff_len = 0;
	PAGE_TYPE pg;
	uchar     pgbuf[34];
	int       len = 0;
	int       i;
	int       flag;

	pg = start_page;
	
   // loop to read in pages of the extended file
   do
   {	
      if(!Read_Page(portnum,SNum,&pgbuf[0],REGMEM,&pg,&len))
      	return FALSE;
      	      
      // add page segment to buffer
      if (maxlen != -1)   // don't add to buff in maxlen is -1
      {
      	if((buff_len+len) > maxlen)
      	{
      		OWERROR(OWERROR_BUFFER_TOO_SMALL);
      		return FALSE;
      	}
      	
      	for(i=0;i<(len-1);i++)
      		file[buff_len++] = pgbuf[i];
      }
                                
      bank = getBank(portnum,SNum,pg,REGMEM);
      
      // flag to indicate that the page should be cleared in the BM
      flag = FALSE;
      // do if have a NVRAM device 
      if(owIsReadWrite(bank,portnum,SNum) && 
         owIsGeneralPurposeMemory(bank,SNum)) 
          flag = TRUE;
         
      // or non written buffered eprom page 
      if(isJob(portnum,SNum))
      {
         // see if page set to write but has not started yet
         if(isJobWritten(portnum,SNum,pg) && del)
         {
            flag = TRUE;
         }
      }
                              
      // clear bit in bitmap     
      if(flag)
      {
         if(!BitMapChange(portnum,SNum,pg,0,BM))
            return FALSE;
      }
                                                                             
      // check on continuation pointer
      if (pgbuf[len-1] == 0)
      {
      	*fl_len = buff_len;
         done = TRUE; // all done
      }
      else
         pg = pgbuf[len-1];
   } 
   while(!done);
   
   return TRUE;
}

//--------------------------------------------------------------------------
// ExtendedRead_Page:
//
// Read a extended page from the current Eprom Touch Memory.
// The record will be placed into a (uchar) memory location
// starting at the location pointed to by (buf).  This function will 
// read a page out of normal memory.  If the page being read is
// redirected then an error is returned
// The following return codes will be provided to the caller:
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the part that the read is
//            to be done on.
// buff       the buffer for the data that was read
// pg         the page that starts the read
//
// return TRUE if the read was successful.
//  
SMALLINT ExtendedRead_Page(int portnum, uchar *SNum, uchar *buff, PAGE_TYPE pg)               
{    
	SMALLINT  bank;
   PAGE_TYPE page;
	uchar     extra[20]; 
   int       len,i;
                     
	bank = getBank(portnum,SNum,pg,REGMEM);
	page = getPage(portnum,SNum,pg,REGMEM);
	
	if(!owIsWriteOnce(bank,portnum,SNum) && ((SNum[0] != 0x18) && 
      (SNum[0] != 0x33) && (SNum[0] != 0xB3)) )
	{
		OWERROR(OWERROR_NOT_WRITE_ONCE);
		return FALSE;
	}
	
   // check on the program job to see if this page is in it 
   if(isJob(portnum,SNum))
   {      
      if(getJobData(portnum,SNum,pg,&buff[1],&len))
      {
         return TRUE;
      }
   }     
  
   if(owIsWriteOnce(bank,portnum,SNum))
   {
      if(!owReadPageExtraCRC(bank,portnum,SNum,page,&buff[0],&extra[0]))
     	   return FALSE;    
   }
   else
   {
      if(!owReadPageExtra(bank,portnum,SNum,page,FALSE,&buff[0],&extra[0]))
         return FALSE;

      for(i=0;i<owGetExtraInfoLength(bank,SNum);i++)
         buff[i+32] = extra[i];

      extra[0] = 0xFF;
   }

	if(extra[0] != 0xFF)
	{
		OWERROR(OWERROR_REDIRECTED_PAGE);
		return FALSE;
	}
   	
   return TRUE;
}