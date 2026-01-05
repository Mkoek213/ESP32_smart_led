package com.example.iot.backend.controller;

import com.example.iot.backend.dto.telemetry.TelemetryResponse;
import com.example.iot.backend.service.TelemetryService;
import lombok.RequiredArgsConstructor;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/telemetry")
@RequiredArgsConstructor
public class TelemetryController {

    private final TelemetryService telemetryService;

    @GetMapping
    public Map<Long, List<TelemetryResponse>> getAllTelemetry(
            @RequestParam(required = false) Long start,
            @RequestParam(required = false) Long end,
            @AuthenticationPrincipal UserDetails userDetails
    ) {
        return telemetryService.getUserTelemetry(getUserId(userDetails), start, end);
    }

    @GetMapping("/{deviceId}")
    public List<TelemetryResponse> getDeviceTelemetry(
            @PathVariable Long deviceId,
            @RequestParam(required = false) Long start,
            @RequestParam(required = false) Long end,
            @AuthenticationPrincipal UserDetails userDetails
    ) {
        return telemetryService.getDeviceTelemetry(getUserId(userDetails), deviceId, start, end);
    }

    private Long getUserId(UserDetails userDetails) {
        if (userDetails instanceof com.example.iot.backend.model.User user) {
            return user.getId();
        }
        throw new IllegalArgumentException("Invalid user details");
    }
}
