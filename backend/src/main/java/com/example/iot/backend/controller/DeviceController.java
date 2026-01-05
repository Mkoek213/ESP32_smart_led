package com.example.iot.backend.controller;

import com.example.iot.backend.dto.device.DeviceClaimRequest;
import com.example.iot.backend.dto.device.DeviceResponse;
import com.example.iot.backend.dto.device.DeviceUpdateRequest;
import com.example.iot.backend.model.User;
import com.example.iot.backend.service.DeviceService;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import org.springframework.http.HttpStatus;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@RestController
@RequestMapping("/devices")
@RequiredArgsConstructor
public class DeviceController {

    private final DeviceService deviceService;

    @GetMapping("/{deviceId}")
    public DeviceResponse getDevice(@PathVariable Long deviceId, @AuthenticationPrincipal UserDetails userDetails) {
        return deviceService.getDevice(getUserId(userDetails), deviceId);
    }

    @GetMapping
    public List<DeviceResponse> getAllDevices(@AuthenticationPrincipal UserDetails userDetails) {
        return deviceService.getAllDevices(getUserId(userDetails));
    }

    @PutMapping("/{deviceId}")
    public DeviceResponse updateDevice(@PathVariable Long deviceId, @RequestBody @Valid DeviceUpdateRequest request, @AuthenticationPrincipal UserDetails userDetails) {
        return deviceService.updateDevice(getUserId(userDetails), deviceId, request);
    }

    @DeleteMapping("/{deviceId}")
    @ResponseStatus(HttpStatus.NO_CONTENT)
    public void deleteDevice(@PathVariable Long deviceId, @AuthenticationPrincipal UserDetails userDetails) {
        deviceService.deleteDevice(getUserId(userDetails), deviceId);
    }

    @PostMapping("/claim")
    @ResponseStatus(HttpStatus.CREATED)
    public DeviceResponse claimDevice(@RequestBody @Valid DeviceClaimRequest request, @AuthenticationPrincipal UserDetails userDetails) {
        return deviceService.claimDevice(getUserId(userDetails), request);
    }

    @DeleteMapping("/{deviceId}/unbind")
    @ResponseStatus(HttpStatus.NO_CONTENT)
    public void unbindDevice(@PathVariable Long deviceId, @AuthenticationPrincipal UserDetails userDetails) {
        deviceService.unbindDevice(getUserId(userDetails), deviceId);
    }

    private Long getUserId(UserDetails userDetails) {
        if (userDetails instanceof User user) {
            return user.getId();
        }
        throw new IllegalArgumentException("UserDetails is not an instance of User");
    }
}
