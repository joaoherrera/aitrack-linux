#pragma once

class IFilter
{
  public:
    virtual void filter( float *in_array, float *out_array ) = 0;
    inline virtual ~IFilter(){};
};

class MAFilter : public IFilter
{
  private:
    int array_size;
    int idx;
    int n_steps;

    float *circular_buffer;

  public:
    MAFilter( int steps, int array_size );
    ~MAFilter();

    void filter( float *in_array, float *out_array );
};

class EAFilter : public IFilter
{
  private:
    int    array_size;
    float *last_value;

  public:
    EAFilter( int array_size );
    ~EAFilter();

    void filter( float *in_array, float *out_array );
};