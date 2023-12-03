cmake -DCMAKE_C_COMPILER=/usr/local/bin/gcc \
      -DCMAKE_CXX_COMPILER=/usr/local/bin/g++ \
      -DCMAKE_Fortran_COMPILER=/usr/local/bin/gfortran \
      -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc  \
      -DCMAKE_BUILD_TYPE=Release \
      -DGAUXC_ENABLE_OPENMP=on \
      -DGAUXC_ENABLE_MPI=off \
      -DGAUXC_ENABLE_CUDA=on \
      -DGAUXC_ENABLE_MAGMA=off \
      -DCMAKE_CUDA_ARCHITECTURES=89 \
      ..

