package com.example.iot.backend.service;

import com.example.iot.backend.dto.config.SystemConfigResponse;
import com.example.iot.backend.mqtt.MqttProperties;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;

@RequiredArgsConstructor
@Service
public class SystemConfigService {

    private final MqttProperties mqttProperties;

    public SystemConfigResponse getSystemConfig() {
        return SystemConfigResponse.builder()
                .mqttBrokerUrl(mqttProperties.getBrokerUrl())
                .build();
    }
}
