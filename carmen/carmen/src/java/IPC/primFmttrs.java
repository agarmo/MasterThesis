/*****************************************************************************
 *       $Id: primFmttrs.java 1280 2005-08-31 20:37:39Z nickr $
 * $Revision: 1280 $
 *     $Date: 2005-08-31 22:37:39 +0200 (Wed, 31 Aug 2005) $
 *   $Author: nickr $
 *    $State$
 *   $Locker$
 *
 * PROJECT:	NM-DS1
 *
 * FILE:	primFmttrs.java
 *
 * DESCRIPTION: JAVA class for marshalling and unmarshalling IPC byte arrays
 *              into JAVA objects..
 *
 * $Log$
 * Revision 1.1  2005/08/31 20:33:47  nickr
 * *** empty log message ***
 *
 * Revision 1.1  2005/08/31 20:25:31  nickr
 * Initial check-in of java class libraries
 *
 * Revision 1.1  2002/01/02 17:40:17  reids
 * Initial (and more, or less, complete) release of Java version of IPC.
 *
 *
 *****************************************************************************/

package IPC;

import java.lang.reflect.*;

public class primFmttrs {
  public static final int INT_FMT       = 1;
  public static final int BOOLEAN_FMT   = 2;
  public static final int FLOAT_FMT     = 3;
  public static final int DOUBLE_FMT    = 4;
  public static final int BYTE_FMT      = 5;
  public static final int STR_FMT       = 7;
  public static final int UBYTE_FMT     = 9;
  public static final int CHAR_FMT      = 16;
  public static final int SHORT_FMT     = 17;
  public static final int LONG_FMT      = 18;
  public static final int USHORT_FMT    = 28;
  public static final int UINT_FMT      = 29;
  public static final int ULONG_FMT     = 30;
  public static final int MAXFORMATTERS = 31;

  public static final int CHAR_SIZE   = 1;
  public static final int BYTE_SIZE   = 1;
  public static final int SHORT_SIZE  = 2;
  public static final int INT_SIZE    = 4;
  public static final int LONG_SIZE   = 4;
  public static final int FLOAT_SIZE  = 4;
  public static final int DOUBLE_SIZE = 8;
  public static final int PTR_SIZE    = 4;

  public  native static char formatGetChar (int buffer);
  public  native static void formatPutChar (int buffer, char theChar);
  private native static boolean formatGetBoolean (int buffer);
  private native static void formatPutBoolean (int buffer, boolean theBoolean);
  private native static byte formatGetByte (int buffer);
  private native static void formatPutByte (int buffer, byte theByte);
  private native static byte formatGetUByte (int buffer);
  private native static void formatPutUByte (int buffer, byte theByte);
  private native static short formatGetShort (int buffer);
  private native static void formatPutShort (int buffer, short theShort);
  private native static short formatGetUShort (int buffer);
  private native static void formatPutUShort (int buffer, short theShort);
  public  native static int formatGetInt (int buffer);
  public  native static void formatPutInt (int buffer, int theInt);
  private native static int formatGetUInt (int buffer);
  private native static void formatPutUInt (int buffer, int theInt);
  private native static long formatGetLong (int buffer);
  private native static void formatPutLong (int buffer, long theLong);
  private native static float formatGetFloat (int buffer);
  private native static void formatPutFloat (int buffer, float theFloat);
  private native static double formatGetDouble (int buffer);
  private native static void formatPutDouble (int buffer, double theDouble);
  private native static String formatGetString (int buffer);
  private native static void formatPutString (int buffer, String theString);

  private static Field getNthField (Object object, int n, Class theClass) 
    throws Exception {
    Field field = object.getClass().getFields()[n];
    if (theClass != null && field.getType() != theClass) {
      throw new Exception("Data structure does not match format"+
			  " --  Should be "+theClass+"; is "+field.getType());
    } else {
      return field;
    }
  }

  private static void setCharField (Object object, int n, char theChar)
    throws Exception {
    Field f = getNthField(object, n, char.class);
    f.setAccessible(true);
    f.setChar(object, theChar); 
  }

  private static char getCharField (Object object, int n) throws Exception {
    Field f = getNthField(object, n, char.class);
    return f.getChar(object); 
  }

  private static void setBooleanField (Object object, int n,
				       boolean theBoolean) throws Exception {
    Field f = getNthField(object, n, boolean.class);
    f.setAccessible(true);
    f.setBoolean(object, theBoolean); 
  }

  private static boolean getBooleanField (Object object,
					  int n) throws Exception {
    Field f = getNthField(object, n, boolean.class);
    return f.getBoolean(object); 
  }

  private static void setByteField (Object object, int n, byte theByte) 
    throws Exception {
    Field f = getNthField(object, n, byte.class);
    f.setAccessible(true);
    f.setByte(object, theByte); 
  }

  private static byte getByteField (Object object, int n) throws Exception {
    Field f = getNthField(object, n, byte.class);
    return f.getByte(object); 
  }

  private static void setIntField (Object object, int n, int theInt)
    throws Exception {
    Field f = getNthField(object, n, int.class);
    f.setAccessible(true);
    f.setInt(object, theInt); 
  }

  public static int getIntField (Object object, int n) throws Exception {
    Field f = getNthField(object, n, int.class);
    return f.getInt(object); 
  }

  private static void setShortField (Object object, int n, short theShort)
    throws Exception {
    Field f = getNthField(object, n, short.class);
    f.setAccessible(true);
    f.setShort(object, theShort); 
  }

  private static short getShortField (Object object, int n) throws Exception {
    Field f = getNthField(object, n, short.class);
    return f.getShort(object); 
  }

  private static void setLongField (Object object, int n, long theLong)
    throws Exception {
    Field f = getNthField(object, n, long.class);
    f.setAccessible(true);
    f.setLong(object, theLong); 
  }

  private static long getLongField (Object object, int n) throws Exception {
    Field f = getNthField(object, n, long.class);
    return f.getLong(object); 
  }

  private static void setFloatField (Object object, int n, float theFloat)
    throws Exception {
    Field f = getNthField(object, n, float.class);
    f.setAccessible(true);
    f.setFloat(object, theFloat); 
  }

  private static float getFloatField (Object object, int n) throws Exception {
    Field f = getNthField(object, n, float.class);
    return f.getFloat(object); 
  }

  private static void setDoubleField (Object object, int n, double theDouble)
    throws Exception {
    Field f = getNthField(object, n, double.class);
    f.setAccessible(true);
    f.setDouble(object, theDouble); 
  }

  private static double getDoubleField (Object object, int n) throws Exception{
    Field f = getNthField(object, n, double.class);
    return f.getDouble(object); 
  }

  private static void setStringField (Object object, int n, String theString)
    throws Exception {
    Field f = getNthField(object, n, String.class);
    f.setAccessible(true);
    f.set(object, theString); 
  }

  private static String getStringField (Object object, int n) throws Exception{
    Field f = getNthField(object, n, String.class);
    return (String)f.get(object); 
  }

  public static void setObjectField (Object object, int n, Object theObject)
    throws Exception {
    Field f = getNthField(object, n, null);
    if (Object.class.isAssignableFrom(f.getType())) {
	f.setAccessible(true);
      f.set(object, theObject); 
    } else {
      throw new Exception("Data structure does not match format"+
			  " --  Should be Object; is "+f.getType());
    }
  }

  public static Object getObjectField (Object object, int n) throws Exception{
    Field f = getNthField(object, n, null);
    if (Object.class.isAssignableFrom(f.getType()))
      return f.get(object); 
    else
      throw new Exception("Data structure does not match format"+
			  " --  Should be Object; is "+f.getType());
  }

  private static class TransFormat {
    TransFormat () { typeSize = 0; }
    TransFormat (int theTypeSize) { typeSize = theTypeSize; }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {}
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {}
    int ELength (Object dataStruct, int dstart) throws Exception {
	return typeSize;
    }
    int ALength () { return 1; }
    boolean SimpleType () { return true; }
    void EncodeElement (Object array, int index, 
			int buffer) throws Exception {}
    void DecodeElement (Object array, int index, int buffer) {}

    private int typeSize;
  }

  private static class STR_Trans extends TransFormat {
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutString(buffer, getStringField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setStringField(dataStruct, dstart, formatGetString(buffer));
    }
    int ELength (Object dataStruct, int dstart) throws Exception { 
	/* One int for the size, plus the number of characters */
      return INT_SIZE + getStringField(dataStruct, dstart).length();
    }
    boolean SimpleType () { return false; }
    void EncodeElement(int type, Object array, int index, int buffer) {
      formatPutString(buffer, ((String[])array)[index]);
    }
    void DecodeElement(int type, Object array, int index, int buffer) {
      ((String[])array)[index] = formatGetString(buffer);
    }
  }

  private static class BYTE_Trans extends TransFormat {
    BYTE_Trans() { super(BYTE_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutByte(buffer, getByteField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setByteField(dataStruct, dstart, formatGetByte(buffer));
    }
    void EncodeElement(int type, Object array, int index, int buffer) {
      formatPutByte(buffer, ((byte[])array)[index]);
    }
    void DecodeElement(int type, Object array, int index, int buffer) {
      ((byte[])array)[index] = formatGetByte(buffer);
    }
  }

  private static class UBYTE_Trans extends TransFormat {
    UBYTE_Trans() { super(BYTE_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutUByte(buffer, getByteField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setByteField(dataStruct, dstart, formatGetUByte(buffer));
    }
    void EncodeElement(int type, Object array, int index, int buffer) {
      formatPutUByte(buffer, ((byte[])array)[index]);
    }
    void DecodeElement(int type, Object array, int index, int buffer) {
      ((byte[])array)[index] = formatGetUByte(buffer);
    }
  }

  private static class SHORT_Trans extends TransFormat {
    SHORT_Trans() { super(SHORT_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutShort(buffer, getShortField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setShortField(dataStruct, dstart, formatGetShort(buffer));
    }
    void EncodeElement(int type, Object array, int index, int buffer) {
      formatPutShort(buffer, ((short[])array)[index]);
    }
    void DecodeElement(int type, Object array, int index, int buffer) {
      ((short[])array)[index] = formatGetShort(buffer);
    }
  }

  private static class INT_Trans extends TransFormat {
    INT_Trans() { super(INT_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutInt(buffer, getIntField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setIntField(dataStruct, dstart, formatGetInt(buffer));
    }
    void EncodeElement(Object array, int index, int buffer) {
      formatPutInt(buffer, ((int[])array)[index]);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((int[])array)[index] = formatGetInt(buffer);
    }
  }

  private static class CHAR_Trans extends TransFormat {
    CHAR_Trans() { super(CHAR_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutChar(buffer, getCharField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setCharField(dataStruct, dstart, formatGetChar(buffer));
    }
    void EncodeElement(Object array, int index, int buffer) {
      formatPutChar(buffer, ((char[])array)[index]);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((char[])array)[index] = formatGetChar(buffer);
    }
  }

  private static class FLOAT_Trans extends TransFormat {
    FLOAT_Trans() { super(FLOAT_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutFloat(buffer, getFloatField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setFloatField(dataStruct, dstart, formatGetFloat(buffer));
    }
    void EncodeElement(Object array, int index, int buffer) {
      formatPutFloat(buffer, ((float[])array)[index]);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((float[])array)[index] = formatGetFloat(buffer);
    }
  }

  private static class DOUBLE_Trans extends TransFormat {
    DOUBLE_Trans() { super(DOUBLE_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutDouble(buffer, getDoubleField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setDoubleField(dataStruct, dstart, formatGetDouble(buffer));
    }
    void EncodeElement(Object array, int index, int buffer) {
      formatPutDouble(buffer, ((double[])array)[index]);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((double[])array)[index] = formatGetDouble(buffer);
    }
  }

  private static class BOOLEAN_Trans extends TransFormat {
    BOOLEAN_Trans() { super(INT_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutBoolean(buffer, getBooleanField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setBooleanField(dataStruct, dstart, formatGetBoolean(buffer));
    }
    boolean SimpleType () { return false; }
    void EncodeElement(Object array, int index, int buffer) {
      formatPutBoolean(buffer, ((boolean[])array)[index]);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((boolean[])array)[index] = formatGetBoolean(buffer);
    }
  }

  private static class USHORT_Trans extends TransFormat {
    USHORT_Trans() { super(SHORT_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutUShort(buffer, getShortField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setShortField(dataStruct, dstart, formatGetUShort(buffer));
    }
    void EncodeElement(Object array, int index, int buffer) {
      formatPutUShort(buffer, ((short[])array)[index]);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((short[])array)[index] = formatGetUShort(buffer);
    }
  }

  private static class UINT_Trans extends TransFormat {
    UINT_Trans() { super(INT_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      formatPutUInt(buffer, getIntField(dataStruct, dstart)); 
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setIntField(dataStruct, dstart, formatGetUInt(buffer));
    }
    void EncodeElement(Object array, int index, int buffer) {
      formatPutUInt(buffer, ((int[])array)[index]);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((int[])array)[index] = formatGetUInt(buffer);
    }
  }

  private static class LONG_Trans extends TransFormat {
    LONG_Trans() { super(LONG_SIZE); }
    void Encode (Object dataStruct, int dstart, int buffer) throws Exception {
      long theLong = getLongField(dataStruct, dstart);
      if ((theLong > 0 && theLong > Integer.MAX_VALUE) ||
	  (theLong < 0 && theLong < Integer.MIN_VALUE)) {
	throw new Exception("Will lose precision in transferring long: "
			    + theLong);
      }
      formatPutLong(buffer, theLong);
    }
    void Decode (Object dataStruct, int dstart, int buffer) throws Exception {
      setLongField(dataStruct, dstart, formatGetLong(buffer));
    }
    void EncodeElement(Object array, int index, int buffer)
      throws Exception {
      long theLong = ((long[])array)[index];
      if ((theLong > 0 && theLong > Integer.MAX_VALUE) ||
	  (theLong < 0 && theLong < Integer.MIN_VALUE)) {
	throw new Exception("Will lose precision in transferring long: "
			    + theLong);
      }      
      formatPutLong(buffer, theLong);
    }
    void DecodeElement(Object array, int index, int buffer) {
      ((long[])array)[index] = formatGetLong(buffer);
    }
  }

  private static TransFormat strTrans = new STR_Trans();
  private static TransFormat byteTrans = new BYTE_Trans();
  private static TransFormat ubyteTrans = new UBYTE_Trans();
  private static TransFormat shortTrans = new SHORT_Trans();
  private static TransFormat intTrans = new INT_Trans();
  private static TransFormat charTrans = new CHAR_Trans();
  private static TransFormat floatTrans = new FLOAT_Trans();
  private static TransFormat doubleTrans = new DOUBLE_Trans();
  private static TransFormat booleanTrans = new BOOLEAN_Trans();
  private static TransFormat ushortTrans = new USHORT_Trans();
  private static TransFormat uintTrans = new UINT_Trans();
  private static TransFormat longTrans = new LONG_Trans();

  private static TransFormat pickTrans (int type) throws Exception {
    switch (type) {
    case STR_FMT: return strTrans;
    case BYTE_FMT: return byteTrans;
    case UBYTE_FMT: return ubyteTrans;
    case SHORT_FMT: return shortTrans;
    case INT_FMT: return intTrans;
    case CHAR_FMT: return charTrans;
    case FLOAT_FMT: return floatTrans;
    case DOUBLE_FMT: return doubleTrans;
    case BOOLEAN_FMT: return booleanTrans;
    case USHORT_FMT: return ushortTrans;
    case UINT_FMT: return uintTrans;
    case LONG_FMT: 
    case ULONG_FMT: return longTrans;
    default: throw new Exception("Unhandled format "+ type);
    }
  }

  public static void Encode(int type, Object dataStruct, int dstart,
			    int buffer) throws Exception {
    pickTrans(type).Encode(dataStruct, dstart, buffer);
  }

  public static void Decode(int type, Object dataStruct, int dstart, 
			    int buffer) throws Exception {
    pickTrans(type).Decode(dataStruct, dstart, buffer);
  }

  public static int ELength(int type, Object dataStruct,
			    int dstart) throws Exception{
    return pickTrans(type).ELength(dataStruct, dstart);
  }

  public static int ALength(int type) throws Exception { 
    return pickTrans(type).ALength();
  }

  public static boolean SimpleType(int type) throws Exception {
   return pickTrans(type).SimpleType();
  }

  public static void EncodeElement(int type, Object array, int index,
				   int buffer) throws Exception {
    pickTrans(type).EncodeElement(array, index, buffer);
  }

  public static void DecodeElement(int type, Object array, int index,
				   int buffer) throws Exception {
    pickTrans(type).DecodeElement(array, index, buffer);
  }
}
