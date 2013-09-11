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
//---------------------------------------------------------------------------
// 
// sprintf.c - Partial implentation for platforms not supported this function.
//

// includes
#include "ownet.h"
#include <stdarg.h>


// local prototyps
int sprintf(char *buffer, char *format, ...);
void fprintf(FILE *fp, char *format,...);

//--------------------------------------------------------------------------
// Partial 'sprintf' implementation
// Supports types: %s, %d, %c, %f and %X.  Both %d and %X can have 0 flag
// (only on non-neg numbers) and width (0..9).  %f can have width and
// precision but decimal part is truncated at precision.
//
// Return: number of characters written to 'buffer'
//
int sprintf(char *buffer, char *format, ...)
{
   va_list ap;
   char *f, *sval, tmp[30], cval;
   unsigned char tch;
   int cnt=0, zerofill, width, prec, tcnt, i, islong, isneg;
   static char error_msg[] = {"ERROR"};
   float fval;
   long ival;

   // get pointer to optional parameter list
   va_start(ap, format);

   // loop through the format string
   for (f = format; *f; f++)
   {
      // check if not in 'insert' arg mode
      if (*f != '%')
      {
         buffer[cnt++] = *f;
         continue;
      }

      // ok, got a %, look at next character
      f++;

      // init options
      zerofill = 0;
      width = 0;
      prec = 0;
      islong = 0;

      // check for zero
      if (*f == '0')
      {
         zerofill = 1;
         // in zero fill mode, got to next character
         f++;
      }

      // check for number of fields
      if ((*f > '0') && (*f <= '9'))
      {
         width = *f - '0';
         // width mode, go to next character
         f++;
      }

      // check for precision
      if (*f == '.')
      {
         // have precision, next char must be a number
         f++;

         // check for number of fields
         if ((*f > '0') && (*f <= '9'))
         {
            prec = *f - '0';
            // go to next character
            f++;
         }
         // error, don't know what to do
         else
            break;
      }

      // check for long flag
      if (*f == 'l')
      {
         islong = 1;
         // in long mode, got to next character
         f++;
      }

      // now better have a type character
      switch (*f)
      {
         // character
         case 'c':
            cval = va_arg(ap, char);
            buffer[cnt++] = cval;
            break;

         // integer
         case 'd':
            if (islong)
               ival = va_arg(ap, long);
            else
               ival = (long)va_arg(ap, int);
            // check for neg
            if (ival < 0)
			{
               isneg = 1;
               ival *= -1;
            }
            else
               isneg = 0;
            // build string in reverse order (easier)
            tcnt = 0;
            do
            {
               tmp[tcnt++] = (unsigned char)((ival % 10l) + '0');
               ival /= 10l;
            }
            while (ival);
            // check if neg
            if (isneg)
              tmp[tcnt++] = '-';
            // check for zero fill (only on non-neg)
            else if (width > tcnt)
            {
               for (i = tcnt; i < width; i++)
               {
                  if (zerofill)
                     tmp[i] = '0';
                  else
                     tmp[i] = ' ';
               }
               tcnt += (width - tcnt);
            }
            // reverse back and add to buffer
            for (i = 0; i < tcnt; i++)
               buffer[cnt++] = tmp[tcnt - i - 1];
            break;

         // string
         case 's':
            for (sval = va_arg(ap, char *); *sval; sval++)
               buffer[cnt++] = *sval;
            break;

         // hex integer
         case 'x': case 'X':
            if (islong)
               ival = va_arg(ap, long);
            else
               ival = (long)va_arg(ap, int);

            // check for neg
            if (ival < 0)
            {
               isneg = 1;
               width = (islong) ? 8 : 4;
            }
            else
               isneg = 0;

            // build string in reverse order (easier)
            tcnt = 0;
            do
            {
               // grab a nibble
               tch = (unsigned char)(ival & 0x0F);
               // convert to a hex character
               if ((tch >= 0x00) && (tch <= 0x09))
                  tmp[tcnt++] = tch + '0';
               else
                  tmp[tcnt++] = tch - 0x0A + 'A';

               ival >>= 4;
            }
            while (ival && !(!(tcnt < width) && isneg));
            // check for zero fill
            if (width > tcnt)
            {
               for (i = tcnt; i < width; i++)
               {
                  if (zerofill)
                     tmp[i] = '0';
                  else
                     tmp[i] = ' ';
               }
               tcnt += (width - tcnt);
            }
            // reverse back and add to buffer
            for (i = 0; i < tcnt; i++)
               buffer[cnt++] = tmp[tcnt - i - 1];
            break;

         // float
         case 'f':
            // set precision to 6 if not specified
            if (prec == 0)
               prec = 6;

            //??? could be float on some platforms
            fval = (float)va_arg(ap, double);
            // truncate to an integer
            ival = (int)fval;
            // check for neg
            if (ival < 0)
			{
               isneg = 1;
               ival *= -1;
               fval *= -1;
            }
            else
               isneg = 0;
            tcnt = 0;
            // build integer part string in reverse order (easier)
            do
            {
               tmp[tcnt++] = (unsigned char)(ival % 10 + '0');
               ival /= 10;
            }
            while (ival);
            // check if neg
            if (isneg)
              tmp[tcnt++] = '-';
            // check for width (no zero fill)
            if (width > (tcnt + prec + isneg + 1))
			{
               i = width - (tcnt + prec + isneg + 1);
               for (; i > 0; i--)
                  tmp[tcnt++] = ' ';
            }
            // reverse back and add to buffer
            for (i = 0; i < tcnt; i++)
               buffer[cnt++] = tmp[tcnt - i - 1];

            // do decimal part
            buffer[cnt++] = '.';
            for (i = 0; i < prec; i++)
			{
               fval = fval - (int)fval;
               fval *= (float)10.0;
               buffer[cnt++] = ((int)fval) % 10 + '0';
            }
            break;

         default:
            for (sval = error_msg; *sval; sval++)
               buffer[cnt++] = *sval;
            break;
      }
   }

   // clean up args
   va_end(ap);

   // zero terminate buffer
   buffer[cnt] = 0;

   // return the number of characters added to buffer
   return cnt;
}


void fprintf(FILE *fp, char *format,...)
{}