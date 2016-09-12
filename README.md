# KFAST

![picture alt](https://raw.githubusercontent.com/komrad36/KFAST/master/results.jpg "Speed comparison of KFAST vs. other implementations")

KFAST is the fastest CPU implementation of the FAST feature detector for computer vision, as described in the 2006 paper by Rosten and Drummond:
 
"Machine learning for high-speed corner detection" by Edward Rosten and Tom Drummond
https://www.edwardrosten.com/work/rosten_2006_machine.pdf

Non-maximal suppression is supported. My implementation uses AVX2, multithreading, and many other careful optimizations to implement the FAST algorithm as described in the paper, but at great speed. This implementation outperforms the next-fastest implementation by 60% single-threaded or 500% multi-threaded (!) while exactly matching its output and capabilities.

The test driver requires OpenCV (for comparison and image load). KFAST itself requires AVX2 (for speeeeeeeeed!) but does NOT require OpenCV; it has no dependencies whatsoever.

Works on Linux (g++) and Windows (MSVC) with C++11 or higher.

Modern compiler and processor *highly* recommended.

On Linux you can do:

    make
    ./KFAST

 All functionality is contained within 'KFAST.h'.
 'main.cpp' is a test driver demonstrating a performance comparison between Dr. Rosten's code, OpenCV's implementation, and my implementation. Dr. Rosten's implementation is contained in the folder 'Rosten'.
 
 Simply plugging KFAST into ORB results in a several-fold speed improvement; however, there are a LOT of other things that need to be improved about ORB too, and I am working on it as time permits. I'll release them all at once when finished.
 
 I would like to thank Dr. Rosten for his brilliant FAST and FASTER corner detectors.