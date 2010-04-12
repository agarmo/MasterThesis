* yprime test routine for gnumex and g77
      subroutine yprime(yp, t, y)
      double precision yp(4), t, y(4), mu, mus, r1, r2
      mu = 1d0/82.45d0
      mus = 1d0 - mu
      r1 = sqrt((y(1)+mu)**2 + y(3)**2)
      r2 = sqrt((y(1)-mus)**2 + y(3)**2)
      yp(1) = y(2)
      yp(2) = 2*y(4)+y(1)-mus*(y(1)+mu)/(r1**3)-mu*(y(1)-mus)/(r2**3)
      yp(3) = y(4)
      yp(4) = -2*y(2)+y(3)-mus*y(3)/(r1**3)-mu*y(3)/(r2**3)
      end
