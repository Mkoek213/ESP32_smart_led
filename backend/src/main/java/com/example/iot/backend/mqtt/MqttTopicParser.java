package com.example.iot.backend.mqtt;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.springframework.stereotype.Component;

import java.util.regex.Pattern;

import static com.example.iot.backend.mqtt.MqttTopicType.CMD;
import static com.example.iot.backend.mqtt.MqttTopicType.STATUS;
import static com.example.iot.backend.mqtt.MqttTopicType.TELEMETRY;
import static java.lang.Long.parseLong;

@Component
public class MqttTopicParser {

    private static final Pattern TOPIC_PATTERN = Pattern.compile("^/?customer/(\\d+)/location/(\\d+)/device/(\\d+)/(telemetry|status|cmd)$");

    public ParsedTopic parse(String topic) {
        var matcher = TOPIC_PATTERN.matcher(topic);
        if (!matcher.matches()) {
            throw new IllegalArgumentException("Invalid topic format: " + topic);
        }

        String suffix = matcher.group(4);
        MqttTopicType type = switch (suffix) {
            case "telemetry" -> TELEMETRY;
            case "status" -> STATUS;
            case "cmd" -> CMD;
            default -> throw new IllegalArgumentException("Unknown topic suffix: " + suffix);
        };

        return new ParsedTopic(
                parseLong(matcher.group(1)),
                parseLong(matcher.group(2)),
                parseLong(matcher.group(3)),
                type
        );
    }

    @Data
    @NoArgsConstructor
    @AllArgsConstructor
    public static class ParsedTopic {
        private Long userId;
        private Long locationId;
        private Long deviceId;
        private MqttTopicType type;
    }
}
