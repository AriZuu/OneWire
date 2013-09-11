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
//  owCache.c - Stores the latest buttons information for faster access
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "owfile.h"

// global data space
static Dentry Space[DEPTH];    // data space for page entries
static uchar Hash[DEPTH];      // table to hold pointer to space
static uchar BitMap[DEPTH];    // record of space available


//--------------------------------------------------------------------------
// Initialize the data hash values
//
void InitDHash(void)
{
   short i;
   
   // set all the pointers to the empty state
   for (i = 0; i < DEPTH; i++)
   {
      Hash[i] = 0xFF;
      BitMap[i] = 0xFF;
      Space[i].Fptr = 0xFF;
      Space[i].Bptr = 0xFF;   
      Space[i].Hptr = 0xFF;
   }
}
  
//--------------------------------------------------------------------------
// Add a page to the hash table.  If the page is already in the hash table
// it is updated and a new time stamp is given.  If the page is new then
// it is added to the table. The data in buf is put in the page and the 
// Space location number is returned.
//
// portnum  the port number of the port being used for the
//          1-Wire Network.
// SNum     the serial number for the part that the read is
//          to be done on.
// pg       the page to add
// buf      the buffer of the page data
// len      len of data for the page
//
// return the space number for the new page
//
uchar AddPage(int portnum, uchar *SNum, PAGE_TYPE pg, uchar *buf, int len)        
{
   uchar hs,p=0xFF; 
   short i=0,m;
   PAGE_TYPE page;
   int tlen;
   uchar cache_page[32];

   page = pg;
                              
   // attempt to see if page already there                           
   if(!FindPage(portnum,SNum,&page,(uchar)(len & 0x80),FALSE,
                &cache_page[0],&tlen,&p))
      return FALSE;
   
   if (p == 0xFF)
   {
      // page not found so add one
      hs = HashFunction(SNum,page);
      p = FindNew(hs);
      
      // attach the device to the chain (if there is one)
      // no other page in hash location
      if (Hash[hs] == 0xFF)   
      {
         Hash[hs] = p;             // hash p to new page
         Space[p].Fptr = 0xFF;     // np front p to nothing
         Space[p].Bptr = 0xFF;     // np back p to nothing
         Space[p].Hptr = hs;       // np hash p to hash location
      }    
      // some other page already there                
      else                            
      {                                                       
         // insert as first page
         Space[p].Fptr = Hash[hs]; // np front p to old first page
         Space[Hash[hs]].Bptr = p; // old first page back p to np
         Hash[hs] = p;             // hash p to np 
         Space[p].Hptr = hs;       // np hash p to hash location
      }

      // set the page number
      Space[p].Page = page;
      // set the rom
      for (i = 0; i < 8; i++)
         Space[p].ROM[i] = SNum[i];            
   }
   
   // set the data
   Space[p].Data[0] = len;
   m = ((len & 0x1F) <= 0x1D) ? (len & 0x1F) : 0;
   for (i = 0; i < m; i++)
      Space[p].Data[i+1] = buf[i];   

   // set the time stamp limit of X seconds
   Space[p].Tstamp = msGettick() + CACHE_TIMEOUT; // (3.10)
   
   // return the Space number of the new page         
   return p;
}

                  
//--------------------------------------------------------------------------
// Find a empty page in Space and return its pointer. First look at the 
// expected spot.  If an empty or expired page is not found then search
// thru the pages.  If one cannot be found then void the oldest one. 
//
// hashnum  the number of the page that might be empty.
//
// return the space location
//                  
uchar FindNew(uchar hashnum) 
{
   ulong tm,otm;
   static uchar spot = 0;  // spot pointer
   uchar t = 0xFF;     
   uchar oldest,i;   
   uchar freepg;
            
   // if current spot is empty then use that 
   if (BitMap[spot] == 0xFF)
      t = spot;
   // not empty so first try and find an empty spot in bitmap
   else
   {                        
      // get the current time
      tm = msGettick();        
      // set the oldest time to the current spot
      otm = Space[spot].Tstamp;              
      oldest = spot;
                    
      // check to see if spot is void
      if (tm > Space[spot].Tstamp)
      {
         freepg = FreePage(spot);
         t = spot;
      }           
      else
      {                 
         // loop through all of the bitmap
         for (i = 0; i < DEPTH; i++)
         {
            if (BitMap[i] == 0xFF)  // check for empty
            {
               t = i;
               break;         
            }
            else if (tm > Space[i].Tstamp) // check for expired
            {
               freepg = FreePage((uchar)i);
               t = i;
               break;
            }
            else if (Space[i].Tstamp < otm) // find oldest
            {
               otm = Space[i].Tstamp;
               oldest = i;
            }
         }         
         
         // check to see if not found one
         if (i == DEPTH)
         {
            // free the current spot            
            freepg = FreePage(oldest);
            t = oldest;   
         }
      }
   } 
   
   // set next spot to the current spot + 1                                          
   spot = (spot == (DEPTH-1)) ? 0 : spot + 1; 
   
   // set the bitmap to say where the page is going
   BitMap[t] = hashnum;                            
   
   // return the space location
   return t;
}
             
             
//--------------------------------------------------------------------------
// Search the hash cache to find the page discribed by the rom and page.
// If it is found and it has not expired, then return its Space number
// or 0xFF if it can not be found. 'mflag' is the memory section flag 
// where 0x00 is normal memory space and 0x80 is status memory space.
//
// portnum   the port number of the port being used for the
//           1-Wire Network.
// SNum      the serial number for the part that the read is
//           to be done on.
// page      the page to start looking
// mflag     the flag to compare in looking for the right page
// time      the expired time to check
// buf       the buffer of the page data
// len       the length of the data on the page
// space_num the space number in the table for the data of the page
//
// return true if the page data was found 
//             
SMALLINT FindPage(int portnum, uchar *SNum, PAGE_TYPE *page, uchar mflag, 
                  uchar time, uchar *buf, int *len, uchar *space_num)
{
   static uchar DidInit=0;
   uchar hs,ptr=0xFF; 
   short i=0;                    
   ulong tm;
    
   // initialize the file page cache (DSS 3.11) automatically
   if (!DidInit)
   {
      InitDHash(); 
      DidInit = 1;   
   }
       
   hs = HashFunction(SNum,*page);
   
   // if not empty
   if (Hash[hs] != 0xFF)
   {          
      // get the current time
      tm = msGettick();
      
      ptr = Hash[hs];  // point to first at hash spot
      do     
      {      
         // check to see if this record is expired
         if (time)
         {
            if (tm > Space[ptr].Tstamp)
            {                              
               ptr = FreePage(ptr);
               continue; // skip to loop check
            }
         }      
         
         // check to see if this page is the correct one (and correct mem space)
         if ((Space[ptr].Page == *page) && (mflag == (Space[ptr].Data[0] & 0x80)))
         {
            for (i = 0; i < 8; i++)
               if (Space[ptr].ROM[i] != SNum[i])
                  break;
                   
            if (i == 8)
               break;
          }                           
          // point to next page at hash location
          ptr = Space[ptr].Fptr;
      }
      while (ptr != 0xFF);
   }

   // check if need to copy over the data
   if (ptr != 0xFF)
   {
      if ((Space[ptr].Data[0] & 0x1F) <= 0x1D)
      {
         *len = Space[ptr].Data[0];
         for (i = 1; i <= (*len & 0x1F); i++)
            buf[i-1] = Space[ptr].Data[i];        
      }
      else
         ptr = 0xFF;
   }
         
   // check result
   if(ptr == 0xFF)
      return FALSE;

   *space_num = ptr;
   return TRUE;                          
}                                
  
  
//--------------------------------------------------------------------------
// Free's the page number passed to it.  Fixes pointers in the Hash 
// structure. Returns the next device in the hash structure unless this
// is the last page in the hash location and return 0xFF.
//
// ptr  the hash location of the data
//
// return the next page
//                                                       
uchar FreePage(uchar ptr)
{                       
   uchar hsptr;
   
   // get the hash location
   hsptr = Space[ptr].Hptr;
   
   // if chained
   if (Space[ptr].Fptr != 0xFF)
   {                        
      // check if first in chain
      if (Hash[hsptr] == ptr)
      {
         Hash[hsptr] = Space[ptr].Fptr;  // set hash p to next in ch
         Space[Hash[hsptr]].Bptr = hsptr;        // set next in chain p to hash
         BitMap[ptr] = 0xFF;             // free page
         ptr = Hash[hsptr];              // change current p
      }  
      //  middle of chain
      else
      {
         // previous in ch p to next in ch
         Space[Space[ptr].Bptr].Fptr = Space[ptr].Fptr; 
         // next in ch p to prevoious in ch
         Space[Space[ptr].Fptr].Bptr = Space[ptr].Bptr;
         BitMap[ptr] = 0xFF;          // free page
         ptr = Space[Space[ptr].Bptr].Fptr;  // change current p
      }   
   }   
   // if first and only on hash spot
   else if (Hash[hsptr] == ptr)
   {
      Hash[hsptr] = 0xFF;  // have hash p to nothing
      BitMap[ptr] = 0xFF;  // free page
      ptr = 0xFF;    // point to nothing            
   }      
   // else last in a chain
   else
   {
      // previous in ch p to nothing
      Space[Space[ptr].Bptr].Fptr = 0xFF; 
      BitMap[ptr] = 0xFF;          // free page
      ptr = 0xFF;  // point to nothing               
   }                    
   
   return 0xFF;  // return the next page or nothing
}         

                    
//--------------------------------------------------------------------------
// The hashing function takes the data provided and returns a hash 
// number in the range 0 to DEPTH-1.    
//
// SNum  the serial number for the part that the read is
//          to be done on.
// page  the page that is being added or looked for
//
// returns the hash number of the given data                   
//
uchar HashFunction(uchar *SNum, int page)                    
{
   ulong h;
   uchar tmp_page;
   
   tmp_page = (uchar) page;
   
   h = (SNum[1] << 12) | (SNum[2] << 8) | (SNum[7] << 4) | tmp_page;
   
   return (uchar)(h % DEPTH);
}