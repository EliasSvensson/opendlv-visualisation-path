# OpenDLV microservice for visualising conetrack and simulated path of a robot on a 2D plane

A microservice for displaying the simulated path of a robot by using inputs of a conetrack and robots current position.

---
## Running the microservice 

To run this microservice using Docker start it as follows:
```bash
docker run --rm -ti --init --net=host -v /tmp:/tmp -v ./:/opt -e DISPLAY=$DISPLAY registry.git.chalmers.se/courses/tme290/2021/group3/opendlv-visualisation-path:v1 --cid=111 --width=800 --height=800 --minX=-2.5 --maxX=3.5 --minY=-4 --maxY=2 --freq=100 --map-path=/opt/conetrack
"
```
The with and height inputs sets the size of the display image. minX, maxX, minY, and maxY is set so that the conetrack is correctly centered in the image. map-path is set to a filepath to a .json file containing the cones locations.

The easiest way to run the microservice together with the other required microservices for autonomous driving is using docker-compose. The code to run this microservice is as following:
```bash
version: "3.6"

services:
  opendlv-visualisation-path:
    image: opendlv-visualisation-path:v1
    network_mode: "host"
    volumes:
      - /tmp:/tmp
      - ./:/opt
    environment:
      - DISPLAY=${DISPLAY}
    command: "/usr/bin/opendlv-visualisation-path --cid=111 --width=800 --height=800 --minX=-2.5 --maxX=3.5 --minY=-4 --maxY=2 --freq=100 --map-path=/opt/conetrack" 
```
