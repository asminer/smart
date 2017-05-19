
/*! \file rng.h
    Random Number Generator Library.
*/

#ifndef RNG_H
#define RNG_H


/** 
  Interface for a random number generation stream.
*/
class rng_stream {
public:
  rng_stream();
  virtual ~rng_stream();

  /** Return a Uniform(0,1) random value, with 32 bits of "resolution".
        
        @return  x / 2^{32},  where x is "randomly chosen"
                              between 1 and 2^{32}-1.
  */
  virtual double Uniform32() = 0;

  /** Return a Uniform(0,1) random value, with 64 bits of "resolution".

        @return  x / 2^{64},  where x is "randomly chosen"
                              between 1 and 2^{64}-1.
  */
  virtual double Uniform64() = 0;

  /** Return a "raw" random value.
      Used primarily for testing the generator.
        @return  32 "random" bits.
  */
  virtual unsigned int RandomWord() = 0;
};


/** Class for managing several RNG streams.
*/
class rng_manager {
public:
  rng_manager();
  virtual ~rng_manager();

  /** Create a new "blank" RNG stream.
      Output of the stream will be all zeroes
      until it is initialized using either
      InitStreamFromSeed() or
      InitStreamByJumping().
  */
  virtual rng_stream* NewBlankStream() = 0;

  /** Initialize an existing stream using the given "seed".
        @param  s     The stream to initialize.
        @param  seed  The seed value.
  */
  virtual void InitStreamFromSeed(rng_stream* s, int seed) = 0;

  /** Initialize an existing stream by "jumping ahead" from another stream.
      The jumping distance is determined by GetJumpValue().
        @param  s  The stream to initialize.
        @param  j  The stream to jump from.
  */
  virtual void InitStreamByJumping(rng_stream* s, const rng_stream* j) = 0;

  /** Create and return a new RNG stream, 
      whose state is initialized using the given "seed".
      Equivalent to calling NewBlankStream() and 
      then calling InitStreamFromSeed().
        @param  seed  The seed value.
        @return       New RNG stream on success, or 0 on failure.
  */
  virtual rng_stream* NewStreamFromSeed(int seed) = 0;

  /** Create and return a new RNG stream,
      whose state is initialized by "jumping ahead" from another stream.
      Equivalent to calling NewBlankStream() and 
      then calling InitStreamByJumping().

        @param  j   The stream to jump from.
        @return     New RNG stream on success, or 0 on failure.
  */
  virtual rng_stream* NewStreamByJumping(const rng_stream* j) = 0;

  /** Set the jump value.
        @param  d   Desired jump value.
                    If it is not between RNG_MinimumJumpValue() and
                    RNG_MaximumJumpValue(), then the current jump value 
                    is not changed.

        @return  true  iff d is a legal jump value.
  */
  virtual bool SetJumpValue(int d) = 0;

  /** Obtain the current jump value.
  
        @return   Jump value d. This means the stream initialized 
                  by NewStreamByJumping() will have distance of 
                  (roughly) 2^d from the other stream.
  */
  virtual int GetJumpValue() = 0;

  /// Determine the minimum allowed jump value.
  virtual int  MinimumJumpValue() = 0;

  /// Determine the maximum allowed jump value.
  virtual int  MaximumJumpValue() = 0;

  /** Get the name and version info of the manager.
      The string should not be modified or deleted.
        @return    Information string.
  */
  virtual const char*  GetVersion() const = 0;
};

/**
  Build a new stream manager.
  
  TBD: any parameters?
*/
rng_manager* RNG_MakeStreamManager();

#endif

