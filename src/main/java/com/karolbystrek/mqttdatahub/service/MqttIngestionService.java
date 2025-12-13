package com.karolbystrek.mqttdatahub.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.karolbystrek.mqttdatahub.dto.TelemetryDto;
import com.karolbystrek.mqttdatahub.parser.TopicParser;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.messaging.handler.annotation.Header;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.stereotype.Service;

import java.util.List;

import static org.springframework.integration.mqtt.support.MqttHeaders.RECEIVED_TOPIC;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttIngestionService {

    private final TelemetryService telemetryService;
    private final DeviceService deviceService;
    private final ObjectMapper objectMapper;
    private final TopicParser topicParser;

    @ServiceActivator(inputChannel = "mqttInboundChannel")
    public void handleMessage(@Payload String payload, @Header(RECEIVED_TOPIC) String topic) {
        log.debug("Received message on topic {}: {}", topic, payload);

        try {
            TopicParser.ParsedTopic parsedTopic = topicParser.parse(topic);

            switch (parsedTopic.getType()) {
                case TELEMETRY -> handleTelemetry(payload, parsedTopic);
                case STATUS -> handleStatus(payload, parsedTopic);
                case CMD -> log.warn("Received message on CMD topic, ignoring: {}", topic);
            }

        } catch (IllegalArgumentException e) {
            log.error("Failed to parse topic: {}", topic, e);
        }
    }

    private void handleTelemetry(String payload, TopicParser.ParsedTopic parsedTopic) {
        try {
            List<TelemetryDto> telemetryDtos = objectMapper.readValue(payload, new TypeReference<>() {
            });

            log.info("Received {} telemetry items for deviceId: {}", telemetryDtos.size(), parsedTopic.getDeviceId());

            telemetryDtos.forEach(telemetryDto -> telemetryService.save(
                    telemetryDto,
                    parsedTopic.getCustomerId(),
                    parsedTopic.getLocationId(),
                    parsedTopic.getDeviceId()
            ));
        } catch (JsonProcessingException e) {
            log.error("Failed to parse telemetry payload: {}", payload, e);
        }
    }

    private void handleStatus(String payload, TopicParser.ParsedTopic parsedTopic) {
        log.info("Device status update - Customer: {}, Location: {}, Device: {}, Status: {}",
                parsedTopic.getCustomerId(),
                parsedTopic.getLocationId(),
                parsedTopic.getDeviceId(),
                payload);

        deviceService.updateDeviceStatus(
                parsedTopic.getCustomerId(),
                parsedTopic.getLocationId(),
                parsedTopic.getDeviceId(),
                payload
        );
    }
}
