package com.karolbystrek.mqttdatahub.parser;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.springframework.stereotype.Component;

import java.util.regex.Pattern;

@Component
public class TopicParser {

    private static final Pattern TELEMETRY_TOPIC_PATTERN = Pattern.compile("^/?customer/(\\d+)/location/(\\d+)/device/(\\d+)/sensor-data$");

    public ParsedTopic parse(String topic) {
        var matcher = TELEMETRY_TOPIC_PATTERN.matcher(topic);
        if (!matcher.matches()) {
            throw new IllegalArgumentException("Invalid topic format: " + topic);
        }
        return new ParsedTopic(
                Long.parseLong(matcher.group(1)),
                Long.parseLong(matcher.group(2)),
                Long.parseLong(matcher.group(3))
        );
    }

    @Data
    @NoArgsConstructor
    @AllArgsConstructor
    public static class ParsedTopic {
        private Long customerId;
        private Long locationId;
        private Long deviceId;
    }
}

