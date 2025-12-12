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
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

import static org.springframework.integration.mqtt.support.MqttHeaders.RECEIVED_TOPIC;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttIngestionService {

    private final TelemetryService telemetryService;
    private final ObjectMapper objectMapper;
    private final TopicParser topicParser;

    @Transactional
    @ServiceActivator(inputChannel = "telemetryChannel")
    public void handleTelemetry(@Payload String payload, @Header(RECEIVED_TOPIC) String topic) {
        log.info("Received message on topic {}: {}", topic, payload);

        try {
            TopicParser.ParsedTopic parsedTopic = topicParser.parse(topic);
            List<TelemetryDto> telemetryDtos = objectMapper.readValue(payload, new TypeReference<>() {
            });

            log.info("Received {} telemetry for deviceId: {}", telemetryDtos.size(), parsedTopic.getDeviceId());

            telemetryDtos.forEach(telemetryDto -> telemetryService.save(
                    telemetryDto,
                    parsedTopic.getCustomerId(),
                    parsedTopic.getLocationId(),
                    parsedTopic.getDeviceId()
            ));

        } catch (JsonProcessingException e) {
            log.error("Failed to parse JSON payload: {}", payload, e);
        } catch (IllegalArgumentException e) {
            log.error("Failed to parse topic: {}", topic, e);
        }
    }
}
