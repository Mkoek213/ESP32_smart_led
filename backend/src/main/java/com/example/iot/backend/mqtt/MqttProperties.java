package com.example.iot.backend.mqtt;

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

    private Ssl ssl;

    @Data
    public static class Credentials {
        private String username;
        private String password;
    }

    @Data
    public static class Ssl {
        private String caCrtFile;
        private String crtFile;
        private String keyFile;
    }
}
