module powerit_mod
contains
  subroutine powerit(a, ir, jc, x, nit, lambda)
    ! Carries out power iteration to find the dominant eigenvalue of an nxn matrix A
    implicit none
    integer, parameter :: dp = kind(0d0)
    real(dp), intent(in)    :: a(0:)  ! nonzero elements of an nxn sparse matrix A
    real(dp), intent(inout) :: x(0:)  ! n-vector (full) to iterate with
    integer, intent(in)     :: ir(0:) ! the i-th element of a is in the ir(i)-th row of A
    integer, intent(in)     :: jc(0:) ! the j-th column of A starts in a(jc(i))
    integer, intent(in)     :: nit    ! the number of iterations to do
    real(dp), intent(out)   :: lambda ! returns estimate of dominant eigenvalue
    !                                 ! jc(n+1) = nz, the number of nonzero elements
    !                                 ! a and ir are of the same length, nzmax
    !                                 ! but only the first nz elements are used.
    real(dp) :: Ax(0:size(x)-1), norm
    integer  :: n, iter, j, kb, ke, i, nz
    !
    n = size(x)
    nz = jc(n)
    Ax = x
    do iter = 1, nit
      norm = sqrt(dot_product(Ax,Ax));
      x = Ax*(1/norm);
      Ax = 0d0
      do j=0,n-1
        kb = jc(j)
        ke = jc(j+1) - 1
        do i = kb, ke
          Ax(ir(i)) = Ax(ir(i)) + a(i)*x(j)
        enddo
        ! Unfortunately the following statement, which is equivalent to the previous
        ! do loop, takes 5-6 times as long to execute both with g95 and gfortran
        ! (using -O3 switch), gfortran being about 30% faster (KJ, Dec. 2007).
        ! Ax(ir(kb:ke)) = Ax(ir(kb:ke)) + a(kb:ke)*x(j)
      enddo
    enddo
    lambda = dot_product(x, Ax)
  end subroutine powerit
end module powerit_mod

subroutine mexFunction(nlhs, plhs, nrhs, prhs)

  ! DESCRIPTION
  !   Demonstration of calling Fortran 95 from Matlab. This example shows how a
  !   sparse matrix may be passed from Matlab to Fortran. It also shows how to
  !   call with integer and optional arguments. The reason for having both a
  !   3 argument and 5 argument possible calls is simply to demonstrate these
  !   possibilities.
  !
  ! USAGE
  !   LAMBDA = POWERIT(X, NIT, A) where A is an n by n sparse symmetric matrix
  !   and X is an n-vector. The vector X is multiplied repeatedly with A (and
  !   normalized in-between), a total of NIT times, and finally the dominant
  !   eigenvalue of A is estimated with the the Rayleigh quotient, x'*A*x/(x'*x)
  !   where x is the NIT-th x value.
  !
  !   LAMBDA = POWERIT(X, NIT, AV, IR, JC) assumes that AV is a vector with the
  !   nonzero elements of the sparse matrix, column by column, IR(i) is the
  !   row number of the i-th element, and JC(j) is the total number of elements
  !   before column j (j=0,1,...,n). AV, IR and JC are zero-based as described
  !   e.g. in the Matlab help on mxSetIr. IR and JC should be of type uint32.
  !
  ! EXAMPLE CALLS
  !   n = 1024
  !   a = gallery('neumann',n);
  !   x = [1;zeros(n-1,1)];
  !   powerit(x, 1000, a)
  !
  !   % Iterate with the matrix [1 0 0 1; 0 2 0 0; 0 0 3 0; 1 0 0 4]
  !   av =        [1 1 2 3 1 4];
  !   ir = uint32([0 3 1 2 0 3]);
  !   jc = uint32([0 2 3 4 6]);
  !   x = [1 1 1 1]';
  !   powerit(x, 10, av, ir, jc)
  !   (should give the answer 4.3015).
  
  use mexinterface
  use powerit_mod
  implicit none
  integer, parameter :: dp = kind(0d0)
  real(dp) :: pr(:), x(:), nit, lambda, y(:)
  integer  :: nlhs, nrhs, plhs(nlhs), prhs(nrhs), ir(:), jc(:), nzmax &
       ,      m1, m2, m3, n1, n2, n3, n, nz
  logical  :: ok
  pointer lambda, x, nit, pr, ir, jc
  allocatable y

  call assert(nrhs == 3 .or. nrhs == 5, 'Powerit has 3 or 5 parameters')
  m1 = mxGetM(prhs(1)); n1 = mxGetN(prhs(1))
  m2 = mxGetM(prhs(2)); n2 = mxGetN(prhs(2))
  m3 = mxGetM(prhs(3)); n3 = mxGetN(prhs(3))
  call assert(min(m1, n1) == 1, 'x must be a vector')
  call assert(m2 == 1 .and. n2 == 1, 'nit must be scalar')
  n = max(m1, n1)
  plhs(1) = mxCreateDoubleMatrix(1, 1, 0)
  call c_f_pointer( mxGetPr   (prhs(1)), x,     [n] )
  call c_f_pointer( mxGetPr   (prhs(2)), nit        )
  call c_f_pointer( mxGetPr   (plhs(1)), lambda     )
  if (nrhs == 3) then
    ok = mxIsSparse(prhs(3)) == 1
    call assert(ok, 'A must be sparse')
    call assert(n3==n .and. m3==n, 'A must be nxn and x must be an n-vector')
    nzmax = mxGetNzmax(prhs(3))
    call c_f_pointer( mxGetPr   (prhs(3)), pr, [nzmax] )
    call c_f_pointer( mxGetIr   (prhs(3)), ir, [nzmax] )
    call c_f_pointer( mxGetJc   (prhs(3)), jc, [n+1]   )
  else ! nrhs == 5
    ok = mxIsSparse(prhs(3)) == 0
    call assert(ok, 'AV must not be sparse')
    call assert(min(n3, m3) == 1, 'AV must be a vector')
    nzmax = max(n3, m3);
    call c_f_pointer( mxGetPr   (prhs(3)), pr, [nzmax] )
    call c_f_pointer( mxGetData (prhs(4)), ir, [nzmax] )
    call c_f_pointer( mxGetData (prhs(5)), jc, [n+1]   )
  endif
  nz = jc(n)
  allocate(y(n))
  y = x
  call powerit(pr, ir, jc, y, nint(nit), lambda)

contains
  
  subroutine assert(s, msg)
    logical s
    character(*) :: msg
    if (.not.s) call mexErrMsgTxt(msg)
  end subroutine assert
  
end subroutine mexFunction
