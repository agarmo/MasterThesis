module mexinterface
  use, intrinsic :: ISO_C_BINDING
  interface
    function mxgetpr(pm) bind(c, name = 'MXGETPR')
      import c_int, c_ptr
      integer(c_int) :: pm
      type(c_ptr) mxgetpr
    end function mxgetpr
    !
    function mxgetm(pm) bind(c, name = 'MXGETM')
      import c_int
      integer(c_int) :: pm, mxgetm
    end function mxgetm
    !
    function mxgetn(pm) bind(c, name = 'MXGETN')
      import c_int
      integer(c_int) pm, mxgetn
    end function mxgetn
    !
    function mxgetdata(pm) bind(c, name = 'MXGETDATA')
      import c_int, c_ptr
      integer(c_int) :: pm
      type(c_ptr) mxgetdata
    end function mxgetdata
    !
    function mxcreatedoublematrix(m,n,type) bind(c, name = 'MXCREATEDOUBLEMATRIX')
      import c_int
      integer(c_int) m, n, type, mxcreatedoublematrix
    end function mxcreatedoublematrix
    !
    subroutine mexerrmsgtxt(s) bind(C, name = 'mexErrMsgTxt')
      import c_char
      character(c_char) s(*)
    end subroutine mexerrmsgtxt
    !
    subroutine mexprintf(s) bind(C, name = 'mexPrintf')
      import c_char
      character(c_char) s(*)
    end subroutine mexprintf
    !
    function mxgetir(pm) bind(C, name = 'MXGETIR')
      import c_int, c_ptr
      integer(c_int) :: pm
      type(c_ptr) :: mxgetir
    end function mxgetir

    function mxgetjc(pm) bind(C, name = 'MXGETJC')
      import c_int, c_ptr
      integer(c_int) :: pm
      type(c_ptr) :: mxgetjc
    end function mxgetjc

    function mxgetnzmax(pm) bind(C, name = 'MXGETNZMAX')
      import c_int
      integer(c_int) :: pm, mxgetnzmax
    end function mxgetnzmax

    function mxissparse(pm) bind(C, name = 'MXISSPARSE')
      import c_int
      integer(c_int) :: pm, mxissparse
    end function mxissparse

  end interface
end module mexinterface
