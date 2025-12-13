package com.karolbystrek.mqttdatahub;

import jakarta.annotation.PostConstruct;
import org.springframework.boot.autoconfigure.SpringBootApplication;

import java.util.TimeZone;

import static org.springframework.boot.SpringApplication.run;

@SpringBootApplication
public class MqttClientApplication {

    public static void main(String[] args) {
        run(MqttClientApplication.class, args);
    }

    @PostConstruct
    public void init() {
        TimeZone.setDefault(TimeZone.getTimeZone("Europe/Warsaw"));
    }

}
