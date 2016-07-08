# KFAST
The fastest CPU implementation of the FAST feature detector for computer vision (Rosten 2006). Outperforms canonical implementation by 59%.

Requires OpenCV (for comparison and image load) and AVX2 (for speeeeeeeeed!).
Profile-guided optimization and a modern g++ and processor highly recommended.

    make clean
    make profile
    ./KFAST
    make clean
    make
    ./KFAST
    
The profile run will be slow, of course; the second run is performant and should surpass OpenCV by 50-60%.
