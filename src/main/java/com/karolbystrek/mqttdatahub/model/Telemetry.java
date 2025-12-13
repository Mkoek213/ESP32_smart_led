package com.karolbystrek.mqttdatahub.model;

import com.karolbystrek.mqttdatahub.dto.TelemetryDto;
import jakarta.persistence.*;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.NoArgsConstructor;
import org.hibernate.annotations.CreationTimestamp;

import java.time.LocalDateTime;

import static jakarta.persistence.FetchType.LAZY;
import static jakarta.persistence.GenerationType.IDENTITY;

@Entity
@Table(name = "telemetry")
@Builder
@Getter
@NoArgsConstructor
@AllArgsConstructor
public class Telemetry {

    @Id
    @GeneratedValue(strategy = IDENTITY)
    private Long id;

    @ManyToOne(fetch = LAZY)
    @JoinColumn(name = "device_id", nullable = false)
    private Device device;

    @CreationTimestamp
    @Column(nullable = false, updatable = false)
    private LocalDateTime createdAt;

    @Column(nullable = false)
    private Long timestamp;

    private Boolean motionDetected;
    private Double temperature;
    private Double humidity;
    private Double pressure;

    public static Telemetry createFrom(TelemetryDto dto, Device device) {
        return Telemetry.builder()
                .device(device)
                .timestamp(dto.timestamp())
                .motionDetected(dto.motionDetected())
                .temperature(dto.temperature())
                .humidity(dto.humidity())
                .pressure(dto.pressure())
                .build();
    }
}
