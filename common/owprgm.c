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
//  owprgm.c - This source file contains the EPROM Program routines.
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "owfile.h"
#include "rawmem.h"
#include "mbeprom.h"
#include <string.h>

// global paramters
SMALLINT   job_started = FALSE;
ProgramJob job;


//--------------------------------------------------------------------------
//  Start an programming job.  This function assumes that the Garbage bag D
//  is at least 11049 bytes long. This function check to see if the
//  current device is of the correct type.  It reads the bitmap of the
//  device for reference during accumulation of Program write jobs.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the job part to be executed on.
//
//  Returns:    TRUE  program job created
//              FALSE error
//
SMALLINT owCreateProgramJob(int portnum, uchar *SNum) 
{
   SMALLINT bank = 1;
   short i,j;
       
   // Make sure there is no current job
   job_started = FALSE;  
   
   // check the device type
   if(!owIsWriteOnce(bank,portnum,SNum))
   {
      OWERROR(OWERROR_WRONG_TYPE);
      return FALSE;
   }
      
   // flush all of the cached pages
   InitDHash();

   // read the bitmap 
   if(!ReadBitMap(portnum,SNum,&job.EBitmap[0]))
      return FALSE;
         
   // keep the original bitmap
   for (i = 0; i < 32; i++)
      job.OrgBitmap[i] = job.EBitmap[i];
   
   // clear all the job pages
   for (i = 0; i < MAX_NUM_PGS; i++)   
   {
      job.Page[i].wr = PJOB_NONE; 
      // clear bits in each page 
      for (j = 0; j < 29; j++)
         job.Page[i].data[j] = 0xFF;
   }      
   // record the current rom
   for (i = 0; i < 8; i++)
      job.ERom[i] = SNum[i];
      
   // Set the job to true
   job_started = TRUE;
   
   return TRUE;
}
         

//--------------------------------------------------------------------------
//  Do an program job.  This function assumes that the Garbage back D
//  is at least 11049 bytes long. This function check to see if the
//  current device is of the correct type.  It reads the bitmap of the
//  device for reference during accumulation of Program write jobs.
//
// portnum    the port number of the port being used for the
//            1-Wire Network.
// SNum       the serial number for the job part to be executed on.
//
//  Returns:    TURE  program job created
//              FALSE error
//
SMALLINT owDoProgramJob(int portnum, uchar *SNum) 
{                       
   uchar BM[32],WPBM[32],RWPBM[32],pgbuf[32],RDB[256],tpg;
   uchar tmpBM[4];
   short i,j,l,Bcnt=0,mBcnt,xtra,pg,inflp=0,redit;
   uchar ROM[8]; 
   SMALLINT  bank = 1;
   PAGE_TYPE page = 0;
   int maxp = 0;
   int flag;
   PAGE_TYPE bmpg,rdpg;
   uchar buff[5];

   memset(tmpBM, 0xFF, 4);
      
   // checks to see if the ROM number has changed
   for(i=0;i<8;i++)
      if(job.ERom[i] != SNum[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }
            
   // Make sure there is a job open
   if (job_started != TRUE)
   {
      OWERROR(OWERROR_NO_PROGRAM_JOB);
      return FALSE;
   }

   // Make sure that only eproms are on the line    
   l = TRUE;  // flag to see if operation is ok
   flag = owFirst(portnum,TRUE,FALSE);
   while(flag)
   {
      // check the family code
      owSerialNum(portnum,&ROM[0],TRUE);
      if(!owIsWriteOnce(bank,portnum,&ROM[0]))
      {
         l = FALSE;
         break;
      }
      
      // look for next device
      flag = owNext(portnum,FALSE,FALSE);
   }                          
   
   // restore the rom number 
   for (i = 0; i < 8; i ++)
      ROM[i] = job.ERom[i];
   owSerialNum(portnum,&ROM[0],FALSE);
      
   // check for fail
   if (l == FALSE)  
   {
      OWERROR(OWERROR_NON_PROGRAM_PARTS);
      return FALSE;
   }
   
   // flush all of the cached pages
   InitDHash();
   
   maxp = maxPages(portnum,SNum);

   // read the write protect bits and redirect write protect bits
   // and redirection bytes       
   if(owIsWriteOnce(bank,portnum,&ROM[0]) && ((ROM[0] == 0x0B) ||
                                              (ROM[0] == 0x0F)))
   {  // DS1985,DS1986
      for(i=0;i<32;i++)
         BM[i] = job.OrgBitmap[i];

      // get number of status bytes needed
      mBcnt = ((maxp+1) / 8) +
         ((((maxp+1) % 8) > 0) ? 1 : 0);

      // get number of pages to get that many bytes
      l = (mBcnt / 8) + ((mBcnt % 8) > 0 ? 1 : 0);

      // loop through the number of write protect bitmap pages
      for(i=0;i<l;i++)
      {
         // read stat mem page
         tpg = (uchar) i+8;

         if(!Read_Page(portnum,&ROM[0],&pgbuf[0],STATUSMEM,&tpg,&flag))
            return FALSE;

         if(flag == 8)
         {
            // copy bitmap read 
            for(j=0;j<8;j++)
               if(Bcnt <= mBcnt)
                  WPBM[Bcnt++] = ~pgbuf[j];
               else
                  break;
         }
         else
            return TRUE;
      }

      // loop through the number of redirection write protect bitmap pages
      Bcnt = 0;
      for(i=0;i<l;i++)
      {
         // read stat mem page
         tpg = i+4;
         if(!Read_Page(portnum,&ROM[0],&pgbuf[0],STATUSMEM,&tpg,&flag))
            return FALSE;
         
         // check results
         if(flag == 8)
         {
            // copy bitmap read buffer
            for(j=0;j<8;j++)
               if(Bcnt <= mBcnt)
                  RWPBM[Bcnt++] = ~pgbuf[j];
               else
                  break;
         }
         else
            return TRUE;
      }

      // read the redirection bytes  (mBcnt number of pages in redirection)
      Bcnt = 0;
      for(i=0;i<mBcnt;i++)
      {
         // read stat mem page
         tpg = i+32;
         if(!Read_Page(portnum,&ROM[0],&pgbuf[0],STATUSMEM,&tpg,&flag))
            return FALSE;
                                  
         // check results                             
         if (flag == 8)
         {              
            // copy bitmap read shorto buffer               
            for (j=0;j<8;j++)
               if (Bcnt <= maxp)
                  RDB[Bcnt++] = ~pgbuf[j];  
               else
                  break;   // quit the loop because done
         }                                 
         else 
            return TRUE;
      }
   }
   else if(owIsWriteOnce(bank,portnum,&ROM[0]))
   {
      // read status memory page 0
      tpg = 0;
      if(!Read_Page(portnum,&ROM[0],&pgbuf[0],STATUSMEM,&tpg,&flag))
         return FALSE;
      
      // check result of status read
      if (flag == 8)           
      {                 
         // extract the bitmap
         BM[0] = ~(pgbuf[0] >> 4) & 0x0F;  
         // extract the write protect bitmap 
         WPBM[0] = (~pgbuf[0]) & 0x0F;
         // extract the redirection write protect   
         RWPBM[0] = 0;  // there are non in the DS1982
         // extract the redirection bytes
         for (i = 0; i < 4; i++)
            RDB[i] = ~(pgbuf[i+1]);
      }
      else
         return TRUE;   
   }   
   else
   {
      OWERROR(OWERROR_NON_PROGRAM_PARTS);
      return FALSE;
   }

   
   // flush all of the cached pages
   InitDHash();

   // loop through all of the pages and check too see how many pages need to 
   // be re-directed.  From that see if there is enough room on the device
   // to do all of the desired operations  
   // Count the extra-pages for later    
   xtra = maxp+1;     
   for(i=0;i<=maxp;i++)
   {                                                            
      // if the page is aleady taken then subtract 2 pages 
      if ((job.Page[i].wr & PJOB_MASK) == PJOB_WRITE)  // (2.01)
         xtra -= (((BM[i / 8] >> (i % 8)) & 0x01) ? 2 : 1);    
      else if ((BM[i / 8] >> (i % 8)) & 0x01)
         xtra--;  
   }                       
   // check to see if the changes will really fit on the devices
   if (xtra < 0)
   {
      OWERROR(OWERROR_OUT_OF_SPACE);
      return FALSE;
   }
   
   // loop to write each page that has an eprom job
   for(i=0;i<=maxp;i++)
   {   
      // check for an infinite loop (if redirection point to each other)
      if (inflp++ > 20480)
      {
         OWERROR(OWERROR_PROGRAM_WRITE_PROTECT);
         return FALSE;
      }
      
      // check the type of operation (1 = page write, 2 redirection)
      // page write
      if ((job.Page[i].wr & PJOB_MASK) == PJOB_WRITE || 
          (job.Page[i].wr & PJOB_MASK) == PJOB_TERM)  
      {     
         // flag the says we are going to re-direct to this page                                   
         redit = FALSE;                                              
         pg = -1;
         
         // check to see if this page is already re-directed
         if (RDB[i] != 0x00)
         {
            // get the page and redirect
            pg = RDB[i];
            redit = TRUE;
            
            // check to see if this could be real
            if ((job.Page[pg].wr & PJOB_MASK) != PJOB_NONE)  
            {
               OWERROR(OWERROR_PROGRAM_WRITE_PROTECT);
               return FALSE;
            }
         }
         // not re-directed
         else
         {         
            // if write protected set or bitmap says already used then write 
            // redirection byte and change the Program job to an empty page  
            // only redirect this is a JOB_WRITE job
            if (((WPBM[i / 8] >> (i % 8)) & 0x01) || 
                (((BM[i / 8] >> (i % 8)) & 0x01) && 
                ((job.Page[i].wr & PJOB_MASK) == PJOB_WRITE)))
            {
               // check if the write-protect redirection byte is set
               if ((RWPBM[i / 8] >> (i % 8)) & 0x01)   
               {
                  OWERROR(OWERROR_PROGRAM_WRITE_PROTECT);
                  return FALSE;
               }
               // able to be re-directed so do it
               else  
                  redit = TRUE;
            }   
            // ok so write the page
            else                   
            {          
               
               // construct the page to write
               for(j=0;j<=job.Page[i].len;j++) 
                  pgbuf[j] = job.Page[i].data[j];
                  
               // set this page as having an attempt made to programming
               job.Page[getLastPage(portnum,SNum)].wr |= PJOB_START;
               
               bank = getBank(portnum,SNum,(PAGE_TYPE)i,REGMEM);
               page = getPage(portnum,SNum,(PAGE_TYPE)i,REGMEM);

               if(!owWritePagePacket(bank,portnum,SNum,page,&pgbuf[0],job.Page[i].len))
                  return FALSE;
                          
               // check to see if the bitmap is not set 
               if (!(BM[i / 8] & (0x01 << (i % 8))))   
               {
                  // set the bitmap bit for this page
                  // EPROM1/EPROM3 exception
                  if((ROM[0] == 0x0B) || (ROM[0] == 0x0F))
                  {
                     // set to 8 for status memory pages and reserved pages
                     bmpg  = 8;

                     if(!owRead(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,SNum,
                                    (i/8),FALSE,buff,1))
                        return FALSE;

                     tmpBM[i/8] = buff[0];
                     tmpBM[i/8] &= ~(0x01 << i%8);

                     if(!owWrite(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,
                                         SNum,(i/8),&tmpBM[i/8],1))
                        return FALSE;
//                     if(owProgramByte(portnum,(~(0x01 << (i%8))),(0x40+(i/8)),0x55,1,TRUE) == -1)
//                        return FALSE;
                  }
                  else
                  {
                     // set to 0 for status memory pages
                     bmpg  = 0;

                     if(!owRead(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,
                                        SNum,0,FALSE,buff,1))
                        return FALSE;

                     tmpBM[i/8] = buff[0];
                     tmpBM[i/8] &= ~(0x01 << (i+4));

                     if(!owWrite(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,
                                         SNum,0,&tmpBM[i/8],1))
                        return FALSE;

//                     if(owProgramByte(portnum,(~(0x01 << (i+4))),0,0x55,0,TRUE) == -1)
//                        return FALSE;
                  }
                     
                  // update the bitmap
                  if(!BitMapChange(portnum,SNum,(uchar)i,1,&BM[0]))
                     return FALSE;
                     
                  // clear this Program job                   
                  job.Page[i].wr = PJOB_NONE;   
               }
            }
         }
         
         // check to see if need to re-direct the page        
         if (redit == TRUE)
         {               
            // find an empty page in the Program job and bitmap
            if (pg == -1)
            {
               for(j=1;j<=maxp;j++)   
               {
                  // loop for a spot that is free in the bitmap, has not
                  // Program job on it, can be redirected, and is not write
                  // protected.
                  if (!((BM[j / 8] >> (j % 8)) & 0x01) && 
                       ((job.Page[j].wr & PJOB_MASK) == PJOB_NONE) && 
                      (RDB[j] == 0x00) && !((WPBM[j / 8] >> (j % 8)) & 0x01) &&
                      !((RWPBM[j / 8] >> (j % 8)) & 0x01))
                  {
                     pg = j;  
                     break;
                  }
               }
   
               // check to see if one was found, if not then we are out of space
               if (pg == -1)
               {
                  OWERROR(OWERROR_OUT_OF_SPACE);
                  return FALSE;
               }
            }
                                         
            // change the Program job to reflect the change
            job.Page[pg].wr = PJOB_WRITE;                       
            job.Page[pg].len = job.Page[i].len;
            // copy the data to the new job location
            for(j=0;j<job.Page[i].len;j++)
               job.Page[pg].data[j] = job.Page[i].data[j];   
                  
            // change the old job page to a redirect job
            job.Page[i].data[0] = (uchar)pg; // redirection page
            job.Page[i].wr = PJOB_REDIR;      // write redireciton job

            // optionally set back the loop if the new page is before
            // the current page
            i--;
            if (pg < i)
               i = pg - 1;           
               
            // loop back
            continue;
         
         }
               
      }            
      // redirection write
      else if ((job.Page[i].wr & PJOB_MASK) == PJOB_REDIR)  
      {
         // get the re-direction byte from the page data
         pg = job.Page[i].data[0];
         
         // check for redirection write protect with incorrect rdb   
         // or not possible write
         if ((((RWPBM[i / 8] >> (i % 8)) & 0x01) && (RDB[i] != pg)) ||
              ((RDB[i] | pg) ^ pg))
         {
            OWERROR(OWERROR_PROGRAM_WRITE_PROTECT);
            return FALSE;
         }
   
         // write the redirection byte and set the bitmap
         // EPROM1/EPROM3 exception
         if((ROM[0] == 0x0B) || (ROM[0] == 0x0F))
         {
            // set to 8 for status memory pages and reserved pages for bitmap
            bmpg  = 8;
            // set to   for redirection pages
            rdpg  = 32;

            if(!owRead(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,SNum,
                               (i/8),FALSE,buff,1))
               return FALSE;

            tmpBM[i/8] = buff[0];
            tmpBM[i/8] &= ~(0x01 << i%8);

            if((!redirectPage(getBank(portnum,SNum,rdpg,STATUSMEM),portnum,
                              SNum,i,pg)) || (!owWrite(getBank(portnum,SNum,
                              bmpg,STATUSMEM),portnum,SNum,(i/8),&tmpBM[i/8],1)))
               return FALSE;

//            if((owProgramByte(portnum,(~pg),(0x100+i),0x55,1,TRUE) == -1)
//               ||(owProgramByte(portnum,(~(0x01 << (i%8))),(0x40+(i/8)),0x55,1,TRUE) == -1))
//               return FALSE;
         }
         else  
         {
            // set to 0 for status memory pages
            bmpg  = 0;

            if(!owRead(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,SNum,0,
                               FALSE,buff,1))
               return FALSE;

            tmpBM[i/8] = buff[0];
            tmpBM[i/8] &= ~(0x01 << (i+4));
            tmpBM[i/8] &= (~pg);

            if((!redirectPage(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,SNum,i,pg)) ||
               (!owWrite(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,SNum,0,&tmpBM[i/8],1)))
               return FALSE;
//            if((owProgramByte(portnum,(~pg),(1+i),0x55,0,TRUE) == -1) 
//               ||(owProgramByte(portnum,(~(0x01 << (i+4))),0,0x55,0,TRUE) == -1))
//               return FALSE;
         }  
            
         if(!BitMapChange(portnum,SNum,(uchar)i,1,&BM[0]))
            return FALSE;
            
         // clear this Program job                   
         job.Page[i].wr = PJOB_NONE; 
      }                            
      // overwrite add file page                                       
      else if((job.Page[i].wr & PJOB_MASK) == PJOB_ADD) 
      {    
         for(j=1;j<=29;j++)    
         {                 
            // check to see if this byte needs to be programmed
            if(job.Page[i].bm[j / 8] & (0x01 << (j % 8)))
            {
               // write the byte
               if((ROM[0] == 0x0B) || (ROM[0] == 0x0F))
               {
                  if(owProgramByte(portnum,job.Page[i].data[j-1],((i<<5)+j),0x0F,1,TRUE) == -1)
                     return FALSE;
               }
               else
               {
                  if(owProgramByte(portnum,job.Page[i].data[j-1],((i<<5)+j),0x0F,0,TRUE) == -1)
                     return FALSE;
               }
            }
         }
         
         // check to see if the bitmap is not set
         if (!(BM[i / 8] & (0x01 << (i % 8))))   
         {         
            // set the bitmap bit for this page
            // EPROM1/EPROM3 exception
            if ((ROM[0] == 0x0B) || (ROM[0] == 0x0F))
            {
               // set to 8 for status memory pages and reserved pages
               bmpg  = 8;

               if(!owRead(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,SNum,
                                  (i/8),FALSE,buff,1))
                  return FALSE;

               tmpBM[i/8] = buff[0];
               tmpBM[i/8] &= ~(0x01 << i%8);

               if(!owWrite(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,
                                   SNum,(i/8),&tmpBM[i/8],1))
                  return FALSE;

//               if(owProgramByte(portnum,(~(0x01 << (i%8))),(0x40+(i/8)),0x55,1,TRUE) == -1)
//                  return FALSE;
            }
            else   
            {
               // set to 0 for status memory pages
               bmpg  = 0;

               if(!owRead(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,SNum,0,
                                  FALSE,buff,1))
                  return FALSE;

               tmpBM[i/8] = buff[0];
               tmpBM[i/8] &= ~(0x01 << (i+4));

               if(!owWrite(getBank(portnum,SNum,bmpg,STATUSMEM),portnum,
                                   SNum,0,&tmpBM[i/8],1))
                  return FALSE;

//               if(owProgramByte(portnum,(~(0x01 << (i+4))),0,0x55,0,TRUE) == -1)
//                  return FALSE;
            }
                              
            if(!BitMapChange(portnum,SNum,(uchar)i,1,&BM[0]))
               return FALSE;
         }
                                     
         // clear this Program job                   
         job.Page[i].wr = PJOB_NONE; 
      }

   }
                      
   // turn off the Program job
   job_started = FALSE;
                      
   // finished
   return TRUE;                   
}                   

//--------------------------------------------------------------------------
// Indicates if a job has already been initialized and waiting to be 
// written.
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
//
// return:  TRUE  : job started
//          FALSE : no jobs 
//                                    
SMALLINT isJob(int portnum, uchar *SNum)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         return FALSE;
      }

   return job_started;
}

//-------------------------------------------------------------------------
// Indicates weather a page needs to be written
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// pg         the page that is checked to see if it needs to
//            be written.
//
// return:  TRUE:   page needs to be written
//          FALSE:  doesn't need to be written
//
SMALLINT isJobWritten(int portnum,uchar *SNum,PAGE_TYPE pg)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }

   return job.Page[pg].wr;
}

//-------------------------------------------------------------------------
// Sets the status for the job on that page
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// pg         the page that is checked to see if it needs to
//            be written.
// status     the status that the page is being set to.
//
// return:  TRUE:   page needs to be written
//          FALSE:  doesn't need to be written
//
SMALLINT setJobWritten(int portnum,uchar *SNum,PAGE_TYPE pg,SMALLINT status)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }

   job.Page[pg].wr = status;

   return TRUE;
}

//-------------------------------------------------------------------------
// The purpose is to get the original bitmap.
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// BM         the bitmap to set to the original bitmap.
//
// return:  TRUE:   page needs to be written
//          FALSE:  doesn't need to be written
//
SMALLINT getOrigBM(int portnum, uchar *SNum, uchar *BM)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }

   for(i=0;i<32;i++)
      BM[i] = job.OrgBitmap[i];

   return TRUE;
}

//-------------------------------------------------------------------------
// Gets the page data of the job
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// page       the page that the data is gotten from.
// buff       the buffer to store the data on that page
// len        the len of the data for that page
//
// return:  TRUE:   page needs to be written
//          FALSE:  doesn't need to be written
//
SMALLINT getJobData(int portnum,uchar *SNum,PAGE_TYPE page,uchar *buff,int *len)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }

   if(job.Page[page].wr != PJOB_NONE)
   {
      for(i=0;i<job.Page[page].len;i++)
         buff[i] = job.Page[page].data[i];

      *len = job.Page[page].len;
   }
   else
   {
      return FALSE;
   }

   return TRUE;
}

//-------------------------------------------------------------------------
// Sets the page data for that page in the job
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// pg         the page that the data is set on.
// buff       the buffer of the data to be programmed
// len        the len of the data for that page
//
// return:  TRUE:   page needs to be written
//          FALSE:  doesn't need to be written
//
SMALLINT setJobData(int portnum,uchar *SNum,PAGE_TYPE pg,uchar *buff,int len)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }

   for(i=0;i<len;i++)
      job.Page[pg].data[i] = buff[i];

   job.Page[pg].len = len;

   if(len > 0)
   {
      job.Page[pg].wr = PJOB_WRITE;
   }

   return TRUE;
}

//-------------------------------------------------------------------------
// Sets a certain job for programming.
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// value      the value to set in the bitmap.
// spot       the spot to set the bitmap.
//
// return:  TRUE:   page needs to be written
//          FALSE:  doesn't need to be written
//
SMALLINT setProgramJob(int portnum,uchar *SNum,uchar value,int spot)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }

   job.EBitmap[spot] = value;

   return TRUE;
}

//-------------------------------------------------------------------------
// Gets a certain number of bitmap job programming starting from the first
// value in the array.
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// BM         the array of values for the bitmap.
// spots      the number of spots copied.
//
// return:  TRUE:   page needs to be written
//          FALSE:  doesn't need to be written
//
SMALLINT getProgramJob(int portnum,uchar *SNum,uchar *BM,int spots)
{
   int i;

   for(i=0;i<8;i++)
      if(SNum[i] != job.ERom[i])
      {
         OWERROR(OWERROR_NONMATCHING_SNUM);
         return FALSE;
      }

   for(i=0;i<spots;i++)
      BM[i] = job.EBitmap[i];

   return TRUE;
}

//--------------------------------------------------------------------------
// Write a byte into the Job space directly
//        
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// pg         the page to write the byte
// spot       the spot on the page to write the byte
// wbyte      the byte that is to be written
//
// return TRUE if the write was successful.
//
SMALLINT WriteJobByte(int portnum, uchar *SNum, PAGE_TYPE pg, short spot,
                      uchar wbyte, short zeros)
{    
   uchar pgbuf[34];   
   short i;
                     
   if(!isJob(portnum,SNum))
      if(!owCreateProgramJob(portnum,SNum))
         return FALSE;

   // check to see if this page is not in the job yet
   if ((job.Page[pg].wr & PJOB_MASK) == PJOB_NONE) 
   {
      for(i=0;i<32;i++)
         pgbuf[i] = 0xFF;

      // read the page first
      if(!ExtendedRead_Page(portnum,SNum,&pgbuf[0],pg))
         return FALSE;

      // copy the page contents into the job file
      for (i = 0; i < 29; i++)
         job.Page[pg].data[i] = pgbuf[i+1];
      
      // zero the bitmap for the page
      for (i = 0; i < 4; i++)
         job.Page[pg].bm[i] = 0;
   } 
   
   // set the program job type (overwrite)
   if (((job.Page[pg].wr & PJOB_MASK) != PJOB_ADD) && 
       ((job.Page[pg].wr & PJOB_MASK) != PJOB_TERM)) 
      job.Page[pg].wr = PJOB_ADD;                      
                                    
   // set the bitmap to mark this byte as one to program
   job.Page[pg].bm[spot / 8] |=  (0x01 << (spot % 8));                                 
                                    
   // depending on 'zeros' set the byte to write
   if (zeros)
      job.Page[pg].data[spot-1] = (uchar)~(~job.Page[pg].data[spot-1] | ~wbyte);       
   else
      job.Page[pg].data[spot-1] = (uchar)wbyte;

   return TRUE;
}                                       


//--------------------------------------------------------------------------
// Set the program job to terminate that page provided in the buffer.  The
// page data is provided to prevent it from being read twice since the calling
// function will read the page to see if it is already terminated.
//
// portnum    the port number of the port being used for the 
//            1-Wire Network
// SNum       the serial number for the part that the read is
//            to be done on.
// pg         the page to terminate
// pgbuf      the data to write to the terminated page
//        
// return TRUE if the page was terminated
//
SMALLINT TerminatePage(int portnum, uchar *SNum, short pg, uchar *pgbuf)
{                                
   short i;

   // page is not in job
   if ((job.Page[pg].wr & PJOB_MASK) == PJOB_NONE)
   {                       
      // copy the page contents into the job file
      for (i = 0; i < 29; i++)
         job.Page[pg].data[i] = pgbuf[i+1];
            
      // zero the bitmap for the page
      for (i = 0; i < 4; i++)
         job.Page[pg].bm[i] = 0;
   }
   // error file is already being written to 
   else if (((job.Page[pg].wr & PJOB_MASK) != PJOB_ADD) && 
           ((job.Page[pg].wr & PJOB_MASK) != PJOB_TERM))                                    
      return FALSE;
   
   // set the length to max since we do not know where the end really is in an add file
   job.Page[pg].len = 29;
                                                                                       
   // set the job to a terminate job                                                                                    
   job.Page[pg].wr = PJOB_TERM; 
   
   return TRUE; 
}    
    
    
