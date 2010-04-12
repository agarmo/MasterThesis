* mex file example for g77
      subroutine mexfunction(nlhs, plhs, nrhs, prhs)
      integer plhs(nlhs), prhs(nrhs), ypp, tp, yp
* check for proper number of arguments
      if (nrhs .ne. 2) then
        call mexerrmsgtxt('yprime requires two input arguments')
      elseif (nlhs .gt. 1) then
        call mexerrmsgtxt('yprime requires one output argument')
      endif
* check the dimensions of y.  it can be 4 x 1 or 1 x 4.
      m = mxgetm(prhs(2))
      n = mxgetn(prhs(2))
      if ((max(m,n) .ne. 4) .or. (min(m,n) .ne. 1)) then
        call mexerrmsgtxt('yprime requires that y be a 4 x 1 vector')
      endif
* create a matrix for return argument
      plhs(1) = mxcreatedoublematrix(m,n,0)
* assign pointers to the various parameters
      ypp = mxgetpr(plhs(1))
      tp = mxgetpr(prhs(1))
      yp = mxgetpr(prhs(2))
* do the actual computations in a subroutine
      call yprime(%val(ypp),%val(tp),%val(yp))
      end
