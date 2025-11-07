package com.karolbystrek.mqttdatahub.model;

import com.karolbystrek.mqttdatahub.dto.SensorDataDto;
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
@Table(name = "sensor_data")
@Builder
@Getter
@NoArgsConstructor
@AllArgsConstructor
public class SensorData {

    @Id
    @GeneratedValue(strategy = IDENTITY)
    private Long id;

    @ManyToOne(fetch = LAZY)
    @JoinColumn(name = "device_id", nullable = false)
    private Device device;

    @Column(nullable = false)
    private LocalDateTime timestamp;

    @CreationTimestamp
    @Column(nullable = false, updatable = false)
    private LocalDateTime createdAt;

    private Double humidity;
    private Boolean motionDetected;
    private Integer lightLevel;

    public static SensorData createSensorDataFrom(SensorDataDto dto, Device device) {
        return SensorData.builder()
                .device(device)
                .timestamp(dto.timestamp())
                .humidity(dto.humidity())
                .motionDetected(dto.motionDetected())
                .lightLevel(dto.lightLevel())
                .build();
    }
}
