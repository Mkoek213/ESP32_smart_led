package com.example.iot.backend.jwt;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;
import org.springframework.validation.annotation.Validated;

@Data
@Validated
@Configuration
@ConfigurationProperties("jwt")
public class JwtProperties {

    @NotBlank
    private String secretKey;

    @NotNull
    private long expirationMs;
}
