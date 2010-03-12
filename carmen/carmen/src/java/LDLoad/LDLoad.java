package LDLoad;

import java.util.*;
import java.util.zip.*;
import java.io.*;

/* The idea and implementation of this came from Ed Olson and the MasLab
   codebase. */

public class LDLoad {

  static public boolean loadFromJar(String jarFilename, String soFilename)
  {    
    //The jar should be in the classpath.  Look for it there.
    String cp = System.getProperty("java.class.path");
    String[] items = cp.split(":");
    for (int i = 0; i < items.length; i++) {
      if (items[i].endsWith(jarFilename) && (new File(items[i])).exists()) {
	try{
	  int size = 0;
	  boolean found = false;

	  ZipFile zf=new ZipFile(items[i]);
	  Enumeration e=zf.entries();
	  while (e.hasMoreElements()) {
            ZipEntry ze=(ZipEntry)e.nextElement();
	    if (ze.getName().equals(soFilename)) {
	      size = (int)ze.getSize();
	      found = true;
	      break;
	    }
	  }
	  zf.close();

	  if (!found)
	    throw new RuntimeException(jarFilename+" does not contain .so "+soFilename);

	  FileInputStream fis = new FileInputStream(items[i]);
	  BufferedInputStream bis = new BufferedInputStream(fis);
	  ZipInputStream zis = new ZipInputStream(bis);

	  File soFile = new File(soFilename);
	  
	  File output = File.createTempFile("lib", soFile.getName());
	  FileOutputStream fos = new FileOutputStream(output);
	  ZipEntry ze = null;
	  while ((ze = zis.getNextEntry()) != null) {
	    if (ze.getName().equals(soFilename)) {
	      int count = 0;
	      byte[] result = new byte[size];
	      while (count < size)
		count += zis.read(result,count,size-count);
	      fos.write(result);
	      break;
	    }
	  }
	  fos.close();

	  System.load(output.toString());
	  output.delete();
	  return true;
	}
	catch(IOException ioe){
	  System.err.println(ioe);
	}
      }
    }
    throw new RuntimeException("Unable to load native library "+soFilename+
			       " from "+jarFilename);
  }
}
