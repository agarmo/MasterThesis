c Gnumex file to test Matlab engine on Windows with g77
c (based on the Matlab distributed fengdemo.f)                                                                    fengdemo.f)
      program main
      integer engOpen, engGetVariable, mxCreateDoubleMatrix
      integer engPutVariable, engEvalString, mxGetPr
      integer ep, T, D, status
      double precision time(10), dist(10)
      data time / 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 /
c      
      ep = engOpen('matlab ')
      if (ep .eq. 0) stop 'Error with engOpen'
      T = mxCreateDoubleMatrix(1, 10, 0)
      call mxCopyReal8ToPtr(time, mxGetPr(T), 10)
      status = engPutVariable(ep, 'T', T)
      status = engEvalString(ep, 'D = 0.5.*(-9.8).*T.^2;')
      status = engEvalString(ep, 'plot(T,D);')
      status = engEvalString(ep, 'title(''Position vs. Time'')')
      status = engEvalString(ep, 'xlabel(''Time (seconds)'')')
      status = engEvalString(ep, 'ylabel(''Position (meters)'')')
      call showint('status =', status, ep)
      D = engGetVariable(ep, 'D')
      call mxCopyPtrToReal8(mxGetPr(D), dist, 10)
      call showdouble('dist(10) =', dist(10), ep)
      call message('fengdemo finished', ep)
      call engClose(ep)
      call mxDestroyArray(T)
      call mxDestroyArray(D)
      end

      subroutine message(s, ep)
      integer ep, status, engEvalString, n
      character s*(*), cmd*100
      n = len(s)
      cmd = 'uiwait(msgbox(''' // s // ''', ''modal''))'
      status = engEvalString(ep, cmd)
      if (status.ne.0) then
        call engClose(ep)
        stop
      endif
      end

      subroutine showint(s, i, ep)
      integer i, ep
      character s*(*), cmd*100
      write(cmd, '(a,i5)') s, i
      call message(cmd(1:len(s)+5), ep)
      end
 
      subroutine showdouble(s, d, ep)
      integer ep
      double precision d
      character s*(*), cmd*100
      write(cmd, '(a,f8.2)') s, d
      call message(cmd(1:len(s)+15), ep)
      end