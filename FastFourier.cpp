/*==============================================================================
  Copyright (c) 2020 - Fielding DSP GmbH

  Created - 19. October 2018
==============================================================================*/
#define FASTFOURIER_DLLBUILD 1
#include "FastFourier.h"

#include <vector>
#include <cassert>

#if __APPLE__
 #define USE_INTEL_FFT 0
 #define USE_NE10_FFT  0
 #define USE_APPLE_FFT 1
#elif (defined(__i386__) || defined(__x86_64__) || defined(_WIN32) || defined(_WIN64))
 #define USE_INTEL_FFT 1
 #define USE_NE10_FFT  0
 #define USE_APPLE_FFT 0
#elif (defined(__arm__) || defined(__aarch64__) || defined(_ARM) || defined(_M_ARM) || defined(_M_ARMT))
 #define USE_INTEL_FFT 0
 #define USE_NE10_FFT  1
 #define USE_APPLE_FFT 0
#endif

//==============================================================================
//==============================================================================
#if USE_INTEL_FFT
  #if defined(_WIN32) || defined (_WIN64)
    #define IPP_STDCALL __stdcall
  #else
    #define IPP_STDCALL
  #endif

  struct IppsDFTSpec_R_32f;

  extern "C"
  {
    int IPP_STDCALL ippsDFTGetSize_R_32f(int, int, int, int*, int*, int*);
    int IPP_STDCALL ippsDFTInit_R_32f(int, int, int, unsigned char*, unsigned char*);
    int IPP_STDCALL ippsDFTFwd_RToCCS_32f(const float*, std::complex<float>*, const unsigned char*, unsigned char*);
    int IPP_STDCALL ippsDFTInv_CCSToR_32f(const std::complex<float>*, float*, const unsigned char*, unsigned char*);
  }
#endif

#if USE_NE10_FFT
extern "C"
{
    int ne10_init();
    int ne10_HasNEON();
    void* ne10_fft_alloc_r2c_float32(int);
    void ne10_fft_destroy_r2c_float32(void*);
    void ne10_fft_r2c_1d_float32_neon(std::complex<float>*, const float*, void*);
    void ne10_fft_c2r_1d_float32_neon(float*, const std::complex<float>*, void*);
}
#endif

//==============================================================================
//==============================================================================
namespace
{
#if USE_INTEL_FFT
struct Pimpl : public FastFourier::Intf
{
    Pimpl(size_t size) : n(static_cast<int> (size))
    {
        assert((size & (size - 1)) == 0);

        int specSize = 0, initSize = 0, workBufSize = 0;

        ippsDFTGetSize_R_32f(n, /*IPP_FFT_NODIV_BY_ANY*/ 8, 0, &specSize, &initSize, &workBufSize);

        std::vector<std::uint8_t> initWorkBuffer(initSize);
        scratchBuffer.resize(workBufSize);
        intelFFT.resize(specSize);

        ippsDFTInit_R_32f(n, /*IPP_FFT_NODIV_BY_ANY*/ 8, 0, intelFFT.data(), initWorkBuffer.data());
    }

    virtual ~Pimpl() = default;

    void release() override
    {
        delete this;
    }

    void forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept override
    {
        ippsDFTFwd_RToCCS_32f(realInput, cmplxOutput, intelFFT.data(), scratchBuffer.data());
    }

    void inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept override
    {
        ippsDFTInv_CCSToR_32f(cmplxInput, realOutput, intelFFT.data(), scratchBuffer.data());
    }

    float getNormalizationFactor() const noexcept override
    {
        return static_cast<float>(n);
    }

    //==============================================================================
    Pimpl(const Pimpl&) = delete;
    Pimpl(Pimpl&&) = delete;
    Pimpl& operator=(const Pimpl&) = delete;
    Pimpl& operator=(Pimpl&&) = delete;

    //==============================================================================
    int n;
    std::vector<std::uint8_t> scratchBuffer;
    std::vector<std::uint8_t> intelFFT;
};

//==============================================================================
#elif USE_NE10_FFT

struct Pimpl : public FastFourier::Intf
{
    Pimpl(size_t size) : n(static_cast<int> (size))
    {
        assert((size & (size - 1)) == 0);

        static bool neonWasInitialised = false;

        if (! neonWasInitialised)
        {
            neonWasInitialised = true;
            ne10_init();

            // you need neon to execute the ffts
            assert(ne10_HasNEON() == 0);
        }

        
        ne10Fft = ne10_fft_alloc_r2c_float32(n);
    }

    virtual ~Pimpl()
    {
        if (ne10Fft != nullptr)
        {
            ne10_fft_destroy_r2c_float32(ne10Fft);
            ne10Fft = nullptr;
        }
    }

    void release() override
    {
        delete this;
    }

    void forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept override
    {
        ne10_fft_r2c_1d_float32_neon(cmplxOutput, realInput, ne10Fft);
    }

    void inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept override
    {
        ne10_fft_c2r_1d_float32_neon(realOutput, cmplxInput, ne10Fft);
    }

    float getNormalizationFactor() const noexcept override
    {
        return 1.0f;
    }

    //==============================================================================
    Pimpl(const Pimpl&) = delete;
    Pimpl(Pimpl&&) = delete;
    Pimpl& operator=(const Pimpl&) = delete;
    Pimpl& operator=(Pimpl&&) = delete;

    //==============================================================================
    int n;
    void* ne10Fft;
};

//==============================================================================
#elif USE_APPLE_FFT
#include <Accelerate/Accelerate.h>

struct Pimpl : public FastFourier::Intf
{
    Pimpl(size_t size)
        : order(static_cast<int>(std::log2(size))),
          fftApple(vDSP_create_fftsetup(order, 2))
    {
        assert((size & (size - 1)) == 0);
    }
    
    virtual ~Pimpl()
    {
        if (fftApple != nullptr)
        {
            vDSP_destroy_fftsetup(fftApple);
            fftApple = nullptr;
        }
    }

    void release() override
    {
        delete this;
    }
    
    void forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept override
    {
        auto numComplex = ((1 << order) >> 1) + 1;
        DSPSplitComplex input  { const_cast<float*>(realInput), const_cast<float*>(realInput) + 1};
        DSPSplitComplex output { reinterpret_cast<float*>(cmplxOutput), reinterpret_cast<float*>(cmplxOutput) + 1};
        
        vDSP_fft_zrop(fftApple, &input, 2, &output, 2, order, kFFTDirection_Forward);
        
        cmplxOutput[numComplex-1] = std::complex<float>(cmplxOutput[0].imag(), 0.0f);
        cmplxOutput[0] = std::complex<float>(cmplxOutput[0].real(), 0.0f);
    }
    
    void inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept override
    {
        // Apple requires packed storage, so we will modify the array to be packed
        // and then unmodify the input array again so that we effectively did not
        // change the input - strictly this is UB!
        auto mutableInput = const_cast<std::complex<float>*> (cmplxInput);
        
        // pack the input
        auto numComplex = ((1 << order) >> 1) + 1;
        mutableInput[0] = std::complex<float>(mutableInput[0].real(), mutableInput[numComplex-1].real());
        
        DSPSplitComplex output  { realOutput, realOutput + 1};
        DSPSplitComplex input { reinterpret_cast<float*>(mutableInput),
                                reinterpret_cast<float*>(mutableInput) + 1};
        
        vDSP_fft_zrop(fftApple, &input, 2, &output, 2, order, kFFTDirection_Inverse);
        
        // undo our changes to the input array
        mutableInput[0] = std::complex<float>(mutableInput[0].real(), 0.0f);
    }
    
    float getNormalizationFactor() const noexcept override
    {
        return static_cast<float>(1 << order) * 4.0f;
    }
    
    //==============================================================================
    Pimpl(const Pimpl&) = delete;
    Pimpl(Pimpl&&) = delete;
    Pimpl& operator=(const Pimpl&) = delete;
    Pimpl& operator=(Pimpl&&) = delete;
    
    //==============================================================================
    int order;
    FFTSetup fftApple;
};
    
#else
#error No suitable FFT engine found for your platform
#endif
}

//==============================================================================
//==============================================================================
void* FASTFOURIER_API __fastfourier_entry(std::size_t size)   { return new Pimpl(size); }
