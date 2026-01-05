package com.example.iot.backend.mqtt;

import com.example.iot.backend.dto.telemetry.TelemetryDto;
import com.example.iot.backend.model.Device;
import com.example.iot.backend.service.DeviceService;
import com.example.iot.backend.service.TelemetryService;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.messaging.handler.annotation.Header;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.stereotype.Service;

import java.util.List;

import static com.example.iot.backend.mqtt.MqttTopicParser.ParsedTopic;
import static org.springframework.integration.mqtt.support.MqttHeaders.RECEIVED_TOPIC;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttIngestionService {

    private final TelemetryService telemetryService;
    private final DeviceService deviceService;
    private final ObjectMapper objectMapper;
    private final MqttTopicParser mqttTopicParser;

    @ServiceActivator(inputChannel = "mqttInboundChannel")
    public void handleMessage(@Payload String payload, @Header(RECEIVED_TOPIC) String topic) {
        log.debug("Received message on topic {}: {}", topic, payload);

        try {
            ParsedTopic parsedTopic = mqttTopicParser.parse(topic);

            switch (parsedTopic.getType()) {
                case TELEMETRY -> handleTelemetry(payload, parsedTopic);
                case STATUS -> handleStatus(payload, parsedTopic);
                case CMD -> log.warn("Received message on CMD topic, ignoring: {}", topic);
            }

        } catch (IllegalArgumentException e) {
            log.error("Failed to parse topic: {}", topic, e);
        }
    }

    private void handleTelemetry(String payload, ParsedTopic parsedTopic) {
        try {
            List<TelemetryDto> telemetryDtos = objectMapper.readValue(payload, new TypeReference<>() {
            });

            log.info("Received {} telemetry items for deviceId: {}", telemetryDtos.size(), parsedTopic.getDeviceId());

            telemetryDtos.forEach(telemetryDto -> telemetryService.saveTelemetry(
                    telemetryDto,
                    parsedTopic.getDeviceId()
            ));
        } catch (JsonProcessingException e) {
            log.error("Failed to parse telemetry payload: {}", payload, e);
        }
    }

    private void handleStatus(String payload, ParsedTopic parsedTopic) {
        log.info("Device status update - User: {}, Location: {}, Device: {}, Status: {}",
                parsedTopic.getUserId(),
                parsedTopic.getLocationId(),
                parsedTopic.getDeviceId(),
                payload);

        deviceService.setDeviceStatus(
                parsedTopic.getUserId(),
                parsedTopic.getDeviceId(),
                Device.Status.valueOf(payload)
        );
    }
}
