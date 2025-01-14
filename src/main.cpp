#include <math.h>
#include <uWS/uWS.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "helpers.h"
#include "json.hpp"
#include "MPC.h"

// for convenience
using nlohmann::json;
using std::string;
using std::vector;
using Eigen::VectorXd;
using Eigen::Matrix2d;


// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

int main() {
  uWS::Hub h;

  // MPC is initialized here!
  MPC mpc;

  h.onMessage([&mpc](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
                     uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    string sdata = string(data).substr(0, length);
    std::cout << sdata << std::endl;
    if (sdata.size() > 2 && sdata[0] == '4' && sdata[1] == '2') {
      string s = hasData(sdata);
      if (s != "") {
        auto j = json::parse(s);
        string event = j[0].get<string>();
        if (event == "telemetry") {
          // j[1] is the data JSON object
          vector<double> ptsx = j[1]["ptsx"];
          vector<double> ptsy = j[1]["ptsy"];
          double px = j[1]["x"];
          double py = j[1]["y"];
          double psi = j[1]["psi"];
          double v = j[1]["speed"];

          /**
           * TODO: Calculate steering angle and throttle using MPC.
           * Both are in between [-1, 1].
           */
          double steer_value {0.0};
          double throttle_value {0.0};

          // Transforming pts from global frame Funity to car frame Fcar
          VectorXd Fcar_waypts_x(ptsx.size());
          VectorXd Fcar_waypts_y(ptsy.size());

          double x=0.0, y=0.0;
          for(int i=0; i<ptsx.size(); ++i)
          {
        	  // Shifting ptsx, ptsy to car frame px,py.. i.e. px, py becomes origin
        	  x = ptsx[i] - px;
        	  y = ptsy[i] - py;

        	  Fcar_waypts_x[i] = x*cos(-psi) - y*sin(-psi);
        	  Fcar_waypts_y[i] = x*sin(-psi) + y*cos(-psi);
          }

          VectorXd coeffs = polyfit(Fcar_waypts_x, Fcar_waypts_y, 3);

          // Find tracking error
          px = 0;
          py = 0;
          double cte = polyeval(coeffs, px) - py;

          // Find orientation error
          psi = 0.0;	          // Shifting psi car frame.. i.e. psi becomes origin
//          double poly_dot = 0.0;
//          for (int i = 1; i < coeffs.size(); ++i)
//          {
//              poly_dot += coeffs[i] * std::pow(px, i-1);
//		  }
          double epsi = psi - atan(coeffs[1]);

          // Form a Eigen Vector with model states
		  VectorXd state(6);
		  state << px, py, psi, v, cte, epsi;

		  // Apply MPC loop
          // Display the MPC predicted trajectory
          vector<double> mpc_x_vals;
          vector<double> mpc_y_vals;
		  auto vars = mpc.Solve(state, coeffs, mpc_x_vals, mpc_y_vals);

          json msgJson;
          // NOTE: Remember to divide by deg2rad(25) before you send the 
          //   steering value back. Otherwise the values will be in between 
          //   [-deg2rad(25), deg2rad(25] instead of [-1, 1].
		  steer_value = (double)vars[6]/deg2rad(25);
		  throttle_value = vars[7];

          msgJson["steering_angle"] = steer_value;
          msgJson["throttle"] = throttle_value;


          /**
           * TODO: add (x,y) points to list here, points are in reference to 
           *   the vehicle's coordinate system the points in the simulator are 
           *   connected by a Green line
           */
          msgJson["mpc_x"] = mpc_x_vals;
          msgJson["mpc_y"] = mpc_y_vals;

          // Display the waypoints/reference line
          vector<double> next_x_vals;
          vector<double> next_y_vals;

          /**
           * TODO: add (x,y) points to list here, points are in reference to 
           *   the vehicle's coordinate system the points in the simulator are 
           *   connected by a Yellow line
           */
          for(int i=0; i<Fcar_waypts_x.size(); ++i)
		  {
        	  next_x_vals.push_back( Fcar_waypts_x[i] );
        	  next_y_vals.push_back( polyeval(coeffs, Fcar_waypts_x[i]) );
		  }

          msgJson["next_x"] = next_x_vals;
          msgJson["next_y"] = next_y_vals;


          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          std::cout << msg << std::endl;
          // Latency
          // The purpose is to mimic real driving conditions where
          //   the car does actuate the commands instantly.
          //
          // Feel free to play around with this value but should be to drive
          //   around the track with 100ms latency.
          //
          // NOTE: REMEMBER TO SET THIS TO 100 MILLISECONDS BEFORE SUBMITTING.
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        }  // end "telemetry" if
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }  // end websocket if
  }); // end h.onMessage

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
                         char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port)) {
    std::cout << "Listening to port " << port << std::endl;
  } else {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  
  h.run();
}
