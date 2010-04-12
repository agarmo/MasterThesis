/* uigetpath */
/* to compile with lcc:
     to make sure lcc is used delete mexopts.bat in the prefdir folder
     (>> delete ([prefdir '\mexopts.bat'])). Then do:
     >> mex uigetpath.c shell32.lib user32.lib
*/

#include <windows.h>
#include <shlobj.h>
#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

  /* browse for directory path name */
  /* arg (if defined) is prompt shown at top of dialog box */
  int len, okf = FALSE;
  char strbuf[MAX_PATH];
  char *title = "Choose directory";

  BROWSEINFO bi;
  LPITEMIDLIST pidl;        /* PIDL for folder selected */
  LPMALLOC lpMalloc;

  if (nlhs < 1)
    mexErrMsgTxt("Expecting to return a file path");

  if (nrhs > 0) {
    len = mxGetN(prhs[0]);
    title = (char *)mxCalloc(len+1, sizeof(char));
    mxGetString(prhs[0],title,len+1);
  } 

  /* deep stuff.  The itemidlists are memory structures allocated
     by the shell, and thus must be freed by the shell using LPMALLOC */
  if (SHGetMalloc(&lpMalloc) != NOERROR){
    if (nrhs > 0)
      mxFree(title);
    mexErrMsgTxt("Failed to get Malloc");
  }

  /* browseinfo structure */
  bi.hwndOwner = NULL;    
  /* start browse at desktop
     it's surprisingly hard to start anywhere else */        
  bi.pidlRoot = NULL;
  bi.pszDisplayName = strbuf;
  bi.lpszTitle = title;
  bi.ulFlags = BIF_RETURNONLYFSDIRS;
  bi.lpfn = NULL;
  bi.lParam = 0; 

  if ((pidl = SHBrowseForFolder(&bi))!= NULL) {
    okf = SHGetPathFromIDList(pidl, strbuf);
    lpMalloc->lpVtbl->Free(lpMalloc, pidl); 
  }
  if (!okf)
    strbuf[0] = 0;
  plhs[0]=mxCreateString(strbuf);

  if (nrhs > 0)
    mxFree(title);
  lpMalloc->lpVtbl->Release(lpMalloc);
}


