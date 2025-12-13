package com.karolbystrek.mqttdatahub.parser;

import com.karolbystrek.mqttdatahub.model.TopicType;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.springframework.stereotype.Component;

import java.util.regex.Pattern;

import static com.karolbystrek.mqttdatahub.model.TopicType.*;

@Component
public class TopicParser {

    private static final Pattern TOPIC_PATTERN = Pattern.compile("^/?customer/(\\d+)/location/(\\d+)/device/(\\d+)/(telemetry|status|cmd)$");

    public ParsedTopic parse(String topic) {
        var matcher = TOPIC_PATTERN.matcher(topic);
        if (!matcher.matches()) {
            throw new IllegalArgumentException("Invalid topic format: " + topic);
        }

        String suffix = matcher.group(4);
        TopicType type = switch (suffix) {
            case "telemetry" -> TELEMETRY;
            case "status" -> STATUS;
            case "cmd" -> CMD;
            default -> throw new IllegalArgumentException("Unknown topic suffix: " + suffix);
        };

        return new ParsedTopic(
                Long.parseLong(matcher.group(1)),
                Long.parseLong(matcher.group(2)),
                Long.parseLong(matcher.group(3)),
                type
        );
    }

    @Data
    @NoArgsConstructor
    @AllArgsConstructor
    public static class ParsedTopic {
        private Long customerId;
        private Long locationId;
        private Long deviceId;
        private TopicType type;
    }
}

