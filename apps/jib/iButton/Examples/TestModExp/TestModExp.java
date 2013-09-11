/*
  {06/11/98 TAC}

  NOTE: This applet will fail after several iterations if the applet
        GC is not enabled.
*/
import java.lang.Object;
import javacard.framework.*;
import com.dalsemi.system.*;

public class TestModExp extends Applet
{
  //The parameters used in selection of this applet.
  public  final static  byte  SELECT_CLA      = (byte)0x00;
  public  final static  byte  SELECT_INS      = (byte)0xA4;

  public  final static  byte  TME_CLA                 = (byte)0x80;
  public  final static  byte  TME_INS_LOAD_MESSAGE    = (byte)0x00;
  public  final static  byte  TME_INS_LOAD_EXPONENT   = 0x01;
  public  final static  byte  TME_INS_LOAD_MODULUS    = 0x02;
  public  final static  byte  TME_INS_CALCULATE       = 0x03;
  public  final static  byte  TME_INS_SEND_RESULT     = 0x04;
  public  final static  byte  TME_INS_SEND_LAST_ERROR = 0x05;
  public  final static  byte  TME_INS_DUMP            = 0x06;

  // extremely large value for debugging.
  public  final static  short MAX_SEND_LENGTH = 1000;

  private boolean messageLoaded   = false,
                  exponentLoaded  = false,
                  modulusLoaded   = false;
  private byte[]  message         = null;
  private byte[]  exponent        = null;
  private byte[]  modulus         = null;
  private byte[]  result          = null;
  private byte[]  error           = null;

  //The constructor.
  TestModExp()
  {
    //Register this applet with the JCRE
    register();
  }

  //Install the applet on the iButton.
  public static void install(APDU apdu)
  {
    new TestModExp();
  }

  //Dispatch APDU's sent to this applet.
  public void process(APDU apdu)
  {
    byte[] buffer = apdu.getBuffer();

    try
    {
      if((buffer[ISO.OFFSET_CLA] == SELECT_CLA) && (buffer[ISO.OFFSET_INS] == SELECT_INS))
      {
        return;
      }

      if(buffer[ISO.OFFSET_CLA] != TME_CLA)
      {
        throw new ISOException(ISO.SW_CLA_NOT_SUPPORTED);
      }
      else
      {
        if(error  ==  null)
        {
          error     = new byte[2];
          error[0]  = (byte)0x90;
          error[1]  = (byte)0;
        }

        switch(buffer[ISO.OFFSET_INS])
        {
          case  TME_INS_LOAD_MESSAGE:
            message = loadBigInt(apdu);
            messageLoaded = true;
          break;

          case  TME_INS_LOAD_EXPONENT:
            exponent = loadBigInt(apdu);
            exponentLoaded = true;
          break;

          case  TME_INS_LOAD_MODULUS:
            modulus = loadBigInt(apdu);
            modulusLoaded = true;
          break;

          case  TME_INS_CALCULATE:
            if(messageLoaded && exponentLoaded && modulusLoaded)
              performModExp(apdu);
            else
              throw new NullPointerException((short)0x55aa);

            apdu.setOutgoing();
            apdu.setOutgoingLength(MAX_SEND_LENGTH);
            apdu.sendBytesLong(error,        (short)0,   (short)error.length);
          break;

          case  TME_INS_SEND_RESULT:
            getLastResult(apdu);
          break;

          case  TME_INS_SEND_LAST_ERROR:
            getLastError(apdu);
          break;

          case  TME_INS_DUMP:
            dumpMsgExpMod(apdu);
          break;

          default:  //What to do if an unexpected INS code is received.
            throw new ISOException(ISO.SW_INS_NOT_SUPPORTED);
        }// switch.
      }// else.
    }// try
    catch(Exception t)
    {
      short reason = t.getReason();
      error = new byte[4];
      //Report what type of exception occurred.
      error[0] = com.dalsemi.system.Properties.getClassNumber(t);
      error[1] = (byte)((reason >>> 8) & 0xFF);
      error[2] = (byte)(reason & 0xFF);
      error[3] =  buffer[ISO.OFFSET_CDATA];
      // hey they don't let me re-throw it so I'll force an exception
//      error[-1] = error[-1];

      apdu.setOutgoing();
      apdu.setOutgoingLength(MAX_SEND_LENGTH);
      apdu.sendBytesLong(error,        (short)0,   (short)error.length);
    }
  }

  byte[]  loadBigInt(APDU apdu)
  {
    short   bufferOffset  = 0;
    byte[]  inBuffer      = apdu.getBuffer();
    short   bytesRead     = apdu.setIncomingAndReceive();
    byte[]  temp          = new byte[inBuffer[ISO.OFFSET_CDATA]&0x00ff];

    // skip the very first byte - the length byte
    javacard.framework.Util.arrayCopyNonAtomic(inBuffer, (short)(ISO.OFFSET_CDATA+1), temp, bufferOffset, (short)(bytesRead-1));
    bufferOffset += bytesRead-1;
    bytesRead     = apdu.receiveBytes(ISO.OFFSET_CDATA);

    // now get the remaining data
    while (bytesRead > 0)
    {
      javacard.framework.Util.arrayCopyNonAtomic(inBuffer, ISO.OFFSET_CDATA, temp, bufferOffset, bytesRead);
      bufferOffset += bytesRead;
      bytesRead     = apdu.receiveBytes(ISO.OFFSET_CDATA);
    }

    apdu.setOutgoing();
    apdu.setOutgoingLength(MAX_SEND_LENGTH);
    apdu.sendBytesLong(temp, (short)0, (short)temp.length);
    return  temp;
  }

  void  dumpMsgExpMod(APDU apdu)
  {
    byte[]  db  = new byte [2];
    db[0] = db[1] = 0x55;

    apdu.setOutgoing();
    apdu.setOutgoingLength(MAX_SEND_LENGTH);
    apdu.sendBytesLong(message,   (short)0,   (short)message.length);
    apdu.sendBytesLong(db,        (short)0,   (short)db.length);
    apdu.sendBytesLong(exponent,  (short)0,   (short)exponent.length);
    apdu.sendBytesLong(db,        (short)0,   (short)db.length);
    apdu.sendBytesLong(modulus,   (short)0,   (short)modulus.length);
  }

  void  getLastResult(APDU apdu)
  {
    apdu.setOutgoing();
    apdu.setOutgoingLength(MAX_SEND_LENGTH);
    apdu.sendBytesLong(result,(short)0,(short)result.length);
  }

  void  getLastError(APDU apdu)
  {
    apdu.setOutgoing();
    apdu.setOutgoingLength(MAX_SEND_LENGTH);
    apdu.sendBytesLong(error,(short)0,(short)error.length);
  }

  // performs the operation d = a^b mod c
  void performModExp(APDU apdu)
    throws CoprocessorException
  {
    try
    {
      result  = Coprocessor.modExp(message, exponent, modulus);

      // see if the result is zero
      if(result.length == 0)
      {
        result = new byte[1];
        result[0] = 0;
      }

      error = new byte[2];
      error[0]  = (byte)0x90;
      error[1]  = (byte)0;
    }
    catch (CoprocessorException cpe)
    {
      error = new byte[5];
      error[0] = (byte)'C';
      error[1] = (byte)'P';
      error[2] = (byte)'E';
      short reason = cpe.getReason();
      error[3] = (byte)((reason >>> 8) & 0xff);
      error[4] = (byte)(reason & 0xff);
      apdu.setOutgoing();
      apdu.setOutgoingLength(MAX_SEND_LENGTH);
      apdu.sendBytesLong(error, (short)0,(short)error.length);
    }
  }
}
