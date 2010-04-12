! MEX FILE EXAMPLE FOR G95 AND GFORTRAN
!
! To turn on g95 mexing, compile, link and try out, use the Matlab commands:
!
!    >> gnumex g95
!    >> mex yprime95.f90
!    >> yprime95(1, 1:4)
!
! The answer "2.0000 8.9685 4.0000 -1.0947" should be displayed.
! To demonstrate how assumed shape arrays may be passed to a computation
! subroutine the test that yp has length 4 has been moved into yprime.

module yprime_mod ! test module for gnumex and g95
  use mexinterface
contains
  subroutine yprime(yp, t, y, error)
    implicit none
    double precision :: yp(:), t, y(:), mu, mus, r1, r2
    logical :: error
    intent(in)  :: y, t
    intent(out) :: yp, error
    !
    error = .true.
    if (size(yp) /= 4) return
    if (size(y) /= 4) return
    error = .false.
    mu = 1d0/82.45d0
    mus = 1d0 - mu
    r1 = sqrt((y(1)+mu)**2 + y(3)**2)
    r2 = sqrt((y(1)-mus)**2 + y(3)**2)
    yp(1) = y(2)
    yp(2) = 2*y(4)+y(1)-mus*(y(1)+mu)/(r1**3)-mu*(y(1)-mus)/(r2**3)
    yp(3) = y(4)
    yp(4) = -2*y(2)+y(3)-mus*y(3)/(r1**3)-mu*y(3)/(r2**3)
  end subroutine yprime
end module yprime_mod

subroutine mexfunction(nlhs, plhs, nrhs, prhs)
  use yprime_mod
  implicit none
  integer :: nlhs, nrhs, plhs(nlhs), prhs(nrhs), m, n, nn
  double precision, pointer :: tp, yp(:), ypp(:)
  logical error
  !
  if (nrhs /= 2) call mexerrmsgtxt('yprime requires two input arguments')
  if (nlhs > 1) call mexerrmsgtxt('yprime requires one output argument')
  call mexprintf('abcd'//char(10))
  call mexprintf('RSTU')
  m = mxgetm(prhs(2))
  n = mxgetn(prhs(2))
  if (min(m,n) /= 1) call mexerrmsgtxt('y must be a vector'//char(0))
  plhs(1) = mxcreatedoublematrix(m,n,0)   ! create a matrix for return argument
  call c_f_pointer(mxgetpr(prhs(1)), tp)  ! assign pointers to parameters
  call c_f_pointer(mxgetpr(prhs(2)), yp, [max(n,m)])
  call c_f_pointer(mxgetpr(plhs(1)), ypp, [max(n,m)])
  call yprime(ypp, tp, yp, error)
  if (error) call mexerrmsgtxt('y must contain 4 elements'//char(0))
end subroutine mexfunction
