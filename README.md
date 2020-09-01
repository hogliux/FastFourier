# FastFourier

A simple FFT wrapper for all the major desktop and mobile OSes. This library will always wrap the fastest (& most free) FFT library for your platform.

On Windows, x86 Linux and x86 Android, FastFourier requires Intel's IPP library. You will need to agree to Intel's IPP license yourself and place the downloaded binaries into a folder named intel_ipp. The folder should have the following structure:

```
intel_ipp/windows/x86_64/ippsmt.lib
intel_ipp/windows/x86_64/ippcoremt.lib
intel_ipp/windows/x86_64/ippvmmt.lib
intel_ipp/windows/x86/ippsmt.lib
intel_ipp/windows/x86/ippcoremt.lib
intel_ipp/windows/x86/ippvmmt.lib
intel_ipp/android/x86_64/libippvm.a
intel_ipp/android/x86_64/libippcore.a
intel_ipp/android/x86_64/libipps.a
intel_ipp/android/x86/libippvm.a
intel_ipp/android/x86/libippcore.a
intel_ipp/android/x86/libipps.a
intel_ipp/linux/x86_64/libippvm.a
intel_ipp/linux/x86_64/libippcore.a
intel_ipp/linux/x86_64/libipps.a
intel_ipp/linux/x86/libippvm.a
intel_ipp/linux/x86/libippcore.a
intel_ipp/linux/x86/libipps.a
```

You will also need to compile Ne10 yourself and put it into a folder with the following structure:

ne10/armeabi-v7a/libNE10.a
ne10/arm64-v8a/libNE10.a
