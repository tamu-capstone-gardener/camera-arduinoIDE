## README

This code is used to program an ESP32S3 camera. The code currently takes a photo every 30 seconds, 
then breaks the frame into chunks, then sends the chunks over MQTT to the web application. 

An important note was this code was develped on the Arduino IDE because it would not work with platform.io
## How to run code on microcontroller

- Plug in power via USB-C
- Input correct credentials for wifi and mqtt
- Press Upload Button
- Open Serial Monitor to debug

## Link to other repos

| Repository Name                | Description                                                   | Link                                                                 |
|--------------------------------|---------------------------------------------------------------|----------------------------------------------------------------------|
| Ruby on Rails Server           | Web server application for our project built with Ruby on Rails | [Repository](https://github.com/tamu-capstone-gardener/rails-react)   |
| Microcontroller Code for ESP32 | Hardware code that runs on the ESP32                          | [Repository](https://github.com/tamu-capstone-gardener/microcontroller)|
| Team Agreement                 | Team agreement that we all signed and agreed to               | [Repository](https://github.com/tamu-capstone-gardener/team-agreement) |
| Team/Individuals Reports       | Contains all team and individual reports                      | [Repository](https://github.com/tamu-capstone-gardener/reports)        |
