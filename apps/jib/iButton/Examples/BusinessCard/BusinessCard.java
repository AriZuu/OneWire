/**
 * BusinessCard.java
 *
 *    This small applet is used for personal information storage and
 * retrieval.
 *
 * @author  DL
 * @version 0.05 
 *
 * revision 6-11: KA
 *       kill the coffee preferences
 *       allow singly retrievable fields and access methods
 *
 */

import javacard.framework.*;

class BusinessCard extends Applet
{

   /*
    * The following static final int's are the instruction codes that the JiBlet will handle
    *
    */

   // Main CLA supported by the BusinessCard applet
   final static byte BC_CLA     = (byte) 0x81;

   // Instructions supported by the BusinessCard applet
   final static byte BC_INS_STORE        = 0x01;   //refers to massive store
   final static byte BC_INS_RETRIEVE     = 0x02;
   final static byte BC_INS_STORE_CP     = 0x03;
   final static byte BC_INS_RETRIEVE_CP  = 0x04;
   final static byte BC_INS_SINGLE_STORE = 0x05;

// data in mass stored like this
// name - 0 - 
// company - 0 -
// title - 0 - 
// voice - 0 - 
// fax - 0 - 
// email - 0 - 
// address  (no ending 0)

   //parameters of single store (P2)
   // name     0x00
   // company  0x01
   // title    0x02
   // voice    0x03
   // fax      0x04
   // email    0x05
   // address  0x06          <--this coding also counts number of 0's that come before this record 

   // Paramters for BC_INS_STORE (also for BC_INS_SINGLE_STORE)
   final static byte BC_P1_SET_LENGTH  = 0x01; 
   final static byte BC_P1_NEXT_PACKET = 0x02; 
   final static byte BC_P1_LAST_PACKET = 0x03; 

   // APDU header stuff used during applet selection
   final static byte SELECT_CLA = 0x00;
   final static byte SELECT_INS = (byte) 0xA4;

   // An sw for a bad instruction sequence
   final static short SW_BAD_INS_SEQUENCE = (short) 0x9101;

   //fields to hold the business card information
   //formerly the data was stored in one massive byte array
   //to ease the transition, the functions that retrieved and formatted that data
   //were unchanged, but once the data is stored in one large array, it is then copied
   //into these smaller arrays (KA)
   byte[] name;
   byte[] company;
   byte[] title;
   byte[] voice;
   byte[] fax;
   byte[] email;
   byte[] address;
   //in addition, when the data is going out, these single arrays are paced into a large array
   //so the send code was changed minimally (KA)


   /* 
    *    The following instance variables are used to hold state across
    * possible multiple APDUs for BC_INS_STORE.
    */
   short  offset;
   int    totalLength;
   byte[] temp;            //temp now stores what used to be the businesscard data,
			   //but it is only reliable from store-to-store, since the 
			   //data is actually kept in other byte arrays now

   /**
    * Construct an instance of BusinessCard. 
    */
   BusinessCard()
   {
      register();
      name    = new byte[0];
      company = new byte[0];
      title   = new byte[0];
      voice   = new byte[0];
      fax     = new byte[0];
      email   = new byte[0];
      address = new byte[0]; 

   }

   /**
    * Install the BusinessCard applet in Java iButton
    * @param apdu APDU
    */
   public static void install(APDU apdu)
   {
      new BusinessCard();
   }

   /**
    * Make this applet as the currently selected applet in the Java iButton. 
    * @param apdu APDU
    */
   public boolean select(APDU apdu)
   {
      return true;
   }

   /**
    * Dispatch APDUs
    * @param apdu APDU
    */
   public void process(APDU apdu) throws ISOException 
   {
      byte[] buffer = apdu.getBuffer();

      // process is called on applet selection but we don't do anything in this applet
      if ((buffer[ISO.OFFSET_CLA] == SELECT_CLA) && (buffer[ISO.OFFSET_INS] == SELECT_INS))
         return;

      if (buffer[ISO.OFFSET_CLA] != BC_CLA)
      {
         // Don't know what to do with this instruction
         throw new ISOException(ISO.SW_CLA_NOT_SUPPORTED);
      }
      else 
      {
         switch (buffer[ISO.OFFSET_INS])
         {
            default:
               // Don't know what to do with this instruction
               throw new ISOException(ISO.SW_INS_NOT_SUPPORTED);

            case BC_INS_STORE:
               businessCardStore(apdu, BC_INS_STORE);
               break;
   
            case BC_INS_RETRIEVE:
               businessCardRetrieve(apdu);
               break;

	    case BC_INS_SINGLE_STORE:
		businessCardStore(apdu,BC_INS_SINGLE_STORE);
		break;
         }
      }
   }

   /** 
    * Store new business card info
    *    This method copies business card data into a temporary byte 
    * array in pieces using receiveBytes. Once we have all the business 
    * card data in temp it will be atomically copied to the bcData array. 
    * This way we should never have partial data in bcData.
    *
    * 6-11 bcData has been killed, in favor of singly storing records
    *      also, this function will assume the functionality
    *      of singly storing records based on the INS byte
    * @param apdu APDU
    */
   int fieldNumber;
   protected void businessCardStore(APDU apdu, int INSNumber) throws ISOException
   {
      byte[] buffer    = apdu.getBuffer();
      short  bytesRead = apdu.setIncomingAndReceive();


      switch (buffer[ISO.OFFSET_P1])
      {
         case BC_P1_SET_LENGTH: 

		/*
		 * The first send in a store is to tell us what length to expect,
		 * so handle appropriately.
		 */
	    
	    fieldNumber = buffer[ISO.OFFSET_P2];   //store the fieldnumber for later	

            // Reset offset into temp array
            offset = 0; 

            // We're expecting a 2 byte length
            if (buffer[ISO.OFFSET_LC] == 2)
            {
               totalLength  = buffer[ISO.OFFSET_CDATA] << 8;
               totalLength |= (buffer[ISO.OFFSET_CDATA+1] & 0xFF);
            }
            else 
 	       //protocol was not followed, so data is invalid
               throw new ISOException(ISO.SW_DATA_INVALID);

            /* 
             *    Create a temporary array to hold the business card info. 
             * Once we've got it all we copy it into bcData. (6-11 : temp[] will do the job just fine)
             */
            temp = new byte[totalLength];
            break;
   
         case BC_P1_NEXT_PACKET:
            if (totalLength > 0)
            {
               while (bytesRead > 0)
               {
                  Util.arrayCopy(buffer, ISO.OFFSET_CDATA, temp, offset, bytesRead);
   
                  offset += bytesRead;
                  // Get the next chunk of business card info 
                  bytesRead = apdu.receiveBytes(ISO.OFFSET_CDATA);
               }
            }
            else 
               throw new ISOException(SW_BAD_INS_SEQUENCE);

            break;

         case BC_P1_LAST_PACKET:
            if (totalLength > 0)
            {
               while (bytesRead > 0)
               {
                  Util.arrayCopy(buffer, ISO.OFFSET_CDATA,
                                 temp,offset, bytesRead);
   
                  offset += bytesRead;
                  // Get the next chunk of business card info 
                  bytesRead = apdu.receiveBytes(ISO.OFFSET_CDATA);
               }

		//now that we are the end we need to do something with this data
		//depending on if this is a single-field store opertation
		//or a total store operation
   
	       if (INSNumber==1)      //then we are doing a mass store
		     setToFields();
	       else                   //we are doing a single field store
		     setField(fieldNumber);
            }
            else 
               throw new ISOException(SW_BAD_INS_SEQUENCE);

            break;

         default:
            throw new ISOException(ISO.SW_WRONG_P1P2);
      }
   }


	//This is the easy part, we just replace the old field with temp
	//since we were only sent a single field
	protected void setField(int num)
	{
		int i=0;
		switch(num)
		{
			case 0 :   name    = new byte[temp.length];
				   while (i<temp.length) {name[i] = temp[i++];}
				   break;
			case 1 :   company = new byte[temp.length];
				   while (i<temp.length) {company[i] = temp[i++];}
				   break;
			case 2 :   title   = new byte[temp.length];
				   while (i<temp.length) {title[i] = temp[i++];}
				   break;
			case 3 :   voice   = new byte[temp.length];
				   while (i<temp.length) {voice[i] = temp[i++];}
				   break;
			case 4 :   fax     = new byte[temp.length];
				   while (i<temp.length) {fax[i] = temp[i++];}
				   break;
			case 5 :   email   = new byte[temp.length];
				   while (i<temp.length) {email[i] = temp[i++];}
				   break;
			case 6 :   address = new byte[temp.length];
				   while (i<temp.length) {address[i] = temp[i++];}
				   break;
			default:   throw new ISOException(ISO.SW_INS_NOT_SUPPORTED);

		}
	}


	//This is the hard part--since we have the data in one large block 
	//we need to break it down into the individual fields
	protected void setToFields()
	{
		int i=0;    //single field looping variables
		int j=0;    //temp looping variable

		//set the sizes on these fields
		while (temp[j]!=0) {i++; j++;}
		name = new byte[i];
		i=0; j++;
		while (temp[j]!=0) {i++; j++;}
		company = new byte[i];
		i=0; j++;
		while (temp[j]!=0) {i++; j++;}
		title = new byte[i];
		i=0; j++;
		while (temp[j]!=0) {i++; j++;}
		voice = new byte[i];
		i=0; j++;
		while (temp[j]!=0) {i++; j++;}
		fax = new byte[i];
		i=0; j++;
		while (temp[j]!=0) {i++; j++;}
		email = new byte[i];
		i=0; j++;
		while (j<temp.length) {i++; j++;}
		address = new byte[i];
		i=0;
		j=0;

		//put the data in these new fields
		while (temp[j]!=0) name[i++] = temp[j++];
		j++; i = 0;
		while (temp[j]!=0) company[i++] = temp[j++];
		j++; i = 0;
		while (temp[j]!=0) title[i++] = temp[j++];
		j++; i = 0;
		while (temp[j]!=0) voice[i++] = temp[j++];
		j++; i = 0;
		while (temp[j]!=0) fax[i++] = temp[j++];
		j++; i = 0;
		while (temp[j]!=0) email[i++] = temp[j++];
		j++; i = 0;
		while (j < temp.length) address[i++] = temp[j++];
        }

   /** 
    * Retrieve business card info
    * @param apdu APDU
    */
   protected void businessCardRetrieve(APDU apdu)
   {
	//Since the data needs to go out in one array, we need
	//to pack it into temp from the individual fields

 	//'6' here is the number of zeroes to offset the seven fields
	temp = new byte[6+name.length+company.length+title.length
   			+voice.length+fax.length+email.length
			+address.length];
	int j=0;
	int i=0;
	for (i=0;i<name.length;i++) {temp[j] = name[i]; j++;}
	temp[j] = 0; j++;
	for (i=0;i<company.length;i++) {temp[j] = company[i]; j++;}
	temp[j] = 0; j++;
	for (i=0;i<title.length;i++) {temp[j] = title[i]; j++;}
	temp[j] = 0; j++;
	for (i=0;i<voice.length;i++) {temp[j] = voice[i]; j++;}
	temp[j] = 0; j++;
	for (i=0;i<fax.length;i++) {temp[j] = fax[i]; j++;}
	temp[j] = 0; j++;
	for (i=0;i<email.length;i++) {temp[j] = email[i]; j++;}
	temp[j] = 0; j++;
	for (i=0;i<address.length;i++) {temp[j] = address[i]; j++;}

	//now send temp on out
      if (temp != null)
      {
	//this is the normal APDU return sequence	
         apdu.setOutgoing(); 						//set mode to sending data
	 apdu.setOutgoingLength((short)temp.length);			//set the length of the data to go
         apdu.sendBytesLong(temp, (short) 0, (short) temp.length); 	//set the data, start index, and ending offset
      }
   }








}   
