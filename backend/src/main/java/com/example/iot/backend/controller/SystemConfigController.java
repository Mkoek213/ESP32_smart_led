package com.example.iot.backend.controller;

import com.example.iot.backend.dto.config.SystemConfigResponse;
import com.example.iot.backend.service.SystemConfigService;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/config")
@RequiredArgsConstructor
public class SystemConfigController {

    private final SystemConfigService systemConfigService;

    @GetMapping
    public SystemConfigResponse getConfig() {
        return systemConfigService.getSystemConfig();
    }
}
