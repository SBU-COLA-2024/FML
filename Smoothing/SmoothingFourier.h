#ifndef SMOOTHINGFOURIER_HEADER
#define SMOOTHINGFOURIER_HEADER

#ifdef USE_MPI
#include <mpi.h>
#endif

#ifdef USE_OMP
#include <omp.h>
#endif

#include <FML/Global/Global.h>
#include <FML/FFTWGrid/FFTWGrid.h>

namespace FML {
  namespace GRID {

    template<int N>
      using FFTWGrid = FML::GRID::FFTWGrid<N>;

    // Low-pass filters
    template<int N>
      void smoothing_filter_fourier_space(
          FFTWGrid<N> & fourier_grid,
          double smoothing_scale,
          std::string smoothing_method)
      {

        // Sharp cut off kR = 1
        std::function<double(double)> filter_sharpk = [=](double k){
          double kR = k*smoothing_scale;
          if(kR < 1.0) return 1.0;
          return 0.0;
        };
        // Gaussian exp(-kR^2/2)
        std::function<double(double)> filter_gaussian = [=](double k){
          double kR = k*smoothing_scale;
          return std::exp( -0.5*kR*kR);
        };
        // Top-hat F[ (|x| < R) ]. Implemented only for 2D and 3D
        std::function<double(double)> filter_tophat_2D = [=](double k){
          double kR = k*smoothing_scale;
          if(kR < 1e-5) return 1.0;
          return 2.0/(kR*kR) * (1.0 - std::cos(kR));
        };
        std::function<double(double)> filter_tophat_3D = [=](double k){
          double kR = k*smoothing_scale;
          if(kR < 1e-5) return 1.0;
          return 3.0 * (std::sin(kR) - kR*std::cos(kR)) / (kR*kR*kR);
        };

        // Select the filter
        std::function<double(double)> filter;
        if(smoothing_method == "sharpk") { 
          filter = filter_sharpk;
        } else if(smoothing_method == "gaussian") { 
          filter = filter_gaussian;
        } else if(smoothing_method == "tophat") { 
          assert_mpi(N == 2 or N == 3, 
              "[smoothing_filter_fourier_space] Tophat filter only implemented in 2D and 3D");
          if(N == 2) filter = filter_tophat_2D;
          if(N == 3) filter = filter_tophat_3D;
        } else {
          throw std::runtime_error("Unknown filter " + smoothing_method + " Options: sharpk, gaussian, tophat");
        }

        // Do the smoothing
        std::vector<double> kvec(N);
        double kmag;
        for(auto & index : fourier_grid.get_fourier_range()){
          fourier_grid.get_fourier_wavevector_and_norm_by_index(index, kvec, kmag);
          auto value = fourier_grid.get_fourier_from_index(index);  
          value *= filter(kmag);
          fourier_grid.set_fourier_from_index(index, value);
        }
      }
  }
}
#endif
