package com.example.iot.backend.model;

import com.example.iot.backend.dto.TelemetryDto;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.Table;
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

    private Double temperature;
    private Double humidity;
    private Double pressure;
    private Integer personCount;

    public static Telemetry createFrom(TelemetryDto dto, Device device) {
        return Telemetry.builder()
                .device(device)
                .timestamp(dto.timestamp())
                .temperature(dto.temperature())
                .humidity(dto.humidity())
                .pressure(dto.pressure())
                .personCount(dto.personCount())
                .build();
    }
}
