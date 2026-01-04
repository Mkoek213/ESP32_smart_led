package com.example.iot.backend.config;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;
import org.springframework.validation.annotation.Validated;

@Data
@Validated
@Configuration
@ConfigurationProperties(prefix = "mqtt")
public class MqttProperties {

    @NotBlank
    private String brokerUrl;

    private Credentials credentials;

    @Data
    public static class Credentials {
        private String username;
        private String password;
    }
}
