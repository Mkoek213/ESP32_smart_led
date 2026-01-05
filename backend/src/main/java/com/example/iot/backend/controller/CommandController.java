package com.example.iot.backend.controller;

import com.example.iot.backend.dto.command.CommandRequest;
import com.example.iot.backend.service.CommandService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.security.Principal;

@Slf4j
@RestController
@RequestMapping("/api/commands")
@RequiredArgsConstructor
public class CommandController {

    private final CommandService commandService;
    ;

    @PostMapping("/{deviceId}")
    @PreAuthorize("authorizationService.canAccessDevice(principal, #deviceId)")
    public ResponseEntity<Void> sendCommand(
            @PathVariable Long deviceId,
            @RequestBody CommandRequest request,
            Principal principal
    ) {
        log.info("Received command request for device {}: {}", deviceId, request);
        commandService.sendCommand(deviceId, request);
        return ResponseEntity.accepted().build();
    }
}
