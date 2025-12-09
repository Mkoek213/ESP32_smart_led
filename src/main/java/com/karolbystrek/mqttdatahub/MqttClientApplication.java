package com.karolbystrek.mqttdatahub;

import org.springframework.boot.autoconfigure.SpringBootApplication;

import static org.springframework.boot.SpringApplication.run;

@SpringBootApplication
public class MqttClientApplication {

    public static void main(String[] args) {
        run(MqttClientApplication.class, args);
    }

}
