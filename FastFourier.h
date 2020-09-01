/*==============================================================================
  Copyright (c) 2020 - Fielding DSP GmbH

  Created - 19. October 2018
==============================================================================*/

#pragma once
#include <cstdint>
#include <complex>

//==============================================================================
#if defined(_MSC_VER)
#define FASTFOURIER_API __stdcall
 #if FASTFOURIER_DLLBUILD
  #define FASTFOURIER_DLL_ENTRY_API extern "C" __declspec(dllexport)
 #else
  #define FASTFOURIER_DLL_ENTRY_API extern "C" __declspec(dllimport)
 #endif
#else
 #define FASTFOURIER_API
 #if FASTFOURIER_DLLBUILD
  #define FASTFOURIER_DLL_ENTRY_API extern "C" __attribute__((visibility("default")))
 #else
  #define FASTFOURIER_DLL_ENTRY_API extern "C"
 #endif
#endif

//==============================================================================
// @internal
FASTFOURIER_DLL_ENTRY_API void* FASTFOURIER_API __fastfourier_entry(std::size_t size);

//==============================================================================
struct FastFourier
{
    //==============================================================================
    /** Constructor

        @param size number of real coefficients in transform.
    */
    inline FastFourier(std::size_t size) : intf(static_cast<Intf*>(__fastfourier_entry(size))) {}

    /** Destructor */
    inline ~FastFourier()                { intf->release(); }

    //==============================================================================
    /** Do a forward Fourier transform (real -> complex)

        @param realInput   real input coefficients. Array must have the same size as the
                           as the size parameter specified in the constructor. The pointer
                           must also be SIMD aligned!
        @param cmplxOutput complex output coefficients. Array must have (size/2)+1 number of
                           complex coefficients. The pointer must also be SIMD aligned!
    */
    inline void forward(const float* realInput, std::complex<float>* cmplxOutput) noexcept
    {
        intf->forward(realInput, cmplxOutput);
    }

    /** Do an inverse Fourier transform (complex -> real)

        @param cmplxInput  complex input coefficients. Array must have (size/2)+1 number of
                           complex coefficients. The pointer must also be SIMD aligned!
        @param realOutput  real output coefficients. Array must have the same size as the
                           as the size parameter specified in the constructor. The pointer
                           must also be SIMD aligned!
    */
    inline void inverse(const std::complex<float>* cmplxInput, float* realOutput) noexcept
    {
        intf->inverse(cmplxInput, realOutput);
    }

    /** Returns the normalization factor

        Depending on which FFT engine is used internally, the normalization factor may
        be different. You can use this to integrate the normalization step into a multiplication
        step which you are doing anyways.
    */
    inline float getNormalizationFactor() const noexcept
    {
        return intf->getNormalizationFactor();
    }
    
    //==============================================================================
#if ! FASTFOURIER_DLLBUILD
private:
#endif
    struct Intf
    {
        virtual void FASTFOURIER_API release() = 0;
        virtual void FASTFOURIER_API forward(const float*, std::complex<float>*) noexcept = 0;
        virtual void FASTFOURIER_API inverse(const std::complex<float>*, float*) noexcept = 0;
        virtual float FASTFOURIER_API getNormalizationFactor() const noexcept = 0;
    };

    Intf* intf;
};
