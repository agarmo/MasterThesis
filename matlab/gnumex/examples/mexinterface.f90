module mexinterface
  use, intrinsic :: iso_c_binding

  interface

    function mxgetpr(pm)
      import c_ptr
      integer :: pm
      type(c_ptr) :: mxgetpr
    end function mxgetpr

    function mxgetdata(pm)
      import c_ptr
      integer :: pm
      type(c_ptr) :: mxgetdata
    end function mxgetdata

    function mxgetm(pm)
      integer :: pm, mxgetm
    end function mxgetm

    function mxgetn(pm)
      integer pm, mxgetn
    end function mxgetn

    function mxcreatedoublematrix(m, n, type)
      integer m, n, type, mxcreatedoublematrix
    end function mxcreatedoublematrix

    subroutine mexerrmsgtxt(s)
      character(*) s
    end subroutine mexerrmsgtxt

    subroutine mexprintf(s)
      character(*) s
    end subroutine mexprintf

    function mxgetir(pm)
      import c_ptr
      integer :: pm
      type(c_ptr) :: mxgetir
    end function mxgetir

    function mxgetjc(pm)
      import c_ptr
      integer :: pm
      type(c_ptr) :: mxgetjc
    end function mxgetjc

    function mxgetnzmax(pm)
      integer :: pm, mxgetnzmax
    end function mxgetnzmax

    function mxissparse(pm)
      integer pm
      logical mxissparse
    end function mxissparse

  end interface
end module mexinterface
