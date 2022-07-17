#ifndef MPC_H
#define MPC_H

#include <vector>
#include <utility>
#include "Eigen-3.3/Eigen/Core"

class MPC {
 public:
  MPC();

  virtual ~MPC();

  // Solve the model given an initial state and polynomial coefficients.
  // Return the first actuations.
  std::vector<double> Solve(const Eigen::VectorXd &state, 
                            const Eigen::VectorXd &coeffs,
							std::vector<double>& mpc_x_vals,
							std::vector<double>& mpc_y_vals);
};

#endif  // MPC_H
