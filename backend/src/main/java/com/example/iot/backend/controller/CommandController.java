package com.example.iot.backend.controller;

import com.example.iot.backend.dto.command.CommandRequest;
import com.example.iot.backend.model.User;
import com.example.iot.backend.service.CommandService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.HttpStatus;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

@Slf4j
@RestController
@RequestMapping("/commands")
@RequiredArgsConstructor
public class CommandController {

    private final CommandService commandService;

    @PostMapping("/{deviceId}")
    @ResponseStatus(HttpStatus.ACCEPTED)
    public void sendCommand(
            @PathVariable Long deviceId,
            @RequestBody CommandRequest command,
            @AuthenticationPrincipal UserDetails userDetails
    ) {
        Long userId = getUserId(userDetails);
        log.info("User {} is sending {} command to device {}", userId, command.type(), deviceId);
        commandService.sendCommand(userId, deviceId, command);
    }

    private Long getUserId(UserDetails userDetails) {
        if (userDetails == null) {
            return 1L;
        }
        if (userDetails instanceof User user) {
            return user.getId();
        }
        throw new IllegalStateException("Authentication principal is not of type User");
    }
}
