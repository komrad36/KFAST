# KFAST
The fastest CPU implementation of the FAST feature detector for computer vision (Rosten 2006). Outperforms canonical implementation by 500% multi-threaded or 60% single-threaded. Non-maximal suppression is supported.

The test driver requires OpenCV (for comparison and image load). KFAST itself requires AVX2 (for speeeeeeeeed!) but does NOT require OpenCV; it has no dependencies whatsoever.

Modern g++ and processor *highly* recommended.

    make
    ./KFAST

 The fastest CPU implementation of the FAST feature detector for computer vision, as described in the 2006 paper by Rosten and Drummond:
 
"Machine learning for high-speed corner detection" by Edward Rosten and Tom Drummond
https://www.edwardrosten.com/work/rosten_2006_machine.pdf

 My implementation uses AVX2, multithreading, and many other careful optimizations to implement the FAST algorithm as described in the paper, but at great speed. This implementation outperforms the reference implementation by 40-60% single-threaded or 500% multi-threaded (!) while exactly matching the reference implementation's output and capabilities.
