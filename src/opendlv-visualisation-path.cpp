/*
Microservice for the kiwi car created by Adrian Lundell, Fredrik Lindelöw and Elias Svensson.
This microservice was created for a project in the course Autonomous Robots given at Chalmers UoT.
 */

#include <vector>
#include <string>
#include <unordered_map>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>

#include <chrono> //Timing
#include "json.hpp" // To load cones from file

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid"))   ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height"))||
         (0 == commandlineArguments.count("minX"))  ||
         (0 == commandlineArguments.count("maxX"))  ||
         (0 == commandlineArguments.count("minY"))  ||
         (0 == commandlineArguments.count("maxY"))  ||
         (0 == commandlineArguments.count("freq"))  ||
         (0 == commandlineArguments.count("map-path")) ) {
        std::cerr << argv[0] << " attaches to a shared memory area containing an ARGB image." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> ... [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to send and receive messages" << std::endl;
        std::cerr << "         --width:  width of the display image" << std::endl;
        std::cerr << "         --height: height of the display image" << std::endl;
        std::cerr << "Example: " << argv[0] << "--cid=111 --width=800 --height=800 --minX=-2.5 --maxX=3.5" 
                << "--minY=-4 --maxY=2 --freq=100 --map-path=/opt/conetrack" << std::endl;
    }
    else {
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const double minX{static_cast<double>(std::stoi(commandlineArguments["minX"]))};
        const double maxX{static_cast<double>(std::stoi(commandlineArguments["maxX"]))};
        const double minY{static_cast<double>(std::stoi(commandlineArguments["minY"]))};
        const double maxY{static_cast<double>(std::stoi(commandlineArguments["maxY"]))};
        float const FREQ = std::stof(commandlineArguments["freq"]);
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        std::string const mapPath{commandlineArguments["map-path"]};

        // Interface to a running OpenDaVINCI session; here, you can send and receive messages.
        cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))}; 

        opendlv::sim::Frame latestFrame;
        std::mutex frameMutex;

        auto onFrame{[&latestFrame, &frameMutex](
            cluon::data::Envelope &&envelope)
        {   
            std::lock_guard<std::mutex> const lock(frameMutex);
            latestFrame = cluon::extractMessage<opendlv::sim::Frame>(std::move(envelope));
        }};

        cv::Mat globalMap(HEIGHT, WIDTH, CV_8UC3, cv::Scalar(0, 0, 0)); 
        
        double widthMap = maxX - minX;
        double heightMap = maxY - minY;
        int xScale = (int) (WIDTH/widthMap);
        int yScale = (int) (HEIGHT/heightMap);
        int xOffset = (int) (WIDTH-maxX*xScale);
        int yOffset = (int) (HEIGHT-maxY*yScale);
        
        nlohmann::json json;
        {
            std::ifstream i(mapPath + "/map.json"); // mapPath = path to conetrack folder
            i >> json;
        }

        {
            if (json.find("model") != json.end()) {
                for (auto const &j : json["model"]) {
                    double c0=0;
                    double c1=0;
                    double c2=0;
                    if (j.find("color") != j.end()) {
                        c0 = j["color"][0];
                        c1 = j["color"][1];
                        c2 = j["color"][2];
                    }
                    if (j.find("instances") != j.end()) {
                        for (auto const &i : j["instances"]) {
                        float x = i[0];
                        float y = i[1];

                        // Cone postion:
                        int32_t iPositionX = (int32_t) (x*xScale + xOffset);
                        int32_t iPositionY = (int32_t) (HEIGHT-(y*yScale + yOffset)); 
                        cv::circle(globalMap, cv::Point(iPositionX,iPositionY), 3, cv::Scalar(c2*255,c1*255,c0*255), 2); 
                        }
                    }
                }
            }
        }

        auto atFrequency{[&VERBOSE, &xScale, &yScale, &xOffset, yOffset, &latestFrame, &frameMutex, 
        &globalMap, &WIDTH, &HEIGHT]()-> bool 
        {  
            double posX;
            double posY;
            {
                std::lock_guard<std::mutex> const lock(frameMutex);
                posX = latestFrame.x();
                posY = latestFrame.y();
            }

            // Car postion:
            int32_t iPositionX0 = (int32_t) (posX*xScale + xOffset);
            int32_t iPositionY0 = (int32_t) (HEIGHT-(posY*yScale + yOffset));    
            
            if ( (posX > 1e-6) || (posX < -1e-6) || (posY > 1e-6) || (posY < -1e-6) ) {
                cv::circle(globalMap, cv::Point(iPositionX0,iPositionY0), 3, cv::Scalar(0,255,0), 2); 
                if (VERBOSE){ 
                    std::cout << posX << " , " << posY << std::endl;
                }
                cv::imshow("Global map", globalMap);
                cv::waitKey(1);
            }

            return true;
        }};

        od4.dataTrigger(opendlv::sim::Frame::ID(), onFrame);
        od4.timeTrigger(FREQ, atFrequency);
        retCode = 0;
    }
    return retCode;
}

