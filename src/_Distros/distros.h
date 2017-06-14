
/*

  Classes for discrete distributions.

*/

#ifndef DISTROS_H
#define DISTROS_H

// ******************************************************************
// *                                                                *
// *                     discrete_pdf  class                        *
// *                                                                *
// ******************************************************************

/**
  PDF of a discrete random variable.
  Stored explicitly,
  so works only for finite distributions.
*/
class discrete_pdf {
  public:
    discrete_pdf();
    discrete_pdf(const discrete_pdf &p);
    /**
      Constructor.
        @param  L         Left truncation point
        @param  R         Right truncation point
        @param  shifted   shifted_f probabilities.
    */
    discrete_pdf(long L, long R, double* shifted);

    ~discrete_pdf();

    void reset(long L, long R, double* shifted);

    /**
        Left truncation point L.
        The probability that the RV takes a value
        less than L is negligible.
    */
    inline long left_trunc() const {
      return Left;
    }

    /**
        Right truncation point R.
        The probability that the RV takes a value
        greater than R is negligible.
    */
    inline long right_trunc() const {
      return Right;
    }

    /**
        Probability mass function.
          @param  i   Desired value
          @return     The probability that the RV takes value i.
    */
    double f(long i) const { 
      if (i<Left) return 0;
      if (i>Right) return 0;
      return probs[i];
    }

    /**
        Probability mass function, shifted.
          @param  i   Desired value
          @return     The probability that the RV takes value L+i.
    */
    double shifted_f(long i) const {
      if (i<0) return 0;
      if (i>Right+Left) return 0;
      return probs_shifted[i];
    }

  private:
    double* probs_shifted;
    double* probs;
    long Left;
    long Right;

};

// ******************************************************************
// *                                                                *
// *                  Some  handy distributions                     *
// *                                                                *
// ******************************************************************

/**
    Compute the poisson PDF.
    The PDF is truncated, so that
    Prob{L <= X <= R} >= 1-epsilon.

    @param  lambda    Poisson parameter.
    @param  epsilon   Precision.
    @param  P         Output: distribution for poisson(lambda).

*/
void computePoissonPDF(double lambda, double epsilon, discrete_pdf &P);

// TBD - other finite discrete distributions?  Binomial, perhaps?


#endif
